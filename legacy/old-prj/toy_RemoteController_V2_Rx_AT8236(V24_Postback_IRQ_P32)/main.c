
#include "my_stc8h_lib.h"
#include "STC8H.h"
#include "STC8G_H_ADC.h"
#include <intrins.h>
#include <stdlib.h>
#include <stdio.h> 
#include "my_nRF24L01.h"


/************************************************************/
/*                                                          */
/*                    常量定义                              */
/*                                                          */
/************************************************************/

// 喇叭开关定义
#define BUZZER_ON 	0
#define BUZZER_OFF	1

// 灯光开关定义
#define LIGHT_ON	0
#define LIGHT_OFF	1

// 按钮开关定义
#define SW_ON		0 
#define SW_OFF		1

// LED指示灯（IO口LED的负极，写0发光）
#define LED_ON		0
#define LED_OFF		1


// TODO 把TX和RX改为设置按键：舵机反向、马达反向，这样就空出P37口
/************************************************************/
/*                                                          */
/*                    IO口定义                              */
/*                                                          */
/************************************************************/

// 接S8550三极管的基极，PNP型，贴片2TY
#define LIGHT_IO			P35 // 灯光
#define BUZZER_IO			P36 // 蜂鸣器
	
// 输出PWM的引脚
#define IN1_IO				P33 // 需要PWM支持, P33 对应 PWM7_2  控制AT8236芯片的 IN1, 10KHz 
#define IN2_IO				P34 // 需要PWM支持, P34 对应 PWM8_2, 控制AT8236芯片的 IN2, 10KHz
#define MOSFET_SW_PWM_IO	P54	// 需要PWM支持, P54 对应 PWM6_2, 控制MOS管电子开关模块的PWM控制信号口, 10KHz
#define STEERING_SERVO_IO	P10	// 需要PWM支持, P10 对应 PWM1P,  控制方向舵机的PWM控制信号口, 50Hz 

// 按钮, 需要接上拉电阻10K
// #define BTN_SETTING_IO 		P37 // 自复位按钮，用于切换舵机是否反向
// 舵机设置口改为了LED指示灯的IO口，处理代码也注释掉了


#define LED 		P37 	// 指示灯（IO口接LED的负极，写0发光）



// ADC, 检测电池电压（2S或3S电池）
#define VoltCheck_IO_ADC_CHANNEL	1 		// ADC通道1 是 P11


#define BTN_RF_CH_ADD		P30	// 接收频道调整为当前+1的频道
#define BTN_RF_CH_MINUS		P31	// 接收频道调整为当前-1的频道

u8 btn_rf_ch_add_flag, btn_rf_ch_minus_flag; // 加减频道按钮的执行标志




/************************************************************/
/*                                                          */
/*                    变量定义                              */
/*                                                          */
/************************************************************/

PWMx_Duty PWMAB_Duty;

u16 rx_timeout_msCounter = 0; // 遥控信号接收的超时计时
u16 msCounter_Action_Interval = 0; // 执行动作的时间间隔计时
u16 voltCheck_msCounter = 0; // 检测电池电压的时间计数
u16 action_msCounter = 0; // 执行各种输出动作的毫秒计数

u16 rx_conn_timeout_msCounter = 0; //  建立连接过程中的等待超时时间


u16 led_blink_period_ms	= 0;
u16 led_blink_msCounter = 0;


/************************************************************/
/*                                                          */
/*                 初始化 IO口                              */
/*                                                          */
/************************************************************/

void IO_Init(){
	P1M1 = 0X00; P1M0 = 0X00; // 准双向口
	P3M1 = 0X00; P3M0 = 0X00; // 准双向口
	P5M1 = 0X00; P5M0 = 0X00; // 准双向口

	// ！！P33和P34接AT8236的IN1和IN2，高电平时测试时0.6V，高电平不对，去掉和IN1或IN2口的连接，就是3.3V的高电平。
	// 经测试，把IO口设为推挽输出（P33, P34 推挽输出），测试的高电平就是3.3V，就好了。
	P3M0 = 0x18; P3M1 = 0x00; 

	// P54设为推挽输出, 注释掉也可以正常运行
    // P5M0 = 0x10; P5M1 = 0x00; 

	// P10设为推挽输出, 注释掉也可以正常运行
	// P1M0 = 0x01; P1M1 = 0x00; 

	// P11设为高阻输入（ADC检测电压用）
	P1M0 = 0x00; P1M1 = 0x02; 
}


	
/************************************************************/
/*                                                          */
/*                 初始化 硬件SPI                           */
/*                                                          */
/************************************************************/

void SPI_Init(){
	
	// 控制nRF24L01芯片：固定主机模式, 先传输MSB后传输LSB，SCLK空闲时低电平，前时钟沿采样
	SPSTAT = 0xC0;		// 清中断标志 (当发送/接收一个字节后，触发SPI的中断，需要软件方式写1进行清零)
	SPCTL = 0XD0; 		// 设置为固定主模式
    IE2 |= 0x02;		// 使能SPI中断. IE2寄存器B1位：ESPI，SPI中断允许位
	
	/*
	// 控制nRF24L01芯片, 采用固定主机模式, 先传输MSB后传输LSB，SCLK空闲时低电平，前时钟沿采样
    SPI_Start(ENABLE);					//SPI启动    ENABLE, DISABLE
	SPI_SSIG_Set(ENABLE);				//片选位     ENABLE(conform Master or Slave by SPI_Mode(ignore SS)), DISABLE(conform Master or Slave by SS pin.) 
	SPI_Mode_Set(SPI_Mode_Master);		//主从选择   SPI_Mode_Master, SPI_Mode_Slave
	SPI_FirstBit_Set(SPI_MSB);			//移位方向   SPI_MSB, SPI_LSB 					// B5位，DORD，0 = 先发送/接收数据的高位（MSB），1 = 先发送/接收数据的地位（LSB）
	SPI_CPOL_Set(SPI_CPOL_Low);			//时钟相位   SPI_CPOL_High,   SPI_CPOL_Low		// B3位，CPOL 极性，0表示SCLK空闲时是低电平，前时钟沿是上升沿，后时钟沿是下降沿; 1表示SCLK空闲时是高电平，前时钟沿是下降沿，后时钟沿是上升沿
	SPI_CPHA_Set(SPI_CPHA_1Edge);		//数据边沿   SPI_CPHA_1Edge,  SPI_CPHA_2Edge		// B2位，CPHA 相位，0表示前时钟沿采样，1表示后时钟沿采样
	SPI_Clock_Select(SPI_Speed_16);		//SPI速度    SPI_Speed_4,SPI_Speed_16,SPI_Speed_64,SPI_Speed_2/SPI_Speed_32
	SPI_SW(SPI_P12_P13_P14_P15);
	*/
}



/************************************************************/
/*                                                          */
/*                 初始化 UART1                             */
/*                                                          */
/************************************************************/

// 初始化串口1，115200bps, 使用定时器1，使用 RxD(P3.0), TxD(P3.1)
void	UART1_Init(void) // 115200bps@12.000MHz
{
	SCON = 0x50;		//8位数据,可变波特率; 允许串口接收数据
	AUXR |= 0x40;		//定时器时钟1T模式
	AUXR &= 0xFE;		//串口1选择定时器1为波特率发生器
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0xE6;			//设置定时初始值
	TH1 = 0xFF;			//设置定时初始值
	ET1 = 0;			//禁止定时器中断
	TR1 = 1;			//定时器1开始计时
	ES = 1;				//使能串口1中断
		
 	P_SW1 &= ~0xc0;		// 功能脚切换   UART1/USART1: RxD(P3.0), TxD(P3.1)
}



/************************************************************/
/*                                                          */
/*                 初始化 ADC                               */
/*                                                          */
/************************************************************/

void	ADC_config(void)
{
	ADC_InitTypeDef		ADC_InitStructure;		//结构定义

	ADC_InitStructure.ADC_SMPduty   = 31;		//ADC 模拟信号采样时间控制, 0~31（注意： SMPDUTY 一定不能设置小于 10）
	ADC_InitStructure.ADC_CsSetup   = 0;		//ADC 通道选择时间控制 0(默认),1
	ADC_InitStructure.ADC_CsHold    = 1;		//ADC 通道选择保持时间控制 0,1(默认),2,3
	ADC_InitStructure.ADC_Speed     = ADC_SPEED_2X16T;		//设置 ADC 工作时钟频率	ADC_SPEED_2X1T~ADC_SPEED_2X16T
	ADC_InitStructure.ADC_AdjResult = ADC_RIGHT_JUSTIFIED;	//ADC结果调整,	ADC_LEFT_JUSTIFIED,ADC_RIGHT_JUSTIFIED
	ADC_Inilize(&ADC_InitStructure);		//初始化
	ADC_PowerControl(ENABLE);				//ADC电源开关, ENABLE或DISABLE
	// NVIC_ADC_Init(DISABLE,Priority_0);		//中断使能, ENABLE/DISABLE; 优先级(低到高) Priority_0,Priority_1,Priority_2,Priority_3
	EADC = 0;			// 不要ADC中断使能，使用的时查询法
}



/************************************************************/
/*                                                          */
/*                 初始化 PWM                               */
/*                                                          */
/************************************************************/

void	PWM_config(void)
{
	
	PWMA_CC1E_Disable();		//关闭输入捕获/比较输出
	PWMA_CC1NE_Disable();		//关闭比较输出
	PWMA_OC1ModeSet(CCMRn_PWM_MODE1);		//设置输出比较模式
	PWMA_CC1E_Enable();			//开启输入捕获/比较输出
	PWMA_ENO |= ENO1P;
	//	PWMA_Duty1(PWMx->PWM_Duty);
	
	
	PWMB_CC6E_Disable();		//关闭输入捕获/比较输出
	PWMB_OC6ModeSet(CCMRn_PWM_MODE1);		//设置输出比较模式
	PWMB_CC6E_Enable();			//开启输入捕获/比较输出
	PWMB_ENO |= ENO6P;
	//	PWMB_Duty6(PWMx->PWM_Duty);
	
	
	PWMB_CC7E_Disable();		//关闭输入捕获/比较输出
	PWMB_OC7ModeSet(CCMRn_PWM_MODE1);		//设置输出比较模式
	PWMB_CC7E_Enable();			//开启输入捕获/比较输出
	PWMB_ENO |= ENO7P;
	//	PWMB_Duty7(PWMx->PWM_Duty);
		
	
	PWMB_CC8E_Disable();		//关闭输入捕获/比较输出
	PWMB_OC8ModeSet(CCMRn_PWM_MODE1);		//设置输出比较模式
	PWMB_CC8E_Enable();			//开启输入捕获/比较输出
	PWMB_ENO |= ENO8P;
	//	PWMB_Duty8(PWMx->PWM_Duty);
	

	// ******** PWMA 设置, 生成50Hz的PWM，用于控制舵机
	// PWM频率 = SYSCLK / ( (PSCR+1) * (ARR+1) ) 
	// 设置时钟分频, 分频值是1~65535之间的值
	// 当前是12MHz情况下: PWMA_PSCR=11，PWM_Period=19999, 就是50Hz
	PWMA_PSCRH=0x00; 
	PWMA_PSCRL=0x0B; // 11


	PWMA_DeadTime(0);	//死区发生器设置, 0~255
	PWMA_AutoReload(16666);	//周期设置，0~65535
	PWMA_BrakeOutputEnable(ENABLE);	//主输出使能
	PWMA_CEN_Enable(ENABLE);	//使能计数器



	// ******** PWMB 设置，生成10KHz的PWM，用于控制AT8236的IN1和IN2，以及MOS电子开关的控制.
	// PWM频率 = SYSCLK / ( (PSCR+1) * (ARR+1) ) 
	// 设置时钟分频, 分频值是1~65535之间的值
	// 当前是12MHz情况下: PWMA_PSCR=11，PWM_Period=19999, 就是50Hz
	PWMB_PSCRH=0x00; 
	PWMB_PSCRL=0x0B; // 11

	PWMB_DeadTime(0);	//死区发生器设置
	PWMB_AutoReload(100);	//周期设置
	PWMB_BrakeOutputEnable(ENABLE);	//主输出使能
	PWMB_CEN_Enable(ENABLE);	//使能计数器
	
	
	
	PWM1_SW(PWM1_SW_P10_P11);		// P10口输出PWM， PWM1_SW_P10_P11,PWM1_SW_P20_P21,PWM1_SW_P60_P61
	PWM6_SW(PWM6_SW_P54);			// P54口输出PWM
	PWM7_SW(PWM7_SW_P33);			// P33口输出PWM
	PWM8_SW(PWM8_SW_P34);			// P34口输出PWM
}




/************************************************************/
/*                                                          */
/*                 定时器0设置                              */
/*                                                          */
/************************************************************/

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR |= 0x80;			//定时器时钟1T模式
	TMOD &= 0xF0;			//设置定时器模式
	TL0 = 0x20;				//设置定时初始值
	TH0 = 0xD1;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;				//使能定时器0中断
}


// 定时器0的中断处理函数
void Timer0_ISR_Handler (void) interrupt TMR0_VECTOR		//进中断时已经清除标志
{
	rx_timeout_msCounter++;
	msCounter_Action_Interval++;
	voltCheck_msCounter++;
	action_msCounter++;
	rx_conn_timeout_msCounter++;
	led_blink_msCounter++;
}



/************************************************************/
/*                                                          */
/*         AT8236的IN1和IN2口的PWM信号控制                   */
/*                                                          */
/************************************************************/

// 设置AT8236芯片的IN1和IN2口的信号数据
void Set_IN1_IN2_UpdatePWM(u16 IN1_PwmValue, u16 IN2_PwmValue)
{
	PWMAB_Duty.PWM7_Duty = IN1_PwmValue; // 0~100, P33, PWM7, 控制 IN1 口
	PWMB_Duty7(PWMAB_Duty.PWM7_Duty);  // PWM占空比生效	
	
	
	PWMAB_Duty.PWM8_Duty = IN2_PwmValue; // 0~100, P34, PWM8, 控制 IN2 口
	PWMB_Duty8(PWMAB_Duty.PWM8_Duty); // PWM占空比生效	
}



/************************************************************/
/*                                                          */
/*            转向舵机的PWM控制信号                          */
/*                                                          */
/************************************************************/

#define  STEERING_SERVO_ANGLE_STEP		2 		// 舵机转动的最小角度间隔，按照这个粒度转动，如果每个角度都转动，电位器都会有一点漂移舵机就会晃动

u16 servoAngle_LastValue;		// 记录转向舵机上一次执行转向的角度


void Set_Servo_Angle(u16 servoAngle)
{
	if(servoAngle == servoAngle_LastValue){
		return;
	}
		
	// 按最小角度间隔进行四舍五入（不能直接减，因为不能处理负数）
	if(servoAngle > servoAngle_LastValue) 
	{
		if(servoAngle - servoAngle_LastValue < STEERING_SERVO_ANGLE_STEP)
		{
			return;
		}
	}
	else
	{
		if(servoAngle_LastValue - servoAngle < STEERING_SERVO_ANGLE_STEP)
		{
			return;
		}
	}
	
	
	// 控制舵机的PWM是50Hz，周期2ms (2000us), 控制信号的脉宽500--2500us，对应0度至180度
	PWMAB_Duty.PWM1_Duty = map(servoAngle, 0, 180, 500, 2500); // PWM周期值19999，1单位约为1us
	PWMA_Duty1(PWMAB_Duty.PWM1_Duty); // PWM占空比生效
	
//	printf("%u,%u,%u", servoAngle, servoAngle_LastValue, PWMAB_Duty.PWM1_Duty);

	
	servoAngle_LastValue = servoAngle;
}



/************************************************************/
/*                                                          */
/*            P32口 IRQ  INT0下降沿中断                      */
/*                                                          */
/************************************************************/


u8 rx_New_Data = 0; // 是否接收到了新数据
u8 rx_inited = 0;	// 是否已经和TX建立了连接，0=未连接，1=连接中，2=连接成功,传输业务数据. 初始为0

// 业务数据
u8 motorDirection	= 0;	// 方向，0是前进，1是后退
u8 motorSpeed 		= 0;	// 速度，0~100
u8 brakeValue		= 0;	// 是否刹车，1表示刹车
u8 lightValue		= 0;	// 灯光开关，1表示打开灯光
u8 buzzerValue		= 0; 	// 喇叭开关，1表示鸣笛
u8 steering_ServoAngle	= 90; // 转向舵机的角度，[0,180]，默认居中90度
u8 steering_ServoAngle_rx	= 90; // 转向舵机的角度，[0,180]，默认居中90度
u8 mosfetPwmDutyCycle	= 0; 	// PWM占空比，[0,100]，用于MOS管模块的PWM控制信号
u8 checkRxVolt = 0; // 是否要检测电池电压
	


// 经测试，不能在此中断函数中写接收数据的代码，写了的话会接收不成功，估计是中断中处理的时间太长影响了。
void INT0_Isr() interrupt 0
{	
	rx_New_Data = 1;	
}




void main(){
	
	u8 rf_ch; 	// 接收端的频道，0~125, 共126个值, 频道时可以修改和保存的
	u8 tx_addr[5] =  {0x20, 0x24, 0x06, 0x12, 0x00}; // 接收端的地址，必须是5个字节. 地址是固定值
	
	
	u8 i, state;
	u8 isSteering_Reverse = 0;	// 舵机方向是否反向，0 = 默认，1 = 反向
	
	u8  rxResult = 0;		// NRF24L01接收的结果, 1是有新数据，0是没有新数据
	u16 pwmValue = 0;
	u8 isBtn1_Pressed 	= 0;
	
	u16 voltCheckADCValue	= 0;	// 检测电池电压, [0, 1023]
	u16 volt 			= 0;	// 电压值, 没有小数点，300表示3V,330表示3.3V
	u16 voltLastValue 	= 0; 	// 上一次检测的电压值
	
	
	PWMAB_Duty.PWM1_Duty = 0; // 初始PWM的脉宽周期，需要在PWM初始化前定义
	PWMAB_Duty.PWM6_Duty = 0; // 初始PWM的脉宽周期，需要在PWM初始化前定义
	PWMAB_Duty.PWM7_Duty = 0; // 初始PWM的脉宽周期，需要在PWM初始化前定义
	PWMAB_Duty.PWM8_Duty = 0; // 初始PWM的脉宽周期，需要在PWM初始化前定义
	
	
	P_SW2 |= 0x80;		// 扩展寄存器访问使能，必须先设置好
	
	IO_Init();  		// 初始化引脚
	Timer0_Init(); 		// 初始化定时器
	SPI_Init(); 		// 初始化硬件SPI
//	UART1_Init(); 		// 初始化串口1
	PWM_config(); 		// 初始化PWM输出
	ADC_config();   	// 初始化ADC
	
	IT0 = 1;			//使能INT0下降沿中断
	EX0 = 1;			//使能INT0中断
 
	EA = 1; 			// 打开中断总开关
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	
	
	
	
	// 读取配置信息
	isSteering_Reverse 	= IapRead(0x0000);	// 舵机是否反向设置，读取之前存储的信息		
	rf_ch			 	= IapRead(0x0001);	// 接收频道	
	
	// 数据值大于正常值，说明之前没有存储过，使用默认值
	if(isSteering_Reverse > 1 || rf_ch > 125)
	{
		isSteering_Reverse = 0;
		rf_ch = 50;
	}
	
	
	
	
	// 参考https://blog.csdn.net/zrb2753/article/details/105488268
	// 上电之后，要等待100ms的时间让其渡过上电不稳定状态，进入TX/RX模式时，有130微秒的等待时间，一定要让PLL准备好，不然数据有可能乱码。
	delay_ms(150);
	init_nrf24l01_io();

	//设置为接收模式
	delay_ms(50);
	// ifnnrf_rx_mode2_ACKPAYLOAD(); // 设置为带ACK_PAYLOAD的接收模式
	ifnnrf_rx_mode3_ACKPAYLOAD(tx_addr, rf_ch); // 设置为带ACK_PAYLOAD的接收模式
	
	// 默认
	LIGHT_IO = LIGHT_OFF;			// 灯光关闭 
	BUZZER_IO = BUZZER_OFF;  		// 蜂鸣器关闭 
	Set_IN1_IN2_UpdatePWM(0, 0);	// 行走马达滑行，休眠	
	steering_ServoAngle = 90; 		// 转向马达的角度为居中90度
	mosfetPwmDutyCycle = 0; 		// MOSFET电子开关关闭
	led_blink_period_ms = 0;		// LED常亮
	

//	printf("RX init success.");
	
	
	while(1){
		
		
		// 有新数据
		if(rx_New_Data == 1){
			rx_New_Data = 0;
			
			
			// 连接过程超过最长等待时间，就置0等待新连接
			if(rx_inited == 1 && rx_conn_timeout_msCounter > 1100)
			{
				rx_conn_timeout_msCounter = 0;
				rx_inited = 0;
			}
			
			
			// ========== 准备要回传的数据 ==========
			//（如果放到接收新数据的代码后面，发射端接收的ACKPAYLOAD就是一次有一次没有交替，放到前面就是正常每次都能收到）
			// ！！经测试，准备ack_payload的代码只能放在这个位置，放到if的外面前后都不行，都会导致不能回传，这边中断是0x40。
			// 可能是放到if外，循环执行的频率非常高，nrf模块来不及？？
			if(rx_inited == 0){
				ack_buf[0] = NRF24L01_DATA_RX_ACK_TEST; // 未建立连接时
			}
			else if(rx_inited == 1)
			{
				ack_buf[0] = NRF24L01_DATA_RX_ACK_CONN; // 正在连接中
			}
			else
			{ // 已经建立连接时
				ack_buf[0] = NRF24L01_DATA_RX_ACK_BIZ; // 第一位必须是约定好的，以便能让发射端检测是否回传的数据有误
				ack_buf[1] = volt / 100; // 电压的整数值
				ack_buf[2] = volt % 100; // 电压的小数值，因为检测时乘了100，所以是volt%100
			}
			
			// 准备ACK_PAYLOAD前需要先清空TX_FIFO
			CSN = 0;
			SPI_RW(FLUSH_TX);
			SPI_RW(0xff);	// 不加这个，只是SPI_RW(FLUSH_TX)会频繁出现发送失败的情况！
			CSN = 1;
			
			// 设置ACK_PAYLOAD
			SPI_Write_Buf(W_ACK_PAYLOAD, ack_buf, ACK_PLOAD_WIDTH);

			
			// ========== 接收新数据 ==========
			state = SPI_Read(STATUS);				// 读取状态寄存器的值  
			SPI_RW_Reg(WRITE_REG+STATUS, state);	//清除中断标志       

			// printf("%c", state);

			//接收到数据, 只接收到新数据没有ack_payloa触发0x40，接收新数据并回传ack_payload触发0x60（0x40+0x20)
			if(state & STA_MARK_RX)								      
			{	
				// 接收新数据
				SPI_Read_Buf(RD_RX_PLOAD, rx_buf, TX_PLOAD_WIDTH);    //读取数据
				SPI_RW_Reg(FLUSH_RX,0xff);					          //清除RX FIFO寄存器，不及时清除的话，满了就不能继续接收新数据了
			
				// 解析数据, 第1位是约定标志，是业务数据就解析
				if(rx_buf[0] == NRF24L01_DATA_TX_CONN)
				{ 	
					rx_inited = 1; // 连接中
				}
				else if(rx_buf[0] == NRF24L01_DATA_TX_BIZ)
				{ 	
					rx_inited = 2;  // 收到业务数据后，表示建立连接成功（超时未接受数据断联后会置0）
					
					motorDirection		= rx_buf[1]; 	// 电机方向，0是前进，1是后退
					motorSpeed 			= rx_buf[2];	// 电机速度，0 -- 100
					brakeValue			= rx_buf[3];	// 是否刹车,1 刹车，0 未刹车
					steering_ServoAngle_rx = rx_buf[4];	// 转向舵机的角度，0 -- 180
					lightValue			= rx_buf[5]; 	// 灯光，钮子开关
					buzzerValue			= rx_buf[6];	// 喇叭，自复位按钮
					mosfetPwmDutyCycle	= rx_buf[7];	// 控制MOS管模块PWM占空比的数据
					checkRxVolt 		= rx_buf[8]; 	// 是否要检测电池电压
					
					 // 接收新数据的毫秒计时清零
					rx_timeout_msCounter = 0;
				}
			}
			
	
//			printf("%c", rx_inited);
		}

		
		// ***************** 舵机反向的设置按钮逻辑  ************************
		// 
		// 处理舵机是否反向（按下一次改变一次：正常和反向来回切换，需要消抖处理。按一次记忆一次）
		
		/*
		if(BTN_SETTING_IO == SW_ON){
			delay_ms(5); // 消抖
			if( BTN_SETTING_IO == SW_ON){
				isBtn1_Pressed = 1;
			}
		}else{ 
			// 在按键松开时执行逻辑
			if(isBtn1_Pressed == 1){
				// ！！使用 isSteering_Reverse = !isSteering_Reverse; 的话，会出现莫名其妙的问题：
				// ！！后续计算出的steering_ServoAngle的数值会乱跳（使用串口打印printf("%c", steering_ServoAngle);）看到的。不知为什么
				// 改为使用三目运算符，就没问题了,后面测试好像还是有问题。
				// 经查，应该时因为程序编译后超过8K大小，引发了莫名其妙的问题，调整代码和设置，编译的程序小于8K，就好了，
				isSteering_Reverse = (isSteering_Reverse == 0 ? 1 : 0); // 切换行走马达的方向
					
				// 保存设置，需要先擦除再写
				IapErase(0x0000);		
				IapProgram(0x0000, isSteering_Reverse);	// 保存舵机反向
				IapProgram(0x0001, rf_ch); 				// 保存接收频道
			}
			isBtn1_Pressed = 0;
		}
		*/
		
		
		
			
		// ***************** LED指示灯的逻辑  ************************
		
		
		if(rx_inited == 0)	led_blink_period_ms = 500; // 未连接状态，慢闪
		if(rx_inited == 1)	led_blink_period_ms = 100; // 连接中，快闪
		if(rx_inited == 2)	led_blink_period_ms = 0;   // 已连接，常亮
		
		if(led_blink_period_ms > 0) // 闪烁
		{
			if(led_blink_msCounter >= led_blink_period_ms)
			{
				LED = !LED;
				led_blink_msCounter = 0;
			}
			
		}else	// 常量
		{
			LED = LED_ON;
		}
		
		
	
	
	
		// ***************** 重设频道的按钮逻辑  ************************
	
		// 按钮：设置为当前+1频道，超过125，就是0
		if(BTN_RF_CH_ADD == SW_ON){
			delay_ms(10); // 消抖
			if(BTN_RF_CH_ADD == SW_ON && btn_rf_ch_add_flag == 0){ // 按下时执行一次
				
				if(rf_ch == 125) 
					rf_ch = 0;
				else 			 
					rf_ch++;
				
				ifnnrf_changeChannel(rf_ch);
				rx_inited = 0;  // 等待新连接
				btn_rf_ch_add_flag = 1;
				
				// 保存设置，需要先擦除再写
				IapErase(0x0000);		
				IapProgram(0x0000, isSteering_Reverse);	// 保存舵机反向
				IapProgram(0x0001, rf_ch); 				// 保存接收频道
			}
		}else{
			btn_rf_ch_add_flag = 0;
		}
		
		
		// 按钮：设置为当前-1频道，低于0，就是125
		if(BTN_RF_CH_MINUS == SW_ON){
			delay_ms(10); // 消抖
			if(BTN_RF_CH_MINUS == SW_ON && btn_rf_ch_minus_flag == 0){ // 按下时执行一次
				
				if(rf_ch == 0)
					rf_ch = 125;
				else
					rf_ch--;
				
				ifnnrf_changeChannel(rf_ch);
				rx_inited = 0;  // 等待新连接
				btn_rf_ch_minus_flag = 1;
				
				// 保存设置，需要先擦除再写
				IapErase(0x0000);		
				IapProgram(0x0000, isSteering_Reverse);	// 保存舵机反向
				IapProgram(0x0001, rf_ch); 				// 保存接收频道
			}
		}else{
			btn_rf_ch_minus_flag = 0;
		}	
		
		
		
		// ***************** 准备ACK_PAYLOAD数据*****************
	
		// 获取电池电压数据
		 if(checkRxVolt == 1) // 检测电压标志为1时，才检测电压
		 {	 
			 
			if(voltCheck_msCounter > 300) // 每隔一段时间检测一次电压
			{
				voltCheck_msCounter = 0;

				//读取8次数据
				voltCheckADCValue  = 0;
				for(i=0; i<8; i++)
				{
					voltCheckADCValue += Get_ADCResult(VoltCheck_IO_ADC_CHANNEL); // ADC1，P11
				}
				voltCheckADCValue >>= 3;  	// 除以8取平均值
				volt = 4.026 * 3.3 * (voltCheckADCValue) / 1024.0  * 100; // 10位ADC最大1023		

				// printf("%u,%u", voltCheckADCValue, volt);
			}
		}
		else
		{
			volt = 0; // 不检测电压时默认是0，不能存有上次的值，因为有可能过去很长时间了
		}
	

		
		/*
		// ******************* 接收数据 *******************
		rxResult = RxPacket_ACKPAYLOAD(rx_buf, ack_buf); // 接收数据，并回传ACK_PAYLOAD数据


		if(rxResult){ // 如果接收成功有新数据
			rx_timeout_msCounter = 0; // 清零遥控超时的计时
			
			// 获取数据
			motorDirection = rx_buf[0]; 		// 电机方向，0是前进，1是后退
			motorSpeed = rx_buf[1];				// 电机速度，0 -- 100
			brakeValue = rx_buf[2];				// 是否刹车,1 刹车，0 未刹车
			steering_ServoAngle_rx = rx_buf[3];	// 转向舵机的角度，0 -- 180
			
			lightValue = rx_buf[4]; 			// 灯光，钮子开关
			buzzerValue = rx_buf[5];			// 喇叭，自复位按钮
			mosfetPwmDutyCycle = rx_buf[6];		// 控制MOS管模块PWM占空比的数据

			checkRxVolt = rx_buf[7]; 			// 是否要检测电池电压
			
			// 调试信息
		//	 printf("%c,%c,%c,%c,%c,%c,%c,%c", motorDirection, motorSpeed, brakeValue, steering_ServoAngle, lightValue, buzzerValue, mosfetPwmDutyCycle, checkRxVolt);
		//	 delay_ms(100);
		}
		*/
	
	
		
		// ****************** 超时500ms没有接收到遥控数据，就停止电机等，各功能均置为默认状态 *************
		if(rx_timeout_msCounter >= 500){ 
			rx_timeout_msCounter = 0;
					
			motorSpeed = 0; 		// 电机速度为0
			brakeValue = 0; 		// 不刹车
			lightValue = 0; 		// 灯光关闭
			buzzerValue = 0;		// 喇叭关闭
			mosfetPwmDutyCycle = 0; // MOSFET电子开关关闭
			// 行走马达的方向不需要置零
			// 转向马达的角度不需要置零
			checkRxVolt = 0;		// 不检测电压
			
			
			// 只有在业务连接断开后，才能置0。因为连接中的状态停留时间可能超过断连超时时间，这是状态置0会导致占频道失败
			if(rx_inited == 2) rx_inited = 0;  // 超时未收到业务数据，表示断连，等待新连接
		}
		
		
		
		// ************************ 根据最新状态，执行各种动作 ******************
		
		if(action_msCounter > 50) // 每隔一段时间执行一次动作，频率不要太高. （经测试设置成100，偶尔感觉到延迟， 50就感觉不到）
		{
			action_msCounter = 0;
			
			
			// 灯光和喇叭
			LIGHT_IO  = lightValue  ? LIGHT_ON  : LIGHT_OFF; // 灯光控制 
			BUZZER_IO = buzzerValue ? BUZZER_ON : BUZZER_OFF;  // 蜂鸣器控制 
			
			// 转向舵机
			// 如果舵机反向，旋转方向左右改变一下：0度变180度，180度变0度
			if(isSteering_Reverse == 1){
				// steering_ServoAngle = (180 - steering_ServoAngle) ; // 这句执行的话，P10输出的PWM信号会占空比乱跳，不知为什么！！！？？？
				// steering_ServoAngle =  steering_ServoAngle ; // 这样没事
				// steering_ServoAngle = 180; // 这样也是正常的PWM
				
				// steering_ServoAngle = (180 - steering_ServoAngle); // 这样就不行，PWM占空比乱跳 
				// delay_ms(30); // 增加延时，减少PWM更改设置的频率，也不行
				
				// 再定义一个变量，这里使用两个变量，就没事了 ！！！ 不知为什么？？？
				steering_ServoAngle = (180 - steering_ServoAngle_rx); 
			}else{
				steering_ServoAngle = steering_ServoAngle_rx; 
			}
			
			// 直接设置舵机角度，中值死区、减少角度灯都在遥控端处理了
			Set_Servo_Angle( steering_ServoAngle );
			
			
			
			// MOS管电子开关模块的PWM控制
			if(mosfetPwmDutyCycle < 10){ // 小于10不启动开关（发送端使用1位数码管，小于10不显示，大于10显示十位数上的数字）
				PWMAB_Duty.PWM6_Duty = 0; // 0~100, PWM的周期设置是100
			}
			else{
				// mosfetPwmDutyCycle范围0~100, PWM的周期设置是100, 所以可以直接赋值使用
				// PWMAB_Duty.PWM6_Duty = mosfetPwmDutyCycle;	
				
				// 10~100控制0~100，这样电压能从0开始，方便控制风扇
				PWMAB_Duty.PWM6_Duty = map(mosfetPwmDutyCycle, 10, 100, 0, 100);		
			}
			PWMB_Duty6(PWMAB_Duty.PWM6_Duty); // PWM占空比生效	
			
			
			
			// 前进后退、速度、刹车
			if(brakeValue == 1){ // 刹车
				// TODO: （周期设定的是100：PWMx_InitStructure.PWM_Period   = 100;）指定为100或200，示波器显示占空比是99.8%左右，并不是100%，不知为什么？？
				// ！！P33和P34接AT8236的IN1和IN2，高电平时测试时0.6V，高电平不对，去掉和IN1或IN2口的连接，就是3.3V的高电平。
				// 经测试，把IO口设为推挽输出（P33, P34 推挽输出），测试的高电平就是3.3V，就好了：设置IN1和IN2均为周期最大值100，就是刹车效果
				Set_IN1_IN2_UpdatePWM(100, 100);  // IN1是1，IN2是1，效果：刹车
			}
			else{ // 未刹车

				// 速度值小于5，不启动电机
				if(motorSpeed < 5){ 
					Set_IN1_IN2_UpdatePWM(0, 0); // IN1是0，IN2是0，效果：滑行，休眠
				}else{
					// IN1是PWM， IN2是0，  正转，快衰减.  PWM占空比越大，速度越快
					// IN1是1，   IN2是PWM，正转，慢衰减.  PWM占空比越大，速度越慢
					// **
					// IN1是0，  IN2是PWM， 反转，快衰减
					// IN1是PWM，IN2是1，   反转，慢衰减
					
					// 速度值0~100对应电压0~VM电压，前面50%的电压太低，马达几乎转不起来，所以从一半电压左右开始为起点调速
					pwmValue = map(motorSpeed, 0, 100, 40, 100);

					// 使用快衰减模式, 这样调整到速度0时，是滑行休眠
					if(motorDirection == 0){ 	// 前进
						Set_IN1_IN2_UpdatePWM(pwmValue, 0); 
					}else{						// 后退
						Set_IN1_IN2_UpdatePWM(0, pwmValue); 
					}
				}
			}
		}
		
	
	}

}