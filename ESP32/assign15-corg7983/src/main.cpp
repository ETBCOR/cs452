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
#include <Arduino_JSON.h>


//---------------- CONSTANTS / DEFINITIONS ----------------//

const char* NET_SSID = "FridaysNetwork2G";
const char* NET_PASS = "localpass";
const char* SERVER_IP = "http://52.23.160.25:5000";


//---------------- PROTOTYPES ----------------//

void connect() {
  // Connect to the WiFi network
  Serial.print("Attempting connection to '");
  Serial.print(NET_SSID);
  Serial.println("'");
  WiFi.begin(NET_SSID, NET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }

  // Print the IP that was obtained
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(NET_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


//---------------- SETUP ----------------//

void setup() {

  // Initialization
  Serial.begin(115200);

  // Connect to WiFi
  connect();

  WiFiClient client;
  HTTPClient http;

  http.begin(client, SERVER_IP);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST("{\"key\":\"2436e8c114aa64ee\"}");

}

void loop() {}
