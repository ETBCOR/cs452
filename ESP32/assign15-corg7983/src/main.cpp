/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 15 - IOT API Setup        *
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

// GP08
#define BUTTONS3 32

const char* NET_SSID = "FridaysNetwork2G";
const char* NET_PASS = "localpass";
const String SERVER_IP = "http://52.23.160.25:5000/IOTAPI/";
const String SECRET_KEY = "2436e8c114aa64ee";
const String IOTID = "1003";

enum Command { DETECT, REGISTER, QUERY, IOTDATA, IOTSHUTDOWN };

//---------------- PROTOTYPES ----------------//

void connect();
String runCommand(Command command = DETECT, String reqData = "");
String commandToPath(Command);


//---------------- SETUP ----------------//

void setup()
{
  // Initialization
  pinMode(BUTTONS3, INPUT);
  Serial.begin(115200);
  delay(250);

  // Connect to WiFi
  connect();

  // Detect the server
  String response = runCommand();
  JSONVar json = JSON.parse(response);
  if (!json.hasOwnProperty("reply") || strcmp((const char*)json["reply"], "I am up and Running")) {
    Serial.println("Failed to detect IOT server.");
  } else {
    // The server was successfully detected
    Serial.print("IOT server successfully detected at ");
    Serial.println(SERVER_IP);

    // Register with the IOT server
    response = runCommand(REGISTER);
    Serial.print("Reigstration response: ");
    Serial.println(response);

    json = JSON.parse(response);
    if (!json.hasOwnProperty("auth_code")) {
      Serial.println("Failed to register with IOT server.");
    } else {
      // Successfully registered with server
      String authCode = json["auth_code"];

      Serial.print("Authorization code obtained: ");
      Serial.println(authCode);
      Serial.println("Press button 3 to shut down.");

      // Shut down the IOT connection when button 3 is pressed
      while (!digitalRead(BUTTONS3));
      Serial.println(runCommand(IOTSHUTDOWN, authCode));
    }
  }
}


//---------------- LOOP ----------------//

void loop() {}


//---------------- FUNCTIONS ----------------//

void connect()
{
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

String runCommand(Command command, String reqData)
{
  WiFiClient client;
  HTTPClient http;
  String address = SERVER_IP + commandToPath(command);

  Serial.print("\nConnecting to …/IOTAPI/");
  Serial.println(commandToPath(command));
  http.begin(client, address);
  http.addHeader("Content-Type", "application/json");

  switch (command) {
    case DETECT:      reqData = "{\"key\":\"" + SECRET_KEY + "\"}"; break;
    case REGISTER:    reqData = "{\"key\":\"" + SECRET_KEY + "\",\"iotid\":\"" + IOTID + "\"}"; break;
    case QUERY:       reqData = "{\"auth_code\":\"" + reqData + "\",\"iotid\":\"" + IOTID + "\"}"; break;
    case IOTDATA:     reqData = "{\"auth_code\":\"" + reqData + "\"}"; break;
    case IOTSHUTDOWN: reqData = "{\"auth_code\":\"" + reqData + "\"}"; break;
    default: reqData = "";
  }

  Serial.print("Posting with with request data: ");
  Serial.println(reqData);
  int rspCode = http.POST(reqData);

  if (rspCode > 0) {
    Serial.print("Post successful. Code: ");
    Serial.println(rspCode);
  } else {
    Serial.print("Post error: ");
    Serial.println(http.errorToString(rspCode));
    return "";
  }
  
  String response = http.getString();
  http.end();
  return response;
}

String commandToPath(Command command)
{
  switch (command)
  {
    case DETECT:      return "DetectServer";
    case REGISTER:    return "RegisterWithServer";
    case QUERY:       return "QueryServerForCommands";
    case IOTDATA:     return "IOTData";
    case IOTSHUTDOWN: return "IOTShutdown";
    default:          return "";
  }
}
