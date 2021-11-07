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


#define LED_PIN    6
#define LED_COUNT 16

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


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

// Uses the RTC to get the current time as a DateTime object
DateTime getCurrentTime() {
  DateTime now = rtc.now();
  return now;
}


// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}


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


void loop() {
  rainbow(3);             // Flowing rainbow cycle along the whole strip

  
  DateTime now = getCurrentTime();

//strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
//    strip.show();                          //  Update strip to match
//  strip.Color(255,   0,   0)
  
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

  // delay(3000);

}
