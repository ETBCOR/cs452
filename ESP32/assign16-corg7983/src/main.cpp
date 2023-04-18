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
  initPins();
  initPixels();
  connectHDC();
  connectSI();
  connectWiFi();

  // Create queues
  qIOTcmd  = xQueueCreate(32,sizeof(IOTCmd) );
  qSfreq   = xQueueCreate(1, sizeof(int)    );
  qCfreq   = xQueueCreate(1, sizeof(int)    );
  qHDCdata = xQueueCreate(8, sizeof(HDCdata));
  qSIdata  = xQueueCreate(8, sizeof(double) );
  qStepper = xQueueCreate(32,sizeof(bool)   );

  // Create ISRs
  attachInterrupt(BUTTONS1, isr1, RISING);
  attachInterrupt(BUTTONS2, isr2, RISING);
  attachInterrupt(BUTTONS3, isr3, RISING);

  // Create pinger tasks
  xTaskCreatePinnedToCore(tCping, "Check Pinger",  1024, NULL, 11, &thCping, 1  );
  xTaskCreatePinnedToCore(tSping,  "Send Pinger",  1024, NULL, 12, &thSping, 1  );

  // Create tasks
  xTaskCreatePinnedToCore(tIOT,    "IOT Task",    65536, NULL, 10, &thIOT,     1);
  xTaskCreatePinnedToCore(tHDC,    "HDC Task",     2048, NULL,  4, &thHDC,     1);
  xTaskCreatePinnedToCore(tSI,     "SI Task",      2048, NULL,  3, &thSI,      1);
  xTaskCreatePinnedToCore(tStepper,"Stepper Task", 2048, NULL,  2, &thStepper, 1);
  xTaskCreatePinnedToCore(tLED,    "LED Task",     2048, NULL,  1, &thLED,     1);

  // Send command to register with IOT server
  IOTCmd cmd = REGISTER;
  xQueueSend(qIOTcmd, &cmd, 0);
}
void loop() {}


//---------------- FUNCTIONS ----------------//

void initPins()
{
  pinMode(BUTTONS1, INPUT);
  pinMode(BUTTONS2, INPUT);
  pinMode(BUTTONS3, INPUT);
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
	Serial.print("\nFound. Manufacturer ID=0x");
	Serial.print(hdc1080.readManufacturerId(), HEX); // 0x5449
	Serial.print(", Device ID=0x");
	Serial.println(hdc1080.readDeviceId(), HEX); // 0x1050
}

void connectSI()
{
  Serial.print("\nScanning for SI1145 on the I2c bus. ");

  while (!si1145.begin()) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(". ");
  }
	Serial.println("\nFound.");
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
  String address, reqData, response;
  String authCode;
  IOTCmd cmd;
  int Cfreq, Sfreq;
  JSONVar json;
  HDCdata hdcData;
  double temp, humid, light;

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
    Serial.println(cmdToPath(cmd));

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
        Serial.println("[IOT Task]: Requesting reading from HDC1080");
        // Notify HDC1080 task to take a reading and wait on it
        xTaskNotifyGive(thHDC);
        xQueueReceive(qHDCdata, &hdcData, portMAX_DELAY);
        temp = hdcData.temp;
        humid = hdcData.humid;
        Serial.print("[IOT Task]: HDC reading recieved (temp: ");
        Serial.print(temp);
        Serial.print(", humid: ");
        Serial.print(humid);
        Serial.println(")");

        // Wait for the SI's data, then set reqData
        Serial.println("[IOT Task]: Requesting reading from SI1145");
        xTaskNotifyGive(thSI);  // notify SI1145 task to take a reading
        xQueueReceive(qSIdata, &light, portMAX_DELAY);
        Serial.print("[IOT Task]: SI reading recieved (light: ");
        Serial.print(light);
        Serial.println(")");
        reqData = "{\"auth_code\":\"" + authCode 
                + "\",\"temperature\":" + temp
                + ",\"humidity\":" + humid + "}";
                // + ",\"light\":" + light
                //+ ",\"latitude\":0.0,\"longitude\":0.0,\"altitude\":0.0,\"time\":\"2023-04-01 00:00:01\"}";
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

          // Default and start timers
          Cfreq = Sfreq = DEFAULT_CHECK_SEND_FREQ;
          xQueueOverwrite(qCfreq, &Cfreq);
          xQueueOverwrite(qSfreq, &Sfreq);
          xTaskNotifyGive(thSping);
          xTaskNotifyGive(thCping);
        }
        break;

      case QUERY:
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
        if (!json.hasOwnProperty("commands")) {
          Serial.println("[IOT Task]: Error querrying IOT server for commands.");
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
                  Serial.println("[IOT Task]: The SetCheckFreq command requires a seconds argument.");
                } else {
                  Cfreq = command["seconds"];
                  Serial.print("[IOT Task]: Updating check pinger delay to ");
                  Serial.print(Cfreq);
                  Serial.println(" seconds.");
                  xQueueOverwrite(qCfreq, &Cfreq);
                  xTaskAbortDelay(thCping);
                }
              } else if (!strcmp((const char *)commandName, "SetSendFreq")) {
                if (!command.hasOwnProperty("seconds")) {
                  Serial.println("[IOT Task]: The SetSendFreq command requires a seconds argument.");
                } else {
                  Sfreq = command["seconds"];
                  Serial.print("[IOT Task]: Updating send pinger delay to ");
                  Serial.print(Sfreq);
                  Serial.println(" seconds.");
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

      case IOTDATA:
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
        break;

      case IOTSHUTDOWN:
        // Clear authCode to mark disconnection
        authCode = "";

        // Turn off pingers
        Cfreq = Sfreq = 0; 
        xQueueOverwrite(qCfreq, &Cfreq);
        xQueueOverwrite(qSfreq, &Sfreq);

      default:
        Serial.print("[IOT Task]: Response: ");
        Serial.println(response);
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

void tStepper(void *)
{
  bool dir;

  while (1) {
    xQueueReceive(qStepper, &dir, portMAX_DELAY);
    Serial.print("[Stepper Task]: Rotating ");
    Serial.println(dir ? "CCW" : "CW");
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