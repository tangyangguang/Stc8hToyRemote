#include "STC8H.h"
#include "STC8H_PWM.h"
#include "STC8_L298N.h"

PWMx_Duty  L298N_PWMA_Duty;

void L298N_UpdatePWM(unsigned int pwmValue){
	L298N_PWMA_Duty.PWM1_Duty = pwmValue; 	
	UpdatePwm(PWMA, &L298N_PWMA_Duty); 			
}




void L298N_Init(PWMx_Duty *_PWMx){
	L298N_PWMA_Duty = *_PWMx;
}


void A_Forward(unsigned int pwmValue)  
{
	_L298N_IN1 = 1;
	_L298N_IN2 = 0;
	L298N_UpdatePWM(pwmValue); // 更新PWM脉宽值 				
}

void A_Backward(unsigned int pwmValue)  
{
	_L298N_IN1 = 0;
	_L298N_IN2 = 1;
	L298N_UpdatePWM(pwmValue); // 更新PWM脉宽值 					
}


void A_Stop()
{
	 _L298N_IN1 = 0;
	 _L298N_IN2 = 0;
	 L298N_UpdatePWM(PWM_MAX_VALUE); // 更新PWM脉宽值 		 			
}


void B_Forward(unsigned int pwmValue)  
{
	_L298N_IN3 = 1;
	_L298N_IN4 = 0;
  L298N_UpdatePWM(pwmValue); // 更新PWM脉宽值 				
}


void B_Backward(unsigned int pwmValue)  
{
	_L298N_IN3 = 0;
	_L298N_IN4 = 1;
	L298N_UpdatePWM(pwmValue); // 更新PWM脉宽值 			
}


void B_Stop()
{
	_L298N_IN3 = 0;
	_L298N_IN4 = 0;
	L298N_UpdatePWM(PWM_MAX_VALUE); // 更新PWM脉宽值
}


