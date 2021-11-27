/**
 * Quick script to set the RTC 
 * 
 * Instructions:
 *   1) Unplug arduino and remove RTC battery
 *   2) Put battery back into RTC
 *   3) Plug in Arduino
 *   4) Build & run this script
 * 
 * Aaron S. Crandall <crandall@gonzaga.edu>, 2021
 */


#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


void setup () {
 while (!Serial);
 Serial.begin(115200);
 
 if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1);
 }
 
 if (rtc.isrunning()) {
   Serial.println("RTC is NOT running! -- Setting Time");
   // following line sets the RTC to the date & time this sketch was compiled
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   // This line sets the RTC with an explicit date & time, for example to set
   // January 21, 2014 at 3am you would call:
   // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
 }
}


void loop () {
 DateTime now = rtc.now();
 Serial.print(now.year(), DEC);
 Serial.print('/');
 Serial.print(now.month(), DEC);
 Serial.print('/');
 Serial.print(now.day(), DEC);
 Serial.print(" (");
 Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
 Serial.print(") ");
 Serial.print(now.hour(), DEC);
 Serial.print(':');
 Serial.print(now.minute(), DEC);
 Serial.print(':');
 Serial.print(now.second(), DEC);
 Serial.println();
 Serial.print(" since midnight 1/1/1970 = ");
 Serial.print(now.unixtime());
 Serial.print("s = ");
 Serial.print(now.unixtime() / 86400L);
 Serial.println("d");
 
 delay(3000);
}
