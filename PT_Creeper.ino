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
#include <math.h>
// test 1
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

// Game mode 1a
#define GAME_1A 4
#define GAME_1A_STEP_0 0
#define GAME_1A_STEP_1 1
#define GAME_1A_STEP_2 2
#define GAME_1A_STEP_3 3
#define GAME_1A_STEP_4 4
#define GAME_1A_STEP_5 5
int game1A_step = GAME_1A_STEP_0;

// Game mode 1b
#define GAME_1B 5
#define GAME_1B_STEP_0 0
#define GAME_1B_STEP_1 1
#define GAME_1B_STEP_2 2
#define GAME_1B_STEP_3 3
#define GAME_1B_STEP_4 4
int game1B_step = GAME_1B_STEP_0;
int knockInterval_ms = 1000;
int lastKnock_ms = 0;
bool fadeOn = true; // True meads fade on, false fade off
int knockCount = 0;
int game1B_knockThreshold = 5;
int game1B_redHold_ms = 20000;
int game1B_blueHold_ms = 20000;
int game1B_lastColorHold = 0;

/* 
  Used to track when to do events. If microcontroller runs at 
  full speed, then events get skipped. This creates a 'heartbeat'
  for actions
*/
uint32_t time_ms = 0;
bool readyAction = false;
uint32_t globalDelay = 50; // in milliseconds, the 'heartbeat' of the device.

/* Knock sensor *************/
int knockSensor = A0;  // the piezo is connected to analog pin 0
int knockThreshold = 100;
uint32_t knockDebounce = 50;
uint32_t lastKnockRead = millis();
int knockReading = 0;
bool knockDetected = false;


/* Tilt sensor *************/
#define TILTSENSORPIN 14//2
#define TILTSENSORPINPOWER 12//3
bool currentDirection = false;
uint32_t lastTiltSwitch = millis();
uint32_t tiltDebounce = 500;
double tiltAverage = 0;
double lastTiltAverage = 0;
double tiltIintegrationSteps = 10;
bool tiltFlag = false;
bool lastTiltFlag = false;
bool flipped = false;

/* Define Neopixel *********/
#define LED_PIN    5//6
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 14
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Neo-pixel predefined colors. For the strip in use, the colors are RGB
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t off = strip.Color(0, 0, 0);

uint32_t currentBrightness = 50; // Tracks current brightness value
uint32_t maxBrightness = 150; // 0-255, above 100 didn't see much difference
uint32_t currentPixelIndex = 0; // Used for sequential pixel actions
uint32_t currentPixelColor = off; //used to set global color
/***************************/

void setup() {

  operationState = GAME_1B;

  // init serial and timers
  Serial.begin(115200);
  delay(100);
  Serial.println("Starting creeper");
  time_ms = millis();
  delay(100);

  // set tilt sensor pin settings
  pinMode(TILTSENSORPIN,INPUT_PULLUP); // Sets tilt sensor pin read to high when not grounded
  
  pinMode(TILTSENSORPINPOWER,OUTPUT); // Set neighboring bin to ground
  digitalWrite(TILTSENSORPINPOWER,LOW);
    
  // Begin Neo-pixel
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(currentBrightness); // Set BRIGHTNESS to about 1/5 (max = 255)
}

byte byte1 = 0;
byte byte2 = 0;
int lastStep =-1;

void loop() {
  // check timer to see if enough time has passed for action to happen
  if(millis()-time_ms > globalDelay) {
    time_ms = millis();
    readyAction = true;
  }
  else {readyAction = false;}

  // Output state to serial if there is a state change (used for debugging)
  if(prevOperationState != operationState) {
    Serial.print("State: ");Serial.println(operationState);
    prevOperationState = operationState;
  }

  if(millis()-lastTiltSwitch > tiltDebounce) {
    currentDirection = digitalRead(TILTSENSORPIN);
    tiltAverage = lastTiltAverage * (tiltIintegrationSteps-1.0)/tiltIintegrationSteps + double(currentDirection)/tiltIintegrationSteps;
    lastTiltAverage = tiltAverage;
    
    if(round(tiltAverage) == 1) {tiltFlag = true;}
    else
    {tiltFlag = false;}

    if(lastTiltFlag != tiltFlag) {
      lastTiltFlag = tiltFlag;
      flipped = true;
      Serial.println("Flipped!");
    }
    
    lastTiltSwitch = millis();
  }

  // If enough time has passed to use knock sensor and the knock sensor reads above
  //  threshold, store knock value. 
  // Insert flags here for code that requires knock sensor reaction. 
  knockReading = analogRead(knockSensor);
  if((millis() - lastKnockRead > knockDebounce) && knockReading > 150){
    
    Serial.print("k: ");Serial.println(knockReading);
    lastKnockRead = millis();
    knockDetected = true;
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
      'ga': Game mode 1a. Single knock on or off
      'gb': Game mode 1b. 
  */  
  byte1 = 0;
  byte2 = 0;

  bool found_g = false;

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
        case 'g': // set state for waiting for knock.
          found_g = true;
      }

      // Set color of action defined in above switch statement
      switch(byte2) {
        case 'r':
          currentPixelColor = red;break;
        case 'g':
          currentPixelColor = green;break;
        case 'l':
          currentPixelColor = blue;break;
        case 'a':
          found_g = false;
          operationState = GAME_1A;
          game1A_step = GAME_1A_STEP_0;
          break;
        case 'b':
          found_g = false;
          operationState = GAME_1B;
          game1B_step = GAME_1B_STEP_0;
          break;
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
      case GAME_1A:
        /*
          Robots must defuse Creepers to protect the village. Multiple strategies are possible:
          Tap to Inert – Tap a Creeper 5 times → it turns RED (inert). Respawns after 10s.
          Flip to Inert – Flip a Creeper to the stone side → it turns BLUE (inert).  
                          Respawns after 20 seconds.
          Dump in Lava – Push Creepers into the lava river → permanently neutralized 
                         (unless recovered by opposing Human Player while active).
          Dungeon - Push Creepers into Dungeon → Human Player interacts, 
                    makes Creeper inert, and ejects it back into field.
 
          Steps for sprint 1a
          1 - Wait for knock, fade on green go to 2
          2 - Wait for knock, fade off green go to 1
              if 1 or 2 picks up knock, go to 3
          3 - Knock detected, turn off pixels go to 4
          4 - Knock detected, turn on pixels green
        */
        if(lastStep != game1A_step) {
          Serial.print("game_1A step: ");
          Serial.println(game1A_step);
          lastStep = game1A_step;
        }
        //Serial.print("flag: ");Serial.println(tiltFlag);

        switch(game1A_step) {
          case GAME_1A_STEP_0:
            currentBrightness = 0;
            currentPixelColor = green;
            flipped = false;
            strip.clear();
            strip.show();
            game1A_step = GAME_1A_STEP_1;
            break;
          case GAME_1A_STEP_1:
            currentBrightness += fadeRateMultiplier;
            
            if(currentBrightness > maxBrightness) {
              currentBrightness = maxBrightness;
              game1A_step = GAME_1A_STEP_2;
            }
            strip.setBrightness(currentBrightness);
            strip.fill(currentPixelColor, 0, LED_COUNT);strip.show();
            if(knockDetected){game1A_step = GAME_1A_STEP_3;}
            break;
          case GAME_1A_STEP_2:
            currentBrightness -= fadeRateMultiplier;
            
            if(currentBrightness+fadeRateMultiplier < fadeRateMultiplier) {
              currentBrightness = 0;
              game1A_step = GAME_1A_STEP_1;
            }
            strip.setBrightness(currentBrightness);
            strip.fill(currentPixelColor, 0, LED_COUNT);strip.show();
            if(knockDetected){game1A_step = GAME_1A_STEP_3;}
            break;
          case GAME_1A_STEP_3:
            knockDetected = false;
            strip.clear();
            strip.show();
            Serial.println("Turning off pixels, knock detected");
            game1A_step = GAME_1A_STEP_4;
            break;
          case GAME_1A_STEP_4:
            if(knockDetected){
              knockDetected = false;
              game1A_step = GAME_1A_STEP_5;
              strip.setBrightness(maxBrightness);
              currentPixelColor = red;
              strip.fill(currentPixelColor, 0, LED_COUNT);strip.show();
              currentPixelColor = green;
            }
            if(tiltFlag) {game1A_step = GAME_1A_STEP_0;}
            break;
          case GAME_1A_STEP_5:
            if(knockDetected){
              knockDetected = false;
              game1A_step = GAME_1A_STEP_4;
              strip.clear();
              strip.show();
              currentPixelColor = green;
            }
            if(tiltFlag) {game1A_step = GAME_1A_STEP_0;}
            break;
        
        }
        break;
      case GAME_1B:
        /* Steps for sprint 1B
          0 - Turn off all pixels, set color to gren
          
             
        */
        knockThreshold = game1B_knockThreshold;
        if(lastStep != game1B_step) {
          Serial.print("game_1B step: ");
          Serial.println(game1B_step);
          lastStep = game1B_step;
        }
        //Serial.print("flag: ");Serial.println(tiltFlag);
        
        switch(game1B_step) {
          case GAME_1B_STEP_0: // Start
            flipped = false;
            currentBrightness = 0;
            currentPixelColor = green;
            strip.fill(green, 0, LED_COUNT);
            strip.clear();
            strip.show();
            game1B_step = GAME_1B_STEP_1;
            currentPixelIndex = 0;
            knockDetected = false;
            break;
          case GAME_1B_STEP_1: // Fade green until knocked or flipped
            if(fadeOn) {currentBrightness += fadeRateMultiplier;}
            else {currentBrightness -= fadeRateMultiplier;}
            
            if(currentBrightness >= maxBrightness){currentBrightness = maxBrightness;fadeOn = false;}
            if(currentBrightness <= 0 || currentBrightness > 1000){currentBrightness = 0;fadeOn = true;}
            
            strip.setBrightness(currentBrightness);
            //Serial.print("pixel index: ");Serial.println(currentPixelIndex);
            for(int i=0;i<LED_COUNT;i++){
              if(i < currentPixelIndex)
              {strip.setPixelColor(i,red);}
              else
              {strip.setPixelColor(i,green);}
            }
            strip.show();

            if(knockDetected && ((millis() - lastKnock_ms) > knockInterval_ms)){
              knockDetected = false;
              lastKnock_ms = millis();
              knockCount++;
              currentPixelIndex = currentPixelIndex+3;
              //strip.setPixelColor(currentPixelIndex--,off);
              //strip.setPixelColor(currentPixelIndex--,off);
              //strip.show();
              if(knockCount >= knockThreshold) {
                knockCount = 0;
                game1B_step = GAME_1B_STEP_2;
                game1B_lastColorHold = millis();
                currentBrightness = maxBrightness;
                currentPixelColor = red;
                strip.fill(currentPixelColor, 0, LED_COUNT);
                strip.show();            
              }
            }

            if(flipped) {
              game1B_step = GAME_1B_STEP_3;
              game1B_lastColorHold = millis();
              knockCount = 0;
              currentBrightness = maxBrightness;
              currentPixelColor = blue;
              strip.fill(currentPixelColor, 0, LED_COUNT);
              strip.show();            
            }

            break;
          case GAME_1B_STEP_2: // wait for red time out
            if(millis() - game1B_lastColorHold > game1B_redHold_ms) {
              game1B_step = GAME_1B_STEP_0;
            }
            break;
          case GAME_1B_STEP_3: // wait for blue time out
            if(millis() - game1B_lastColorHold > game1B_blueHold_ms) {
              game1B_step = GAME_1B_STEP_0;
            }
            break;
          case GAME_1B_STEP_4: // unused
            break;

        }
        break;
    }
  }

}

