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

// ArduinoIDE system libraries
#include <Adafruit_NeoPixel.h>
#include "RTClib.h"   // Library for Real Time Clock DS1307

// Project libraries
#include "pitches.h"  // My set of musical note pitches


// Real Time Clock Object/API
RTC_DS1307 rtc;

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

// RGB LED Ring pins and base offsets Configuration
#define LED_PIN    9
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

// Prior state recording - for hour & half hour chimes
int lastHour = -1;
int lastMinute = 0;

// Photoresistor settings
#define PHOTORESISTOR_PIN A0
#define PHOTORESISTOR_OFF_THRESHOLD 40

// Light up sign settings
#define SIGN_LED_PIN 6


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
  LED_Blink(1, 750, 0);
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

// ** Uses the RTC to get the current time as a DateTime object
DateTime getCurrentTime() {
  DateTime now = rtc.now();
  return now;
}

// ** Reads RTC and handled Daylight Savings Time adjustments
int getCurrHour() {
  DateTime now = getCurrentTime();
  int currHour = now.hour();
  if( isDSTEnabled() ) {
    currHour = (currHour + 1) % 24;
  }  
  return currHour;
}

// ** Show the current hour on the inner LED ring
void showCurrHour(int currHour) {
  int ledIndex = 0;
  if( currHour >= 15 ) {
    ledIndex = LED_HOUR_BASE_INDEX + (currHour - 15);
  }
  else {
    ledIndex = LED_HOUR_BASE_INDEX + 9 + currHour;
  }
  strip.setPixelColor(ledIndex, hourColor);
}


// ** Show current Minute on the outer LED Ring
void showCurrMin(int currMin) {
  int ledIndex = calcOuterRingIndex(currMin);
  strip.setPixelColor(ledIndex, minColor);
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


// ** Clear all RGB LEDs
void clearLEDs() {
  for( int i = 0; i < LED_COUNT; i++ ) {
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
}

// ** Calculate new clock LED lights
void updateClockLEDs() {
  DateTime now = getCurrentTime();
  int currHour = getCurrHour();
  int currSec = now.second();
  int currMin = now.minute();

  clearLEDs();

  // Render out the LED colors
  showCurrSec(currSec);
  showCurrMin(currMin);
  showCurrHour(currHour);

  strip.show(); // Ensure all updates are rendered
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


// ** On board LED controls
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

// ** Light up sign status
void handleSignState() {
  int curr_photoresistor_read_value = analogRead(PHOTORESISTOR_PIN);
  // Serial.print("Photo read: ");
  // Serial.println(curr_photoresistor_read_value);
  if( curr_photoresistor_read_value < PHOTORESISTOR_OFF_THRESHOLD ) {
    digitalWrite(SIGN_LED_PIN, HIGH);
  } else {
    digitalWrite(SIGN_LED_PIN, LOW);
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

  pinMode(SIGN_LED_PIN, OUTPUT);
  pinMode(PHOTORESISTOR_PIN, INPUT);
  
  // Turn on the serial interface for debugging/testing
  Serial.begin(115200);

  LED_Blink(5, 200, 100);

  // Setup the RGB LED strip
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(LED_BRIGHTNESS); // Set BRIGHTNESS to about 1/5 (max = 255)

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
  handleSignState();

  delay(1000);
}
