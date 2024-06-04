// Moonlite-compatible stepper controller
//
// Uses AccelStepper (http://www.airspayce.com/mikem/arduino/AccelStepper/)
// Uses AFMotor and the Adafruit v1.2 Motor Shield https://learn.adafruit.com/adafruit-motor-shield
// Uses MicStep for generic H-Bridge stepper driver
//
// Requires a 10uf - 100uf capacitor between RESET and GND on the motor shield; this prevents the
// Arduino from resetting on connect (via DTR going low).  Without the capacitor, this sketch works
// with the stand-alone Moonlite control program (non-ASCOM) but the ASCOM driver does not detect it.
// Adding the capacitor allows the Arduino to respond quickly enough to the ASCOM driver probe
//
// Author: michele.gz@gmail.com
// Based on original work by orly.andico@gmail.com

//-------------------------------------------------------
// BEGIN CONFIGURATION
//-------------------------------------------------------

//choose main configuration between MOTORSHIELD or HBRIDGE
#define MOTORSHIELD
#define MOTORSHIELD_STEPS 200
#define MOTORSHIELD_NUMBER 2

//#define HBRIDGE
//#define HBRDIGE_MOTORPIN1 5
//#define HBRDIGE_MOTORPIN2 6
//#define HBRDIGE_MOTORPIN3 7
//#define HBRDIGE_MOTORPIN4 8
//#define HBRDIGE_PWMPIN_A 9
//#define HBRDIGE_PWMPIN_B 10
//#define HBRDIGE_POWERFACTOR 1

//use DHT11 temp sensor
#define DHT11
#define DHT11_PIN 23

//MAXSPEED must be 20*SPEEDMULT
#define MAXSPEED 100
//speed will be SPEEDMULT* 02, 04, 08, 10, 20
#define SPEEDMULT 5
#define ACCELERATION 10

//-------------------------------------------------------
// END CONFIGURATION
//-------------------------------------------------------

#define MAXCOMMAND 8

#ifdef MOTORSHIELD
#include <AFMotor.h>
AF_Stepper motor(MOTORSHIELD_STEPS, MOTORSHIELD_NUMBER);
#endif

#ifdef HBRIDGE
#include <MicStep.h>
Stepper motor(HBRDIGE_MOTORPIN1,HBRDIGE_MOTORPIN2,HBRDIGE_MOTORPIN3,HBRDIGE_MOTORPIN4,HBRDIGE_PWMPIN_A,HBRDIGE_PWMPIN_B,HBRDIGE_POWERFACTOR);
#endif

#include <AccelStepper.h>

#ifdef DHT11
#include <DHT11.h>
DHT11 dht11(DHT11_PIN);
#endif

void forwardstep() {  
  #ifdef MOTORSHIELD
  if (!halfstep) 
  motor.onestep(FORWARD, DOUBLE);
  else motor.onestep(FORWARD, INTERLEAVE);
  #endif
  
  #ifdef HBRIDGE
  if (!halfstep) 
  motor.step(FORWARD, DOUBLE, 0);
  else motor.step(FORWARD, INTERLEAVE, 0);
  #endif
}

void backwardstep() {  
  #ifdef MOTORSHIELD
  motor.onestep(BACKWARD, DOUBLE);
  #endif
  
  #ifdef HBRIDGE
  motor.step(BACKWARD, DOUBLE, 0);
  #endif
}

AccelStepper stepper(forwardstep, backwardstep);

char inChar;
char cmd[MAXCOMMAND];
char param[MAXCOMMAND];
char line[MAXCOMMAND];
long pos;
int isRunning = 0;
int speed = 32;
int eoc = 0;
int idx = 0;
long millisLastMove = 0;
bool halfstep = false;

void setup()
{  
  Serial.begin(9600);

  // we ignore the Moonlite speed setting because Accelstepper implements
  // ramping, making variable speeds un-necessary
  stepper.setSpeed(MAXSPEED);
  stepper.setMaxSpeed(MAXSPEED);
  stepper.setAcceleration(ACCELERATION);
  stepper.enableOutputs();
  memset(line, 0, MAXCOMMAND);
  dht11.readTemperature();
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
      if ((millis() - millisLastMove) > 15000) {
        stepper.disableOutputs();
        motor1.release();
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
      stepper.setCurrentPosition(8000);
      stepper.moveTo(0);
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

      #ifdef DHT11
       int temperature = dht11.readTemperature();

        if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
            Serial.print(String(temperature) + "#");
        } else {
            // error
            Serial.println("66#");
        }
      #else
            Serial.println("0#");
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
      stepper.setMaxSpeed(speed * SPEEDMULT);
    
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
      stepper.setCurrentPosition(pos);
    }

    // set new motor position
    if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
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



