#ifndef	__MY_STC8H_LIB_H
#define	__MY_STC8H_LIB_H


#endif




//========================================================================
//                               主时钟定义
//========================================================================
#ifndef		__CONFIG_H
#define		__CONFIG_H

#define MAIN_Fosc		12000000L	//定义主时钟

#endif
//===================================================




//========================================================================
//                               类型定义(Type_def.h)
//========================================================================
#ifndef		__TYPE_DEF_H
#define		__TYPE_DEF_H

typedef unsigned char   u8;     //  8 bits 
typedef unsigned int    u16;    // 16 bits 
typedef unsigned long   u32;    // 32 bits 

typedef signed char     int8;   //  8 bits 
typedef signed int      int16;  // 16 bits 
typedef signed long     int32;  // 32 bits 

typedef unsigned char   uint8;  //  8 bits 
typedef unsigned int    uint16; // 16 bits 
typedef unsigned long   uint32; // 32 bits 

#define	TRUE	1
#define	FALSE	0

#define ENABLE		1
#define DISABLE		0

#define SUCCESS		0
#define FAIL		-1

#define	PIE			0x20	//1: 比较结果由0变1, 产生上升沿中断
#define	NIE			0x10	//1: 比较结果由1变0, 产生下降沿中断

#define	PWMA	128
#define	PWMB	129

#endif
//===================================================




//========================================================================
//                            功能脚切换设置
//========================================================================

#ifndef	__STC8G_H_SWITCH_H
#define	__STC8G_H_SWITCH_H

#define  PWM1_SW(Pin)				PWMA_PS = (PWMA_PS & 0xFC) | (Pin)
#define  PWM2_SW(Pin)				PWMA_PS = (PWMA_PS & 0xF3) | (Pin << 2)
#define  PWM3_SW(Pin)				PWMA_PS = (PWMA_PS & 0xCF) | (Pin << 4)
#define  PWM4_SW(Pin)				PWMA_PS = (PWMA_PS & 0x3F) | (Pin << 6)

#define  PWM5_SW(Pin)				PWMB_PS = (PWMB_PS & 0xFC) | (Pin)
#define  PWM6_SW(Pin)				PWMB_PS = (PWMB_PS & 0xF3) | (Pin << 2)
#define  PWM7_SW(Pin)				PWMB_PS = (PWMB_PS & 0xCF) | (Pin << 4)
#define  PWM8_SW(Pin)				PWMB_PS = (PWMB_PS & 0x3F) | (Pin << 6)



#define	PWM1_SW_P10_P11		0
#define	PWM1_SW_P20_P21		1
#define	PWM1_SW_P60_P61		2

#define	PWM2_SW_P12_P13		0
#define	PWM2_SW_P22_P23		1
#define	PWM2_SW_P62_P63		2

#define	PWM3_SW_P14_P15		0
#define	PWM3_SW_P24_P25		1
#define	PWM3_SW_P64_P65		2

#define	PWM4_SW_P16_P17		0
#define	PWM4_SW_P26_P27		1
#define	PWM4_SW_P66_P67		2
#define	PWM4_SW_P34_P33		3

#define	PWM5_SW_P20				0
#define	PWM5_SW_P17				1
#define	PWM5_SW_P00				2
#define	PWM5_SW_P74				3

#define	PWM6_SW_P21				0
#define	PWM6_SW_P54				1
#define	PWM6_SW_P01				2
#define	PWM6_SW_P75				3

#define	PWM7_SW_P22				0
#define	PWM7_SW_P33				1
#define	PWM7_SW_P02				2
#define	PWM7_SW_P76				3

#define	PWM8_SW_P23				0
#define	PWM8_SW_P34				1
#define	PWM8_SW_P03				2
#define	PWM8_SW_P77				3




#endif
//===================================================




//========================================================================
//                              UART设置
//========================================================================
#ifndef __STC8G_H_UART_H
#define __STC8G_H_UART_H	 


#define	TimeOutSet1		5
#define	COM_TX1_Lenth	128
#define	COM_RX1_Lenth	128

typedef struct
{ 
	u8	TX_send;		//已发送指针
	u8	TX_write;		//发送写指针
	u8	B_TX_busy;		//忙标志

	u8 	RX_Cnt;			//接收字节计数
	u8	RX_TimeOut;		//接收超时
} COMx_Define; 


extern	COMx_Define	COM1;
extern	u8	xdata TX1_Buffer[COM_TX1_Lenth];	//发送缓冲
extern	u8 	xdata RX1_Buffer[COM_RX1_Lenth];	//接收缓冲

void TX1_write2buff(u8 dat);	//串口1发送函数
void PrintString1(u8 *puts);


#endif
//===================================================







//========================================================================
//                              硬件SPI
//========================================================================
 
#ifndef	__STC8G_H_SPI_H
#define	__STC8G_H_SPI_H

#define		SPI_SSIG_Set(n)				SPCTL = (SPCTL & ~0x80) | (n << 7)		/* SS引脚功能控制 */
#define		SPI_Start(n)				SPCTL = (SPCTL & ~0x40) | (n << 6)		/* SPI使能控制位 */
#define		SPI_FirstBit_Set(n)			SPCTL = (SPCTL & ~0x20) | (n << 5)		/* 数据发送/接收顺序 MSB/LSB */
#define		SPI_Mode_Set(n)				SPCTL = (SPCTL & ~0x10) | (n << 4)		/* SPI主从模式设置 */
#define		SPI_CPOL_Set(n)				SPCTL = (SPCTL & ~0x08) | (n << 3)		/* SPI时钟极性控制 */
#define		SPI_CPHA_Set(n)				SPCTL = (SPCTL & ~0x04) | (n << 2)		/* SPI时钟相位控制 */
#define		SPI_Clock_Select(n)			SPCTL = (SPCTL & ~0x03) | (n)			/* SPI时钟频率选择 */


extern bit B_SPI_Busy; //发送忙标志


#endif
//===================================================




//========================================================================
//                              EEPROM 操作 @12MHz
//========================================================================

#ifndef	__STC8G_H_EEPROM_H
#define	__STC8G_H_EEPROM_H

void IapIdle();
char IapRead(int addr);
void IapProgram(int addr, char dat);
void IapErase(int addr);

#endif
//===================================================



//========================================================================
//                               PWM设置
//========================================================================

#ifndef __STC8H_PWM_H
#define __STC8H_PWM_H	 

#define PWMA_CCER1_Disable()			PWMA_CCER1 = 0x00			//关闭所有输入捕获/比较输出
#define PWMA_CC1E_Enable()				PWMA_CCER1 |= 0x01		//1：开启输入捕获/比较输出
#define PWMA_CC1E_Disable()				PWMA_CCER1 &= ~0x01		//0：关闭输入捕获/比较输出
#define PWMA_CC1P_LowValid()			PWMA_CCER1 |= 0x02		//1：低电平有效
#define PWMA_CC1P_HighValid()			PWMA_CCER1 &= ~0x02		//0：高电平有效
#define PWMA_CC1P_CaptureRise()		PWMA_CCER1 |= 0x02		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMA_CC1P_CaptureFall()		PWMA_CCER1 &= ~0x02		//0：捕获发生在 TI1F 或 TI2F 的上升沿
#define PWMA_CC1NE_Enable()				PWMA_CCER1 |= 0x04		//1：开启比较输出
#define PWMA_CC1NE_Disable()			PWMA_CCER1 &= ~0x04		//0：关闭比较输出
#define PWMA_CC1NP_LowValid()			PWMA_CCER1 |= 0x08		//1：低电平有效
#define PWMA_CC1NP_HighValid()		PWMA_CCER1 &= ~0x08		//0：高电平有效

#define PWMA_CC2E_Enable()				PWMA_CCER1 |= 0x10		//1：开启输入捕获/比较输出
#define PWMA_CC2E_Disable()				PWMA_CCER1 &= ~0x10		//0：关闭输入捕获/比较输出
#define PWMA_CC2P_LowValid()			PWMA_CCER1 |= 0x20		//1：低电平有效
#define PWMA_CC2P_HighValid()			PWMA_CCER1 &= ~0x20		//0：高电平有效
#define PWMA_CC2P_CaptureRise()		PWMA_CCER1 |= 0x20		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMA_CC2P_CaptureFall()		PWMA_CCER1 &= ~0x20		//0：捕获发生在 TI1F 或 TI2F 的上升沿
#define PWMA_CC2NE_Enable()				PWMA_CCER1 |= 0x40		//1：开启比较输出
#define PWMA_CC2NE_Disable()			PWMA_CCER1 &= ~0x40		//0：关闭比较输出
#define PWMA_CC2NP_LowValid()			PWMA_CCER1 |= 0x80		//1：低电平有效
#define PWMA_CC2NP_HighValid()		PWMA_CCER1 &= ~0x80		//0：高电平有效

#define PWMB_CCER1_Disable()			PWMB_CCER1 = 0x00			//关闭所有输入捕获/比较输出
#define PWMB_CC5E_Enable()				PWMB_CCER1 |= 0x01		//1：开启输入捕获/比较输出
#define PWMB_CC5E_Disable()				PWMB_CCER1 &= ~0x01		//0：关闭输入捕获/比较输出
#define PWMB_CC5P_LowValid()			PWMB_CCER1 |= 0x02		//1：低电平有效
#define PWMB_CC5P_HighValid()			PWMB_CCER1 &= ~0x02		//0：高电平有效
#define PWMB_CC5P_CaptureRise()		PWMB_CCER1 |= 0x02		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMB_CC5P_CaptureFall()		PWMB_CCER1 &= ~0x02		//0：捕获发生在 TI1F 或 TI2F 的上升沿

#define PWMB_CC6E_Enable()				PWMB_CCER1 |= 0x10		//1：开启输入捕获/比较输出
#define PWMB_CC6E_Disable()				PWMB_CCER1 &= ~0x10		//0：关闭输入捕获/比较输出
#define PWMB_CC6P_LowValid()			PWMB_CCER1 |= 0x20		//1：低电平有效
#define PWMB_CC6P_HighValid()			PWMB_CCER1 &= ~0x20		//0：高电平有效
#define PWMB_CC6P_CaptureRise()		PWMB_CCER1 |= 0x20		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMB_CC6P_CaptureFall()		PWMB_CCER1 &= ~0x20		//0：捕获发生在 TI1F 或 TI2F 的上升沿

//						                 7     6     5    4    3     2    1    0    Reset Value
//sfr PWMA_CCER2 = 0xFECDH;  CC4NP CC4NE CC4P CC4E CC3NP CC3NE CC3P CC3E  0000,0000  /* 捕获/比较使能寄存器 2 */ 
//sfr PWMB_CCER2 = 0xFEEDH;    -     -   CC8P CC8E   -     -   CC7P CC7E  0000,0000  /* 捕获/比较使能寄存器 2 */ 

#define PWMA_CCER2_Disable()			PWMA_CCER2 = 0x00			//关闭所有输入捕获/比较输出
#define PWMA_CC3E_Enable()				PWMA_CCER2 |= 0x01		//1：开启输入捕获/比较输出
#define PWMA_CC3E_Disable()				PWMA_CCER2 &= ~0x01		//0：关闭输入捕获/比较输出
#define PWMA_CC3P_LowValid()			PWMA_CCER2 |= 0x02		//1：低电平有效
#define PWMA_CC3P_HighValid()			PWMA_CCER2 &= ~0x02		//0：高电平有效
#define PWMA_CC3P_CaptureRise()		PWMA_CCER2 |= 0x02		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMA_CC3P_CaptureFall()		PWMA_CCER2 &= ~0x02		//0：捕获发生在 TI1F 或 TI2F 的上升沿
#define PWMA_CC3NE_Enable()				PWMA_CCER2 |= 0x04		//1：开启比较输出
#define PWMA_CC3NE_Disable()			PWMA_CCER2 &= ~0x04		//0：关闭比较输出
#define PWMA_CC3NP_LowValid()			PWMA_CCER2 |= 0x08		//1：低电平有效
#define PWMA_CC3NP_HighValid()		PWMA_CCER2 &= ~0x08		//0：高电平有效

#define PWMA_CC4E_Enable()				PWMA_CCER2 |= 0x10		//1：开启输入捕获/比较输出
#define PWMA_CC4E_Disable()				PWMA_CCER2 &= ~0x10		//0：关闭输入捕获/比较输出
#define PWMA_CC4P_LowValid()			PWMA_CCER2 |= 0x20		//1：低电平有效
#define PWMA_CC4P_HighValid()			PWMA_CCER2 &= ~0x20		//0：高电平有效
#define PWMA_CC4P_CaptureRise()		PWMA_CCER2 |= 0x20		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMA_CC4P_CaptureFall()		PWMA_CCER2 &= ~0x20		//0：捕获发生在 TI1F 或 TI2F 的上升沿
#define PWMA_CC4NE_Enable()				PWMA_CCER2 |= 0x40		//1：开启比较输出
#define PWMA_CC4NE_Disable()			PWMA_CCER2 &= ~0x40		//0：关闭比较输出
#define PWMA_CC4NP_LowValid()			PWMA_CCER2 |= 0x80		//1：低电平有效
#define PWMA_CC4NP_HighValid()		PWMA_CCER2 &= ~0x80		//0：高电平有效

#define PWMB_CCER2_Disable()			PWMB_CCER2 = 0x00			//关闭所有输入捕获/比较输出
#define PWMB_CC7E_Enable()				PWMB_CCER2 |= 0x01		//1：开启输入捕获/比较输出
#define PWMB_CC7E_Disable()				PWMB_CCER2 &= ~0x01		//0：关闭输入捕获/比较输出
#define PWMB_CC7P_LowValid()			PWMB_CCER2 |= 0x02		//1：低电平有效
#define PWMB_CC7P_HighValid()			PWMB_CCER2 &= ~0x02		//0：高电平有效
#define PWMB_CC7P_CaptureRise()		PWMB_CCER2 |= 0x02		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMB_CC7P_CaptureFall()		PWMB_CCER2 &= ~0x02		//0：捕获发生在 TI1F 或 TI2F 的上升沿

#define PWMB_CC8E_Enable()				PWMB_CCER2 |= 0x10		//1：开启输入捕获/比较输出
#define PWMB_CC8E_Disable()				PWMB_CCER2 &= ~0x10		//0：关闭输入捕获/比较输出
#define PWMB_CC8P_LowValid()			PWMB_CCER2 |= 0x20		//1：低电平有效
#define PWMB_CC8P_HighValid()			PWMB_CCER2 &= ~0x20		//0：高电平有效
#define PWMB_CC8P_CaptureRise()		PWMB_CCER2 |= 0x20		//1：捕获发生在 TI1F 或 TI2F 的下降沿
#define PWMB_CC8P_CaptureFall()		PWMB_CCER2 &= ~0x20		//0：捕获发生在 TI1F 或 TI2F 的上升沿


#define PWMA_CC1S_Direction(n)		PWMA_CCMR1 = (PWMA_CCMR1 & ~0x03) | (n)		//捕获/比较 1 选择。这两位定义通道的方向（输入/输出），及输入脚的选择
#define PWMB_CC5S_Direction(n)		PWMB_CCMR1 = (PWMB_CCMR1 & ~0x03) | (n)		//捕获/比较 5 选择。这两位定义通道的方向（输入/输出），及输入脚的选择



//						                 7     6     5    4    3     2    1    0    Reset Value
//sfr PWMA_CCR1H = 0xFED5H;                   CCR1[15:8]                  0000,0000  /* 捕获/比较寄存器 1 高 8 位 */ 
//sfr PWMB_CCR5H = 0xFEF5H;                   CCR5[15:8]                  0000,0000  /* 捕获/比较寄存器 1 高 8 位 */ 
//sfr PWMA_CCR1L = 0xFED6H;                   CCR1[7:0]                   0000,0000  /* 捕获/比较寄存器 1 低 8 位 */ 
//sfr PWMB_CCR5L = 0xFEF6H;                   CCR5[7:0]                   0000,0000  /* 捕获/比较寄存器 1 低 8 位 */ 

#define PWMA_Duty1(n)							{PWMA_CCR1H = (n>>8); PWMA_CCR1L = (n);}		//捕获/比较寄存器 1 设置
#define PWMB_Duty5(n)							{PWMB_CCR5H = (n>>8); PWMB_CCR5L = (n);}		//捕获/比较寄存器 1 设置

//						                 7     6     5    4    3     2    1    0    Reset Value
//sfr PWMA_CCR2H = 0xFED7H;                   CCR2[15:8]                  0000,0000  /* 捕获/比较寄存器 2 高 8 位 */ 
//sfr PWMB_CCR6H = 0xFEF7H;                   CCR6[15:8]                  0000,0000  /* 捕获/比较寄存器 2 高 8 位 */ 
//sfr PWMA_CCR2L = 0xFED8H;                   CCR2[7:0]                   0000,0000  /* 捕获/比较寄存器 2 低 8 位 */ 
//sfr PWMB_CCR6L = 0xFEF8H;                   CCR6[7:0]                   0000,0000  /* 捕获/比较寄存器 2 低 8 位 */ 

#define PWMA_Duty2(n)							{PWMA_CCR2H = (n>>8); PWMA_CCR2L = (n);}		//捕获/比较寄存器 2 设置
#define PWMB_Duty6(n)							{PWMB_CCR6H = (n>>8); PWMB_CCR6L = (n);}		//捕获/比较寄存器 2 设置

//						                 7     6     5    4    3     2    1    0    Reset Value
//sfr PWMA_CCR3H = 0xFED9H;                   CCR3[15:8]                  0000,0000  /* 捕获/比较寄存器 3 高 8 位 */ 
//sfr PWMB_CCR7H = 0xFEF9H;                   CCR7[15:8]                  0000,0000  /* 捕获/比较寄存器 3 高 8 位 */ 
//sfr PWMA_CCR3L = 0xFEDAH;                   CCR3[7:0]                   0000,0000  /* 捕获/比较寄存器 3 低 8 位 */ 
//sfr PWMB_CCR7L = 0xFEFAH;                   CCR7[7:0]                   0000,0000  /* 捕获/比较寄存器 3 低 8 位 */ 

#define PWMA_Duty3(n)							{PWMA_CCR3H = (n>>8); PWMA_CCR3L = (n);}		//捕获/比较寄存器 3 设置
#define PWMB_Duty7(n)							{PWMB_CCR7H = (n>>8); PWMB_CCR7L = (n);}		//捕获/比较寄存器 3 设置

//						                 7     6     5    4    3     2    1    0    Reset Value
//sfr PWMA_CCR4H = 0xFEDBH;                   CCR4[15:8]                  0000,0000  /* 捕获/比较寄存器 4 高 8 位 */ 
//sfr PWMB_CCR8H = 0xFEFBH;                   CCR8[15:8]                  0000,0000  /* 捕获/比较寄存器 4 高 8 位 */ 
//sfr PWMA_CCR4L = 0xFEDCH;                   CCR4[7:0]                   0000,0000  /* 捕获/比较寄存器 4 低 8 位 */ 
//sfr PWMB_CCR8L = 0xFEFCH;                   CCR8[7:0]                   0000,0000  /* 捕获/比较寄存器 4 低 8 位 */ 

#define PWMA_Duty4(n)							{PWMA_CCR4H = (n>>8); PWMA_CCR4L = (n);}		//捕获/比较寄存器 4 设置
#define PWMB_Duty8(n)							{PWMB_CCR8H = (n>>8); PWMB_CCR8L = (n);}		//捕获/比较寄存器 4 设置


#define PWMA_DeadTime(n)					PWMA_DTR = n		//死区发生器设置
#define PWMB_DeadTime(n)					PWMB_DTR = n		//死区发生器设置

#define PWMA_AutoReload(n)				{PWMA_ARRH = (n>>8); PWMA_ARRL = (n);}		//自动重装载寄存器设置
#define PWMB_AutoReload(n)				{PWMB_ARRH = (n>>8); PWMB_ARRL = (n);}		//自动重装载寄存器设置


#define PWMA_BrakeOutputEnable(n)				(n==1?(PWMA_BKR |= 0x80):(PWMA_BKR &= ~0x80))		//1：主输出使能
#define PWMB_BrakeOutputEnable(n)				(n==1?(PWMB_BKR |= 0x80):(PWMB_BKR &= ~0x80))		//1：主输出使能
//#define PWMA_BrakeOutputEnable()				PWMA_BKR |= 0x80		//1：主输出使能
#define PWMA_BrakeOutputDisable()				PWMA_BKR &= ~0x80		//0：主输出禁止
//#define PWMB_BrakeOutputEnable()				PWMB_BKR |= 0x80		//1：主输出使能
#define PWMB_BrakeOutputDisable()				PWMB_BKR &= ~0x80		//0：主输出禁止



#define PWMA_CEN_Disable()				PWMA_CR1 &= ~0x01		//0：禁止计数器
#define PWMA_CEN_Enable(n)				(n==1?(PWMA_CR1 |= 0x01):(PWMA_CR1 &= ~0x01))		//1：使能计数器

#define PWMB_CEN_Disable()				PWMB_CR1 &= ~0x01		//0：禁止计数器
#define PWMB_CEN_Enable(n)				(n==1?(PWMB_CR1 |= 0x01):(PWMB_CR1 &= ~0x01))		//1：使能计数器




#define PWMA_OC1ModeSet(n)				PWMA_CCMR1 = (PWMA_CCMR1 & ~0x70) | (n)		//输出比较模式设置
#define PWMB_OC5ModeSet(n)				PWMB_CCMR1 = (PWMB_CCMR1 & ~0x70) | (n)		//输出比较模式设置

#define PWMA_OC2ModeSet(n)				PWMA_CCMR2 = (PWMA_CCMR2 & ~0x70) | (n)		//输出比较模式设置
#define PWMB_OC6ModeSet(n)				PWMB_CCMR2 = (PWMB_CCMR2 & ~0x70) | (n)		//输出比较模式设置

#define PWMA_OC3ModeSet(n)				PWMA_CCMR3 = (PWMA_CCMR3 & ~0x70) | (n)		//输出比较模式设置
#define PWMB_OC7ModeSet(n)				PWMB_CCMR3 = (PWMB_CCMR3 & ~0x70) | (n)		//输出比较模式设置

#define PWMA_OC4ModeSet(n)				PWMA_CCMR4 = (PWMA_CCMR4 & ~0x70) | (n)		//输出比较模式设置
#define PWMB_OC8ModeSet(n)				PWMB_CCMR4 = (PWMB_CCMR4 & ~0x70) | (n)		//输出比较模式设置




#define CCMRn_FREEZE							0x00		//冻结
#define CCMRn_MATCH_VALID					0x10		//匹配时设置通道 n 的输出为有效电平
#define CCMRn_MATCH_INVALID				0x20		//匹配时设置通道 n 的输出为无效电平
#define CCMRn_ROLLOVER						0x30		//翻转
#define CCMRn_FORCE_INVALID				0x40		//强制为无效电平
#define CCMRn_FORCE_VALID					0x50		//强制为有效电平
#define CCMRn_PWM_MODE1						0x60		//PWM 模式 1
#define CCMRn_PWM_MODE2						0x70		//PWM 模式 2



#define ENO1P       0x01
#define ENO1N       0x02
#define ENO2P       0x04
#define ENO2N       0x08
#define ENO3P       0x10
#define ENO3N       0x20
#define ENO4P       0x40
#define ENO4N       0x80

#define ENO5P       0x01
#define ENO6P       0x04
#define ENO7P       0x10
#define ENO8P       0x40

#define	PWM1	1
#define	PWM2	2
#define	PWM3	3
#define	PWM4	4
#define	PWM5	5
#define	PWM6	6
#define	PWM7	7
#define	PWM8	8



typedef struct
{ 
	u16	PWM1_Duty;			//PWM1占空比时间, 0~Period
	u16	PWM2_Duty;			//PWM2占空比时间, 0~Period
	u16	PWM3_Duty;			//PWM3占空比时间, 0~Period
	u16	PWM4_Duty;			//PWM4占空比时间, 0~Period
	u16	PWM5_Duty;			//PWM5占空比时间, 0~Period
	u16	PWM6_Duty;			//PWM6占空比时间, 0~Period
	u16	PWM7_Duty;			//PWM7占空比时间, 0~Period
	u16	PWM8_Duty;			//PWM8占空比时间, 0~Period
} PWMx_Duty; 



#endif
//===================================================



//========================================================================
//                              工具函数 @12MHz
//========================================================================

#ifndef	__STC8G_H_UTILS_H
#define	__STC8G_H_UTILS_H


void _delay_us(unsigned int _us);
void delay_ms(unsigned int ms);
long map(long x, long in_min, long in_max, long out_min, long out_max);

#endif
//===================================================

