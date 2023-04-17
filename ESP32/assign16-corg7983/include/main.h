//---------------- INCLUDES ----------------//

#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include "Adafruit_SI1145.h"


//---------------- CONSTANTS ----------------//

const char*  NET_SSID   = "FRIDAYSPC";
const char*  NET_PASS   = "localpass";
const String SERVER_IP  = "http://52.23.160.25:5000/IOTAPI/";
const String SECRET_KEY = "2436e8c114aa64ee";
const String IOTID      = "1003";


//---------------- DEFINITIONS ----------------//

#define DEFAULT_CHECK_SEND_FREQ 300

// Buttons
#define BUTTONS1 19 // GP19
#define BUTTONS2 15 // GP09
#define BUTTONS3 32 // GP08
#define BTN_REG_TICK_GAP 200 / portTICK_PERIOD_MS

//---------------- ENUMS / STRUCTS ----------------//

enum IOTCmd { DETECT, REGISTER, QUERY, IOTDATA, IOTSHUTDOWN };

struct HDCdata {
  double temp, humid;
};


//---------------- PROTOTYPES ----------------//

// Functions
void initPins();
void connectHDC();
void connectSI();
void connectWiFi();
String cmdToPath(IOTCmd);

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

BaseType_t tru = pdTRUE;
BaseType_t nah = pdFALSE;
TickType_t lastBtnRegTick;