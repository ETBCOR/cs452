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
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML>
<html>
  <head>
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
    <meta http-equiv="refresh" content="5; /" />
    <link rel=\"icon\" href=\"data:,\" />
    <title>ESP32 IOT Web Server</title>
  </head>
  <body>
    <h1>ESP32 IOT Web Server</h1>
    <h2>IOT Commands:</h2>
    <div style="display:inline-block">
      <button onclick="window.location.href='/Detect';">Detect</button>
      <button onclick="window.location.href='/Register';">Register</button>
      <button onclick="window.location.href='/QueryNow';">Query Now</button>
      <button onclick="window.location.href='/SendNow';">Send Now</button>
      <button onclick="window.location.href='/Shutdown';">Shutdown</button>
    </div>
    <h2>Device Commands:</h2>
    <div style="display:inline-block">
      <button onclick="window.location.href='/Flash';">Flash</button>
      <button onclick="window.location.href='/RotQCW';">RotQCW</button>
      <button onclick="window.location.href='/RotQCCW';">RotQCCW</button>
    </div>
    <h2>Latest Data</h2>
    <p>Temperature:     %f</p>
    <p>Humidity:        %f</p>
    <p>Light:           %f</p>
  </body>
</html>

)rawliteral";
const char index_html_no_data[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML>
<html>
  <head>
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
    <meta http-equiv="refresh" content="5; /" />
    <link rel=\"icon\" href=\"data:,\" />
    <title>ESP32 IOT Web Server</title>
  </head>
  <body>
    <h1>ESP32 IOT Web Server</h1>
    <h2>IOT Commands:</h2>
    <div style="display:inline-block">
      <button onclick="window.location.href='/Detect';">Detect</button>
      <button onclick="window.location.href='/Register';">Register</button>
      <button onclick="window.location.href='/QueryNow';">Query Now</button>
      <button onclick="window.location.href='/SendNow';">Send Now</button>
      <button onclick="window.location.href='/Shutdown';">Shutdown</button>
    </div>
    <h2>Device Commands:</h2>
    <div style="display:inline-block">
      <button onclick="window.location.href='/Flash';">Flash</button>
      <button onclick="window.location.href='/RotQCW';">RotQCW</button>
      <button onclick="window.location.href='/RotQCCW';">RotQCCW</button>
    </div>
    <h2>Latest Data</h2>
    <p>No data has been read yet!</p>
  </body>
</html>

)rawliteral";


//---------------- DEFINITIONS ----------------//

#define TIMEOUT 2000

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
#define QUATER_TURN 512  // steps needed for a quater turn
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

// 
#define SCL 20
#define SDA 22


//---------------- ENUMS / STRUCTS ----------------//

enum IOTCmd { DETECT, REGISTER, QUERY, IOTDATA, IOTSHUTDOWN };

struct HDCdata {
  double temp, humid;
};

struct GPSdata {
  double lat, lon, alt;
};

struct fullData {
  double temp, humid, light;
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
void tServer(void *);
void tHDC(void *);
void tSI (void *);
void tGPS(void *);
void tStepper(void *);
void tLED(void *);


//---------------- GLOBAL HANDLES ----------------//

// Queue handles
QueueHandle_t qIOTcmd;
QueueHandle_t qServer;
QueueHandle_t qCfreq;
QueueHandle_t qSfreq;
QueueHandle_t qHDCdata;
QueueHandle_t qSIdata;
// QueueHandle_t qGPSdata;
QueueHandle_t qStepper;

// Pinger tasks handles
TaskHandle_t thCping;
TaskHandle_t thSping;

// Tasks handles
TaskHandle_t thIOT;
TaskHandle_t thServer;
TaskHandle_t thHDC;
TaskHandle_t thSI;
// TaskHandle_t thGPS;
TaskHandle_t thStepper;
TaskHandle_t thLED;

// Other
ClosedCube_HDC1080 hdc1080;
Adafruit_SI1145    si1145;
Adafruit_NeoPixel  pixels(NUM_PIXELS, PIXEL_PIN, NEO_GRBW + NEO_KHZ800);

BaseType_t tru = pdTRUE;
BaseType_t nah = pdFALSE;
TickType_t lastBtnRegTick;