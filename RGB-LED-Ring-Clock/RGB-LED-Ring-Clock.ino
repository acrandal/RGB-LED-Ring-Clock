/**
 *  Digital clock made of RGB LED Rings
 *    Time kept with a real time clock (RTC)
 *  
 *  @author Aaron S. Crandall <crandall@gonzaga.edu>
 *  @copyright 2021
 *  @license MIT
 *  @version 1.0.0
 *  
 */

#include "RTClib.h"
#include "pitches.h"

#include <Adafruit_NeoPixel.h>


RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


// Pin pezieo is attached to
int buzzerPin = 8;

// Westminster Chimes music:
int melody[] = {
  NOTE_A2, NOTE_G2, NOTE_F2, NOTE_C2, REST, REST, NOTE_F2, NOTE_G2, NOTE_A2, NOTE_F2, REST, REST, NOTE_A2, NOTE_F2, NOTE_G2, NOTE_C2, REST, REST, NOTE_C2, NOTE_G2, NOTE_A2, NOTE_F2, REST, REST
};

// Chimes music durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

int noteQuantity = 24;


#define LED_PIN    9
#define LED_COUNT 84

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int lastHour = -1;
int lastMinute = 0;


// ***************************************************************************************************
// ** Plays full chimes on pezieo
void playWestminsterChimes() {
  for (int thisNote = 0; thisNote < noteQuantity; thisNote++) {
    int noteDuration = 1200 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(buzzerPin);
  }  
}

// ** Play a bong on the pezieo
void playBong(int bongDurationMs) {
  tone(buzzerPin, NOTE_B0, bongDurationMs);
  delay(bongDurationMs);
  noTone(buzzerPin);
}


// ** Play chimes on the hour
void playHourlyChimes(int currHour) {
  playWestminsterChimes();
  int numBongs = currHour % 12;
  if( numBongs == 0 ) { numBongs = 12; }

  for( int i = 0; i < numBongs; i++ ) {
    playBong(1200);
    delay(600);
  }
}


// ** Uses the RTC to get the current time as a DateTime object
DateTime getCurrentTime() {
  DateTime now = rtc.now();
  return now;
}


// ** Calculate new clock LED lights
void updateClockLEDs() {
  DateTime now = getCurrentTime();

  for( int i = 0; i < LED_COUNT; i++ ) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  
  int secLED = now.second() % 60;
  strip.setPixelColor(secLED, strip.Color(127, 0, 0));

  int minLED = now.minute() % 60;
  strip.setPixelColor(minLED, strip.Color(0, 127, 0));

  int hourLED = now.hour() % 24 + 60;
  strip.setPixelColor(hourLED, strip.Color(0, 0, 127));

  strip.show();
}


// ** Decide if the chimes should run and do em
void handleChimes() {
  DateTime now = getCurrentTime();

  int currHour = now.hour();
  int currMinute = now.minute();
  if( lastHour != currHour ) {
    lastHour = currHour;
    // The hour updated, time to play some chimes!
    playHourlyChimes(currHour);
  }

  if( currMinute == 30 && lastMinute != 30 ) {
    // Play half hour bong
    playBong(1200);
  }
  lastMinute = currMinute;
}


// *******************************************************************************************
void setup() {
// Support for ATtiny85 clock and the RGB LEDs
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  
  // Turn on the serial interface for debugging/testing
  Serial.begin(115200);

  // Setup the RGB LED strip
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  // Connect over I2C to the RTC chip
  if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is not running - need to program for current time");
    while(1);
  }

  Serial.println("RTC and LEDs up - starting main operations");

}


// ** Main Loop
void loop() {
  updateClockLEDs();
  handleChimes();

  delay(1000);
}


// ***********************************************************************************************************
//  Serial.print(now.year(), DEC);
//  Serial.print('/');
//  Serial.print(now.month(), DEC);
//  Serial.print('/');
//  Serial.print(now.day(), DEC);
//  Serial.print(" (");
//  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
//  Serial.print(") ");
//  Serial.print(now.hour(), DEC);
//  Serial.print(':');
//  Serial.print(now.minute(), DEC);
//  Serial.print(':');
//  Serial.print(now.second(), DEC);
//  Serial.println();
//  Serial.print(" since midnight 1/1/1970 = ");
//  Serial.print(now.unixtime());
//  Serial.print("s = ");
//  Serial.print(now.unixtime() / 86400L);
//  Serial.println("d");



//strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
//    strip.show();                          //  Update strip to match
//  strip.Color(255,   0,   0)
  
