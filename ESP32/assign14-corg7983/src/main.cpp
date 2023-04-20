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


//---------------- DEFINITIONS ----------------//

// Network credential constants
const char* NET_SSID = "FridaysNetwork2G";
const char* NET_PASS = "localpass";

// Network connection timeout
#define TIMEOUT 2000

// Dipswitch pin definitions
#define NUM_PINS 8
#define DIP0 37 // GP06
#define DIP1 14 // GP07
#define DIP2 20 // SCL
#define DIP3 22 // SDA
#define DIP4 13 // GP13
#define DIP5 26 // GP26
#define DIP6 8  // GP00
#define DIP7 7  // GP01
const int PINS[NUM_PINS] = {DIP0, DIP1, DIP2, DIP3, DIP4, DIP5, DIP6, DIP7};

const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML>
<html>
  <head>
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
  	<meta http-equiv="refresh" content="5" />
    <link rel=\"icon\" href=\"data:,\">
    <title>ESP32 Web Server Dipswitch Display</title>
  </head>
  <body>
    <h1>ESP32 Web Server Dipswitch Display</h1>
    <p>Dipswitch states:</p>
    <p>%i%i%i%i%i%i%i%i</p>
  </body>
</html>

)rawliteral";


//---------------- PROTOTYPES ----------------//

// Utility function to intialize pins
void init_pins();

void vWebServerTsk(void* parm);


//---------------- SETUP ----------------//

void setup() {

  // Initialization
  Serial.begin(115200);
  init_pins();

  Serial.println("/------------------------------------\\");
  Serial.println("| ESP32 Web Server Dipswitch Display |");
  Serial.println("\\------------------------------------/");

  // Create the tasks
  xTaskCreatePinnedToCore(vWebServerTsk, "Web_Server_Task", 4096, NULL, 1, NULL, 1);
}


//---------------- TASKS ----------------//

void vWebServerTsk(void* parm) {

  // Initialize webserver on port 80
  WiFiServer server(80);

  // Stores HTTP request
  String header;

  // Time variables
  unsigned long currTime = millis();
  unsigned long prevTime = 0;

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
  Serial.println("Connected to the WiFi network.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the web server
  server.begin();

  Serial.println("Web_Server_task start.");
  
  while (true) {

    // Listen for incoming clients
    WiFiClient client = server.available();

    if (client) {
      // A new client is trying to connect

      currTime = prevTime = millis();
      Serial.println("New client.");
      String currLine = ""; // holds incoming data from the client

      while (client.connected() && currTime - prevTime <= TIMEOUT) {
        // The connection is still running

        currTime = millis();
        if (client.available()) {

          char c = client.read();
          Serial.write(c);
          header += c;

          if (c == '\n') {
            // The end of a line has been reached

            if (currLine.length() == 0) {
              // The request has ended, so send a response
              client.println("HTTP/1.1 200 OK\nContent-type:text/html\nConnection: close\n");
              char index_text[1024];
              snprintf(index_text, sizeof(index_text), index_html, digitalRead(DIP0), digitalRead(DIP1), digitalRead(DIP2), digitalRead(DIP3), digitalRead(DIP4), digitalRead(DIP5), digitalRead(DIP6), digitalRead(DIP7));
              client.println(index_text);
              break; // break the while-client-connected loop
            } else {
              // The request hasn't ended, but the line has - so clear the line buffer
              currLine = "";
            }
          } else if (c != '\r') {
            // Any character other than the carridge return gets added to the line buffer
            currLine += c;
          }
        }
      }

      // Clear the header var
      header = "";

      // Close the connection
      client.stop();
      Serial.println("Client disconnected.");
      Serial.println();
    } // if client (is trying to connect)
  }
}


// Utility function to intialize pins
void init_pins() {
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(PINS[i], INPUT);
  }
}


//---------------- LOOP ----------------//

void loop() {}
