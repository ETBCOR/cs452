/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 16 - FreeRTOS IOT Final   *
 * File:        main.cpp                             *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "main.h"


//---------------- CONSTANTS ----------------//

const char*  NET_SSID   = "FridaysNetwork2G";
const char*  NET_PASS   = "localpass";
const String SERVER_IP  = "http://52.23.160.25:5000/IOTAPI/";
const String SECRET_KEY = "2436e8c114aa64ee";
const String IOTID      = "1003";


//---------------- ENUMS / STRUCTS ----------------//

enum eIOTCommand { DETECT, REGISTER, QUERY, IOTDATA, IOTSHUTDOWN };

struct IOTCommand {
  eIOTCommand cmd;
  String arg;
};

//---------------- PROTOTYPES ----------------//

// Functions
void connectHDC();
void connectWiFi();
String cmdToPath(eIOTCommand);

// Tasks
void tIOT(void *);
void tHDC(void *);
void tCheckPing(void *);
void tSendPing(void *);

// Other
QueueHandle_t qIOT;
ClosedCube_HDC1080 hdc1080;


//---------------- SETUP ----------------//

void setup()
{
  // Initialization
  Serial.begin(115200);
  connectHDC();
  connectWiFi();

  // Create queues
  qIOT = xQueueCreate(32, sizeof(IOTCommand));

  // Create tasks
  xTaskCreatePinnedToCore(tIOT, "IOT Task", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(tHDC, "HDC Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(tCheckPing, "Check Pinger", 2048, NULL, 10, NULL, 1);
  xTaskCreatePinnedToCore(tSendPing,   "Send Pinger", 2048, NULL, 10, NULL, 1);

  IOTCommand cmd = { QUERY, "" };
  xQueueSend(qIOT, &cmd, portMAX_DELAY);
  cmd.cmd = DETECT;
  xQueueSend(qIOT, &cmd, portMAX_DELAY);
  cmd.cmd = REGISTER;
  xQueueSend(qIOT, &cmd, portMAX_DELAY);
}

void loop() {}


//---------------- TASKS ----------------//

void tIOT(void *)
{
  WiFiClient client;
  HTTPClient http;
  String address, reqData, response;
  String authCode;
  IOTCommand command;
  JSONVar json;



  while (1) {
    xQueueReceive(qIOT, &command, portMAX_DELAY);
    if (authCode == "" && (command.cmd == QUERY || command.cmd == IOTDATA || command.cmd == IOTSHUTDOWN)) {
      Serial.print("[IOT Task]: Cannot run ");
      Serial.print(cmdToPath(command.cmd));
      Serial.println(" before authorizing with server.");
      continue;
    }
    Serial.print("[IOT Task]: Command recieved: ");
    Serial.println(cmdToPath(command.cmd));

    address = SERVER_IP + cmdToPath(command.cmd);
    http.begin(client, address);
    http.addHeader("Content-Type", "application/json");

    switch (command.cmd) {
      case DETECT:      reqData = "{\"key\":\""       + SECRET_KEY + "\"}";                             break;
      case REGISTER:    reqData = "{\"key\":\""       + SECRET_KEY + "\",\"iotid\":\"" + IOTID + "\"}"; break;
      case QUERY:       reqData = "{\"auth_code\":\"" + authCode   + "\",\"iotid\":\"" + IOTID + "\"}"; break;
      case IOTDATA:     reqData = "{\"auth_code\":\"" + authCode   + "\"," + command.arg + "}";         break;
      case IOTSHUTDOWN: reqData = "{\"auth_code\":\"" + authCode   + "\"}";                             break;
      default: reqData = "";
    }

    Serial.print("[IOT Task]: Posting: ");
    Serial.println(reqData);
    int rspCode = http.POST(reqData);

    if (rspCode > 0) {
      Serial.print("[IOT Task]: Post successful (code: ");
      Serial.print(rspCode);
      Serial.println(")");
    } else {
      Serial.print("[IOT Task]: Post error: ");
      Serial.println(http.errorToString(rspCode));
      http.end();
      continue;
    }
    
    response = http.getString();
    http.end();
    json = JSON.parse(response);

    switch(command.cmd) {
      case DETECT:
        if (!json.hasOwnProperty("reply") || strcmp((const char *)json["reply"], "I am up and Running")) {
          Serial.println("[IOT Task]: Server could not be detected.");
        } else {
          Serial.print("[IOT Task]: Server detected at ");
          Serial.println(SERVER_IP);
        }
       break;
      case REGISTER:
        if (!json.hasOwnProperty("auth_code")) {
          Serial.println("[IOT Task]: Failed to register with server.");
        } else {
          // Successfully registered with server
          authCode = (const char *)json["auth_code"];

          Serial.print("[IOT Task]: Authorization code obtained: ");
          Serial.println(authCode);
        }
        // break;
      case QUERY:
        // break;
      case IOTDATA:
        // break;
      case IOTSHUTDOWN:
      default:
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
    }
  }
}

void tHDC(void *)
{

  while(1);
}

void tCheckPing(void *)
{
  IOTCommand command = { QUERY, "" };

  while(1) {
    // xQueueSend(qIOT, &command, portMAX_DELAY);
    vTaskDelay(10 * 1000 / portTICK_PERIOD_MS); // 10 seconds
  }
}

void tSendPing(void *)
{
  while(1) {
    vTaskDelay(10 * 1000 / portTICK_PERIOD_MS); // 10 seconds
  }
}


//---------------- FUNCTIONS ----------------//

void connectHDC()
{
  Serial.print("\nScanning for HDC1080 on the I2C bus. ");

  hdc1080.begin(0x40);
  while (Wire.endTransmission() != 0 || hdc1080.readDeviceId() != 0x1050) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(". ");
    hdc1080.begin(0x40);
  }
	Serial.print("\nFound. Manufacturer ID=0x");
	Serial.print(hdc1080.readManufacturerId(), HEX); // 0x5449
	Serial.print(", Device ID=0x");
	Serial.println(hdc1080.readDeviceId(), HEX); // 0x1050
}

void connectWiFi()
{
  Serial.print("\nAttempting connection to ");
  Serial.print(NET_SSID);
  WiFi.begin(NET_SSID, NET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(". ");
  }
  Serial.print("\nConnected (IP: ");
  Serial.print(WiFi.localIP());
  Serial.println(")\n");
}

String cmdToPath(eIOTCommand cmd)
{
  switch (cmd) {
    case DETECT:      return "DetectServer";
    case REGISTER:    return "RegisterWithServer";
    case QUERY:       return "QueryServerForCommands";
    case IOTDATA:     return "IOTData";
    case IOTSHUTDOWN: return "IOTShutdown";
    default:          return "";
  }
}
