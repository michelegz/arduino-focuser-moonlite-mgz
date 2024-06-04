//choose main configuration between Adafruit compatible v1 MOTORSHIELD or generic HBRIDGE
#define MOTORSHIELD
#define MOTORSHIELD_STEPS 200
#define MOTORSHIELD_NUMBER 2

//#define HBRIDGE
//#define HBRDIGE_MOTORPIN1 5
//#define HBRDIGE_MOTORPIN2 6
//#define HBRDIGE_MOTORPIN3 7
//#define HBRDIGE_MOTORPIN4 8

//optional PWM for HBRIDGE
//#define HBRDIGE_PWM_ENABLED
//#define HBRDIGE_PWMPIN_A 9
//#define HBRDIGE_PWMPIN_B 10
//#define HBRDIGE_POWERFACTOR 1

//use DHT11 temp sensor
//#define DHT11_PRESENT
#define DHT11_PIN 2

//MAXSPEED limits the maximumum speed, I suggest you to set this to 20*SPEEDMULT once you found the right SPEEDMULT by trial
#define MAXSPEED 100
//selectable speeds will be SPEEDMULT* 02, 04, 08, 10, 20
#define SPEEDMULT 5
#define DEFAULT_SPEED 10
#define ACCELERATION 20
#define MOTOR_RELEASE_TIMEOUT 30000
#define HOME_POSITION 32000