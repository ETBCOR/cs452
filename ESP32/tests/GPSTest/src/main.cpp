/*
 * Air530Allfunction.ino: Demonstrate AIR530 GPS full-function configuration example
 * Test on esp32
 * Copyright 2020 Lewis he
 * 
 * MODIFIED BY ETHAN CORAGTELLI
 */

#include <Arduino.h>
#include <HTTPClient.h>
// #include <SoftwareSerial.h>
#include "Air530.h"

const char * NET_SSID   = "FridaysNetwork2G";
const char * NET_PASS   = "localpass";
const char * NTP_SERVER = "pool.ntp.org";

#define GPS_RX                      8
#define GPS_TX                      7
#define GPS_BAUD_RATE               9600

// SoftwareSerial * swSerial = nullptr;
Air530         * gps = nullptr;

uint32_t         last = 0;

struct myTime {
  uint16_t year;
  uint8_t month, day, hour, minute, seconds;
};

void connectWiFi();
struct myTime getTime();


void setup(void)
{
  Serial.begin(115200);
  Serial2.begin(GPS_BAUD_RATE, 134217756U, GPS_RX, GPS_TX);
  Serial.println("AIR530 GPS full-function configuration example");
  connectWiFi();
  configTime(0, 0, NTP_SERVER);

  //Initialize the uart
  // swSerial = new SoftwareSerial(GPS_RX, GPS_TX);
  // swSerial->begin(GPS_BAUD_RATE);
  gps = new Air530(&Serial2);
  Serial.println("Inited uart and made Air530 object");

  /*Set the GPS module to the normal mode,
    it may not be able to be set successfully in other modes, please pay attention */
  gps->setNormalMode();
  delay(500);
  Serial.println("Set GPS mode to normal");

  /*When you need to configure the GPS module,
    turn off the output of NMEA sentences to improve the success rate*/
  gps->disableNMEAOutput();
  Serial.println("NMEA output disabled");


  // Set the current GPS module RTC and time
  struct myTime t = getTime();
  gps->setDateTime(t.year, t.month, t.day, t.hour, t.minute, t.seconds);
  delay(500);
  Serial.printf("Time info recieved  year:%u, month:%u, day:%u, hour:%u,  min:%u, sec:%u\n", t.year, t.month, t.day, t.hour, t.minute, t.seconds);

  // Get the current GPS module RTC and time
  if (gps->getDateTime(t.year, t.month, t.day, t.hour, t.minute, t.seconds)) {
    Serial.printf("GPS TIME  year:%u, month:%u, day:%u, hour:%u,  min:%u, sec:%u\n", t.year, t.month, t.day, t.hour, t.minute, t.seconds);
  } else {
    Serial.println("Failed to fetch time data from GPS");
  }

  // Get the current GPS module software version number
  Serial.print("Soft version: ");
  Serial.println(gps->getSoftVersion());

  // Configure the output frequency of NMEA sentences, in milliseconds
  gps->setNMEAInterval(1000);

  // Get the output frequency of NMEA sentences, in milliseconds
  Serial.print("NMEA message interval: ");
  Serial.print(gps->getNMEAInterval());
  Serial.println("ms");

  /*You can set gps to tracking mode,
    let it run for 5 seconds and sleep for 10 seconds*/
  // gps->setCycleTrackingMode(5000, 10000);

  /*You can set gps to a low-power operating mode,
    let it run for 5 seconds and sleep for 1 seconds*/
  // gps->setCycleLowPowerMode(5000, 1000);
  // Serial.println("Set cycle low power mode");

  /*You can set gps to tracking mode,
    it will not output any data until
    you call gps->wakeup() to wake it up*/
  // gps->setTrackingMode();

  /*The gps can be set to automatic low power consumption mode,
    and the running time and sleep time will be automatically
    controlled by it*/
  // gps->setAutoLowPowerMode();

  /*The gps can be set to automatic tracking mode,
    and the running time and sleep time will be automatically
    controlled by it*/
  // gps->setAutoTrackingMode();


  /*You can set the satellite to be used by Air530,
    true is used, if not used, then set it to false*/
  bool GPS = true;
  bool GLONASS = true;
  bool BEIDOU = true;
  bool GALIEO = true;
  gps->setSearchMode(GPS, GLONASS, BEIDOU, GALIEO);
  Serial.println("Set search mode");

  /*Calling the following function will enable or disable
    Quasi-Zenith Satellite System*/
  // gps->enableQZSS();
  // gps->disableQZSS();

  /*Calling the following function will enable or disable
    Satellite-Based Augmentation System*/
  gps->enableSBAS();
  // gps->disableSBAS();

  /*1PPS mode has the following options:
      AIR530_1PPS_OFF,        //Disable PPS output
      AIR530_1PPS_FIRST_FIX,  //First fix
      AIR530_1PPS_3D_FIX,     //3D fix
      AIR530_1PPS_2D3D_FIX,   //2D or 3D fix
      AIR530_1PPS_ALWAYS,     //Always on

    Call the following function to set PPS to output a pulse
    every 500ms when 2D/3D is repaired*/
  // gps->setPPS(AIR530_1PPS_2D3D_FIX, 500);

  // Call the following function to control the output of NMEA sentences
  bool gll = true;
  bool rmc = true;
  bool vtg = true;
  bool gga = true;
  bool gsa = true;
  bool gsv = true;
  bool grs = true;
  bool gst = true;
  gps->setNMEAStatement(gll, rmc, vtg, gga, gsa, gsv, grs, gst);

  /*You can speed up GPS positioning by presetting approximate latitude,
    longitude and time*/
  // float lat = 114.22;
  // float lng = 22.156;
  // uint16_t altitude = 30;
  // year = 2020;
  // month = 12;
  // day = 29;
  // hour = 10;
  // minute = 0;
  // seconds = 0;
  // gps->setProbablyLoaction( lat,  lng, altitude, year, month, day, hour, minute, seconds);

  // Get whether the Satellite-Based Augmentation System is enabled
  Serial.print("SBAS enabled: ");
  Serial.println(gps->getSBASEnable());

  /*After the configuration is complete,
    call enableNMEAOutput to enable the output of NMEA sentences*/
  gps->enableNMEAOutput();
  Serial.println("Re-enabled NMEA output");

  Serial.println("Configure Done!\n\n");

  // enter sleep mode. need call gps->restart() wakeup
  // gps->sleep();

  // enter stop mode . need call gps->restart() wakeup
  // gps->stop();

  /*When the call mode goes to sleep
    gps->setTrackingMode()
    gps->setCycleTrackingMode()
    gps->setAutoTrackingMode()
  you need to call gps->wakeup() to wake up the GPS module*/
  // gps->wakeup();


  /*AIR530_HOT_START
    AIR530_WARM_START
    AIR530_COLD_START
    Call restart to start the gps module*/
  gps->restart(AIR530_COLD_START);
}

void loop(void)
{
  gps->process();

  if (gps->location.isUpdated()) {
    Serial.print(F("LOCATION   Fix Age="));
    Serial.print(gps->location.age());
    Serial.print(F("ms Raw Lat="));
    Serial.print(gps->location.rawLat().negative ? "-" : "+");
    Serial.print(gps->location.rawLat().deg);
    Serial.print("[+");
    Serial.print(gps->location.rawLat().billionths);
    Serial.print(F(" billionths],  Raw Long="));
    Serial.print(gps->location.rawLng().negative ? "-" : "+");
    Serial.print(gps->location.rawLng().deg);
    Serial.print("[+");
    Serial.print(gps->location.rawLng().billionths);
    Serial.print(F(" billionths],  Lat="));
    Serial.print(gps->location.lat(), 6);
    Serial.print(F(" Long="));
    Serial.println(gps->location.lng(), 6);
  } else if (gps->date.isUpdated()) {
    Serial.print(F("DATE Fix Age="));
    Serial.print(gps->date.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps->date.value());
    Serial.print(F(" Year="));
    Serial.print(gps->date.year());
    Serial.print(F(" Month="));
    Serial.print(gps->date.month());
    Serial.print(F(" Day="));
    Serial.println(gps->date.day());
  } else  if (gps->time.isUpdated()) {
    Serial.print(F("TIME Fix Age="));
    Serial.print(gps->time.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps->time.value());
    Serial.print(F(" Hour="));
    Serial.print(gps->time.hour());
    Serial.print(F(" Minute="));
    Serial.print(gps->time.minute());
    Serial.print(F(" Second="));
    Serial.print(gps->time.second());
    Serial.print(F(" Hundredths="));
    Serial.println(gps->time.centisecond());
  } else if (gps->speed.isUpdated()) {
    Serial.print(F("SPEED      Fix Age="));
    Serial.print(gps->speed.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps->speed.value());
    Serial.print(F(" Knots="));
    Serial.print(gps->speed.knots());
    Serial.print(F(" MPH="));
    Serial.print(gps->speed.mph());
    Serial.print(F(" m/s="));
    Serial.print(gps->speed.mps());
    Serial.print(F(" km/h="));
    Serial.println(gps->speed.kmph());
  } else if (gps->altitude.isUpdated()) {
    Serial.print(F("ALTITUDE   Fix Age="));
    Serial.print(gps->altitude.age());
    Serial.print(F("ms Raw="));
    Serial.print(gps->altitude.value());
    Serial.print(F(" Meters="));
    Serial.print(gps->altitude.meters());
    Serial.print(F(" Miles="));
    Serial.print(gps->altitude.miles());
    Serial.print(F(" KM="));
    Serial.print(gps->altitude.kilometers());
    Serial.print(F(" Feet="));
    Serial.println(gps->altitude.feet());
  } else if (gps->satellites.isUpdated()) {
    Serial.print(F("SATELLITES Fix Age="));
    Serial.print(gps->satellites.age());
    Serial.print(F("ms Value="));
    Serial.println(gps->satellites.value());
  } else if (gps->hdop.isUpdated()) {
    Serial.print(F("HDOP       Fix Age="));
    Serial.print(gps->hdop.age());
    Serial.print(F("ms Value="));
    Serial.println(gps->hdop.value());
  }

  if (millis() - last > 5000) {
    Serial.print(F("DIAGS      Chars="));
    Serial.print(gps->charsProcessed());
    Serial.print(F(" Sentences-with-Fix="));
    Serial.print(gps->sentencesWithFix());
    Serial.print(F(" Failed-checksum="));
    Serial.print(gps->failedChecksum());
    Serial.print(F(" Passed-checksum="));
    Serial.println(gps->passedChecksum());
    last = millis();
  }
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

struct myTime getTime()
{
  struct tm ti;
  struct myTime t;
  char buf[10];
  if (getLocalTime(&ti)) {
    strftime(buf, 10, "%G", &ti);
    t.year      = std::stoi(buf);
    strftime(buf, 10, "%m", &ti);
    t.month     = std::stoi(buf);
    strftime(buf, 10, "%d", &ti);
    t.day       = std::stoi(buf);
    strftime(buf, 10, "%H", &ti);
    t.hour      = std::stoi(buf);
    strftime(buf, 10, "%M", &ti);
    t.minute    = std::stoi(buf);
    strftime(buf, 10, "%S", &ti);
    t.seconds   = std::stoi(buf);
  }
  return t;
}