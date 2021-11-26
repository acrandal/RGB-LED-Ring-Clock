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

#include "RTClib.h"   // Library for Real Time Clock DS1307
#include "pitches.h"  // My set of musical note pitches

#include <Adafruit_NeoPixel.h>


// Real Time Clock Driver
RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


#define DST_ENABLE_PIN 4    // Daylight savings time switch
#define CHIMES_ENABLE_PIN 3 // Clock chimes enable switch


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

// RGB LED Ring Base Configuration
#define LED_PIN    9
#define LED_COUNT 84

#define LED_SEC_BASE_INDEX 0
#define LED_MIN_BASE_INDEX 0
#define LED_HOUR_BASE_INDEX 60
#define LED_SEC_COUNT 60
#define LED_MIN_COUNT 60
#define LED_HOUR_COUNT 24



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

// ** SHOW hourly chimes when buzzer disabled
void showHourlyChimes(int currHour) {
  LED_Blink(currHour, 100, 100);
}

// ** SHOW a half hour "bong"
void showHalfHourChimes() {
  LED_Blink(1, 1200, 0);
}


// ** Uses the RTC to get the current time as a DateTime object
DateTime getCurrentTime() {
  DateTime now = rtc.now();
  return now;
}

int getCurrHour() {
  DateTime now = getCurrentTime();
  int currHour = now.hour();
  if( isDSTEnabled() ) {
    currHour = (currHour + 1) % 24;
  }  
  return currHour;
}


// ** Calculate new clock LED lights
void updateClockLEDs() {
  DateTime now = getCurrentTime();
  int currHour = getCurrHour();

  for( int i = 0; i < LED_COUNT; i++ ) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  
  int secLED = now.second() % 60;
  strip.setPixelColor(secLED, strip.Color(127, 0, 0));

  int minLED = now.minute() % 60;
  strip.setPixelColor(minLED, strip.Color(0, 127, 0));

  int hourLED = currHour % 24 + 60;
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
    // The hour updated and chimes are enabled, time to play some chimes!
    if(isChimesEnabled()) {
      playHourlyChimes(currHour);
    } else {
      showHourlyChimes(currHour);
    }
  }

  if( currMinute == 30 && lastMinute != 30 ) {
    // Play half hour bong
    if(isChimesEnabled()) {
      playBong(1200);
    } else {
      showHalfHourChimes();
    }
  }
  lastMinute = currMinute;
}

bool isChimesEnabled() {
  return( digitalRead(CHIMES_ENABLE_PIN) == LOW );
}

bool isDSTEnabled() {
  return( digitalRead(DST_ENABLE_PIN) == LOW );
}


// On board LED controls
void LED_ON() { digitalWrite(LED_BUILTIN, HIGH); }
void LED_OFF() { digitalWrite(LED_BUILTIN, LOW); }
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


// *******************************************************************************************
void setup() {
// Support for ATtiny85 clock and the RGB LEDs
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

  pinMode(LED_BUILTIN, OUTPUT);   // Setup onboard LED
  pinMode(DST_ENABLE_PIN, INPUT_PULLUP);
  pinMode(CHIMES_ENABLE_PIN, INPUT_PULLUP);
  
  // Turn on the serial interface for debugging/testing
  Serial.begin(115200);

  LED_Blink(5, 200, 100);

  // Setup the RGB LED strip
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  // Connect over I2C to the RTC chip
  if (! rtc.begin()) {
   Serial.println("Couldn't find RTC");
   while (1) { LED_Blink(10, 50, 50); }
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is not running - need to program for current time");
    while(1) { LED_Blink(10, 50, 100); }
  }

  // Get current hour to prevent chimes on boot
  if(!isChimesEnabled()) {
    Serial.println("Chimes disabled");
    DateTime now = getCurrentTime();
    lastHour = now.hour();
  } else {
    Serial.println("Chimes enabled - play warm-up music");
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
  
