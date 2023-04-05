/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 10 - Stepper/Temperature  *
 * File:        main.c                               *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */


//---------------- INCLUDES ----------------//
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <tusb.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


//---------------- DEFINITIONS ----------------//

#define BTN_REG_TICK_GAP 200 / portTICK_PERIOD_MS
#define BTN_WINDOW_TICK_LENGTH (2 * 1000) / portTICK_PERIOD_MS

#define NUM_IN_PINS 3

#define BTN1 19
#define BTN2 9
#define BTN3 8

// Keep an array of the IN pins for easy initialization
const int IN_PINS[NUM_IN_PINS] = {BTN1, BTN2, BTN3};

#define NUM_OUT_PINS 14

// Seven segment display pins
#define SevenSegCC1 11  // Right digit
#define SevenSegCC2 10  // Left digit
#define SevenSegA 26    // N  seg
#define SevenSegB 27    // NE seg
#define SevenSegC 29    // SE seg
#define SevenSegD 18    // S  seg
#define SevenSegE 25    // SW seg
#define SevenSegF 7     // NW seg
#define SevenSegG 28    // Middle seg
#define SevenSegDP 24   // Decimal points

// Stepper moter pins
#define STEPPER_PIN_1 12
#define STEPPER_PIN_2 13
#define STEPPER_PIN_3 00
#define STEPPER_PIN_4 01

// Keep an array of the OUT pins for easy initialization
const int OUT_PINS[NUM_OUT_PINS] = {
    SevenSegCC1, SevenSegCC2,
    SevenSegA, SevenSegB, SevenSegC, SevenSegD, SevenSegE, SevenSegF, SevenSegG,
    SevenSegDP,
    STEPPER_PIN_1, STEPPER_PIN_2, STEPPER_PIN_3, STEPPER_PIN_4
};

// Definitions for the HDC1080 sensor and I2C bus mode
#define I2C_ADDRESS 0x40
#define I2C_PORT i2c1
#define TI_MFID 0x5449
#define HDC1080_DVID 0x1050
#define HDC1080_TMP_REG 0x00
#define HDC1080_HMD_REG 0x01
#define HDC1080_CNF_REG 0x02
#define HDC1080_SN1_REG 0xFB
#define HDC1080_SN2_REG 0xFC
#define HDC1080_SN3_REG 0xFD
#define I2C_MFID_REG 0xFE
#define I2C_DVID_REG 0xFF

// This enum represents an I2C register to be read
enum HDC1080_REG {
	TMP_REG = HDC1080_TMP_REG,
	HMD_REG = HDC1080_HMD_REG,
	CNF_REG = HDC1080_CNF_REG,
	SN1_REG = HDC1080_SN1_REG,
	SN2_REG = HDC1080_SN2_REG,
	SN3_REG = HDC1080_SN3_REG,
	MFID_REG = I2C_MFID_REG,
	DVID_REG = I2C_DVID_REG
};

const static bool steps[4][4] = {
	{0, 0, 1, 1},
	{0, 1, 1, 0},
	{1, 1, 0, 0},
	{1, 0, 0, 1}
};


//---------------- PROTOTYPES / HANDLES ----------------//

// Utility function prototypes for the seven segment display
static void putDigit(int, bool);
static void clearDigits();
static void put7Pins(int, int, int, int, int, int, int);

// Utility funciton prototypes for the i2c bus
static void i2c_init_HDC1080();
static bool i2c_connect_HDC1080();
static int  read_hdc_reg(enum HDC1080_REG);
static bool i2c_w(const uint8_t *, size_t);
static bool i2c_r(uint8_t *, size_t);

// Other utility function prototypes
static void gpio_init_pins();
static void cls();

// ISR prototypes
static void vButtonISR(uint, uint32_t);

// Task prototypes
static void vMainTsk();
static void vButtonTsk();
static void vSwitcherTsk();
static void vSevSegTsk();
static void vHDC1080ConnectorTsk();
static void vHDC1080Tsk();
static void vStepperTsk();

// Task handles
static TaskHandle_t xMainTsk		 = NULL;
static TaskHandle_t xButtonTsk		 = NULL;
static TaskHandle_t xSwitcherTsk	 = NULL;
static TaskHandle_t xSevSegTsk		 = NULL;
static TaskHandle_t xHDC1080CScanTsk = NULL;
static TaskHandle_t xHDC1080Tsk      = NULL;
static TaskHandle_t xStepperTsk		 = NULL;

// Queue handles
static QueueHandle_t xSevSegQueue  = NULL;
static QueueHandle_t xBtnQueue	   = NULL;
static QueueHandle_t xHDCQueue	   = NULL;
static QueueHandle_t xStepperQueue = NULL;

// Semaphore handles
static SemaphoreHandle_t xMvmtStateSem = NULL;
static SemaphoreHandle_t xDsplStateSem = NULL;
static SemaphoreHandle_t xDsplModeSem  = NULL;

// This enum represents the possible main-states of the program
static enum MovementStateEnum {
	MV_ST_IDLE,
	MV_ST_TMP,
	MV_ST_HMD,
	MV_ST_CLOCKWISE,
	MV_ST_COUNTERCW,
	MV_ST_BACK_N_FORTH
} mvmtState = MV_ST_IDLE;

static enum DisplayStateEnum {
	DS_ST_IDLE,
	DS_ST_TMP,
	DS_ST_HMD,
	DS_ST_MTR
} dsplState = DS_ST_IDLE;

static enum DisplayModeEnum {
	DM_DEC,
	DM_HEX
} dsplMode = DM_DEC;


typedef struct BtnMsgStruct {
	int pin;
	int tick;
} BtnMsg;

// Struct for seven segment display messages
typedef struct SevSegMsgStruct {
    int l;
    int r;
    int display_ticks;
} SevSegMsg;

typedef struct HDCReadingStruct {
	int tmpC;
	int tmpF;
	int hmd;
} HDCReading;

//---------------- MAIN ----------------//
int main()
{
	// Initialization
	stdio_init_all();
	gpio_init_pins();
	i2c_init_HDC1080();

	// Enable ISR callbacks
	gpio_set_irq_enabled_with_callback(BTN1, GPIO_IRQ_EDGE_RISE, true, &vButtonISR);
	gpio_set_irq_enabled              (BTN2, GPIO_IRQ_EDGE_RISE, true);
	gpio_set_irq_enabled              (BTN3, GPIO_IRQ_EDGE_RISE, true);

	// Wait until a serial port to print to is connected
	while (!tud_cdc_connected()) { sleep_ms(100); }
	cls();
	sleep_ms(10);
	printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n┃ Serial debugging connected! ┃\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");

	// Create queues
    xSevSegQueue = xQueueCreate(11, sizeof(SevSegMsg));
	xBtnQueue = xQueueCreate(5, sizeof(BtnMsg));
	xHDCQueue = xQueueCreate(1, sizeof(HDCReading));
	xStepperQueue = xQueueCreate(1, sizeof(int));

	// Create semaphores
	xMvmtStateSem = xSemaphoreCreateBinary();
	xDsplStateSem = xSemaphoreCreateBinary();
	xDsplModeSem  = xSemaphoreCreateBinary();

	// Init smaphores to available
	xSemaphoreGive(xMvmtStateSem);
	xSemaphoreGive(xDsplStateSem);
	xSemaphoreGive(xDsplModeSem);

	// Create the tasks
	xTaskCreate(vMainTsk,	          "Main_Task",            	256, NULL, 1, &xMainTsk);
	xTaskCreate(vButtonTsk,           "Button_Task",            256, NULL, 6, &xButtonTsk);
	xTaskCreate(vSwitcherTsk,		  "Switcher_Task",			256, NULL, 3, &xSwitcherTsk);
	xTaskCreate(vSevSegTsk,           "Sev_Seg_Task",           256, NULL, 5, &xSevSegTsk);
	xTaskCreate(vHDC1080ConnectorTsk, "HDC1080_Connector_Task", 256, NULL, 7, &xHDC1080CScanTsk);
	xTaskCreate(vHDC1080Tsk,       	  "HDC1080_Task",           256, NULL, 2, &xHDC1080Tsk);
	xTaskCreate(vStepperTsk,       	  "Stepper_Task",           256, NULL, 4, &xStepperTsk);

	// Start the task scheduler
	vTaskStartScheduler();

	// Allow main to run indefinitely 
	while(1);
}


//---------------- UTILITY FUNCTIONS ----------------//

// ---- 7 SEG DISPLAY ---- //

// Puts a digit onto one of the two 7-seg displays
static void putDigit(int digit, bool leftSide)
{
    gpio_put(SevenSegCC1, !leftSide);
    gpio_put(SevenSegCC2, leftSide);

    // Draw the digit (the passed arg mod 16)
    switch (digit % 16) {
        case 0: put7Pins(1, 1, 1, 1, 1, 1, 0); break;
        case 1: put7Pins(0, 1, 1, 0, 0, 0, 0); break;
        case 2: put7Pins(1, 1, 0, 1, 1, 0, 1); break;
        case 3: put7Pins(1, 1, 1, 1, 0, 0, 1); break;
        case 4: put7Pins(0, 1, 1, 0, 0, 1, 1); break;
        case 5: put7Pins(1, 0, 1, 1, 0, 1, 1); break;
        case 6: put7Pins(1, 0, 1, 1, 1, 1, 1); break;
        case 7: put7Pins(1, 1, 1, 0, 0, 0, 0); break;
        case 8: put7Pins(1, 1, 1, 1, 1, 1, 1); break;
        case 9: put7Pins(1, 1, 1, 1, 0, 1, 1); break;
        case 10: put7Pins(1, 1, 1, 0, 1, 1, 1); break;
        case 11: put7Pins(0, 0, 1, 1, 1, 1, 1); break;
        case 12: put7Pins(1, 0, 0, 1, 1, 1, 0); break;
        case 13: put7Pins(0, 1, 1, 1, 1, 0, 1); break;
        case 14: put7Pins(1, 0, 0, 1, 1, 1, 1); break;
        case 15: put7Pins(1, 0, 0, 0, 1, 1, 1); break;
        case -1: default: put7Pins(0, 0, 0, 0, 0, 0, 0); // Blanks the screen by default
    }
}

// Clears the digits of the 7-seg displays
static void clearDigits()
{
    gpio_put(SevenSegCC1, 1);
    gpio_put(SevenSegCC2, 1);
    gpio_put(SevenSegDP, 0);
    put7Pins(0, 0, 0, 0, 0, 0, 0);
}

// This is a subroutine used in the switch-case statement
// in putDigit (above), to reduce code duplication.
static void put7Pins(int A, int B, int C, int D, int E, int F, int G)
{
    gpio_put(SevenSegA, A);
    gpio_put(SevenSegB, B);
    gpio_put(SevenSegC, C);
    gpio_put(SevenSegD, D);
    gpio_put(SevenSegE, E);
    gpio_put(SevenSegF, F);
    gpio_put(SevenSegG, G);
}

// ---- I2C BUS ---- //

// Function to initialize the I2C bus for the HDC1080 sensor
static void i2c_init_HDC1080()
{
	// Set up I2C1 (the default SDA and SCL pins) functionality
	i2c_init(I2C_PORT, 100 * 1000); 
	gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
	gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
	gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

	// Make the I2C pins available to pico
	bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN,PICO_DEFAULT_I2C_SCL_PIN,GPIO_FUNC_I2C));
}

// Function to connect to the HDC1080 sensor on the I2C bus
// * Returns false if the sensor is not detected or if there was an error
static bool i2c_connect_HDC1080()
{
	// Fields to store needed information
	int config, sn1, sn2, sn3, mfID, dvID;

	// Read in the registers' data
	if ((mfID = read_hdc_reg(MFID_REG)) != TI_MFID
	||  (dvID = read_hdc_reg(DVID_REG)) != HDC1080_DVID) return false;
	config = read_hdc_reg(CNF_REG);
	sn1 = read_hdc_reg(SN1_REG);
	sn2 = read_hdc_reg(SN2_REG);
	sn3 = read_hdc_reg(SN3_REG);

	// If any of the above register reads failed, return false
	if (config == -1 || sn1 == -1 || sn2 == -1 || sn3 == -1 || mfID == -1 || dvID == -1) return false;

	// Print out the data that was just read
	// printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
	// printf("\n┃ The HDC1080 sensor was detected.        ┃");
	// printf("\n┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫");
	// printf("\n┃\tConfiguration:    0x%02x\t  ┃", config   );
	// printf("\n┃\tSerial ID:        %02x-%02x-%02x\t  ┃", sn1, sn2, sn3);
	// printf("\n┃\tManufacturer ID:  0x%02x\t  ┃", mfID     );
	// printf("\n┃\tDevice ID:        0x%02x\t  ┃", dvID     );
	// printf("\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");
	printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n┃ The HDC1080 sensor was detected.        ┃\n┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n┃\tConfiguration:    0x%02x\t  ┃\n┃\tSerial ID:        %02x-%02x-%02x\t  ┃\n┃\tManufacturer ID:  0x%02x\t  ┃\n┃\tDevice ID:\t  0x%02x\t  ┃\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n", config, sn1, sn2, sn3, mfID, dvID);

	// HDC1080 found: return success
	return true;
}

// Reads from a register on the HDC1080 using an i2c_w call and an i2c_r call
int read_hdc_reg(enum HDC1080_REG reg)
{
	uint8_t val = reg;  // used to send desired address
	uint8_t buf[2];     // used to store received data

	// Perform the read
	if (!i2c_w(&val, 1)) return -1;
	// (If we're reading temperature or humidity, delay so that the reading can happen)
	if (reg == TMP_REG || reg == HMD_REG) vTaskDelay(20 / portTICK_PERIOD_MS);
	if (!i2c_r(buf, 2)) return -1;

	// If we're reading temperature or humidity, return the value after appropriate conversion
	if (reg == TMP_REG) return round(((buf[0]<<8|buf[1]) / 65536.0) * 165.0 - 40.0);
	if (reg == HMD_REG) return round(((buf[0]<<8|buf[1]) / 65536.0) * 100.0);

	// Otherwise return the raw value
	return buf[0]<<8|buf[1];
}

// Shortcut function for writing to the I2C bus
static bool i2c_w(const uint8_t *src, size_t len)
{
	int ret = i2c_write_blocking(I2C_PORT, I2C_ADDRESS, src, len, false);
	return (ret != PICO_ERROR_GENERIC && ret != PICO_ERROR_TIMEOUT);
}

// Shortcut function for reading from the I2C bus
static bool i2c_r(uint8_t *dst, size_t len)
{
	int ret = i2c_read_blocking(I2C_PORT, I2C_ADDRESS, dst, len, false);
	return (ret != PICO_ERROR_GENERIC && ret != PICO_ERROR_TIMEOUT);
}

// ---- OTHER ---- //

// Utility function to init all the needed pins
static void gpio_init_pins()
{
	for (int i = 0; i < NUM_IN_PINS; i++) {
		gpio_init(IN_PINS[i]);
		gpio_set_dir(IN_PINS[i], GPIO_IN);
	}
    for (int i = 0; i < NUM_OUT_PINS; i++) {
        gpio_init(OUT_PINS[i]);
        gpio_set_dir(OUT_PINS[i], GPIO_OUT);
    }
}

// Function that clears standard output
static void cls()
{
	for (int n = 0; n < 100; n++) printf("\n");
	printf("\e[%d;%df",1,1);
}


//---------------- ISRs ----------------//

static TickType_t lastBtnRegTick;
static void vButtonISR(uint gpio, uint32_t events)
{
	static BaseType_t xHPTW = pdFALSE;

	// Check that the last button register was sufficiently long ago to fliter out bouncing
	if (lastBtnRegTick < xTaskGetTickCountFromISR() - BTN_REG_TICK_GAP) {

		// Register a button press (record the time of registration and notify the button task)
		lastBtnRegTick = xTaskGetTickCountFromISR();
		xTaskNotifyFromISR(xButtonTsk, gpio, eSetValueWithOverwrite, &xHPTW);
	}
}


//---------------- TASKS ----------------//

static void vMainTsk()
{
	// printf("[Main_Task]: Entry\n");
	HDCReading reading;
	SevSegMsg msg;
	msg.display_ticks = 2;

	while (true) {
		vTaskDelay(2);

		xSemaphoreTake(xDsplStateSem, portMAX_DELAY);
		switch (dsplState) {
			int val;
			case DS_ST_IDLE: break;
			case DS_ST_TMP:
			case DS_ST_HMD:
			case DS_ST_MTR:
				xQueuePeek(xHDCQueue, &reading, 0);
				xQueuePeek(xStepperQueue, &val, 0);

				if (dsplState == DS_ST_TMP) val = reading.tmpF;
				else if (dsplState == DS_ST_HMD) val = reading.hmd;

				if (val == -1) {
					msg.l = -1;
					msg.r = -1;
				} else {
					xSemaphoreTake(xDsplModeSem, portMAX_DELAY);
					if (dsplMode) {
						// Display with hex
						msg.l = (val - (val % 16))/16;
						msg.r = val % 16;
					} else {
						// Display with decimal
						msg.l = (val - (val % 10))/10;
						msg.r = val % 10;
					}
					xSemaphoreGive(xDsplModeSem);
				}
				if (dsplState == DS_ST_MTR) msg.l = -1;
				xQueueSend(xSevSegQueue, &msg, 0);
				break;
		}
		xSemaphoreGive(xDsplStateSem);
	}
}

// This task handles messages from the button ISRs
static void vButtonTsk()
{
	// printf("[Button_Task]: Entry\n");
	BaseType_t xResult;
	uint32_t ulNotifiedValue = 0;
	BtnMsg msg;

	while (true) {

		// Wait for a button press
		xResult = xTaskNotifyWait(pdFALSE, pdFALSE, &ulNotifiedValue, portMAX_DELAY);

		msg.pin = ulNotifiedValue;
		msg.tick = xTaskGetTickCount();

		// Send the button event message
		xQueueSend(xBtnQueue, &msg, 0);
	}
}

static void vSwitcherTsk()
{
	// printf("[Switcher_Task]: Entry\n");
	BtnMsg bm1, bm2;
	int pressCount;
	bool failed;
	SevSegMsg msg;
	msg.display_ticks = 500 / portTICK_PERIOD_MS;
	msg.l = -1;
	msg.r = -1;

	while (true) {

		pressCount = 0;
		failed = false;

		// Wait for an initial button press
		xQueueReceive(xBtnQueue, &bm1, portMAX_DELAY);
		pressCount++;
		//printf("[Switcher_Task]: Starting button combo (presses: %i)\n", pressCount);

		while (xTaskGetTickCount() < bm1.tick + BTN_WINDOW_TICK_LENGTH) {
			if (xQueueReceive(xBtnQueue, &bm2, xTaskGetTickCount() - bm1.tick + BTN_WINDOW_TICK_LENGTH) == pdPASS) {
				if (bm1.pin == bm2.pin) {
					pressCount++;
					//printf("[Switcher_Task]: Continuing button combo (presses: %i)\n", pressCount);
				} else {
					failed = true;
					break;
				}
			}
		}

		if (failed) {
			xQueueReset(xBtnQueue);
			printf("[Switcher_Task]: Invalid button combo.\n");
			continue;
		}

		switch (pressCount) {
			case 1:
				switch (bm1.pin) {
					case BTN1:
						// Move stepper on temperature
						printf("[Switcher_Task]: Move stepper on temperature change\n");
						xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
						mvmtState = MV_ST_TMP;
						xSemaphoreGive(xMvmtStateSem);
						break;
					case BTN2:
						// Continuously move stepper clockwise
						printf("[Switcher_Task]: Continuously move stepper clockwise\n");
						xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
						mvmtState = MV_ST_CLOCKWISE;
						xSemaphoreGive(xMvmtStateSem);
						break;
					case BTN3:
						// Change display to show temperature
						printf("[Switcher_Task]: Change display to show temperature\n");
						xSemaphoreTake(xDsplStateSem, portMAX_DELAY);
						dsplState = DS_ST_TMP;
						xQueueSend(xSevSegQueue, &msg, 0);
						vTaskDelay(msg.display_ticks);
						xSemaphoreGive(xDsplStateSem);
						break;
				}
				break;
			case 2:
				switch (bm1.pin) {
					case BTN1:
						// Move stepper on humidity
						printf("[Switcher_Task]: Move stepper on humidity change\n");
						xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
						mvmtState = MV_ST_HMD;
						xSemaphoreGive(xMvmtStateSem);
						break;
					case BTN2:
						// Continouosly move stepper counterclockwise
						printf("[Switcher_Task]: Continouosly move stepper counterclockwise\n");
						xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
						mvmtState = MV_ST_COUNTERCW;
						xSemaphoreGive(xMvmtStateSem);
						break;
					case BTN3:
						// Change display to show humidity
						printf("[Switcher_Task]: Change display to show humidity\n");
						xSemaphoreTake(xDsplStateSem, portMAX_DELAY);
						dsplState = DS_ST_HMD;
						xQueueSend(xSevSegQueue, &msg, 0);
						vTaskDelay(msg.display_ticks);
						xSemaphoreGive(xDsplStateSem);
						break;
				}
				break;
			case 3:
				switch (bm1.pin) {
					case BTN1:
						// Stop everything and display "EE" on the 7-seg display
						printf("[Switcher_Task]: Stop everything and display \"EE\" on the 7-seg display\n");
						msg.l = 14;
						msg.r = 14;
						msg.display_ticks = 5 * 1000 / portTICK_PERIOD_MS;
						xQueueReset(xSevSegQueue);
						xQueueSend(xSevSegQueue, &msg, 0);
						xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
						mvmtState = MV_ST_IDLE;
						xSemaphoreGive(xMvmtStateSem);
						xSemaphoreTake(xDsplStateSem, portMAX_DELAY);
						dsplState = DS_ST_IDLE;
						xSemaphoreGive(xDsplStateSem);
						xSemaphoreTake(xDsplModeSem, portMAX_DELAY);
						dsplMode = DM_DEC;
						xSemaphoreGive(xDsplModeSem);
						vTaskDelay(msg.display_ticks);
						xQueueReset(xSevSegQueue);
						msg.l = -1;
						msg.r = -1;
						msg.display_ticks = 500 / portTICK_PERIOD_MS;
						break;
					case BTN2:
						// Back and forth movement mode
						printf("[Switcher_Task]: Back and forth movement mode\n");
						xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
						mvmtState = MV_ST_BACK_N_FORTH;
						xSemaphoreGive(xMvmtStateSem);
						break;
					case BTN3:
						// Change display to show motor status
						printf("[Switcher_Task]: Change display to show motor status\n");
						xSemaphoreTake(xDsplStateSem, portMAX_DELAY);
						dsplState = DS_ST_MTR;
						xQueueSend(xSevSegQueue, &msg, 0);
						vTaskDelay(msg.display_ticks);
						xSemaphoreGive(xDsplStateSem);
						break;
				}
				break;
			case 4:
				if (bm1.pin == BTN1) {
					xSemaphoreTake(xDsplModeSem, portMAX_DELAY);
					dsplMode = !dsplMode;
					printf("[Switcher_Task]: Switch display mode to %s\n", dsplMode ? "hexadecimal" : "decimal");
					xQueueSend(xSevSegQueue, &msg, 0);
					vTaskDelay(msg.display_ticks);
					xSemaphoreGive(xDsplModeSem);
					break;
				};
			default: printf("[Switcher_Task]: Too many presses.\n");
		}
	}
}

// This task receives messages from the queue and
// displays them on the seven segment display.
static void vSevSegTsk()
{
	// printf("[Sev_Seg_Task]: Entry\n");
    SevSegMsg msg;
    TickType_t until;

    while (true) {
        
        // Block until a message is received
        xQueueReceive(xSevSegQueue, &msg, portMAX_DELAY);
        until = xTaskGetTickCount() + msg.display_ticks;

        // Display the received message until its time is up
        while (xTaskGetTickCount() < until) {

            putDigit(msg.l, true);  // Put the left digit
            vTaskDelay(1);          // Delay this task so that left digit is visible

            putDigit(msg.r, false); // Put the right digit
            vTaskDelay(1);          // Delay this task so that right digit is visible

            // Check if the queue has filled up
            if (uxQueueSpacesAvailable(xSevSegQueue) == 0) {
                printf("[Sev_Seg_Tsk]: The message queue overflowed! Waiting 5 seconds, then clearing.\n");

                // Display "OF" (overflow) on the display for 5 seconds
                until = xTaskGetTickCount() + 5 * 1000 / portTICK_PERIOD_MS;
                while (xTaskGetTickCount() < until) {
                    putDigit(0,  true);  vTaskDelay(1);
                    putDigit(15, false); vTaskDelay(1);
                }

                // Clear the queue and break the while loop
                printf("[Sev_Seg_Tsk]: Clearing the queue.\n");
                xQueueReset(xSevSegQueue);
                break;
            }
        }

        // Clear the display
        clearDigits();
		// vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void vHDC1080ConnectorTsk()
{
	// printf("[HDC1080_Connector_Task]: Entry\n");
	while (true) {
		// Wait for HDC Task to request a connection
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		printf("[HDC1080_Connector_Task]: Looking for the HDC1080 sensor...\n");

		// Retry scanning for the sensor until it works
		while (!i2c_connect_HDC1080()) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			printf("[HDC1080_Connector_Task]: Retrying...\n");
		}

		// If we reach here, the sensor has been detected
		printf("[HDC1080_Connector_Task]: HDC1080 connected!\n");
		vTaskDelay(2000 / portTICK_PERIOD_MS);

		// Notify the HDC Task that a connection has been made
		xTaskNotifyGive(xHDC1080Tsk);
	}
}

// The driver task for the HDC1080, which prints temperature / humidity readings
static void vHDC1080Tsk()
{
	// printf("[HDC1080_Task]: Entry\n");
	// Fields for temp in C, temp in F, and humidity
	HDCReading reading;
	reading.tmpC = -1;
	reading.tmpF = -1;
	reading.hmd = -1;

	xQueueOverwrite(xHDCQueue, &reading);

	// Tell the HDC Connector Task to wake up (request a connection)
	printf("[HDC1080_Task]: Requesting a connection.\n");
	xTaskNotifyGive(xHDC1080CScanTsk);

	// Then wait for it to tell us that the connection has been made
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	printf("[HDC1080_Task]: Connection Established.\n");

	while (true) {

		// Read the temperature and humidity
		reading.tmpC = read_hdc_reg(TMP_REG);
		reading.hmd = read_hdc_reg(HMD_REG);

		// Re-init the sensor if the register read failed
		if (reading.tmpC == -1 || reading.hmd == -1) {
			printf("\n[HDC1080_Task]: Connection to the HDC1080 was lost. Trying to reconnect.\n\n");
			// Tell the HDC Connector Task to wake up (request a connection)
			xTaskNotifyGive(xHDC1080CScanTsk);

			// Then wait for it to tell us that the connection has been made
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		} else {
			// Readings succeeded

			// Calculate the temperature in F
			reading.tmpF = (reading.tmpC * 1.8) + 32;

			// Overwrite the queue with the most recent sensor reading
			xQueueOverwrite(xHDCQueue, &reading);

			// Delay to take a reading only every so often
			vTaskDelay(100 / portTICK_PERIOD_MS);
		} 
	}
}

static void vStepperTsk()
{
	HDCReading reading, readingPrev;
	TickType_t changeTime;

	int curStep = 0;
	int counter = 0;
	int direction = 1;

	while (true) {
		xSemaphoreTake(xMvmtStateSem, portMAX_DELAY);
		// printf("[Stepper_Task]: mvmtState = %i\n", mvmtState);
		switch (mvmtState) {
			case MV_ST_IDLE: break;
			case MV_ST_HMD:
				xQueuePeek(xHDCQueue, &reading, 0);
				if (readingPrev.hmd != reading.hmd) {
					changeTime = xTaskGetTickCount();
				} else if (xTaskGetTickCount() < changeTime + 100) {
					if (--curStep < 0) curStep = 3;
				}
				readingPrev = reading;
				break;
			case MV_ST_TMP:
				xQueuePeek(xHDCQueue, &reading, 0);
				if (readingPrev.tmpF != reading.tmpF) {
					changeTime = xTaskGetTickCount();
				} else if (xTaskGetTickCount() < changeTime + 100) {
					if (--curStep < 0) curStep = 3;
				}
				readingPrev = reading;
				break;
			case MV_ST_CLOCKWISE:
				if (--curStep < 0) curStep = 3; 
				break;
			case MV_ST_COUNTERCW:
				if (++curStep > 3) curStep = 0;
				break;
			case MV_ST_BACK_N_FORTH:
				if (direction > 0) {
					if (--curStep < 0) curStep = 3;
				} else {
					if (++curStep > 3) curStep = 0;
				}
				counter += direction;
				if (counter == 0 || counter == 2080) direction *= -1;
				break;
		}
		xSemaphoreGive(xMvmtStateSem);

		xQueueOverwrite(xStepperQueue, &curStep);

		gpio_put(STEPPER_PIN_1, steps[curStep][0]);
		gpio_put(STEPPER_PIN_2, steps[curStep][1]);
		gpio_put(STEPPER_PIN_3, steps[curStep][2]);
		gpio_put(STEPPER_PIN_4, steps[curStep][3]);

		vTaskDelay(2);
	}
}