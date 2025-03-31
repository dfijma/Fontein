#include <Adafruit_NeoPixel.h>
#include <Wire.h> 
#include <Adafruit_GFX.h>
#include <limits.h>
#include <Adafruit_LEDBackpack.h>
#include <RTClib.h>

// light sensor
const int lightPin = A0;
const int hysteresisPin = A1;
const int ledPin = 7;

const int LIGHT_THRESHOLD = 512;

bool dark = false;

// relay
const int relayPin = 11;

// neopixels
const int pixelPin = 6;
const int pixelCount = 10;

Adafruit_NeoPixel strip(pixelCount, pixelPin, NEO_GRB + NEO_KHZ800);

int b[pixelCount];

// clock, display and RTC

Adafruit_7segment matrix = Adafruit_7segment();

const int I2C_7SEGMENT = 0x70;
const int BUTTON_1_PIN = 8;
const int BUTTON_2_PIN = 9;
const int BUZZER_PIN = 10;

RTC_DS1307 rtc = RTC_DS1307();

int hours;
int minutes;

struct BUTTON_STATE {
  int activeState; // active or inactive
  int holdState; // if active, normal or hold
  unsigned long activeEntered;
  unsigned long lastIncrementedDuringHold;  
};

BUTTON_STATE buttonsState = { HIGH, HIGH, 0L, 0L };
unsigned long lastTriggerShowBufferTime = LONG_MIN;

inline int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

void drawHoursMinutes(int hours, int minutes) {
  matrix.writeDigitNum(0, hours / 10, false);
  matrix.writeDigitNum(1, hours % 10, false);
  matrix.writeDigitNum(3, minutes / 10, false);
  matrix.writeDigitNum(4, minutes % 10, false);
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  dark = analogRead(lightPin) < LIGHT_THRESHOLD;

  strip.begin();
  strip.show();
  strip.setBrightness(50);
  for (int i=0; i < pixelCount; i++) {
    b[i] = random(256);
  }

  pinMode(relayPin, OUTPUT);

  matrix.begin(0x70);
  matrix.setBrightness(5);
  pinMode(BUTTON_1_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  rtc.begin();
}

void loop() {

  int light = analogRead(lightPin);
  int hysteresis = analogRead(hysteresisPin);

  if (dark) {
    if (light > LIGHT_THRESHOLD + (hysteresis / 2)) {
      dark = false;
      //Serial.print("went light ");
    }
  } else {
    if (light < LIGHT_THRESHOLD - (hysteresis / 2)) {
      dark = true;
      // Serial.print("went dark ");
    }
  }

  // DEBUG
  Serial.print(hysteresis);
  Serial.print(" ");
  Serial.println(light);



  // get input and current time
  int button1 = digitalRead(BUTTON_1_PIN);
  int button2 = digitalRead(BUTTON_2_PIN);
  unsigned long currentMillis = millis();
  DateTime now = rtc.now();

  bool setRTC = false;

  // are we displaying actual clock time or buffer?
  boolean showBufferTime = (currentMillis - lastTriggerShowBufferTime) < 5000;

  // the most complicated part: the key processing state machine
  if (buttonsState.activeState == HIGH) {
    // current state: INACTIVE (clock is running)
    
    if (button1 == LOW) {
      // going to ACTIVE (setting time, not yet 'holding')

      // ACTION

      // init clock-setbuffer from RTC
      hours = now.hour();
      minutes = now.minute();

      if (showBufferTime) {
        // if already showing the buffer, increment of decrement it
        minutes = minutes + 1;
      } else {
            //... otherwise, 'eat' this press to display the buffer
        showBufferTime = true;
      }
      
      // SHIFT STATE
      buttonsState.activeState = LOW;
      buttonsState.holdState = HIGH;
      buttonsState.activeEntered = currentMillis;
      lastTriggerShowBufferTime = currentMillis;
    }
  } else if (buttonsState.activeState == LOW) {
    // ACTIVE or ACTIVE HOLD
    if (button1 == HIGH) {
      // going to INACTIVE

      // ACTION
      setRTC = true;

      // SHIFT STATE
      buttonsState.activeState = HIGH;


    } else {
      if (buttonsState.holdState == HIGH) {
        // ACTIVE, but not (yet) holding
        if ((currentMillis - buttonsState.activeEntered) > 2000) {
          // but now HOLDING, as 2000 millisecs elapsed since first press
          buttonsState.holdState = LOW;
        }
      } else {
        // ACTIVE HOLDING, increment or decrement with 30 minutes every 500 millieseconds
        if ((currentMillis - buttonsState.lastIncrementedDuringHold) > 500) {

          // ACTION
          minutes += 30; 

          // SHIFT STATE
          buttonsState.lastIncrementedDuringHold = currentMillis;
          lastTriggerShowBufferTime = currentMillis;
        }
      }
    }
  } 

  // correct buffer from over-/underflow
  // TODO: use positive_modulo
  if (minutes < 0) {
    hours--;
    minutes += 60;
  }
  
  hours += (minutes / 60);
  minutes = minutes % 60;

  // TODO: use positive_modulo
  hours = hours % 24;
  if (hours < 0) {
    hours += 24; 
  }

  // set RTC from buffer
  if (setRTC) {
    // we actually don't care about the date
    rtc.adjust(DateTime(2020, 1, 1, hours, minutes, 0));
  }

  // update display   
  if (showBufferTime) {
    drawHoursMinutes(hours, minutes);   
    matrix.drawColon(true);   
  } else {
    drawHoursMinutes(now.hour(), now.minute());
    matrix.drawColon(now.second() % 2 == 0);   
  }

  // DEBUG
  // Serial.print(now.hour()); Serial.print(":"); Serial.println(now.minute());

  matrix.writeDisplay();

  if (dark && !showBufferTime && (now.hour() >= 6) ) {
    neoPixelStep();
  } else {
    strip.clear();
    strip.show();
  }
  
  digitalWrite(ledPin, dark );
  digitalWrite(relayPin, true);

  delay(50);
}

void neoPixelStep() {
  for (int i = 0; i<pixelCount; i++) {
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(180*256, b[i], b[i])));

    b[i]--;
    if (b[i]<0) b[i] = random(256);
  }
  strip.show();
}
