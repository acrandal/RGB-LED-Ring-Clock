/*
 * RGB LED Ring Clock - version 2.0
 *
 * Aaron S. Crandall <crandall@gonzaga.edu>
 * Copyright: 2023
 *
 * A bunch of the NTP/time code comes from here (it's amazing!):
 *  https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
 */

#ifndef STASSID
#define STASSID "RobotNet"                            // set your SSID
#define STAPSK  "HomeworkGood"                        // set your wifi password
#endif

/* Configuration of NTP */
#define MY_NTP_SERVER "pool.ntp.org"           
#define MY_TZ "PST8PDT,M3.2.0,M11.1.0"


/* Necessary Includes */
#include <ESP8266WiFi.h>            // we need wifi to get internet access
#include <time.h>                   // time() ctime()
#include <Adafruit_NeoPixel.h>

// Project libraries
#include "pitches.h"  // My set of musical note pitches

// ** Chimes configuration and settings
#define CHIMES_ENABLE_PIN D0 // Clock chimes enable switch
int buzzerPin = D2;          // Piezio pin
int chimes_lastHour = -1;   // Tracking if chimes should be played
int chimes_lastMinute = 0;
bool chimes_firstBoot = true;

// Westminster Chimes music:
int melody[] = {
  NOTE_A2, NOTE_G2, NOTE_F2, NOTE_C2, REST, REST, NOTE_F2, NOTE_G2, NOTE_A2, NOTE_F2, REST, REST, NOTE_A2, NOTE_F2, NOTE_G2, NOTE_C2, REST, REST, NOTE_C2, NOTE_G2, NOTE_A2, NOTE_F2, REST, REST
};

// Chimes music durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

int noteQuantity = 24;

// ********************************************************************************* //
// ** Time system Global variables 
time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convenient way


// ********************************************************************************* //
// ** Clock LED rendering and pixel libraries
// RGB LED Ring pins and base offsets Configuration
#define LED_PIN    D4
#define LED_COUNT 84
#define LED_BRIGHTNESS 100      // One-off max LED brightness

#define LED_SEC_BASE_INDEX 0
#define LED_MIN_BASE_INDEX 0
#define LED_HOUR_BASE_INDEX 60
#define LED_SEC_COUNT 60
#define LED_MIN_COUNT 60
#define LED_HOUR_COUNT 24

#define COMET_ENABLED true
#define COMET_FADE_RATE 0.40

// RGB LED / NeoPixel Setup
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t hourColor = strip.Color(0,0,255);    // Hours are blue?
uint32_t minColor = strip.Color(0,255,0);   // Minutes are green!
uint32_t secColor = strip.Color(255,0,0);     // Seconds are red?


// ************************************************************************** //
void showTime() {
//  time(&now);                       // read the current time
//  localtime_r(&now, &tm);           // update the structure tm with the current time
  Serial.print("year:");
  Serial.print(tm.tm_year + 1900);  // years since 1900
  Serial.print("\tmonth:");
  Serial.print(tm.tm_mon + 1);      // January = 0 (!)
  Serial.print("\tday:");
  Serial.print(tm.tm_mday);         // day of month
  Serial.print("\thour:");
  Serial.print(tm.tm_hour);         // hours since midnight  0-23
  Serial.print("\tmin:");
  Serial.print(tm.tm_min);          // minutes after the hour  0-59
  Serial.print("\tsec:");
  Serial.print(tm.tm_sec);          // seconds after the minute  0-61*
  Serial.print("\twday");
  Serial.print(tm.tm_wday);         // days since Sunday 0-6
  if (tm.tm_isdst == 1)             // Daylight Saving Time flag
    Serial.print("\tDST");
  else
    Serial.print("\tstandard");
  Serial.println();
}

// ** On board LED controls - NOTE: ESP8266 builtin is inverted logic
void LED_ON() { digitalWrite(LED_BUILTIN, LOW); }
void LED_OFF() { digitalWrite(LED_BUILTIN, HIGH); }
void LED_Blink(int delay_ms) {
  LED_ON();
  delay(delay_ms);
  LED_OFF();
}

void LED_Blink(int count, int on_ms, int off_ms) {
  for(int i = 0; i < count; i++) {
    LED_ON();
    delay(on_ms);
    LED_OFF();
    delay(off_ms);
  }
}

// ****************** CHIMES STUFF *************************************
// ** Reads GPIO to decide if chimes are enabled
bool isChimesEnabled() {
  if( tm.tm_year < 100 || chimes_firstBoot ) {  // Never play on first test to speed up booting
    chimes_firstBoot = false;
    return false;
  } else if( isQuietHours() ) {
    return false;
  } else {
    return( digitalRead(CHIMES_ENABLE_PIN) == LOW );
  }
}

// ** No chimes after 9 pm or before 9 am
bool isQuietHours() {
  const int quiet_pm_hour = 9;
  const int quiet_am_hour = 9;
  if( tm.tm_hour < quiet_am_hour || tm.tm_hour > quiet_pm_hour ) {
    return true;
  } else {
    return false;
  }
}

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

// ** Play a "bong" on the pezieo
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

// ** Main chimes detection and handling function - hourly and half hourly
void handleChimes() {
  int currHour = tm.tm_hour;
  int currMinute = tm.tm_min;

  // ** Detect hour changover
  if( chimes_lastHour != currHour ) {
    chimes_lastHour = currHour;
    if( isChimesEnabled() ) {
      playHourlyChimes(currHour);
    }
  }

  if( currMinute == 30 && chimes_lastMinute != 30 ) {
    if( isChimesEnabled() ) {
      playBong(1200);
    }
  }
  chimes_lastMinute = currMinute;
}

// ** Updates the calculated time 
void updateTime() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
}

// ********************* RGB LED Clock Handling ********************************** //
void updateClockLEDs() {
  int currHour = tm.tm_hour;
  int currSec = tm.tm_sec;
  int currMin = tm.tm_min;

  clearLEDs();

  // Render out the LED colors
  showCurrSec(currSec);
  showCurrMin(currMin);
  showCurrHour(currHour);

  strip.show();
}

// ** Clear all RGB LEDs
void clearLEDs() {
  for( int i = 0; i < LED_COUNT; i++ ) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
}

// ** Calculates outer ring light index based on input index (usually a base 60 time)
int calcOuterRingIndex(int inIndex) {
  int ledIndex = 0;
  if( inIndex >= 30 ) {
    ledIndex = LED_SEC_BASE_INDEX + (inIndex - 30);
  }
  else {
    ledIndex = LED_SEC_BASE_INDEX + (inIndex + 30);
  }
  return ledIndex;
}

// ** A simple "fading comet" to make seconds more interesting
void showComet(int currSec) {
  int red = 255;
  int minRed = 10;
  while( red > minRed ) {
    red *= COMET_FADE_RATE;
    currSec--;
    if( currSec < 0 ) {
      currSec = 59;
    }
    int ledIndex = calcOuterRingIndex(currSec);
    strip.setPixelColor(ledIndex, strip.Color(red, 0, 0));
  }
}

// ** Show current second on the outer LED Ring
void showCurrSec(int currSec) {
  int ledIndex = calcOuterRingIndex(currSec);
  strip.setPixelColor(ledIndex, secColor);

  if(COMET_ENABLED){
    showComet(currSec);
  }
}

// ** Show current Minute on the outer LED Ring
void showCurrMin(int currMin) {
  int ledIndex = calcOuterRingIndex(currMin);
  strip.setPixelColor(ledIndex, minColor);
}

// ** Show the current hour on the inner LED ring
void showCurrHour(int currHour) {
  int ledIndex = 0;
  if( currHour >= 15 ) {
    ledIndex = LED_HOUR_BASE_INDEX + (currHour - 15);
  } else {
    ledIndex = LED_HOUR_BASE_INDEX + 9 + currHour;
  }
  strip.setPixelColor(ledIndex, hourColor);
}


// ************************************************************************** //
void setup() {
  Serial.begin(115200);
  Serial.println("\nNTP TZ DST - bare minimum");

  configTime(MY_TZ, MY_NTP_SERVER); // --> Here is the IMPORTANT ONE LINER needed in your sketch!

  pinMode(LED_BUILTIN, OUTPUT);   // Setup onboard LED
  pinMode(CHIMES_ENABLE_PIN, INPUT_PULLUP);


  LED_Blink(5, 200, 100); // Boot ack/light blink


  // Start up the RGB LEDs for output
  strip.begin();           // Initialize the NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(LED_BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)


  // Start network *************************
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    LED_Blink(1, 200, 200);
    Serial.print ( "." );
  }
  LED_OFF();

  WiFi.setAutoReconnect(true);
  Serial.println("\nWiFi connected");

  // ** First pass time update - May not work if NTP has not sync'd
  updateTime();
}


// ************************************************************************** //
void loop() {
  updateTime();
  showTime(); // Serial output of time

  updateClockLEDs();
  handleChimes();

  Serial.println(isChimesEnabled());

  delay(500); // dirty delay - less than 1 sec to keep UI from missing seconds
}