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
#define DHT11_PRESENT
#define DHT11_PIN 22
#define TEMP_OFFSET 0

//MAXSPEED limits the maximumum speed
#define LIMIT_STEPPER_SPEED 1000 //just for safety, when speed is configured via UI, it will always limited by this value
#define DEFAULT_STEPPER_SPEED 200
//selectable speeds will be STEPPER_SPEED*speedFactor()
#define ACCELERATION 150
#define MOTOR_RELEASE_TIMEOUT 1000
#define HOME_POSITION 32000
#define HALFSTEP false