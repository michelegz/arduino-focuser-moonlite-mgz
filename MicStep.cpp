#include "MicStep.h"

//CONSTRUCTOR
    Stepper::Stepper(int ain1, int ain2, int bin1, int bin2, int pwma, int pwmb, float pwm_factor) {
      
      this->ain1 = ain1;
      this->ain2 = ain2;
      this->bin1 = bin1;
      this->bin2 = bin2;
      this->pwma = pwma;
      this->pwmb = pwmb;
      this->pwm = true;

      this->current_step = 0;

      pinMode(this->ain1, OUTPUT);
      pinMode(this->ain2, OUTPUT);
      pinMode(this->bin1, OUTPUT);
      pinMode(this->bin2, OUTPUT);
      pinMode(this->pwma, OUTPUT);
      pinMode(this->pwmb, OUTPUT);

      this->pwm_factor=constrain(pwm_factor,0.0,1.0);
      this->pwm_max = this->pwm_factor*255;
      
    }

    Stepper::Stepper(int ain1, int ain2, int bin1, int bin2) {
      
      this->ain1 = ain1;
      this->ain2 = ain2;
      this->bin1 = bin1;
      this->bin2 = bin2;
      //this->pwma = pwma;
      //this->pwmb = pwmb;
      this->pwm = false;

      this->current_step = 0;

      pinMode(this->ain1, OUTPUT);
      pinMode(this->ain2, OUTPUT);
      pinMode(this->bin1, OUTPUT);
      pinMode(this->bin2, OUTPUT);
      //pinMode(this->pwma, OUTPUT);
      //pinMode(this->pwmb, OUTPUT);

      //this->pwm_factor=constrain(pwm_factor,0.0,1.0);
      //this->pwm_max = this->pwm_factor*255;
      
    }

    void Stepper::step(int direction, int mode, long delay_us) {

        int max_step;
        
        if(mode == INTERLEAVE) max_step = 8;
          else max_step = 4;

        if(this->current_step >= max_step ) this->current_step = 0;
          else  if(this->current_step <= -1 ) this->current_step = max_step - 1;
                 
        switch(mode) {
  
            case SINGLE:
              
              if(this->pwm) {
                analogWrite(pwma,pwm_max);
                analogWrite(pwmb,pwm_max);
              }
              
              digitalWrite(ain1,single_steps[current_step][0]);
              digitalWrite(bin1,single_steps[current_step][1]);
              digitalWrite(ain2,single_steps[current_step][2]);
              digitalWrite(bin2,single_steps[current_step][3]);
              
              delayMicroseconds(delay_us);
             
            break;
            
            case DOUBLE:
            
              if(this->pwm) {
                analogWrite(pwma,pwm_max);
                analogWrite(pwmb,pwm_max);
              }
              
              digitalWrite(ain1,double_steps[current_step][0]);
              digitalWrite(bin1,double_steps[current_step][1]);
              digitalWrite(ain2,double_steps[current_step][2]);
              digitalWrite(bin2,double_steps[current_step][3]);
              
              delayMicroseconds(delay_us);

            break;
  
            case INTERLEAVE:

              if(this->pwm) {
                analogWrite(pwma,pwm_max);
                analogWrite(pwmb,pwm_max);
              }
              
              digitalWrite(ain1,interleave_steps[current_step][0]);
              digitalWrite(bin1,interleave_steps[current_step][1]);
              digitalWrite(ain2,interleave_steps[current_step][2]);
              digitalWrite(bin2,interleave_steps[current_step][3]);
              
              delayMicroseconds(delay_us);

            break;

            case MICROSTEP:
            
              if (this->pwm) {
                int j, k;
                
                digitalWrite(ain1,microsteps_steps[current_step][0]);
                digitalWrite(bin1,microsteps_steps[current_step][1]);
                digitalWrite(ain2,microsteps_steps[current_step][2]);
                digitalWrite(bin2,microsteps_steps[current_step][3]);

                if(direction == 0) {
                  
                    j = current_step*8;
                    k = j+8;
                    
                    for(int i=j;i<k;i++) {
      
                      analogWrite(pwma,microstep_curve_a[i]*pwm_factor);
                      analogWrite(pwmb,microstep_curve_b[i]*pwm_factor);
                      delayMicroseconds(delay_us/8);
                      //Serial.println(delay_us/8);
                      }
                    }

              else {
                    
                    j = current_step*8+7;
                    k = j-8;
                    
                    for(int i=j;i>k;i--) {

                      analogWrite(pwma,microstep_curve_a[i]*pwm_factor);
                      analogWrite(pwmb,microstep_curve_b[i]*pwm_factor);
                      delayMicroseconds(delay_us/8);
                      //Serial.println(delay_us/8);
                      }
                    }
          }
            
            break;

            default:
            break;
            
          }

          if(direction == 0) this->current_step = this->current_step + 1;
            else  this->current_step = this->current_step - 1;

       }

       void Stepper::step(int direction, int mode) {
        this->step(direction, mode, 0);
       }


       // release coils
       void Stepper::release() {

              digitalWrite(ain1,LOW);
              digitalWrite(ain2,LOW);
              digitalWrite(bin1,LOW);
              digitalWrite(bin2,LOW);
        }

