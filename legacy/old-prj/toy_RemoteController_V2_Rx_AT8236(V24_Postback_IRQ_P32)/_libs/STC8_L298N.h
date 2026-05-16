#ifndef __STC8_L298N_H
#define __STC8_L298N_H



// L298N引脚
#define _L298N_ENA P10
#define _L298N_IN1 P37
#define _L298N_IN2 P36


#define _L298N_ENB P54
#define _L298N_IN3 P30
#define _L298N_IN4 P31



#define PWM_MAX_VALUE 10000


void L298N_Init(PWMx_Duty *PWMx);


void A_Forward(unsigned int pwmValue);
void A_Backward(unsigned int pwmValue);
void A_Stop();

void B_Forward(unsigned int pwmValue);
void B_Backward(unsigned int pwmValue);
void B_Stop();


/** 
** 使用这个库，需要开启PWM功能，PWM的周期是10000
** A电机用的是PWMA的ENO1P, P10口
** B电机用的是PWMA的ENO2P, P12口
**/


#endif
