// UNIVERSAL H-BRIDGE STEPPER LIBRARY
// AUTHOR: MICHELE GUZZINI - michele.gz@gmail.com
// v1.0

// USAGE:
//
//	CONSTRUCTOR:
//  	Stepper motor(COIL_A_PIN1,COIL_A_PIN2,COIL_B_PIN1,COIL_B_PIN2,PWM_A_PIN,PWM_B_PIN,POWER_FACTOR); //with PWM control, MICROSTEP capable
//    Stepper motor(COIL_A_PIN1,COIL_A_PIN2,COIL_B_PIN1,COIL_B_PIN2); //without PWM, no MICROSTEP capable
//	
//		POWER_FACTOR can be used to limit motor current via pwm, possibile values: 0.0->1.0
//
//  METHODS: step(int direction,int mode,long delay_us), step(int direction,int mode), release()
//
//  	motor.step(FORWARD,SINGLE,delay_us);
//		motor.step(BACKWARD,DOUBLE,delay_us);
//  	motor.step(FORWARD,INTERLEAVE,delay_us);
//  	motor.step(BACKWARD,MICROSTEP,delay_us); //only if PWM pins are specified in constructor
//	
//		motor.release()
//
//	delay_us is the time delay in microseconds at the end of each step, if you need a costant movement just call the step() method in a loop
//
//		EXAMPLE:
//		**************************************
//		Stepper motor(3,2,5,6,10,9,1.0);
//  	int speed = 200; // speed in steps/min
//		long m = 60000000;
//  	long delay_us = m/speed;
//  	while (true) {
//		motor.step(FORWARD,SINGLE,delay_us);
//		}
//		***************************************
//
//	TIP FOR MICROSTEPPING: You may need to change the PWM timer frequency by editing the ATmega registries to ensure smooth rotation and avoid resonance:
//						   https://arduino-info.wikispaces.com/Arduino-PWM-Frequency
//
//


#include <Arduino.h>

#define SINGLE 1
#define DOUBLE 2
#define INTERLEAVE 3
#define MICROSTEP 4

#define FORWARD 0
#define BACKWARD 1

class Stepper {

  public:
    
    //constructor method
    Stepper(int ain1, int ain2, int bin1, int bin2, int pwma, int pwmb,float pwm_factor);
    Stepper(int ain1, int ain2, int bin1, int bin2);

    //move method
    void step(int direction,int mode, long delay_us);
    void step(int direction,int mode);

    //release coils method
    void release();

  private: 
    int ain1;
    int ain2;
    int bin1;
    int bin2;
    int pwma;
    int pwmb;
    bool pwm;

    float pwm_factor; // from 0.0 to 1.0, use it to reduce motor power and vibration
    int pwm_max;
    
    int current_step;

                 //  COILS SEQUENCE |A| |B| |-A| |-B|
    const int single_steps[4][4] = {  {1,0,0,0},
                                      {0,1,0,0},
                                      {0,0,1,0},
                                      {0,0,0,1} };
    
    const int double_steps[4][4] = {  {1,1,0,0},
                                      {0,1,1,0},
                                      {0,0,1,1},
                                      {1,0,0,1}  };

    const int interleave_steps[8][4] = {  {1,0,0,0},
                                          {1,1,0,0},
                                          {0,1,0,0},
                                          {0,1,1,0},
                                          {0,0,1,0},
                                          {0,0,1,1},
                                          {0,0,0,1},
                                          {1,0,0,1}   };

    const int microsteps_steps[4][4] = {  {1,1,0,0},
                                          {0,1,1,0},
                                          {0,0,1,1},
                                          {1,0,0,1} };

    // 8 MICROSTEPS TRANSITION FOR EACH FULL STEP
    const int microstep_curve_a[32] = {255,249,230,199,159,111,57,0,  0,57,111,159,199,230,249,255,   255,249,230,199,159,111,57,0,   0,57,111,159,199,230,249,255 };
    const int microstep_curve_b[32] = {0,57,111,159,199,230,249,255,  255,249,230,199,159,111,57,0,  0,57,111,159,199,230,249,255,   255,249,230,199,159,111,57,0  };

};
