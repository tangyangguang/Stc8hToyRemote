
/***
nRF24L01一次最多发送32位数据，约定的数据格式为：

Byte 0: 约定的标志
后续： 业务数据

*/
/**
  V2.1
  1：两个刹车功能：编码器上的按钮是刹车并且速度置零，单独的刹车按钮是按下刹车，速度不归零，松开还是执行原有速度。
  2：前进后退改为使用钮子开关（常开常关），原来是按钮按下切换方向。
  3，灯光改为自复位按钮，按下开松开关（原来是钮子开关）。
  4，增加方向反向的设置项。
  
  

V2.2 改动
 增加了P33口的电位器控制速度，原编码器不再控制速度（还能用来刹车和配置）；
 修改了屏幕的接口为 CLK --> P34, DIO --> P31
 单独刹车按钮接口为 P30



V2.3改动
  增加了电池电压检测功能
  修改：nRF24L01的第8Pin（IRQ）改成了P32, P32支持中断（sbit IRQ = P3^2;）
  修改：马达速度的调整使用EC11旋转编码器
  
V2.4改动
  修改nRF24L01的配置，增加接收端回传数据的功能（比如锂电池电压）
  * 以下拷贝自Si24R1芯片的数据手册
	“接收端在 回 复 ACK 信号时，可以同时发送带有负载数据的 ACK 信 号
	（ACKPAYLOAD）。开启这一功能需要配置 FETURE 寄存器中的 EN_ACK_PAY 位，
	并且双方必须开启动态负载长度。”
*/

#include "my_stc8h_lib.h"
#include "STC8H.h"
#include "STC8G_H_ADC.h"
#include <intrins.h>
#include <stdlib.h>
#include <stdio.h> 

#include "TM1637.h"
#include "my_nRF24L01.h"


/************************************************************/
/*                                                          */
/*                    常量定义                              */
/*                                                          */
/************************************************************/

// 按钮开关的值，按钮上拉，所以按下是0
#define SW_ON	0 
#define SW_OFF	1



/************************************************************/
/*                                                          */
/*                    IO口定义                              */
/*                                                          */
/************************************************************/

// 接EC11旋转编码器.  EC11编码器的三个引脚都需要接上拉电阻10K
// 控制行车马达的速度, 中间按键控制是否刹车
#define EC11_SW		P54
#define EC11_A		P11
#define EC11_B		P10

// 接电位器. 不需要接上拉或下拉电阻，需要把IO口设置为高阻输入模式
#define ADC_STEERING_IO			11 	// ADC通道11是P33, 接10K电位器, 控制方向舵机的方向. 

// 接按钮，需要10K上拉电阻，按下是0
#define SW_MOTOR_DIR_IO 		P37 // 钮子开关，改变马达前进后退状态
#define BTN_LIGHT_IO 			P36 // 自复位按钮，控制灯光
#define BTN_BUZZER_IO 			P35 // 自复位按钮，控制喇叭
#define BTN_MOTOR_BRAKE_IO		P30 // 自复位按钮，控制刹车
#define BTN_FN_IO 				P34 // 自复位按钮，Fn功能键，显示电池电压



/************************************************************/
/*                                                          */
/*                    变量定义                              */
/*                                                          */
/************************************************************/

// 进入配置模式后，数码管闪烁效果的毫秒计时
u16 blinkMsCounter = 0;

// 进入配置模式时按键按下的时长毫秒计时
u16 configBtnMsCounter = 0; 

// 显示电压信息的毫秒计时
u16 voltShow_msCounter = 0;

// 上一次成功发射数据的毫秒计数
u16 tx_timeout_msCounter = 0;

/************************************************************/
/*                                                          */
/*                 处理EC11旋转编码器的逻辑                  */
/*                                                          */
/************************************************************/

int EC11_Value = 0; // 旋转的当前值

// 旋转编码器累加值的最大值和最小值
#define EC11_VALUE_MAX 		100		
#define EC11_VALUE_MIN		0	

// 初始间隔100ms, 即默认不是快速旋转 
u16 EC11_Interval_msCounter = 100; 	// EC11旋转时两格之间的时间间隔(毫秒数)，表示旋转速度快慢，速度快就间隔小. 

void EC11_Action()
{
	static bit EC11_Flag = 0, Left_Flag = 0, Right_Flag = 0;
	if ((EC11_A) && (EC11_B)) // 两个都是1，表示开始第一个脉冲
	{
		EC11_Flag = 1; // 标记为第一个信号收到，可以进行下一个信号
	}
	if (EC11_A != EC11_B) // 第二个信号
	{
		Left_Flag = EC11_A; // 记录AB的信号状态
		Right_Flag = EC11_B;
	}
	if (EC11_Flag)
	{
		if ((EC11_A == 0) && (EC11_B == 0)) // 第三个信号，处理第二个信号的值
		{
			signed char offset = 1; // 默认转一格加减1
			if(EC11_Interval_msCounter <= 3){ // 快速转就一次加减20（转动之间的间隔小于指定毫秒数）
				offset = 20;
			}
			else if(EC11_Interval_msCounter <= 6){ // 快速转就一次加减10（转动之间的间隔小于指定毫秒数）
				offset = 10;
			}
			else if(EC11_Interval_msCounter <= 15){ //快速转就一次加减5（转动之间的间隔小于指定毫秒数）
				offset = 5;
			}
			
			EC11_Interval_msCounter = 0;
			
			
			if (Left_Flag) // 左转
			{
				EC11_Value -= offset; // 旋转的值加
				if(EC11_Value < EC11_VALUE_MIN)		EC11_Value = EC11_VALUE_MIN;			
			}
			else if (Right_Flag)
			{
				EC11_Value += offset; // 旋转的值减
				if(EC11_Value > EC11_VALUE_MAX)		EC11_Value = EC11_VALUE_MAX;
			}
			EC11_Flag = 0; // 把标志位清零，等待下一个周期
		}
	}
}



/************************************************************/
/*                                                          */
/*                 初始化 IO口                              */
/*                                                          */
/************************************************************/

// 1, 电位器引脚 对应的IO口必须要是高阻输入模式
// 2, 必须要接ADC_VRef+引脚（接VCC）,
// 3, 按钮要是 准双向口模式
void IO_Init(){
	P1M1 = 0X00; P1M0 = 0X00; // 准双向口
	P3M1 = 0X00; P3M0 = 0X00; // 准双向口
	P5M1 = 0X00; P5M0 = 0X00; // 准双向口
	
	// P33是高阻输入(P33, ADC11)，其他是准双向口
	P3M0 = 0x00; P3M1 = 0x08; 	
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
/*                 定时器0设置                              */
/*                                                          */
/************************************************************/

void Timer0_Init(void)  //1毫秒@12MHz
{	
	AUXR |= 0x80;		//定时器时钟1T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x20;			//设置定时初始值
	TH0 = 0xD1;			//设置定时初始值
	TF0 = 0;			//清除TF0标志
	TR0 = 1;			//定时器0开始计时	
	ET0=1;				// 使能定时器0中断，Timer0开中断 
}


// 定时器0的中断处理函数
void Timer0_ISR_Handler (void) interrupt TMR0_VECTOR		//进中断时已经清除标志
{
	EC11_Interval_msCounter++;
	configBtnMsCounter++;
	blinkMsCounter++;
	voltShow_msCounter++;
	tx_timeout_msCounter++;
	
	// 检测EC11的动作，根据动作加减值
	EC11_Action();  			
}


	
void main(){
	
	u8 rf_ch = 0; // 0~125, 共126个值
	u8 tx_addr[5] =  {0x20, 0x24, 0x06, 0x12, 0x00}; // 必须是5个字节
	u8 cfg_last_rf_ch;
	
	u8 i = 0;
	
	u8 config_Mode_Flag = 0;	// 1表示配置模式
	u8 isPressed_ModeBtn = 0;	// 配置模式按钮是否按下
	u8 isExecuted_ModeBtn = 0;	// 是否已经处理了按下配置键的逻辑
	u8 isScreenBlack = 0; 		// 闪烁时是否黑屏了
	
	u8 is_Config_First_Action = 0; // 是否时刚刚进入配置模式
	u8 is_Config_Unsaved = 0; 	// 是否有未保存的已修改配置（配置模式下） 
	
	
	u8 cfg_EC11_Current_Item = 0; 			// 旋转编码器当前修改的配置项，0 表示舵机中值设置项，1 表示舵机左右减少角度的配置项
	u8 cfg_Steering_Reduced_Angle_MAX = 60; // 舵机减少的角度，最大值
	u8 cfg_Steering_Middle_Value_MIN = 20;	// 舵机中值可以设置的最小值（1单位表示舵机2度）20表示40度
	u8 cfg_Steering_Middle_Value_MAX = 70;	// 舵机中值可以设置的最大值（1单位表示舵机2度）70表示140度
	
	u8 cfg_Steering_Reverse = 0; // 舵机是否反向，1 = 反向
	u8 cfg_Steering_Reduced_Angle = 0;		// 舵机减少的角度，取值[0,MAX], 数值表示两边同时减少的角度，如0表示 0--180; 10 表示 10--170; 20 表示 20--160
	u8 cfg_Steering_Middle_Value = 45;		// 舵机中值，默认45（表示90度），因为只能2位显示，使用1单位表示舵机2度

	u8 cfg_Direction_Reverse= 0; // 前进后退方向的值是否反转

	u8 cfg_Steering_Reduced_Angle_DEFAULT = 20; // 舵机减少的角度，默认值, 第一次运行遥控器时的默认值
	
	
	// 电位器的原始值，取值[0, 1023]
	u16	vr1_Value = 0;
	
	// 旋转编码器的上一次的值，用于对比是否旋转了编码器
	u16 Last_EC11_Value = 0;

	
	// 业务数据
	u8 motorDirection	= 0;	// 方向，0是前进，1是后退
	u8 motorSpeed 		= 0;	// 速度，0~100
	u8 brakeValue		= 0;	// 是否刹车，1表示刹车
	u8 lightValue		= 0;	// 灯光开关，1表示打开灯光
	u8 buzzerValue		= 0; 	// 喇叭开关，1表示鸣笛
	u8 steering_ServoAngle	= 0; // 转向舵机的角度，[0,180]，默认居中90度
	
	u16 voltCheckADCValue	= 0;	// 检测电池电压, [0, 1023]
	u16 volt = 0;				// 电压值, 没有小数点，300表示3V,330表示3.3V
	u8  isShowVolt 		= 0; 	// 是否显示电压值
	
	u8 checkRxVolt 		= 0;	// 是否检测接收器端的电池电压, 按下Fn功能键后检测
	
	u8 steering_middleAngle = 90; // 方向舵机的中值，可以在配置模式配置
	u8 steering_Deadband	= 15; // 方向舵机中值的左右死区角度，15表示中值向左15度和向右15度均为中值。
	
	
	// 
	u8 tm1637Values[5] = { 8, 8, 8, 8, 1 };	// TM1637芯片的4位屏显示内容，默认全显示 
	u8 txResult 		= 0; 			// nrf24l01发送结果: 0x10:达到最大重发次数，发送失败.  0x20:发送成功.  0xff:发送失败  
	u8 isBtn1_Pressed 	= 0;
	u8 isBtn3_Pressed 	= 0;
	u8 isBtn_EC11_SW_Pressed = 0;
	
	
	u8 ack_payload_width = 0; 
	u8 rx_volt_INT, rx_volt_DEC, isShowRxVolt = 0; 

	
	P_SW2 |= 0x80;	// 扩展寄存器访问使能，必须先设置好
	IO_Init();  	// 初始化引脚
	Timer0_Init();	// 初始化定时器0
	ADC_config();   // ADC初始化
	SPI_Init(); 	// 初始化硬件SPI
//	UART1_Init();	// 初始化串口1
	EA = 1; 		// 打开中断总开关 
 
	
	
	// 上电后先显示一下，否则如果没插nRF24L01模块的话，屏幕一直是黑
	tm1637Values[0] = 22; // 22是'-'
	tm1637Values[1] = 22;
	tm1637Values[2] = 22;
	tm1637Values[3] = 22;
	tm1637Values[4] = 0; // 不显示冒号
	TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
			

	
	
	// 参考https://blog.csdn.net/zrb2753/article/details/105488268
	// 上电之后，要等待100ms的时间让其渡过上电不稳定状态，进入TX/RX模式时，有130微秒的等待时间，一定要让PLL准备好，不然数据有可能乱码。
	delay_ms(150);
	init_nrf24l01_io(); // 初始化nRF24L01
		

	// 发送模式
	delay_ms(50);
	// ifnnrf_tx_mode2_ACKPAYLOAD();  // 设置为带ACK_PAYLOAD的发送模式
	//ifnnrf_tx_mode3_ACKPAYLOAD(RF_CH_value, tx_addr);  // 设置为带ACK_PAYLOAD的发送模式，使用指定的频道和地址



	// 读取配置信息, 只读取一次
	cfg_Steering_Reverse 		= IapRead(0x0000);	// 舵机是否反向设置，读取之前存储的信息
	cfg_Steering_Reduced_Angle	= IapRead(0x0001);	// 舵机减少角度设置，读取之前存储的信息
	cfg_Steering_Middle_Value	= IapRead(0x0002);	// 舵机中值设置，读取之前存储的信息
	cfg_Direction_Reverse		= IapRead(0x0003);	// 方向是否反转设置，读取之前存储的信息
	cfg_last_rf_ch		= IapRead(0x0004);	// 上一次联通的频道，读取之前存储的信息
		
	// 数据值大于正常值，说明之前没有存储过，使用默认值
	if(cfg_Steering_Reverse > 1 
			|| cfg_Steering_Reduced_Angle > cfg_Steering_Reduced_Angle_MAX
			|| cfg_Direction_Reverse > 1
			|| cfg_Steering_Middle_Value > 180)
	{
		cfg_Steering_Reverse 		= 0;
		cfg_Steering_Reduced_Angle	= cfg_Steering_Reduced_Angle_DEFAULT;
		cfg_Steering_Middle_Value	= 45;
		cfg_Direction_Reverse		= 0;
		cfg_last_rf_ch				= 0;
	}
		

	// 频道要正确才行
	if(cfg_last_rf_ch > 125)	
		cfg_last_rf_ch = 0; 

	// 寻找频道，设为发送模式，没找到就一直找，并且屏显当前正在测试的频道。优先寻找上次联通的频道
	tm1637Values[0] = 21; // 21是黑屏不显示
	tm1637Values[4] = 0;  // 不显示冒号
		
	// 先使用上次成功连接的频道尝试
	rf_ch = cfg_last_rf_ch;
	txResult = ifnnrf_tx_mode3_ACKPAYLOAD_setAllAndTest2(tx_addr, rf_ch, 3);  // 3次都发送成功，才是成功
	delay_ms(10);
	
	
	// 上次的频道不能连接，就扫描
	if(txResult == 0)
	{
		for(rf_ch = 0; rf_ch <= 125; rf_ch += 1)
		{
			tm1637Values[1] = (rf_ch >= 100) ? 1 : 21;
			tm1637Values[2] = (rf_ch >= 100) ? ((rf_ch-100) / 10) : (rf_ch / 10);
			tm1637Values[3] = rf_ch  % 10;
			TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
			
			txResult = ifnnrf_tx_mode3_ACKPAYLOAD_setChannelAndTest2(rf_ch, 3);    // 3次都发送成功，才是成功
			if(txResult == 1){
				break;
			}
			
			// 找不到，就一直找
			if(rf_ch == 125) 
			{
				rf_ch = 0;
			}
		}
		
		// 找到频道后存储起来
		if(cfg_last_rf_ch != rf_ch){
			cfg_last_rf_ch = rf_ch;
			IapErase(0x0000); 	 // 需要先擦除再写	
			IapProgram(0x0000, cfg_Steering_Reverse);
			IapProgram(0x0001, cfg_Steering_Reduced_Angle);
			IapProgram(0x0002, cfg_Steering_Middle_Value);
			IapProgram(0x0003, cfg_Direction_Reverse);
			IapProgram(0x0004, cfg_last_rf_ch);
		}

	}
	
	// 找到等待连接的接收端后，先占下来
	ifnnrf_tx_mode3_ACKPAYLOAD_Conn_CurrChannel();
	ifnnrf_tx_mode3_ACKPAYLOAD_Conn_CurrChannel();
	ifnnrf_tx_mode3_ACKPAYLOAD_Conn_CurrChannel();
	
	
	// 再显示一次，否则可能会出现显示比找到的频道小1，可能是找的快的话，显示跟不上的原因
	tm1637Values[1] = (rf_ch >= 100) ? 1 : 21;
	tm1637Values[2] = (rf_ch >= 100) ? ((rf_ch-100) / 10) : (rf_ch / 10);
	tm1637Values[3] = rf_ch  % 10;
	TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
	
	delay_ms(750); // 定格显示一段时间最后寻找到的频道
	

	
		
//	printf("Init success.");
	
	while(1){
		
	
		
		// 同时按下编码器和单独的刹车键, 到达一定时间，进入或退出配置模式
		if(BTN_MOTOR_BRAKE_IO == SW_ON && EC11_SW == SW_ON){
		
			if(isPressed_ModeBtn == 0){
				// 刚开始按下配置键
				configBtnMsCounter = 0; 	// 开始计时
				isPressed_ModeBtn = 1;		// 按下了配置按钮的百标志
			}else{
				
				if(isExecuted_ModeBtn == 0){ // 按下一次只执行一次进入动作
					// 按下配置键到达指定时长
					if(configBtnMsCounter > 3000){ // 3秒
						
						config_Mode_Flag = 1; 	// 进入或退出配置模式
						is_Config_First_Action = 1; // 刚进入配置模式的标志
						
						// 刚进入配置模式，显示之前的配置
						blinkMsCounter = 0; // 闪烁计时置零
						cfg_EC11_Current_Item = 0;				 // 刚进入配置模式，编码器默认是调整舵机两边减少角度
						EC11_Value = cfg_Steering_Reduced_Angle; // 刚进入配置模式，编码器默认是调整舵机两边减少角度
						
						isExecuted_ModeBtn = 1; // 表示已经执行过了
					}
				}
			}			
		}else{
			isPressed_ModeBtn = 0;
			isExecuted_ModeBtn = 0;
		}
		
		
		// ====================================================== 配置模式 
		if(config_Mode_Flag == 1)
		{
			
			// 进入了配置模式，并且两个按钮都松开了
			if(BTN_MOTOR_BRAKE_IO == SW_OFF && EC11_SW == SW_OFF){
				delay_ms(5); // 消抖
				if(BTN_MOTOR_BRAKE_IO == SW_OFF && EC11_SW == SW_OFF){
					is_Config_First_Action = 0; // 切换标志
				}
			}
			
			
			
			// ****** 处理前后方向调整（方向是钮子开关/常开常关，当前所处的状态是向前）
			if(cfg_Direction_Reverse == 0)
			{
				if(SW_MOTOR_DIR_IO == SW_ON)
				{
					delay_ms(5); // 消抖
					if( SW_MOTOR_DIR_IO == SW_ON)
					{ 
						cfg_Direction_Reverse = 1; // 方向反转
						is_Config_Unsaved = 1;  // 有未保存的修改配置
					}
				}
			}
			else if(cfg_Direction_Reverse == 1)
			{
				if(SW_MOTOR_DIR_IO == SW_OFF){
					delay_ms(5); // 消抖
					if( SW_MOTOR_DIR_IO == SW_OFF){ 
						cfg_Direction_Reverse = 0; //  方向不反转
						is_Config_Unsaved = 1;  // 有未保存的修改配置
					}
				}
			}
		
			
					
			// ****** 处理舵机反向（按下按键时改变，需要消抖处理）
			if(BTN_MOTOR_BRAKE_IO == SW_ON){
				delay_ms(5); // 消抖
				if(BTN_MOTOR_BRAKE_IO == SW_ON){
					if(isBtn1_Pressed == 0 && is_Config_First_Action == 0){ // 第二个条件时防止进入配置模式时按下的按键改变了配置项
						cfg_Steering_Reverse = !cfg_Steering_Reverse; // 切换舵机反向
						
						isBtn1_Pressed = 1; 	// 只执行一次
						is_Config_Unsaved = 1;  // 有未保存的修改配置
					}
				}
			}else{
				isBtn1_Pressed = 0;
			}
			
			
			// ****** 编码器调整的配置项，在“舵机中值”和“舵机减少角度”之间切换（按下按键时改变，需要消抖处理）
			if(EC11_SW == SW_ON){
				delay_ms(5); // 消抖
				if(EC11_SW == SW_ON){
					if(isBtn_EC11_SW_Pressed == 0 && is_Config_First_Action == 0){ // 第二个条件时防止进入配置模式时按下的按键改变了配置项
						isBtn_EC11_SW_Pressed = 1; 	// 只执行一次
						
						cfg_EC11_Current_Item = !cfg_EC11_Current_Item; // 切换
						
						// 切换时显示当前配置项的数据值
						if(cfg_EC11_Current_Item == 0) // 0 表示舵机左右减少角度的配置项
						{
							EC11_Value = cfg_Steering_Reduced_Angle;
							Last_EC11_Value = EC11_Value;
						}
						else // 1 表示舵机中值设置项
						{
							EC11_Value = cfg_Steering_Middle_Value;
							Last_EC11_Value = EC11_Value;
						}
						
					}
				}
			}else{
				isBtn_EC11_SW_Pressed = 0;
			}
			
			
			// ****** 处理编码器调整的配置项，舵机中值、舵机左右减少的角度
			if(EC11_Value != Last_EC11_Value) // 最新和上次不一致，表示转动了编码器
			{ 
				Last_EC11_Value = EC11_Value;
			
				 // 0 表示舵机左右减少角度的配置项
				if(cfg_EC11_Current_Item == 0)
				{	
					cfg_Steering_Reduced_Angle = EC11_Value;
					if(cfg_Steering_Reduced_Angle < 0)
					{
						EC11_Value = 0;
						cfg_Steering_Reduced_Angle = 0; // 最小值
					}
					if(cfg_Steering_Reduced_Angle > cfg_Steering_Reduced_Angle_MAX)
					{
						EC11_Value = cfg_Steering_Reduced_Angle_MAX;
						cfg_Steering_Reduced_Angle = cfg_Steering_Reduced_Angle_MAX; // 最大值
					}
				}
				// 1 表示舵机中值设置项
				else
				{
					cfg_Steering_Middle_Value = EC11_Value;
					if(cfg_Steering_Middle_Value < cfg_Steering_Middle_Value_MIN)
					{
						EC11_Value = cfg_Steering_Middle_Value_MIN;
						cfg_Steering_Middle_Value = cfg_Steering_Middle_Value_MIN; // 最小值
					}
					if(cfg_Steering_Middle_Value > cfg_Steering_Middle_Value_MAX)
					{
						EC11_Value = cfg_Steering_Middle_Value_MAX;
						cfg_Steering_Middle_Value = cfg_Steering_Middle_Value_MAX; // 最大值
					}
				}
				
				// 有未保存的修改配置
				is_Config_Unsaved = 1;  
			}
			
			
			// 保存配置到EEPROM
			if(is_Config_Unsaved == 1){
				is_Config_Unsaved = 0;
				
				// 需要先擦除再写
				IapErase(0x0000); 		
				IapProgram(0x0000, cfg_Steering_Reverse);
				IapProgram(0x0001, cfg_Steering_Reduced_Angle);
				IapProgram(0x0002, cfg_Steering_Middle_Value);
				IapProgram(0x0003, cfg_Direction_Reverse);
			}
			
			
			// 闪烁的方式显示配置信息
			if(isScreenBlack == 0 && blinkMsCounter > 300)
			{
				isScreenBlack = 1;
				blinkMsCounter = 0;
				
				// 显示舵机两边减少角度
				tm1637Values[0] = cfg_Steering_Reduced_Angle / 10; // 显示十位数
				tm1637Values[1] = cfg_Steering_Reduced_Angle % 10; // 显示个位数
				
				// 显示舵机中值
				tm1637Values[2] = cfg_Steering_Middle_Value  / 10; 
				tm1637Values[3] = cfg_Steering_Middle_Value  % 10;
						
				// 显示舵机反向设置
				tm1637Values[4] = (cfg_Steering_Reverse == 1 ? 1 : 0); // 1表示舵机反向，显示为中间的冒号，0是不显示冒号
				
				TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
			}
			if(isScreenBlack == 1 && blinkMsCounter > 80)
			{
				isScreenBlack = 0;
				blinkMsCounter = 0;
				
				tm1637Values[0] = 21; // 21是黑屏不显示
				tm1637Values[1] = 21; // 21是黑屏不显示
				tm1637Values[2] = 21; // 21是黑屏不显示
				tm1637Values[3] = 21; // 21是黑屏不显示
				tm1637Values[4] = 0;  // 不显示中间的冒号
			
				TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
			}
		
			
			// ****** 喇叭按钮 退出配置模式（需要消抖处理）, 
			if(BTN_BUZZER_IO == SW_ON){
				delay_ms(5); // 消抖
				if( BTN_BUZZER_IO == SW_ON){
					isBtn3_Pressed = 1;
				}
			}else{ 
				// 在按键松开时执行逻辑
				if(isBtn3_Pressed == 1){
					config_Mode_Flag = 0; // 退出配置模式
					
					// 刚退出配置模式，速度清零
					// 这段代码放到配置模式最后才行，放前面，可能会影响EC11_Value的值
					EC11_Value = 0;
					motorSpeed = 0;
		
				}
				isBtn3_Pressed = 0;
			}
		}
		
		
		
		// ======================================================= 正常模式
		else
		{
		
			// ***************** 处理行走马达的前进后退的方向（钮子开关，需要消抖处理） ************************
			// 
			if(motorDirection == 0)
			{
				if(SW_MOTOR_DIR_IO == SW_ON)
				{
					delay_ms(5); // 消抖
					if( SW_MOTOR_DIR_IO == SW_ON)
					{ 
						motorDirection = 1; // 切换行走马达的方向
					}
				}
			}
			else if(motorDirection == 1)
			{
				if(SW_MOTOR_DIR_IO == SW_OFF){
					delay_ms(5); // 消抖
					if( SW_MOTOR_DIR_IO == SW_OFF){ 
						motorDirection = 0; // 切换行走马达的方向
					}
				}
			}
		
			// ***************** 处理行走马达的前进后退的油门大小（旋转编码器的处理逻辑放到了定时器的中断函数中，否则主循环中有delay会影响编码器的处理） ************************
			// 
	
			// 旋转编码器控制速度
			motorSpeed = EC11_Value;	// 行走马达的速度值
			if(motorSpeed > 99) motorSpeed = 99;  // 最大值99，100不好显示

			// ***************** 处理刹车逻辑（按下刹车，速度置为0，需要消抖处理） ************************
			
			if((EC11_SW == SW_ON) || (BTN_MOTOR_BRAKE_IO == SW_ON))
			{
				// 编码器的按钮是刹车并速度置零
				if(EC11_SW == SW_ON){
					delay_ms(5); // 消抖
					if(EC11_SW == SW_ON){
						// 在按下时时执行逻辑, 速度置为0
						brakeValue = 1; // 是否刹车，1表示刹车
						EC11_Value = 0; 
						motorSpeed = 0;
					}
				}
				
				// 单独的刹车按钮是只刹车，不归零速度
				if(BTN_MOTOR_BRAKE_IO == SW_ON){
					delay_ms(5); // 消抖
					if(BTN_MOTOR_BRAKE_IO == SW_ON){ // 在按下时时执行逻辑
						brakeValue = 1; // 是否刹车，1表示刹车
					}
				}
			}
			else
			{ 
				brakeValue = 0;
			}
				
			
			// ***************** 获取灯光、喇叭的状态（按下执行，松开停止, 需要消抖） ************************
			//
			if(BTN_LIGHT_IO == SW_ON){
				delay_ms(5); // 消抖
				if(BTN_LIGHT_IO == SW_ON){ // 在按下时时执行逻辑
					lightValue = 1; // 灯光
				}
			}else{ 
				lightValue = 0;
			}
			
			
			if(BTN_BUZZER_IO == SW_ON){
				delay_ms(5); // 消抖
				if(BTN_BUZZER_IO == SW_ON){ // 在按下时时执行逻辑
					buzzerValue = 1; // 喇叭
				}
			}else{ 
				buzzerValue = 0;
			}
	
			
			
			// ***************** 获取转向舵机的数据（电位器，中值表示居中90度） ************************
			// 
			vr1_Value = Get_ADCResult(ADC_STEERING_IO); // 获取电位器的值	
			steering_ServoAngle = (u8)map(vr1_Value, 0, 1023, 0, 180);  // 把电位器的值映射为角度值 0~180
	
			
			
			// ***************** 获取电池电压数据 ************************
			// 

			
			// Fn按钮，屏幕显示电池电压值
			if(BTN_FN_IO == SW_ON){
				delay_ms(5); // 消抖
				if(BTN_FN_IO == SW_ON && voltShow_msCounter > 500) // 500ms切换一次
				{ 	
					voltShow_msCounter = 0;
					
					
					checkRxVolt = 1; // 按下显示电压的按键，检测接收器端的电池电压标志置1
					
					if(isShowRxVolt == 0) // 显示遥控器的电压，显示中间的冒号
					{	
						//读取8次数据
						voltCheckADCValue  = 0;
						for(i=0; i<8; i++)
						{
							// 使用内部参考电压，ADC的第15通道是专门测量内部1.19V参考信号源的通道。
							voltCheckADCValue += Get_ADCResult(15); 
						}
						voltCheckADCValue >>= 3;  	// 除以8取平均值
						volt = (1.178 * 1024 * 100) / (voltCheckADCValue + 1) ; // 使用内部参考电压，10位ADC最大1023
						
						if(volt / 1000 == 0)
						{ 
							tm1637Values[0] = 21; 
						}
						else
						{
							tm1637Values[0] = volt / 1000;	
						}
						volt = volt % 1000;
						tm1637Values[1] = volt / 100;	volt = volt % 100;
						tm1637Values[2] = volt / 10; 
						tm1637Values[3] = volt % 10;	
						tm1637Values[4] = 1; // 显示中间的冒号
					}
					else // 显示接收器端的电压，不显示中间的冒号
					{
						tm1637Values[0] = rx_volt_INT >= 10 ? (rx_volt_INT / 10) : 21; // rx_volt_INT 接收端电压值的 小数点前面的数，最大两位.
						tm1637Values[1] = rx_volt_INT % 10; 
						tm1637Values[2] = rx_volt_DEC / 10; // rx_volt_DEC接收端电压值的 小数点后面的数，最大两位
						tm1637Values[3] = rx_volt_DEC % 10;	
						tm1637Values[4] = 0; // 不显示中间的冒号
					}
					
					TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
					isShowVolt = 1;
					
					// printf("%u,%u", voltCheckValue, volt);
				
					isShowRxVolt = !isShowRxVolt; // 切换显示电压
				}
			}
			else
			{
				isShowVolt = 0;
				checkRxVolt = 0; 
				isShowRxVolt = 0; // 看电压时先显示本端电压，这样能留出时间获取RX端电压 
			}
			
		
			
			// 应该先处理舵机反向（放到后面计算的话，计算的中位左右偏移就会相反）
			if(cfg_Steering_Reverse == 1){
				steering_ServoAngle = 180 - steering_ServoAngle;
			}
			
			
			// ****** 处理舵机中值
			
			
			// 方向舵机的中值角度
			steering_middleAngle = cfg_Steering_Middle_Value * 2; //（因配置时只能显示2位数不够用，1单位表示舵机2度）
			
			
			// 处理时注意，u8取值范围时正数，不能出现负值，否则数据乱套
			if(steering_middleAngle != 90)
			{
				// 这样是防止出现小于0的值或大于255的值，数据乱套
				if(steering_middleAngle < 165) // 防止出现大于255的值（180 + 75 = 255， 默认中值90 + 偏移75 = 165 ）
				{
					// ！！ if(steering_ServoAngle > (90 - (cfg_Steering_Middle_Value * 2))) // 如果cfg_Steering_Middle_Value大于45，这个表达式中出现了负数，就会出问题
					if((int)steering_ServoAngle > (int)(90 - steering_middleAngle)) //转为int在参与计算，就没有错误了
					{
						// 根据中值修正角度，感觉应该按中值修正电位器值才对
						// 需要注意, 这里计算后可能会超过180（比如中位角度设定位最大值70，即140度，导致所有角度+50，原始角度超过130时这里就超180了
						steering_ServoAngle = steering_ServoAngle - (90 - steering_middleAngle);
						if(steering_ServoAngle > 180) { steering_ServoAngle = 180; }
					}
					else
					{
						steering_ServoAngle = 0; 
					}
				} 
				
				// ！！ if(steering_ServoAngle < 0)		{ steering_ServoAngle = 0;   } // 这样会出错，因为u8类型不能是负数
				// ！！ if(steering_ServoAngle > 180)	{ steering_ServoAngle = 180; } // 这样可能会出错，因为u8类型最大255
			}
			

			
			
			// 处理舵机中值的死区，（中值左右留出的旷量，旷量内都算中值）
			if(steering_ServoAngle >= (steering_middleAngle - steering_Deadband) && steering_ServoAngle <= (steering_middleAngle + steering_Deadband))
			{
				steering_ServoAngle = steering_middleAngle;
			}
			else if(steering_ServoAngle < (steering_middleAngle - steering_Deadband)) // 左转超过死区角度
			{
				steering_ServoAngle = map(steering_ServoAngle, 0, (steering_middleAngle - steering_Deadband), 0, steering_middleAngle); 
			}
			else if(steering_ServoAngle > (steering_middleAngle + steering_Deadband))
			{
				steering_ServoAngle = map(steering_ServoAngle, (steering_middleAngle + steering_Deadband), 180, steering_middleAngle, 180);// 不转到最大角度，留了10度
			}
		
			
			
			// 处理左右减少的角度（不管其他配置，只按照180度的情况减少角度）
			if(steering_ServoAngle < cfg_Steering_Reduced_Angle)
			{
				steering_ServoAngle = cfg_Steering_Reduced_Angle;
			}
			else if(steering_ServoAngle > (180 - cfg_Steering_Reduced_Angle))
			{
				steering_ServoAngle = (180 - cfg_Steering_Reduced_Angle);
			}
			
		
			
			
			// 无线发送数据，准备数据
			tx_buf[0] = NRF24L01_DATA_TX_BIZ; // 第1位是约定的标志
			tx_buf[1] = (cfg_Direction_Reverse == 1) ? !motorDirection : motorDirection; 	// 电机方向, 需要处理是否反向
			tx_buf[2] = motorSpeed;			// 电机速度，由电位器控制
			tx_buf[3] = brakeValue;			// 是否刹车
			tx_buf[4] = steering_ServoAngle;	// 转向舵机的角度
			tx_buf[5] = lightValue; 		// 灯光，钮子开关
			tx_buf[6] = buzzerValue;		// 喇叭，自复位按钮
		//	tx_buf[7] = mosfetPwmDutyCycle;		// 控制MOS管模块PWM占空比的数据
			tx_buf[8] = checkRxVolt; 		// 是否检测接收器端的电池电压，1=检测

			
			
			if(isShowVolt == 0){
			
				// 显示速度和方向（第1位是方向，第3、4位是速度）
				if(brakeValue == 1){ // 刹车中显示横线
					tm1637Values[0] = 22; // 刹车显示横线（22是横线）
					tm1637Values[1] = 21; // 21是黑屏不显示
					
					if(motorSpeed == 0)
					{
						tm1637Values[2] = 22; // 22是横线
						tm1637Values[3] = 22; // 22是横线
					}
					else 
					{
						tm1637Values[2] = motorSpeed / 10; // 最大值100时显示A0
						tm1637Values[3] = motorSpeed % 10;		
					}
				}
				else{	// 未刹车时，正常显示
					tm1637Values[0] = (motorDirection ? 24 : 23); // 不刹车：0是前进显示上箭头，1是后退显示下箭头
					tm1637Values[1] = 21; // 21是黑屏不显示
					
					tm1637Values[2] = motorSpeed / 10;
					tm1637Values[3] = motorSpeed % 10;			
				}
			
		
				TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]); // 先显示一次，否则在发送数据时可能阻塞导致看不到显示
			}
			

			// 超时就清空ack_buf的内容。没有每次直接清空，是想在间隔时间内只有有一次成功回传就能连贯显示，因为有可能刚好有一次未收到ACK_PAYLOAD就会显示0..
			if(tx_timeout_msCounter > 500)
			{
				for(i=0;i<8;i++) ack_buf[i] = 0; // 清空ack_buf
			}
			
			
			// 无线发送数据，发送数据
			txResult = TxPacket_ACKPAYLOAD(tx_buf, ack_buf); // 发送数据, 并接收ACKPAYLOAD
			// ack_payload_width = Ack_Payload_Width();		// 获取ACKPAYLOAD的负载数据长度
			
			
			// 经测试，有时回传的数据第一个字节可能时0xFF，不是正确的数据，所以回传的第一个数据约定一个值，不是的话就表示回传错误
			rx_volt_INT = 0; // 电压的正数部分，最大2位
			rx_volt_DEC = 0; // 电压的小数部分，最大2位
			if(ack_buf[0] == NRF24L01_DATA_RX_ACK_BIZ) // 第一个字节不是约定的内容，就是无效数据
			{
				rx_volt_INT = ack_buf[1]; // 接收端电压值的 小数点前面的数，最大两位
				rx_volt_DEC = ack_buf[2]; // 接收端电压值的 小数点后面的数，最大两位
			}
			
			// 调试信息
			// printf("%c%c\n%c%c%c%c%c%c%c%c\n\n", txResult, ack_payload_width, ack_buf[0], ack_buf[1], ack_buf[2], ack_buf[3], ack_buf[4], ack_buf[5], ack_buf[6], ack_buf[7]);
			// delay_ms(200);

			
			// 距离远点或隔着墙时，state是0x1E，但是接收端能收到数据，估计是收到了数据但返回ack时不成功，TX端才有0x10状态（最大重发次数）
			if(txResult & 0x20){ 
				tx_timeout_msCounter = 0; // 上一次成功发射的时间计数清零
				tm1637Values[4] = 0; // 发送成功，不显示中间的冒号
			}else{
				tm1637Values[4] = 1; // 发送失败，显示中间的冒号
			} 
			
			
			// 显示
			if(isShowVolt == 0){
				TM1637_Display(tm1637Values[0],tm1637Values[1],tm1637Values[2],tm1637Values[3],tm1637Values[4]);
			}
			
			// 每隔一段时间发送一次数据
			delay_ms(50);
			
		}
	}

}

