
#include <Adafruit_NeoPixel.h>

// light sensor
const int lightPin = A0;
const int hysteresisPin = A1;
const int ledPin = 7;

const int LIGHT_THRESHOLD = 512;

bool dark = false;

// relay
const int relayPin = 13;

// neopixels
const int pixelPin = 6;
const int pixelCount = 10;

Adafruit_NeoPixel strip(pixelCount, pixelPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  dark = analogRead(lightPin) < LIGHT_THRESHOLD;

  strip.begin();
  strip.show();
  strip.setBrightness(50);

  pinMode(relayPin, OUTPUT);
}

void loop() {

  int light = analogRead(lightPin);
  int hysteresis = analogRead(hysteresisPin);

  if (dark) {
    if (light > LIGHT_THRESHOLD + (hysteresis / 2)) {
      dark = false;
      Serial.print("went light ");
    }
  } else {
    if (light < LIGHT_THRESHOLD - (hysteresis / 2)) {
      dark = true;
      Serial.print("went dark ");
    }
  }

  Serial.print(hysteresis);
  Serial.print(" ");
  Serial.println(light);

  digitalWrite(ledPin, dark);
  digitalWrite(relayPin, dark);

  if (dark) {
    theaterChaseRainbow(50);
  } else {
    strip.clear();
    strip.show();
  }

}

void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
