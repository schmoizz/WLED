#pragma once

#include "wled.h"
#include <RTClib.h>

#ifdef ARDUINO_ARCH_ESP32
  #define HW_PIN_SCL 22
  #define HW_PIN_SDA 21
#else
  #define HW_PIN_SCL 5
  #define HW_PIN_SDA 4
#endif

class PingPongClockUsermod : public Usermod
{
private:
  // Private class members. You can declare variables and functions only accessible to your usermod here
  unsigned long lastTime = 0;
  bool colonOn = true;
  bool rtcEnabled = false;
  bool rtcErrorDisabled = false;
  String rtcError = "No Error";
  RTC_DS3231 rtc;

  // ---- Variables modified by settings below -----
  // set your config variables to their boot default value (this can also be done in readFromConfig() or a constructor if you prefer)
  bool pingPongClockEnabled = true;
  int colorR = 0xFF;
  int colorG = 0xFF;
  int colorB = 0xFF;
  int sdaPin = HW_PIN_SDA;
  int sclPin = HW_PIN_SCL;

  // ---- Variables for correct LED numbering below, edit only if your clock is built different ----

  int baseH = 8;              // Adress for the one place of the hours
  int baseHH = 36;            // Adress for the tens place of the hours
  int baseM = 78;             // Adress for the one place of the minutes
  int baseMM = 106;           // Adress for the tens place of the minutes
  int colon[2] = { 64, 65 };  // Adress for the first colon led

  // Matrix for the illumination of the numbers
  // Note: These only define the increments of the base adress. e.g. to define the second Minute you have to add the baseMM to every led position
  const int numbers[10][10] = 
    {
      {  0,  1,  4,  6, 13, 15, 18, 19, -1, -1 }, // 0: null
      { 13, 14, 15, 18, 19, -1, -1, -1, -1, -1 }, // 1: eins
      {  0,  4,  5,  6, 13, 14, 15, 19, -1, -1 }, // 2: zwei
      {  4,  5,  6, 13, 14, 15, 18, 19, -1, -1 }, // 3: drei
      {  1,  4,  5, 14, 15, 18, 19, -1, -1, -1 }, // 4: vier
      {  1,  4,  5,  6, 13, 14, 15, 18, -1, -1 }, // 5: fünf
      {  0,  5,  6, 10, 13, 14, 15, 18, -1, -1 }, // 6: sechs
      {  4,  6,  9, 13, 14, 19, -1, -1, -1, -1 }, // 7: sieben
      {  0,  1,  4,  5,  6, 13, 14, 15, 18, 19 }, // 8: acht
      {  1,  4,  5,  6,  9, 13, 14, 19, -1, -1 }  // 9: neun
    };

public:
  void setup()
  {
    if(rtcEnabled) {
      PinManagerPinType pins[2] = { { sclPin, true }, { sclPin, true } };
      if (!pinManager.allocateMultiplePins(pins, 2, PinOwner::HW_I2C)) 
      { 
        rtcError = "Pin Alloc Error";
        rtcErrorDisabled = true; 
        return; 
      }
      Wire.pins(sdaPin, sclPin);
      if(!rtc.begin())
      {
        rtcError = "No Chip Error";
        rtcErrorDisabled = true;
        return;
      }
      if(!rtc.lostPower()) 
      {
        DateTime rtcTime = rtc.now();
        toki.setTime(rtcTime.unixtime(), TOKI_NO_MS_ACCURACY, TOKI_TS_RTC);
        updateLocalTime();
      }
    }
  }

  void loop()
  {
    if (millis() - lastTime > 1000)
    {
      lastTime = millis();
      colonOn = !colonOn;
    }
    if(rtcEnabled)
    {
      if(strip.isUpdating()) return;
      if(!rtcErrorDisabled && toki.isTick())
      {
        time_t t = toki.second();
        if(t != rtc.now().unixtime()) 
        {
          rtc.adjust(DateTime(t));
        }
      }
    }
  }

  void addToJsonInfo(JsonObject& root)
  {
    JsonObject user = root["u"];
    if (user.isNull()) user = root.createNestedObject("u");

    JsonArray lightArr = user.createNestedArray("Uhrzeit-Anzeige"); //name
    lightArr.add(pingPongClockEnabled ? "aktiv" : "inaktiv"); //value
    lightArr.add(""); //unit

    JsonArray rtcArr = user.createNestedArray("RTC");
    rtcArr.add(rtcEnabled ? "aktiv" : "inaktiv");

    JsonArray rtcErrorArr = user.createNestedArray("RTC Error");
    rtcErrorArr.add(rtcError);

  }

  void addToConfig(JsonObject &root)
  {
    JsonObject top = root.createNestedObject("Ping Pong Clock");
    top["enabled"] = pingPongClockEnabled;
    top["colorR"]  = colorR;
    top["colorG"]  = colorG;
    top["colorB"]  = colorB;
    top["sda"]     = sdaPin;
    top["scl"]     = sclPin;  
  }

  bool readFromConfig(JsonObject &root)
  {
    JsonObject top = root["Ping Pong Clock"];

      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top["enabled"], pingPongClockEnabled);
      configComplete &= getJsonValue(top["colorR"], colorR);
      configComplete &= getJsonValue(top["colorG"], colorG);
      configComplete &= getJsonValue(top["colorB"], colorB);
      configComplete &= getJsonValue(top["sda"], sdaPin);
      configComplete &= getJsonValue(top["scl"], sclPin);

      return configComplete;
  }

  void drawNumber(int base, int number)
  {
    for(int i = 0; i < 10; i++)
    {
      if(numbers[number][i] > -1)
        strip.setPixelColor(numbers[number][i] + base, RGBW32(colorR, colorG, colorB, 0));
    }
  }

  void handleOverlayDraw()
  {
    if(pingPongClockEnabled){
      if(colonOn)
      {
        strip.setPixelColor(colon[0], RGBW32(colorR, colorG, colorB, 0));
        strip.setPixelColor(colon[1], RGBW32(colorR, colorG, colorB, 0));
      }
      drawNumber(baseHH, (hour(localTime) / 10) % 10);
      drawNumber(baseH, hour(localTime) % 10); 
      drawNumber(baseM, (minute(localTime) / 10) % 10);
      drawNumber(baseMM, minute(localTime) % 10);
    }
  }
  
  uint16_t getId()
  {
    return USERMOD_ID_PING_PONG_CLOCK;
  }

};