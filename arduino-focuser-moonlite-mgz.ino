// Moonlite-compatible stepper controller
//
// Author: Michele Guzzini <michele.gz@gmail.com>
// Based on original work by Orlando Andico <orly.andico@gmail.com>
//
// THIS PROJECT IS STILL IN EXPERIMENTAL STAGE
// This software provided "as is".
// The author is not responsible for any hardware or software issue.
// Use at your own risk.

// Main enhancements from original version:
// - possibility to choose between Motorshield v1 and generic H-Bridge motor
// - half step implementation
// - DHT11 temperature support
// - speed selection from Moonlite GUI
// - save last position to EEPROM
// - compatible with Ardufocus ASCOM driver, this allow to avoid the capacitor for easier firmware upload (see note below)
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
// Uses AFMotor and the Adafruit v1.2 Motor Shield https://learn.adafruit.com/adafruit-motor-shield
// Uses my custom MicStep library for generic H-Bridge stepper driver
//
// Original note by Orly:
// Requires a 10uf - 100uf capacitor between RESET and GND on the motor shield; this prevents the
// Arduino from resetting on connect (via DTR going low).  Without the capacitor, this sketch works
// with the stand-alone Moonlite control program (non-ASCOM) but the ASCOM driver does not detect it.
// Adding the capacitor allows the Arduino to respond quickly enough to the ASCOM driver probe
//


//-------------------------------------------------------
// CONFIGURATION IS INCLUDED IN EXTERNAL FILE
//-------------------------------------------------------

#include "config_nano.h"

//------------------------------------------------------------------
// Don't edit below this line unless you know what you are doing
//-------------------------------------------------------------------

//#define DEBUG_LOG
#define RESTORE_POSITION_ON_STARTUP //read last position from EEPROM on startup, comment out to always start from HOME_POSITION
#define MAXCOMMAND 12
#define READ_TEMP_INTERVAL 60000

/*
---- From Moonlight documentation about speed selection ---
Valid values to send are 02,
04, 08, 10 and 20, which correspond to a stepping
delay of 250, 125, 63, 32 and 16 steps per second
respectively.
---
We use these values to change speed_factor to our custom values
*/

#define DEFAULT_MOONLITE_SPEED 0x08 //the middle value corresponds to a speed factor 1


#ifdef MOTORSHIELD
  #include <AFMotor.h>
  AF_Stepper motor(MOTORSHIELD_STEPS, MOTORSHIELD_NUMBER);
#endif

#ifdef HBRIDGE
  #include "MicStep.h"
  #ifdef HBRIDGE_PWM_ENABLED
    Stepper motor(HBRIDGE_MOTORPIN1,HBRIDGE_MOTORPIN2,HBRIDGE_MOTORPIN3,HBRIDGE_MOTORPIN4,HBRIDGE_PWMPIN_A,HBRIDGE_PWMPIN_B,HBRIDGE_POWERFACTOR);
  #else
    Stepper motor(HBRIDGE_MOTORPIN1,HBRIDGE_MOTORPIN2,HBRIDGE_MOTORPIN3,HBRIDGE_MOTORPIN4);
  #endif
#endif

#include <AccelStepper.h>
#include <EEPROMex.h>

#ifdef DHT11_PRESENT
  #include <DHT11.h>
  DHT11 dht11(DHT11_PIN);
#endif

#define EEPROM_SIZE 512
#define EEPROM_VERSION 10 //change this to force re-initialization of EEPROM
#define EEPROM_VERSION_ADDR 0
#define POS_ADDR 4
#define WRITE_DELAY 120000 // delay between eeprom writes

char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
long pos = HOME_POSITION;
int isRunning = 0;
int moonlite_speed = DEFAULT_MOONLITE_SPEED;
int eoc = 0;
int idx = 0;
unsigned long millisLastMove = 0;
unsigned long millisLastTemp = 0;
bool halfstep = HALFSTEP;
int temperature = 0;
const int temp_offset = TEMP_OFFSET;
unsigned long lastSaveTime = 0;
bool needsSave = false;
long savedPosition = HOME_POSITION;

void forwardstep() {  
  #ifdef MOTORSHIELD
    if (!halfstep) 
      motor.onestep(FORWARD, DOUBLE);
    else motor.onestep(FORWARD, INTERLEAVE);
  #endif
  
  #ifdef HBRIDGE
    if (!halfstep) 
      motor.step(FORWARD, DOUBLE);
    else motor.step(FORWARD, INTERLEAVE);
  #endif
}

void backwardstep() {  
  #ifdef MOTORSHIELD
    if (!halfstep) 
      motor.onestep(BACKWARD, DOUBLE);
    else motor.onestep(BACKWARD, INTERLEAVE);
  #endif
  
  #ifdef HBRIDGE
    if (!halfstep) 
      motor.step(BACKWARD, DOUBLE);
    else motor.step(BACKWARD, INTERLEAVE);
  #endif
}

AccelStepper stepper(forwardstep, backwardstep);

float speedFactor() {

  switch(moonlite_speed) {

      case 0x20:
        return 0.25;
      case 0x10:
        return 0.5;
      case 0x08:
        return 1.0;
      case 0x04:
        return 1.5;
      case 0x02:
        return 2;

      default:
        return 1.0;

  }
}

void logSerial(String message) {

 #ifdef DEBUG_LOG
  Serial.println(message);
#endif

  
} 

void setup()
{  
  Serial.begin(9600);

  
  pinMode(PIN_LED, OUTPUT);
  
  EEPROM.setMemPool(0, EEPROM_SIZE);

  // read and check EEPROM version
  int eepromVersion = EEPROM.readInt(EEPROM_VERSION_ADDR);

  logSerial("EEPROM version: " + String(eepromVersion));

  if (eepromVersion == EEPROM_VERSION) {
    // Correct version - read the position
    savedPosition = readPositionFromEEPROM();
    logSerial("Last saved position: " + String(savedPosition));
  } else {
    // First run or version mismatch - initialize EEPROM
    initializeEEPROM();
    savedPosition = HOME_POSITION;
  }

  stepper.setSpeed(DEFAULT_STEPPER_SPEED);
  stepper.setMaxSpeed(DEFAULT_STEPPER_SPEED);
  stepper.setAcceleration(ACCELERATION);
  stepper.enableOutputs();

  #ifdef RESTORE_POSITION_ON_STARTUP
  stepper.setCurrentPosition(savedPosition);
  #else
  stepper.setCurrentPosition(HOME_POSITION);
  #endif


  memset(line, 0, MAXCOMMAND);
  
  #ifdef DHT11_PRESENT
    temperature = dht11.readTemperature();
    millisLastTemp = millis();
  #endif

  millisLastMove = millis();
  
}

void loop(){
  // run the stepper if there's no pending command and if there are pending movements
  if (!Serial.available())
  {
    if (isRunning) {
      stepper.run();
      millisLastMove = millis();
    } 
    else {
      // reported on INDI forum that some steppers "stutter" if disableOutputs is done repeatedly
      // over a short interval; hence we only disable the outputs and release the motor some seconds
      // after movement has stopped
      if ((millis() - millisLastMove) > MOTOR_RELEASE_TIMEOUT) {
        stepper.disableOutputs();
        delay(10);
        motor.release();
      }

        // DHT11 temp reading is very slow so we read it during idle time instead of reading it on serial request
        #ifdef DHT11_PRESENT
        if ((millis() - millisLastTemp) > READ_TEMP_INTERVAL) {
         int t = dht11.readTemperature();
         if (t != DHT11::ERROR_CHECKSUM && t != DHT11::ERROR_TIMEOUT)
             temperature = t + temp_offset;

          millisLastTemp = millis();
        }
        #endif

    }

    if (stepper.distanceToGo() == 0) {
      //stepper.run();
      isRunning = 0;
    }
  } 
  else {

    // read the command until the terminating # character
    while (Serial.available() && !eoc) {
      inChar = Serial.read();
      if (inChar != '#' && inChar != ':') {
        line[idx++] = inChar;
        if (idx >= MAXCOMMAND) {
          idx = MAXCOMMAND - 1;
        }
      } 
      else {
        if (inChar == '#') {
          eoc = 1;
        }
      }
    }
  } // end if (!Serial.available())

  // process the command we got
  if (eoc) {
    memset(cmd, 0, MAXCOMMAND);
    memset(param, 0, MAXCOMMAND);

    int len = strlen(line);
    if (len >= 2) {
      strncpy(cmd, line, 2);
    }

    if (len > 2) {
      strncpy(param, line + 2, len - 2);
    }

    memset(line, 0, MAXCOMMAND);
    eoc = 0;
    idx = 0;

    // the stand-alone program sends :C# :GB# on startup
    // :C# is a temperature conversion, doesn't require any response

    // LED backlight value, always return "00"
    if (!strcasecmp(cmd, "GB")) {
      Serial.print("00#");
    }

    // home the motor, hard-coded, ignore parameters since we only have one motor
    if (!strcasecmp(cmd, "PH")) { 
      
      pos=HOME_POSITION;
      stepper.setCurrentPosition(HOME_POSITION);

      isRunning = 1;
    }

    // firmware value, always return "10"
    if (!strcasecmp(cmd, "GV")) {
      Serial.print("10#");
    }

    // get the current motor position
    if (!strcasecmp(cmd, "GP")) {
      pos = stepper.currentPosition();
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the new motor position (target)
    if (!strcasecmp(cmd, "GN")) {
      pos = stepper.targetPosition();
      char tempString[6];
      sprintf(tempString, "%04X", pos);
      Serial.print(tempString);
      Serial.print("#");
    }

    // get the last temperature read
    if (!strcasecmp(cmd, "GT")) {

      #ifdef DHT11_PRESENT

       char tempString[6];

       sprintf(tempString, "%04X", temperature*2); //must be multplied by 2 to be compliant with Moonlite protocol

        if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
            Serial.print(tempString);
            Serial.print("#");
        } else {
            // error
            Serial.print("0000#");
        }
      #else
            Serial.print("0000#");
      #endif
    }

    // get the temperature coefficient, hard-coded
    if (!strcasecmp(cmd, "GC")) {
      Serial.print("02#");
    }

    // get the current motor speed, only values of 02, 04, 08, 10, 20
    if (!strcasecmp(cmd, "GD")) {
      char tempString[6];
      sprintf(tempString, "%02X", moonlite_speed);
      Serial.print(tempString);
      Serial.print("#");
    }

    // set speed
    if (!strcasecmp(cmd, "SD")) {
      moonlite_speed = hexstr2long(param);

   /*   Serial.print("_SDPARAM:");
      Serial.print(param);
      Serial.print("_MOOLITESPEED1:");
      Serial.print(moonlite_speed);*/

    if (moonlite_speed != 0x02 &&
        moonlite_speed != 0x04 &&
        moonlite_speed != 0x08 &&
        moonlite_speed != 0x10 &&
        moonlite_speed != 0x20) {
          moonlite_speed = 0x08;  // fallback
    }
      
    /*  Serial.print("_MOOLITESPEED2:");
      Serial.print(moonlite_speed);*/

      float newspeed = DEFAULT_STEPPER_SPEED*speedFactor();
      int newspeed2 = (int)newspeed;

      if (newspeed2 > LIMIT_STEPPER_SPEED) newspeed2 = LIMIT_STEPPER_SPEED;

      stepper.setSpeed(newspeed2);
      stepper.setMaxSpeed(newspeed2);

      /*Serial.print("_NEWSPEED:");
      Serial.print(newspeed2);*/
    
    }
      //set half step mode
      if (!strcasecmp(cmd, "SH")) {
        halfstep = true;
    }

    //set full step mode
      if (!strcasecmp(cmd, "SF")) {
        halfstep = false;
    }
    
    // return half step mode
    if (!strcasecmp(cmd, "GH")) {
      if (halfstep) {
      Serial.print("FF#"); 
      }
      else {
        Serial.print("00#");
      }
    }

    // motor is moving - 01 if moving, 00 otherwise
    if (!strcasecmp(cmd, "GI")) {
      if (abs(stepper.distanceToGo()) > 0) {
        Serial.print("01#");
      } 
      else {
        Serial.print("00#");
      }
    }

    // set current motor position
    if (!strcasecmp(cmd, "SP")) {
      pos = hexstr2long(param);
      pos = clampPosition(pos);
      if(pos==0) pos=HOME_POSITION;

      stepper.setCurrentPosition(pos);
    }

    // set new motor position
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
      pos = clampPosition(pos);
      if(pos==0) pos=HOME_POSITION;
      stepper.moveTo(pos);
    }

    // initiate a move
    if (!strcasecmp(cmd, "FG")) {
      isRunning = 1;
      stepper.enableOutputs();
    }

    // stop a move
    if (!strcasecmp(cmd, "FQ")) {
     // isRunning = 0;
     // stepper.moveTo(stepper.currentPosition());

      stepper.stop();

      isRunning = 0;
    }
  }


  if (stepper.isRunning()) {
    needsSave = true;
  }

   if (!stepper.isRunning() && needsSave) {
    savePositionSafely(stepper.currentPosition());
}


} // end loop

long hexstr2long(char *line) {
  /*
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
  */
  return (long)(strtoul(line, NULL, 16));
}

void initializeEEPROM() {
  // save the version
  EEPROM.updateInt(EEPROM_VERSION_ADDR, EEPROM_VERSION);
  // save the home position
  EEPROM.updateLong(POS_ADDR, HOME_POSITION);
}

long readPositionFromEEPROM() {
  long value = HOME_POSITION;
  value = EEPROM.readLong(POS_ADDR);
  return value;
}

void savePositionSafely(long position) {
  
  position = clampPosition(position);

  // save position to EEPROM if needed every 2 minutes and if position changed more than 10 steps
  if (needsSave && (millis() - lastSaveTime > WRITE_DELAY) && 
      abs(position - savedPosition) > 10) {
        
    digitalWrite(PIN_LED, HIGH); // turn the LED on to indicate EEPROM write
    delay(25);
    EEPROM.updateLong(POS_ADDR, position);
    delay(25); 
    digitalWrite(PIN_LED, LOW); // turn the LED off after write

    savedPosition = position;
    needsSave = false;
    lastSaveTime = millis();
  }
}

long clampPosition(long value) {
  if (value < 0) value = 0;
  if (value > 0xFFFF) value = 0xFFFF;
  return value;
}