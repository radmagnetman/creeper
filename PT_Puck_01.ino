/*
  Creeper control v0.1
  This code base interfaces with an arduino-like microcontroller to ultimately
  control a string of neopixels. Attached to the arduino are the neo-pixels, 
  a knock sensor, and a tilt sensor. 
  This code was created for <redacted> to be used in <redacted>.

  Author: T. Wright
  Date: 8/31/2025
*/
#include <Adafruit_NeoPixel.h>

/*
  The following declarations create the various state machines
  needed for operation
*/
// Stopped state, turn off neo-pixels
#define STOPPED 0
int operationState = STOPPED;
int prevOperationState = STOPPED+1;

// Turn on each neo-pixel in strip until all are on, then
// turn them off sequentially
#define RUNNING_UP 1
#define RUNNING_UP_OFF 0
#define RUNNING_UP_ON 1
int runningUpState = RUNNING_UP_OFF;

// Fade all neo-pixels on then off.
// fadeRateMultiplier changes the speed of the fade. Higher
//  means faster. 1 lets it run at the default rate (too slow imo)
#define RUNNING_FADE 2
#define RUNNING_FADE_IN 0
#define RUNNING_FADE_OUT 1
int runningFadeState = RUNNING_FADE_IN;
int fadeRateMultiplier = 5;

// Turn on all neo-pixels at once.
#define RUNNING_ALLON 3

/* 
  Used to track when to do events. If microcontroller runs at 
  full speed, then events get skipped. This creates a 'heartbeat'
  for actions
*/
long time = 0;
bool readyAction = false;
uint32_t globalDelay = 50; // in milliseconds, the 'heartbeat' of the device.

/* Knock sensor *************/
int knockSensor = A0;  // the piezo is connected to analog pin 0
int knockThreshold = 100;
long knockDebounce = 1000;
long lastKnockRead = millis();
int knockReading = 0;

/* Tilt sensor *************/
#define TILTSENSORPIN 2
bool currentDirection = false;
bool lastDirection = false;
long lastTiltSwitch = millis();
long tiltDebounce = 3000;

/* Define Neopixel *********/
#define LED_PIN    6
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 16
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Neo-pixel predefined colors. For the strip in use, the colors are RGB
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t off = strip.Color(0, 0, 0);

uint32_t currentBrightness = 50; // Tracks current brightness value
uint32_t maxBrightness = 100; // 0-255, above 100 didn't see much difference
uint32_t currentPixelIndex = 0; // Used for sequential pixel actions
uint32_t currentPixelColor = off; //used to set global color
/***************************/

void setup() {
  // init serial and timers
  Serial.begin(115200);
  Serial.println("Starting creeper");
  time = millis();
  delay(1000);

  // set tilt sensor pin settings
  pinMode(TILTSENSORPIN,INPUT);
  digitalWrite(TILTSENSORPIN, HIGH);
  
  // Begin Neo-pixel
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(currentBrightness); // Set BRIGHTNESS to about 1/5 (max = 255)
}

byte byte1 = 0;
byte byte2 = 0;

void loop() {
  // check timer to see if enough time has passed for action to happen
  if(millis()-time > globalDelay) {
    time = millis();
    readyAction = true;
  }
  else {readyAction = false;}

  // Output state to serial if there is a state change (used for debugging)
  if(prevOperationState != operationState) {
    Serial.print("State: ");Serial.println(operationState);
    prevOperationState = operationState;
  }

  // debounce timing and update of tilt sensor status
  if(millis()-lastTiltSwitch > tiltDebounce) {
    currentDirection = digitalRead(TILTSENSORPIN);
    if(currentDirection != lastDirection) {
      Serial.print("Direction: ");
      Serial.println(currentDirection);
      lastDirection = currentDirection;
      lastTiltSwitch = millis();
    }
  }

  // If enough time has passed to use knock sensor and the knock sensor reads above
  //  threshold, store knock value. 
  // Insert flags here for code that requires knock sensor reaction. 
  knockReading = analogRead(knockSensor);
  if((millis() - lastKnockRead > knockDebounce) && (knockReading > knockThreshold)){
    
    Serial.print("k: ");Serial.println(knockReading);
    lastKnockRead = millis();
  }

  /* 
    These commands can be hijacked for other uses such as a web interface. 
    Serial command set:
      's': Stop, all off neopiexl
      'r': Turn all on red
      'g': Turn all on green
      'b': Turn all on blue
      'ur','ug','ub': Turn on color, 1 by 1, then turn off, repeat
      'fr','fg','fb': Fade all on at specified color, then fade off
  */  
  byte1 = 0;
  byte2 = 0;
  if(Serial.available()) {
    byte1 = Serial.read();
    delay(50); // Normally delays not welcome, but successive read commands were getting skipped
    byte2 = Serial.read();
    while(Serial.available()){Serial.println("flush");Serial.read();} // flush buffer
    
    Serial.print("1: ");Serial.println((int) byte1);
    Serial.print("2: ");Serial.println((int) byte2);
    
    // if the second byte is new line character, then working with single character command
    if(byte2 == '\n') { 
      switch(byte1) {
        case 's': // received stop command
          operationState = STOPPED;
          break;
        case 'r': // Turn on all to red
          currentPixelColor = red;
          operationState = RUNNING_ALLON;
          break;
        case 'g': // Turn on all to green
          currentPixelColor = green;
          operationState = RUNNING_ALLON;
          break;
        case 'b': // Turn on all to blue
          currentPixelColor = blue;
          operationState = RUNNING_ALLON;
          break;
      }
    }
    else {
      switch(byte1) {
        case 'u': // Set state for sequential turning on of all LEDs. 
          operationState = RUNNING_UP;
          runningUpState = RUNNING_UP_ON;
          currentPixelIndex = 0; 
          break;
        case 'f': // Set state for fading pixels on and then off
          operationState = RUNNING_FADE;
          runningFadeState = RUNNING_FADE_IN;
          currentBrightness = 0;
          break;
      }

      // Set color of action defined in above switch statement
      switch(byte2) {
        case 'r':
          currentPixelColor = red;break;
        case 'g':
          currentPixelColor = green;break;
        case 'b':
          currentPixelColor = blue;break;
      }
    }
  }

  // STATE MACHINE
  // Using above commands, react to inputs.
  // Using readyAction to put everything on a global 'heartbeat'
  if(readyAction) {
    // Determine which state command was received from above.
    switch(operationState) {
      case STOPPED: // Turn off all pixels.
        strip.clear();
        strip.show();
        break;
      case RUNNING_UP: 
      // Turn on pixel to set color and brightness, increment index for next loop
      //  Using runningUpState to indicate whether turning on or off. When reached 0
      //  or max pixels, reset to zero and repeat with opposite state.
        if(runningUpState == RUNNING_UP_ON) { 
          currentPixelIndex++;
          strip.setPixelColor(currentPixelIndex,currentPixelColor);
          strip.show();

          if(currentPixelIndex > LED_COUNT) {
            runningUpState = RUNNING_UP_OFF;
            currentPixelIndex = 0;
          }
        }
        if(runningUpState == RUNNING_UP_OFF) {
          currentPixelIndex++;
          strip.setPixelColor(currentPixelIndex,off);
          strip.show();
          if(currentPixelIndex > LED_COUNT) {
            runningUpState = RUNNING_UP_ON;
            currentPixelIndex = 0;
          }
        }
        break;
      case RUNNING_FADE:
      // Set global brightess by increments of fadeRateMultiplier. Update all pixels
      // at the same time. When reach max brightness set fade direction downwards and 
      // subtract by multiplier. Repat sequence when bightness reaches zero.
        if(runningFadeState == RUNNING_FADE_IN) {
          currentBrightness += fadeRateMultiplier;
          if(currentBrightness > maxBrightness) {currentBrightness = maxBrightness;}
          strip.setBrightness(currentBrightness);
          if(currentBrightness == maxBrightness) {runningFadeState = RUNNING_FADE_OUT;}
        }
        if(runningFadeState == RUNNING_FADE_OUT) {
          currentBrightness -= fadeRateMultiplier;
          if(currentBrightness < 0) {currentBrightness = 0;}
          strip.setBrightness(currentBrightness);
          if(currentBrightness == 0) {runningFadeState = RUNNING_FADE_IN;}
        }
        strip.fill(currentPixelColor, 0, LED_COUNT);
        strip.show();
        break;
      case RUNNING_ALLON:
      // Turn on all pixels to the designated color.
        strip.fill(currentPixelColor, 0, LED_COUNT);
        strip.show();
        break;
    }
  }

}

