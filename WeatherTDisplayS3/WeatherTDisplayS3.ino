#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager  version 2.0.17
#include <TFT_eSPI.h>
#include <ArduinoJson.h>  // 7.1.0
#include <HTTPClient.h>   // https://github.com/arduino-libraries/ArduinoHttpClient   version 0.6.1
#include <ESP32Time.h>    // https://github.com/fbiego/ESP32Time  verison 2.0.6
#include "NotoSansBold15.h"
#include "tinyFont.h"
#include "smallFont.h"
#include "middleFont.h"
#include "bigFont.h"
#include "font18.h"
#include "config.h"
#include "theme.h"
#include "ErrorHandling.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite errSprite = TFT_eSprite(&tft);
ESP32Time rtc(0);
//#################### config.h extracts  ###################
int zone = TIMEZONE;
String town = towns[0];
// defined in config.h
String myAPI = OPENWEATHER_API_KEY;
String units = METRICORIMPERIAL;  //  metric, imperial

// updateTimer in milliseconds; remember you have a limited number of free calls to OpenWeatherAPI
// However, ALSO remember that changing this affects the graph so it will no longer be 12 hours
// 180000 ms = 3 minutes - default
// 600000 ms = 10 minutes
// 1800000 ms = 30 minutes
int updateTimer = 180000;

// If 180000 ms/3 minutes gives 12 hours of graphs, that is 240 data points
// for a graph that is not 24 points wide, so 1 in 10. Not sure if it takes
// an average or just 1 data point

const char* ntpServer = NTPSERVER;
// String server = "https://api.openweathermap.org/data/2.5/weather?q=" + town + "&appid=" + myAPI + "&units=" + units;
String OpenWeatherServer = WEATHERSERVER
String server = OpenWeatherServer + town + "&appid=" + myAPI + "&units=" + units;

//#################### end of config.h extracts ###################

//additional variables
int ani = 100;
float maxT;
float minT;
unsigned long timePassed = 0;
int counter = 0;

//................colors
#define bck TFT_BLACK
unsigned short grays[13];

// static strings of data showed on right side
char* PPlbl[] = { "HUM", "PRESS", "WIND" };
String PPlblU[] = { "%", "hPa", "m/s" };

//data that changes
float temperature = 66.6;
float wData[3];
float PPpower[24] = {};   //graph
float PPpowerT[24] = {};  //graph
int PPgraph[24] = { 0 };  //graph


//scroling message on bottom right side
String Wmsg = "";

void setTime() {
  configTime(3600 * zone, 0, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
}

void setup() {
  Serial.begin();

  // using this board can work on battery
  pinMode(15, OUTPUT);
  digitalWrite(15, 1);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BACKGROUND_COLOR);
  tft.drawString("Connecting to WIFI!!", 30, 50, 4);
  sprite.createSprite(320, 170);
  errSprite.createSprite(164, 15);


  //set brightness
  ledcSetup(0, 10000, 8);
  ledcAttachPin(38, 0);
  ledcWrite(0, 130);

  //connect board to wifi , if cant, esp32 will make wifi network, connect to that network with password "password"
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(5000);

  if (!wifiManager.autoConnect("VolosWifiConf", "password")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
  }
  Serial.println("Connected.");
  setTime();
  getData();

  // generate 13 levels of gray
  int co = 210;
  for (int i = 0; i < 13; i++) {
    grays[i] = tft.color565(co, co, co);
    co = co - 20;
  }
}

void getData() {
    // Ensure Wi-Fi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi not connected!");
        ErrorHandler::displayError(errSprite, "Wi-Fi Disconnected");
        return;
    }

    // Verify the server URL
    Serial.println("Requesting data from: " + server);

    HTTPClient http;
    if (!http.begin(server)) {
        Serial.println("HTTP begin failed!");
        ErrorHandler::displayError(errSprite, "HTTP Begin Failed");
        return;
    }

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String payload = http.getString();
        Serial.println("Received payload: " + payload);

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            temperature = doc["main"]["temp"].as<float>();
            wData[0] = doc["main"]["humidity"].as<float>();
            wData[1] = doc["main"]["pressure"].as<float>();
            wData[2] = doc["wind"]["speed"].as<float>();

            int visibility = doc["visibility"].as<int>();
            const char* description = doc["weather"][0]["description"];
            long dt = doc["dt"].as<long>();

            Wmsg = "#Description: " + String(description) + 
                   "  #Visibility: " + String(visibility) + 
                   " #Updated: " + rtc.getTime();

            Serial.print("Temperature: ");
            Serial.println(temperature);

        } else {
            Serial.print("JSON Deserialization Error: ");
            Serial.println(error.c_str());
            ErrorHandler::displayError(errSprite, "JSON Error");
        }
    } else {
        Serial.print("HTTP Response Code: ");
        Serial.println(httpResponseCode);
        ErrorHandler::displayError(errSprite, "HTTP Error " + String(httpResponseCode));
    }

    http.end();
}


void draw() {

  errSprite.fillSprite(grays[10]);
  errSprite.setTextColor(grays[1], grays[10]);
  errSprite.drawString(Wmsg, ani, 4);

  // Below: Draw background for most of the screen
  sprite.fillSprite(BACKGROUND_COLOR);
  // Below: Top to bottom divider line
  sprite.drawLine(138, 10, 138, 164, grays[6]);
  // Below: Divider line under temperature decimal and above seconds
  sprite.drawLine(100, 108, 134, 108, grays[6]);
  sprite.setTextDatum(0);

  //LEFTSIDE
  sprite.loadFont(middleFont);
  sprite.setTextColor(grays[1], BACKGROUND_COLOR);
  sprite.drawString("WEATHER", 6, 10);
  sprite.unloadFont();

  sprite.loadFont(font18);
  sprite.setTextColor(grays[7], BACKGROUND_COLOR);
  sprite.drawString("TOWN:", 6, 110);
  sprite.setTextColor(grays[2], BACKGROUND_COLOR);
  if (units == "metric")
    sprite.drawString("C", 14, 50);
  if (units == "imperial")
    sprite.drawString("F", 14, 50);


  sprite.setTextColor(grays[3], BACKGROUND_COLOR);
  sprite.drawString(town, 46, 110);
  sprite.fillCircle(8, 52, 2, grays[2]);
  sprite.unloadFont();

  // draw time without seconds
  sprite.loadFont(tinyFont);
  sprite.setTextColor(grays[4], BACKGROUND_COLOR);
  sprite.drawString(rtc.getTime().substring(0, 5), 6, 132);
  sprite.unloadFont();

  // draw some static text
  sprite.setTextColor(grays[5], BACKGROUND_COLOR);
  sprite.drawString("INTERNET", 86, 10);
  sprite.drawString("STATION", 86, 20);
  sprite.setTextColor(grays[7], BACKGROUND_COLOR);
  sprite.drawString("SECONDS", 92, 157);

  // draw temperature
  sprite.setTextDatum(4);
  sprite.loadFont(bigFont);
  sprite.setTextColor(grays[0], BACKGROUND_COLOR);
  sprite.drawFloat(temperature, 1, 69, 80);
  sprite.unloadFont();


  //draw sec rectangle
  sprite.fillRoundRect(90, 132, 42, 22, 2, grays[2]);
  //draw seconds
  sprite.loadFont(font18);
  sprite.setTextColor(BACKGROUND_COLOR, grays[2]);
  sprite.drawString(rtc.getTime().substring(6, 8), 111, 144);
  sprite.unloadFont();


  sprite.setTextDatum(0);
  //RIGHT SIDE
  sprite.loadFont(font18);
  sprite.setTextColor(grays[1], BACKGROUND_COLOR);
  sprite.drawString("LAST 12 HOURS", 144, 10);
  sprite.unloadFont();

  sprite.fillRect(144, 28, 84, 2, grays[10]);

  sprite.setTextColor(grays[3], BACKGROUND_COLOR);
  sprite.drawString("MIN:" + String(minT), 254, 10);
  sprite.drawString("MAX:" + String(maxT), 254, 20);
  sprite.fillSmoothRoundRect(144, 34, 174, 60, 3, grays[10], BACKGROUND_COLOR);
  sprite.drawLine(170, 39, 170, 88, TFT_WHITE);
  sprite.drawLine(170, 88, 314, 88, TFT_WHITE);

  sprite.setTextDatum(4);

  for (int j = 0; j < 24; j++)
    for (int i = 0; i < PPgraph[j]; i++)
      sprite.fillRect(173 + (j * 6), 83 - (i * 4), 4, 3, grays[2]);

  sprite.setTextColor(grays[2], grays[10]);
  sprite.drawString("MAX", 158, 42);
  sprite.drawString("MIN", 158, 86);

  sprite.loadFont(font18);
  sprite.setTextColor(grays[7], grays[10]);
  sprite.drawString("T", 158, 58);
  sprite.unloadFont();


  for (int i = 0; i < 3; i++) {
    sprite.fillSmoothRoundRect(144 + (i * 60), 100, 54, 32, 3, grays[9], BACKGROUND_COLOR);
    sprite.setTextColor(grays[3], grays[9]);
    sprite.drawString(PPlbl[i], 144 + (i * 60) + 27, 107);
    sprite.setTextColor(grays[2], grays[9]);
    sprite.loadFont(font18);
    sprite.drawString(String((int)wData[i]) + PPlblU[i], 144 + (i * 60) + 27, 124);
    sprite.unloadFont();

    sprite.fillSmoothRoundRect(144, 148, 174, 16, 2, grays[10], BACKGROUND_COLOR);
    errSprite.pushToSprite(&sprite, 148, 150);
  }
  sprite.setTextColor(grays[4], BACKGROUND_COLOR);
  sprite.drawString("CURRENT WEATHER", 190, 141);
  sprite.setTextColor(grays[9], BACKGROUND_COLOR);
  sprite.drawString(String(counter), 310, 141);

  sprite.pushSprite(0, 0);
}

void updateData() {

  //update alsways
  //part needed for scroling weather msg
  ani--;
  if (ani < -420)
    ani = 100;


  //

  if (millis() > timePassed + updateTimer) {
    timePassed = millis();
    counter++;
    getData();

    if (counter == 10) {
      setTime();
      counter = 0;
      maxT = -50;
      minT = 1000;
      PPpower[23] = temperature;
      for (int i = 23; i > 0; i--)
        PPpower[i - 1] = PPpowerT[i];

      for (int i = 0; i < 24; i++) {
        PPpowerT[i] = PPpower[i];
        if (PPpower[i] < minT) minT = PPpower[i];
        if (PPpower[i] > maxT) maxT = PPpower[i];
      }

      for (int i = 0; i < 24; i++) {
        PPgraph[i] = map(PPpower[i], minT, maxT, 0, 12);
      }
    }
  }
}

void loop() {
  updateData();
  draw();
}
