// Moonlite-compatible stepper controller
//
// Author: michele.gz@gmail.com
// Based on original work by orly.andico@gmail.com
//
// Main enhanchments from original version:
// - possibility to choose between Motorshield v1 and generic H-Bridge motor
// - half step implementation
// - DHT11 temperature support
// - speed selection from Moonlite GUI
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
// Uses AFMotor and the Adafruit v1.2 Motor Shield https://learn.adafruit.com/adafruit-motor-shield
// Uses my custom MicStep library for generic H-Bridge stepper driver
//
// Original note by Orly (untested):
// Requires a 10uf - 100uf capacitor between RESET and GND on the motor shield; this prevents the
// Arduino from resetting on connect (via DTR going low).  Without the capacitor, this sketch works
// with the stand-alone Moonlite control program (non-ASCOM) but the ASCOM driver does not detect it.
// Adding the capacitor allows the Arduino to respond quickly enough to the ASCOM driver probe
//


//-------------------------------------------------------
// BEGIN CONFIGURATION
//-------------------------------------------------------

#include "config.h"

//------------------------------------------------------------------
// END CONFIGURATION
// Don't edit below this line unless you know what you are doing
//-------------------------------------------------------------------

#define MAXCOMMAND 8

#ifdef MOTORSHIELD
  #include <AFMotor.h>
  AF_Stepper motor(MOTORSHIELD_STEPS, MOTORSHIELD_NUMBER);
#endif

#ifdef HBRIDGE
  #include "MicStep.h"
  #ifdef HBRDIGE_PWM_ENABLED
    Stepper motor(HBRDIGE_MOTORPIN1,HBRDIGE_MOTORPIN2,HBRDIGE_MOTORPIN3,HBRDIGE_MOTORPIN4,HBRDIGE_PWMPIN_A,HBRDIGE_PWMPIN_B,HBRDIGE_POWERFACTOR);
  #else
    Stepper motor(HBRDIGE_MOTORPIN1,HBRDIGE_MOTORPIN2,HBRDIGE_MOTORPIN3,HBRDIGE_MOTORPIN4);
  #endif
#endif

#include <AccelStepper.h>

#ifdef DHT11_PRESENT
  #include <DHT11.h>
  DHT11 dht11(DHT11_PIN);
#endif

char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
long pos = HOME_POSITION;
int isRunning = 0;
int speed = DEFAULT_SPEED;
int eoc = 0;
int idx = 0;
long millisLastMove = 0;
bool halfstep = false;

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


void setup()
{  
  Serial.begin(9600);

  stepper.setSpeed(speed*SPEEDMULT);
  stepper.setMaxSpeed(MAXSPEED);
  stepper.setAcceleration(ACCELERATION);
  stepper.enableOutputs();
  stepper.setCurrentPosition(HOME_POSITION);

  memset(line, 0, MAXCOMMAND);
  
  #ifdef DHT11_PRESENT
    dht11.readTemperature();
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
        motor.release();
      }
    }

    if (stepper.distanceToGo() == 0) {
      stepper.run();
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

    // get the current temperature via DHT11
    if (!strcasecmp(cmd, "GT")) {

      #ifdef DHT11_PRESENT

       char tempString[6];
       int temperature = dht11.readTemperature();

       sprintf(tempString, "%04X", temperature);

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
      sprintf(tempString, "%02X", speed);
      Serial.print(tempString);
      Serial.print("#");
    }

    // set speed, only acceptable values are 02, 04, 08, 10, 20
    if (!strcasecmp(cmd, "SD")) {
      speed = hexstr2long(param);

      stepper.setSpeed(speed * SPEEDMULT);
    
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

      if(pos==0) pos=HOME_POSITION;

      stepper.setCurrentPosition(pos);
    }

    // set new motor position
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
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
      isRunning = 0;
      stepper.moveTo(stepper.currentPosition());
      stepper.run();
    }
  }
} // end loop

long hexstr2long(char *line) {
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
}



