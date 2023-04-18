//---------------- INCLUDES ----------------//

#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include "Adafruit_SI1145.h"
#include "Adafruit_NeoPixel.h"


//---------------- CONSTANTS ----------------//

const char * NET_SSID   = "FridaysNetwork2G";
const char * NET_PASS   = "localpass";
const String SERVER_IP  = "http://52.23.160.25:5000/IOTAPI/";
const String SECRET_KEY = "2436e8c114aa64ee";
const String IOTID      = "1003";
const char * NTP_SERVER = "pool.ntp.org";


//---------------- DEFINITIONS ----------------//

#define DEFAULT_CHECK_SEND_FREQ 300
#define MIN_CHECK_SEND_FREQ 10

// Buttons
#define BUTTON_1 19 // GP19
#define BUTTON_2 15 // GP09
#define BUTTON_3 32 // GP08
#define BTN_REG_TICK_GAP 200 / portTICK_PERIOD_MS

// Stepper moter pins
#define STEPPER_PIN_1 12 // GP12
#define STEPPER_PIN_2 13 // GP13
#define STEPPER_PIN_3 27 // GP11
#define STEPPER_PIN_4 33 // GP10
#define QUATER_TURN 510  // steps needed for a quater turn
#define MS_BETWEEN_STEPS 20
const bool STEPPER_STEPS[4][4] = {
	{0, 0, 1, 1},
	{0, 1, 1, 0},
	{1, 1, 0, 0},
	{1, 0, 0, 1}
};

// Pixels
#define NUM_PIXELS 4
#define PIXEL_PIN 21

//---------------- ENUMS / STRUCTS ----------------//

enum IOTCmd { DETECT, REGISTER, QUERY, IOTDATA, IOTSHUTDOWN };

struct HDCdata {
  double temp, humid;
};


//---------------- PROTOTYPES ----------------//

// Functions
void initPins();
void initPixels();
void connectHDC();
void connectSI();
void connectWiFi();
String cmdToPath(IOTCmd);
String getTime();

// ISRs
void IRAM_ATTR isr1();
void IRAM_ATTR isr2();
void IRAM_ATTR isr3();

// Pinger tasks
void tCping(void *);
void tSping(void *);

// Tasks
void tIOT(void *);
void tHDC(void *);
void tSI(void *);
void tStepper(void *);
void tLED(void *);


//---------------- GLOBAL HANDLES ----------------//

// Queue handles
QueueHandle_t qIOTcmd;
QueueHandle_t qCfreq;
QueueHandle_t qSfreq;
QueueHandle_t qHDCdata;
QueueHandle_t qSIdata;
QueueHandle_t qStepper;

// Pinger tasks handles
TaskHandle_t thCping;
TaskHandle_t thSping;

// Tasks handles
TaskHandle_t thIOT;
TaskHandle_t thHDC;
TaskHandle_t thSI;
TaskHandle_t thStepper;
TaskHandle_t thLED;

// Other
ClosedCube_HDC1080 hdc1080;
Adafruit_SI1145 si1145;
Adafruit_NeoPixel pixels(NUM_PIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);

BaseType_t tru = pdTRUE;
BaseType_t nah = pdFALSE;
TickType_t lastBtnRegTick;