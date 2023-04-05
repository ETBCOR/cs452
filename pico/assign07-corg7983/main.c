/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 7 - Temp/Humid to Serial  *
 * File:        main.c                               *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */


//---------------- INCLUDES ----------------//
#include <math.h>
#include <FreeRTOS.h>
#include <task.h>
#include <tusb.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"


//---------------- DEFINITIONS ----------------//

// Information for the HDC1080 sensor and I2C bus mode
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

// This enum represents a register to be read
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

// LED PIN
static const uint LED_PIN = PICO_DEFAULT_LED_PIN;


//---------------- PROTOTYPES / HANDLES ----------------//

// Utility function prototypes
static void i2c_init_HDC1080();
static bool i2c_connect_HDC1080();
static int  read_hdc_reg(enum HDC1080_REG);
static bool i2c_w(const uint8_t *, size_t);
static bool i2c_r(uint8_t *, size_t);
static void cls();

// Task prototypes
static void vHDC1080ConnectorTsk();
static void vHDC1080Tsk();
static void vBlinkerTsk();

// Task/semaphore handles
static TaskHandle_t xHDC1080ConnectorTsk = NULL;
static TaskHandle_t xHDC1080Tsk          = NULL;
static TaskHandle_t xBlinkerTsk          = NULL;


//---------------- MAIN ----------------//
int main()
{
	// Initialization
	stdio_init_all();
	i2c_init_HDC1080();
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	// Wait until a serial port to print to is connected
	while (!tud_cdc_connected()) { sleep_ms(100); }
	cls();
	sleep_ms(10);
	printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n┃ Serial debugging connected! ┃\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n\n");

	// Create the tasks
	xTaskCreate(vHDC1080ConnectorTsk, "HDC1080_Connector_Task", 256, NULL, 2, &xHDC1080ConnectorTsk);
	xTaskCreate(vHDC1080Tsk, "HDC1080_Task", 256, NULL, 1, &xHDC1080Tsk);
	xTaskCreate(vBlinkerTsk, "Blinker_Task", 256, NULL, 1, &xBlinkerTsk);

	// Start the task scheduler
	vTaskStartScheduler();

	// Allow main to run indefinitely 
	while(1){}
}


//---------------- UTILITY FUNCTIONS ----------------//

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

// Function that clears standard output
static void cls()
{
	for (int n = 0; n < 100; n++) printf("\n");
	printf("\e[%d;%df",1,1);
}


//---------------- TASKS ----------------//

static void vHDC1080ConnectorTsk()
{
	while (true) {
		// Wait for HDC Task to request a connection
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		printf("Looking for the HDC1080 sensor...\n");

		// Retry scanning for the sensor until it works
		while (!i2c_connect_HDC1080()) {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			printf("Retrying...\n");
		}

		// If we reach here, the sensor has been detected
		printf("HDC1080 connected!\n");
		vTaskDelay(2000 / portTICK_PERIOD_MS);

		// Notify the HDC Task that a connection has been made
		xTaskNotifyGive(xHDC1080Tsk);
	}
}

// The driver task for the HDC1080, which prints temperature / humidity readings
static void vHDC1080Tsk()
{
	// Fields for temp in C, temp in F, and humidity
	int tmpC, tmpF, hmd;

	// Tell the HDC Connector Task to wake up (request a connection)
	xTaskNotifyGive(xHDC1080ConnectorTsk);

	// Then wait for it to tell us that the connection has been made
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	while (true) {

		// Delay to slow output
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		// notify the blink task to unblock
		xTaskNotifyGive(xBlinkerTsk);

		// Read the temperature and humidity
		tmpC = read_hdc_reg(TMP_REG);
		hmd = read_hdc_reg(HMD_REG);

		// Re-init the sensor if the register read failed
		if (tmpC == -1 || hmd == -1) {
			printf("\nConnection to the HDC1080 was lost. Trying to reconnect.\n\n");
			// Tell the HDC Connector Task to wake up (request a connection)
			xTaskNotifyGive(xHDC1080ConnectorTsk);

			// Then wait for it to tell us that the connection has been made
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		} else {
			// Readings succeeded

			// Calculate the temperature in F
			tmpF = (tmpC * 1.8) + 32;

			// Print the information we just read / calculated
			/*  printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
				printf("\n┃ Temperature:\t%i°C (%i°F)\t┃", tmpC, tmpF);
				printf("\n┃ Humidity:\t%i\t\t┃", hmd);
				printf("\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n"); */
			printf("\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n┃ Temperature:\t%i°C (%i°F)\t┃\n┃ Humidity:\t%i\t\t┃\n┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n", tmpC, tmpF, hmd);
		} 
	}
}

// This task waits for a notification and blinks the LED on for 3 ticks
static void vBlinkerTsk()
{
	while (true) {

		// block until notified to blink (by vCountTsk)
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// flash the LED on for 3 ticks
		gpio_put(LED_PIN, 1);
		vTaskDelay(3);
		gpio_put(LED_PIN, 0);
	}
}