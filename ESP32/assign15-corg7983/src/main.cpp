/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 14 - ESP32 Web Server     *
 * File:        main.cpp                             *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */


//---------------- INCLUDES ----------------//

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <HTTPClient.h>
// #include <Arduino_JSON.h>


//---------------- CONSTANTS / DEFINITIONS ----------------//

const char* NET_SSID = "FridaysNetwork2G";
const char* NET_PASS = "localpass";
const String SERVER_IP = "http://52.23.160.25:5000/IOTAPI/";


//---------------- PROTOTYPES ----------------//

void connect();
String runCommand();


//---------------- SETUP ----------------//

void setup() {

  // Initialization
  Serial.begin(115200);

  // Connect to WiFi
  connect();
  Serial.println(runCommand());
}


//---------------- LOOP ----------------//

void loop() {}


//---------------- TASKS ----------------//




//---------------- FUNCTIONS ----------------//

void connect() {
  // Connect to the WiFi network
  Serial.print("Attempting connection to ");
  Serial.print(NET_SSID);
  Serial.println();
  WiFi.begin(NET_SSID, NET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }

  // Print the IP that was obtained
  Serial.println();
  Serial.print("Connected to ");
  Serial.print(NET_SSID);
  Serial.print(". IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

String runCommand() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[runCommand]: WiFi unexpectedly disconnected.");
    return "";
  }

  WiFiClient client;
  HTTPClient http;
  String address = SERVER_IP + "DetectServer";

  http.begin(client, address);
  http.addHeader("Content-Type", "application/json");
  
  String reqData = "{\"key\":\"2436e8c114aa64ee\"}";

  int postCode = http.POST(reqData);
  if (postCode <= 0) {
    Serial.print("[runCommand]: Post error (");
    Serial.print(http.errorToString(postCode));
    Serial.println(")");
    return "";
  } else {
    Serial.print("[runCommand]: Post response: ");
    Serial.println(postCode);
  }

  int getCode = http.GET();
  if (getCode <= 0) {
    Serial.print("[runCommand]: Get error (");
    Serial.print(http.errorToString(getCode)); // HTTPC_ERROR_SEND_HEADER_FAILED
    Serial.println(")");
    return "";
  } else {
    Serial.print("[runCommand]: Get response: ");
    Serial.println(getCode);
  }
  
  String response = http.getString();
  http.end();
  return response;
}
