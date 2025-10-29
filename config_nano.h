//choose main configuration between Adafruit compatible v1 MOTORSHIELD or generic HBRIDGE
//#define MOTORSHIELD
//#define MOTORSHIELD_STEPS 200
//#define MOTORSHIELD_NUMBER 2

#define HBRIDGE
#define HBRIDGE_MOTORPIN1 5
#define HBRIDGE_MOTORPIN2 6
#define HBRIDGE_MOTORPIN3 7
#define HBRIDGE_MOTORPIN4 8

//optional PWM for HBRIDGE
//#define HBRIDGE_PWM_ENABLED
//#define HBRIDGE_PWMPIN_A 9
//#define HBRIDGE_PWMPIN_B 10
//#define HBRIDGE_POWERFACTOR 1

//use DHT11 temp sensor
 #define DHT11_PRESENT
#define DHT11_PIN 2
#define TEMP_OFFSET 0

//MAXSPEED limits the maximumum speed
#define LIMIT_STEPPER_SPEED 1000 //just for safety, when speed is configured via UI, it will always limited by this value
#define DEFAULT_STEPPER_SPEED 200
//selectable speeds will be STEPPER_SPEED*speedFactor()
#define ACCELERATION 150
#define MOTOR_RELEASE_TIMEOUT 1000
#define HOME_POSITION 32000
#define HALFSTEP false