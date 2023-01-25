//#include <Wire.h>
#include "RTClib.h"

RTC_PCF8523 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


void setup() {
  Serial.begin(115200);
  while (!Serial)
  {
      ;
  }
  Serial.println("Setup Start");
  if(!rtcSetup()){
    Serial.println("RTC false loop");
    while(1);
  }
  Serial.println("rtc ok.");  
}

bool rtcSetup(){
  Serial.println("rtc setup");
  if(!rtc.begin()){
    Serial.println("Could not find RTC module!");
    return false;
  }
  Serial.println("begin ok.");
  if(!rtc.initialized()){
    Serial.println("RTC is not Running!");

    // 時刻を調整する関数。最初の一回だけ動作させればOKです。
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  return true;
}

void loop () {
  // 時刻の読み出し
  DateTime time = rtc.now();
  printSerialTime(time);

  delay(1000);
}

// シリアルモニターに打刻する
void printSerialTime(DateTime time){
    Serial.print(time.year(), DEC);
    Serial.print("/");
    Serial.print(time.month(), DEC);
    Serial.print("/");
    Serial.print(time.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[time.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(time.hour(), DEC);
    Serial.print(":");
    Serial.print(time.minute(), DEC);
    Serial.print(":");
    Serial.print(time.second(), DEC);
    Serial.println(); 
}
