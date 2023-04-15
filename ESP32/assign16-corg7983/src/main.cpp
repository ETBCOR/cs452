/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 16 - FreeRTOS IOT Final   *
 * File:        main.cpp                             *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */


//---------------- INCLUDES ----------------//

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>


//---------------- CONSTANTS / ENUMS ----------------//

const char* NET_SSID = "FridaysNetwork2G";
const char* NET_PASS = "localpass";
const String SERVER_IP = "http://52.23.160.25:5000/IOTAPI/";
const String SECRET_KEY = "2436e8c114aa64ee";
const String IOTID = "1003";

enum eIOTCommand { DETECT, REGISTER, QUERY, IOTDATA, IOTSHUTDOWN };

struct IOTCommand {
  eIOTCommand cmd;
  String arg;
};

//---------------- PROTOTYPES ----------------//

// Functions
void connect();
String cmdToPath(eIOTCommand);

// Tasks
void tIOT(void *);

// Handles
TaskHandle_t thIOT;
QueueHandle_t qIOT;

//---------------- SETUP ----------------//

void setup()
{
  // Initialization
  Serial.begin(115200);
  delay(250);

  // Connect to WiFi
  connect();

  // Create queues
  qIOT = xQueueCreate(32, sizeof(IOTCommand));

  // Create tasks
  xTaskCreatePinnedToCore(tIOT, "IOT Task", 2048, NULL, 1, &thIOT, 1);

  IOTCommand cmd = { DETECT, "" };
  xQueueSend(qIOT, &cmd, portMAX_DELAY);
}


//---------------- LOOP ----------------//

void loop() {}


//---------------- TASKS ----------------//

void tIOT(void *)
{
  WiFiClient client;
  HTTPClient http;
  String address, authCode, reqData, response;
  IOTCommand command;
  JSONVar json;

  while (1) {
    xQueueReceive(qIOT, &command, portMAX_DELAY);
    Serial.print("[IOT Task]: Command recieved: ");
    Serial.println(cmdToPath(command.cmd));

    address = SERVER_IP + cmdToPath(command.cmd);
    http.begin(client, address);
    http.addHeader("Content-Type", "application/json");

    switch (command.cmd) {
      case DETECT:      reqData = "{\"key\":\"" + SECRET_KEY + "\"}"; break;
      case REGISTER:    reqData = "{\"key\":\"" + SECRET_KEY + "\",\"iotid\":\"" + IOTID + "\"}"; break;
      case QUERY:       reqData = "{\"auth_code\":\"" + authCode + "\",\"iotid\":\"" + IOTID + "\"}"; break;
      case IOTDATA:     reqData = "{\"auth_code\":\"" + authCode + "\"}"; break;
      case IOTSHUTDOWN: reqData = "{\"auth_code\":\"" + authCode + "\"}"; break;
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
      Serial.print("Post error: ");
      Serial.println(http.errorToString(rspCode));
      http.end();
      continue;
    }
    
    response = http.getString();
    http.end();
    json = JSON.parse(response);

    switch(command.cmd) {
      case DETECT:
        if (!json.hasOwnProperty("reply") || strcmp((const char*)json["reply"], "I am up and Running")) {
          Serial.println("[IOT Task]: Server could not be detected.");
        } else {
          // The server was successfully detected
          Serial.print("[IOT Task]: Server detected at ");
          Serial.print(SERVER_IP);
        }
       break;
      case REGISTER:
        // break;
      case QUERY:
        // break;
      case IOTDATA:
        // break;
      case IOTSHUTDOWN:
        // break;
      default:
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
    }
  }
}


//---------------- FUNCTIONS ----------------//

void connect()
{
  // Connect to the WiFi network
  Serial.print("\nAttempting connection to ");
  Serial.print(NET_SSID);
  WiFi.begin(NET_SSID, NET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(". ");
  }

  // Print the IP that was obtained
  Serial.print("connected (IP: ");
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
