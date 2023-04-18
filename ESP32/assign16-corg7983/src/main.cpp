/* * * * * * * * * * * * * * * * * * * * * * * * * * *\
 * Class:       CS452 (Real-Time Operating Systems)  *
 * Instructor:  John Shovic                          *
 * Student:     Ethan Corgatelli (corg7983)          *
 * Project:     Assignment 16 - FreeRTOS IOT Device  *
 * File:        main.cpp                             *
 *                                                   *
\* * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "main.h"


//---------------- SETUP ----------------//

void setup()
{
  // Initialization
  Serial.begin(115200);
  // swSer.begin(9600);
  initPins();
  initPixels();
  connectHDC();
  connectSI();
  connectWiFi();
  configTime(0, 0, NTP_SERVER);
  String curTime = getTime();
  Serial.print("Initialization complete (time: ");
  Serial.print(curTime);
  Serial.println(")\n");

  // Create queues
  qIOTcmd  = xQueueCreate(32,sizeof(IOTCmd) );
  qSfreq   = xQueueCreate(1, sizeof(int)    );
  qCfreq   = xQueueCreate(1, sizeof(int)    );
  qHDCdata = xQueueCreate(8, sizeof(HDCdata));
  qSIdata  = xQueueCreate(8, sizeof(double) );
  // qGPSdata = xQueueCreate(1, sizeof(String) );
  qStepper = xQueueCreate(32,sizeof(bool)   );

  // Create ISRs
  attachInterrupt(BUTTON_1, isr1, RISING);
  attachInterrupt(BUTTON_2, isr2, RISING);
  attachInterrupt(BUTTON_3, isr3, RISING);

  // Create pinger tasks
  xTaskCreatePinnedToCore(tCping,  "Check Pinger", 1024, NULL,11, &thCping,  1);
  xTaskCreatePinnedToCore(tSping,  "Send Pinger",  1024, NULL,12, &thSping,  1);

  // Create tasks
  xTaskCreatePinnedToCore(tIOT,    "IOT Task",    65536, NULL,10, &thIOT,    1);
  xTaskCreatePinnedToCore(tHDC,    "HDC Task",     2048, NULL, 5, &thHDC,    1);
  xTaskCreatePinnedToCore(tSI,     "SI Task",      2048, NULL, 4, &thSI,     1);
  // xTaskCreatePinnedToCore(tGPS,    "GPS Task",    65536, NULL, 3, &thGPS,    1);
  xTaskCreatePinnedToCore(tStepper,"Stepper Task", 2048, NULL, 2, &thStepper,1);
  xTaskCreatePinnedToCore(tLED,    "LED Task",     2048, NULL, 1, &thLED,    1);

  // Send command to register with IOT server
  IOTCmd cmd = REGISTER;
  xQueueSend(qIOTcmd, &cmd, 0);
}
void loop() {}


//---------------- FUNCTIONS ----------------//

void initPins()
{
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);

  pinMode(STEPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_PIN_3, OUTPUT);
  pinMode(STEPPER_PIN_4, OUTPUT);
}

void initPixels()
{
  pixels.begin();
  pixels.setBrightness(50);
  pixels.clear();
  pixels.show();
}

void connectHDC()
{
  Serial.print("\nScanning for HDC1080 on the I2C bus. ");

  hdc1080.begin(0x40);
  while (Wire.endTransmission() != 0 || hdc1080.readDeviceId() != 0x1050) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(". ");
    hdc1080.begin(0x40);
  }
	Serial.print("\nFound (Manufacturer ID=0x");
	Serial.print(hdc1080.readManufacturerId(), HEX); // 0x5449
	Serial.print(", Device ID=0x");
	Serial.print(hdc1080.readDeviceId(), HEX); // 0x1050
  Serial.println(")");
}

void connectSI()
{
  Serial.print("\nScanning for SI1145 on the I2c bus. ");

  while (!si1145.begin()) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(". ");
  }
	Serial.println("\nFound");
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
  Serial.println(")");
}

String cmdToPath(IOTCmd cmd)
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

String getTime()
{
  struct tm ti;
  if (getLocalTime(&ti)) {
    char buf[30];
    strftime(buf, 30, "%F %T", &ti);
    return String(buf);
  } else return "";
}

//---------------- ISRs ----------------//

void IRAM_ATTR isr1()
{
  if (lastBtnRegTick < xTaskGetTickCountFromISR() - BTN_REG_TICK_GAP) {
		lastBtnRegTick = xTaskGetTickCountFromISR();
    IOTCmd cmd = REGISTER;
    xQueueSendFromISR(qIOTcmd, &cmd, &tru);
  }
}

void IRAM_ATTR isr2()
{
  if (lastBtnRegTick < xTaskGetTickCountFromISR() - BTN_REG_TICK_GAP) {
		lastBtnRegTick = xTaskGetTickCountFromISR();
    IOTCmd cmd = QUERY;
    xQueueSendFromISR(qIOTcmd, &cmd, &tru);
  }
}

void IRAM_ATTR isr3()
{
  if (lastBtnRegTick < xTaskGetTickCountFromISR() - BTN_REG_TICK_GAP) {
		lastBtnRegTick = xTaskGetTickCountFromISR();
    IOTCmd cmd = IOTSHUTDOWN;
    xQueueSendFromISR(qIOTcmd, &cmd, &tru);
  }
}


//---------------- PINGER TASKS ----------------//

void tCping(void *)
{
  IOTCmd cmd = QUERY;
  int delaySec = 0;

  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  while (1) {
    xQueuePeek(qCfreq, &delaySec, portMAX_DELAY);
    while (delaySec < MIN_CHECK_SEND_FREQ) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      xQueuePeek(qCfreq, &delaySec, portMAX_DELAY);
    }
    vTaskDelay(delaySec * 1000 / portTICK_PERIOD_MS);
    xQueuePeek(qCfreq, &delaySec, portMAX_DELAY);
    if (delaySec < MIN_CHECK_SEND_FREQ)
      continue;
    Serial.println("[Check Pinger]: Sending ping to check IOT server for new commands.");
    xQueueSend(qIOTcmd, &cmd, portMAX_DELAY);
  }
}

void tSping(void *)
{
  IOTCmd cmd = IOTDATA;
  int delaySec = 0;

  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  while (1) {
    xQueuePeek(qSfreq, &delaySec, portMAX_DELAY);
    while (delaySec < MIN_CHECK_SEND_FREQ) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      xQueuePeek(qSfreq, &delaySec, portMAX_DELAY);
    }
    vTaskDelay(delaySec * 1000 / portTICK_PERIOD_MS);
    xQueuePeek(qSfreq, &delaySec, portMAX_DELAY);
    if (delaySec < MIN_CHECK_SEND_FREQ)
      continue;
    Serial.println("[Send Pinger]: Sending ping to send data to IOT server.");
    xQueueSend(qIOTcmd, &cmd, portMAX_DELAY);
  }
}


//---------------- TASKS ----------------//

void tIOT(void *)
{
  WiFiClient client;
  HTTPClient http;
  String authCode, address, reqData, response, curTime;
  IOTCmd cmd;
  int Cfreq, Sfreq;
  JSONVar json;
  HDCdata hdcData;
  GPSdata * gpsData = NULL;
  double temp, humid, light, lat, lon, alt;

  while (1) {
    // Wait for a command
    xQueueReceive(qIOTcmd, &cmd, portMAX_DELAY);

    // Skip this command if authCode is required but empty
    if (authCode == "" && (cmd == QUERY || cmd == IOTDATA || cmd == IOTSHUTDOWN)) {
      Serial.print("[IOT Task]: Must be registered to run ");
      Serial.println(cmdToPath(cmd));
      continue;
    }

    // Print the command that was recieved
    Serial.print("[IOT Task]: Command recieved: ");
    Serial.print(cmdToPath(cmd));
    Serial.print(" <");
    for (int i = 0; i < 50; i++)
      Serial.print("-");
    Serial.println("<");

    // Start the HTTPClient
    address = SERVER_IP + cmdToPath(cmd);
    http.begin(client, address);
    http.addHeader("Content-Type", "application/json");

    // Set reqData appropriately
    switch (cmd) {
      case DETECT:      reqData = "{\"key\":\""       + SECRET_KEY + "\"}";                             break;
      case REGISTER:    reqData = "{\"key\":\""       + SECRET_KEY + "\",\"iotid\":\"" + IOTID + "\"}"; break;
      case QUERY:       reqData = "{\"auth_code\":\"" + authCode   + "\",\"iotid\":\"" + IOTID + "\"}"; break;
      case IOTDATA: {
        // Notify HDC1080 task to take a reading and wait on it
        Serial.println("[IOT Task]: Requesting reading from HDC1080");
        xTaskNotifyGive(thHDC);
        xQueueReceive(qHDCdata, &hdcData, portMAX_DELAY);
        temp  = hdcData.temp;
        humid = hdcData.humid;
        Serial.print("[IOT Task]: HDC reading recieved (temp: ");
        Serial.print(temp);
        Serial.print(", humid: ");
        Serial.print(humid);
        Serial.println(")");

        // Notify SI1145 task to take a reading and wait on it
        Serial.println("[IOT Task]: Requesting reading from SI1145");
        xTaskNotifyGive(thSI);
        xQueueReceive(qSIdata, &light, portMAX_DELAY);
        Serial.print("[IOT Task]: SI reading recieved (light: ");
        Serial.print(light);
        Serial.println(")");

        // Notify GPS task to take a reading and wait on it
        Serial.println("[IOT Task]: Saving latest GPS reading from queue");
        // xQueuePeek(qGPSdata, gpsData, 0);
        if (gpsData != NULL) {
          lat = gpsData->lat;
          lon = gpsData->lon;
          alt = gpsData->alt;
          Serial.print("[IOT Task]: GPS reading recieved (lat: ");
          Serial.print(lat);
          Serial.print(", lon: ");
          Serial.print(lon);
          Serial.print(", alt: ");
          Serial.print(alt);
          Serial.println(")");
        } else {
          Serial.println("[IOT Task]: GPS reading was not found");
        }

        // Get time data
        Serial.println("[IOT Task]: Storing current time");
        curTime = getTime();

        // Set reqData
        reqData = "{\"auth_code\":\"" + authCode 
                + "\",\"temperature\":" + temp
                + ",\"humidity\":" + humid
                + ",\"light\":" + light
                + (gpsData == NULL ? "" : ",\"latitude\":" + String(lat)
                                        + ",\"longitude\":" + String(lon)
                                        + ",\"altitude\":" + String(alt))
                + (!curTime.length() ? "}" : ",\"time\":\"" + curTime + "\"}");
        break;
      }
      case IOTSHUTDOWN: reqData = "{\"auth_code\":\"" + authCode + "\"}";                               break;
      default:          reqData = "";
    }

    // Post to server
    Serial.print("[IOT Task]: Posting: ");
    Serial.println(reqData);
    int rspCode = http.POST(reqData);

    // Print post error/success code
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
    
    // Handle response
    response = http.getString();
    http.end();
    json = JSON.parse(response);

    switch(cmd) {
      case DETECT: {
        if (!json.hasOwnProperty("reply") || strcmp((const char *)json["reply"], "I am up and Running")) {
          Serial.println("[IOT Task]: Server could not be detected");
        } else {
          Serial.print("[IOT Task]: Server detected at ");
          Serial.println(SERVER_IP);
        }
       break;
      }
      case REGISTER: {
        if (!json.hasOwnProperty("auth_code")) {
          Serial.println("[IOT Task]: Failed to register with server");
        } else {
          // Successfully registered with server
          authCode = (const char *)json["auth_code"];
          Serial.print("[IOT Task]: Authorization code obtained: ");
          Serial.println(authCode);

          // Default and start timers
          Cfreq = Sfreq = DEFAULT_CHECK_SEND_FREQ;
          xQueueOverwrite(qCfreq, &Cfreq);
          xQueueOverwrite(qSfreq, &Sfreq);
          xTaskNotifyGive(thSping);
          xTaskNotifyGive(thCping);
        }
        break;
    }
      case QUERY: {
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
        if (!json.hasOwnProperty("commands")) {
          Serial.println("[IOT Task]: Error querrying IOT server for commands");
        } else {
          json = json["commands"];
          for (int i = 0; i < json.length(); i++) {
            JSONVar command = json[i];
            if (!command.hasOwnProperty("command")) {
              Serial.print("[IOT Task]: Error parsing command from IOT server: ");
              Serial.println(command);
            } else {
              JSONVar commandName = command["command"];
              if (!strcmp((const char *)commandName, "RotQCW")) {
                xQueueSend(qStepper, &nah, 0);
              } else if (!strcmp((const char *)commandName, "RotQCCW")) {
                xQueueSend(qStepper, &tru, 0);
              } else if (!strcmp((const char *)commandName, "Flash")) {
                xTaskNotifyGive(thLED);
              } else if (!strcmp((const char *)commandName, "SendNow")) {
                cmd = IOTDATA;
                xQueueSend(qIOTcmd, &cmd, 0);
              } else if (!strcmp((const char *)commandName, "SetCheckFreq")) {
                if (!command.hasOwnProperty("seconds")) {
                  Serial.println("[IOT Task]: The SetCheckFreq command requires a seconds argument");
                } else {
                  Cfreq = command["seconds"];
                  Serial.print("[IOT Task]: Updating check pinger delay to ");
                  Serial.print(Cfreq);
                  Serial.println(" seconds");
                  xQueueOverwrite(qCfreq, &Cfreq);
                  xTaskAbortDelay(thCping);
                }
              } else if (!strcmp((const char *)commandName, "SetSendFreq")) {
                if (!command.hasOwnProperty("seconds")) {
                  Serial.println("[IOT Task]: The SetSendFreq command requires a seconds argument");
                } else {
                  Sfreq = command["seconds"];
                  Serial.print("[IOT Task]: Updating send pinger delay to ");
                  Serial.print(Sfreq);
                  Serial.println(" seconds");
                  xQueueOverwrite(qSfreq, &Sfreq);
                  xTaskAbortDelay(thSping);
                }
              } else {
                Serial.print("[IOT Task]: Unknown command from IOT server: ");
                Serial.println(commandName);
              }
            }
          }
        }
        break;
      }
      case IOTDATA: {
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
        break;
      }
      case IOTSHUTDOWN: {
        // Clear authCode to mark disconnection
        authCode = "";

        // Turn off pingers
        Cfreq = Sfreq = 0; 
        xQueueOverwrite(qCfreq, &Cfreq);
        xQueueOverwrite(qSfreq, &Sfreq);
      }
      default: {
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
      }
    }
  }
}

void tHDC(void *)
{
  HDCdata data;

  while (1) {
    // Wait for a notification to take a reading
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Take a reading and put it on the data queue
    data.temp = hdc1080.readT();
    data.humid = hdc1080.readH();
    xQueueSend(qHDCdata, &data, 0);
  }
}

void tSI(void *)
{
  double light;

  while (1) {
    // Wait for a notification to take a reading
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    // Take a reading and put it on the data queue
    light = (double)si1145.readVisible() / 100.0;
    xQueueSend(qSIdata, &light, 0);
  }
}

void tGPS(void *)
{
  bool found = false;
  char buf[16];
  int count = 0;
  String buffer;
  String latestLocationData;

  while (1); /* {
    // Check if there is data to read
    if (swSer.available()) {
      while (swSer.available()) {
        swSer.readBytes(buf, 1);
        if (buf[0] == '\n' && found == false) {
          buffer += buf[0];
          // Line just finished - check if we care about this next line
          count = swSer.readBytes(buf, 7);
          if (count == 7 && strcmp((const char *)buf, "GPGGA, ")) {
            found = true;
            // This line contains GPS location data - read it in
            while (swSer.available() && buf[0] != '\n') {
              swSer.readBytes(buf, 1);
              buffer += buf[0];
            }
          } else {
            // This line doesn't contain location data - continue as normal
            buf[count] = '\0';
            buffer += String(buf);
          }
        } else if (buf[0] == '\n' && found == true) {
          buffer += buf[0];
          // The locatin data line just finished - post it to the queue

        } else {
          // Add the single character (that isnt \n) to the String buffer
          buffer += buf[0];
        }
      }
    } else {
      // Wait for a moment before checking for input again
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  } */
}

void tStepper(void *)
{
  bool dir;
  int i, o, m;

  while (1) {
    xQueueReceive(qStepper, &dir, portMAX_DELAY);
    Serial.print("[Stepper Task]: Rotating 1/4 turn ");
    Serial.println(dir ? "counter-clockwise" : "clockwise");

    i = dir ? 0 : QUATER_TURN;
    o = dir ? 1 : -1;
    while (0 <= i && i <= QUATER_TURN) {
      m = i % 4;
      digitalWrite(STEPPER_PIN_1, STEPPER_STEPS[m][0]);
      digitalWrite(STEPPER_PIN_2, STEPPER_STEPS[m][1]);
      digitalWrite(STEPPER_PIN_3, STEPPER_STEPS[m][2]);
      digitalWrite(STEPPER_PIN_4, STEPPER_STEPS[m][3]);
      vTaskDelay(MS_BETWEEN_STEPS / portTICK_PERIOD_MS);
      i += o;
    }
    Serial.println("[Stepper Task]: Completed rotation");
    digitalWrite(STEPPER_PIN_1, 0);
    digitalWrite(STEPPER_PIN_2, 0);
    digitalWrite(STEPPER_PIN_3, 0);
    digitalWrite(STEPPER_PIN_4, 0);
  }
}

void tLED(void *)
{
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    Serial.println("[LED Task]: Flash!");
    for (int i = 0; i < NUM_PIXELS; i++)
      pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    pixels.show();
    vTaskDelay(250 / portTICK_PERIOD_MS);
    pixels.clear();
    pixels.show();
  }
}
