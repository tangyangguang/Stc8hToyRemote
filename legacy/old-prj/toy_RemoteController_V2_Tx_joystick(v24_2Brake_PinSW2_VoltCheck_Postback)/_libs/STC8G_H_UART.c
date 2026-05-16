/*---------------------------------------------------------------------*/
/* --- STC MCU Limited ------------------------------------------------*/
/* --- STC 1T Series MCU Demo Programme -------------------------------*/
/* --- Mobile: (86)13922805190 ----------------------------------------*/
/* --- Fax: 86-0513-55012956,55012947,55012969 ------------------------*/
/* --- Tel: 86-0513-55012928,55012929,55012966 ------------------------*/
/* --- Web: www.STCAI.com ---------------------------------------------*/
/* --- BBS: www.STCAIMCU.com  -----------------------------------------*/
/* --- QQ:  800003751 -------------------------------------------------*/
/* 如果要在程序中使用此代码,请在程序中注明使用了STC的资料及程序            */
/*---------------------------------------------------------------------*/

#include "STC8G_H_UART.h"

//========================================================================
//                               本地变量声明
//========================================================================

#ifdef UART1
COMx_Define	COM1;
u8	xdata TX1_Buffer[COM_TX1_Lenth];	//发送缓冲
u8 	xdata RX1_Buffer[COM_RX1_Lenth];	//接收缓冲
#endif

//========================================================================
// 函数: UART_Configuration
// 描述: UART初始化程序.
// 参数: UARTx: UART组号, COMx结构参数,请参考UART.h里的定义.
// 返回: none.
// 版本: V1.0, 2012-10-22
//========================================================================
u8 UART_Configuration(u8 UARTx, COMx_InitDefine *COMx)
{
#if  defined( UART1 ) || defined( UART2 ) || defined( UART3 ) || defined( UART4 )
	u16	i;
	u32	j;
#else
	UARTx = NULL;
	COMx = NULL;
#endif

#ifdef UART1
	if(UARTx == UART1)
	{
		COM1.TX_send    = 0;
		COM1.TX_write   = 0;
		COM1.B_TX_busy  = 0;
		COM1.RX_Cnt     = 0;
		COM1.RX_TimeOut = 0;
		
		for(i=0; i<COM_TX1_Lenth; i++)	TX1_Buffer[i] = 0;
		for(i=0; i<COM_RX1_Lenth; i++)	RX1_Buffer[i] = 0;

		SCON = (SCON & 0x3f) | COMx->UART_Mode;	//模式设置
		if((COMx->UART_Mode == UART_9bit_BRTx) || (COMx->UART_Mode == UART_8bit_BRTx))	//可变波特率
		{
			j = (MAIN_Fosc / 4) / COMx->UART_BaudRate;	//按1T计算
			if(j >= 65536UL)	return FAIL;	//错误
			j = 65536UL - j;
			if(COMx->UART_BRT_Use == BRT_Timer1)
			{
				TR1 = 0;
				AUXR &= ~0x01;		//S1 BRT Use Timer1;
				TMOD &= ~(1<<6);	//Timer1 set As Timer
				TMOD &= ~0x30;		//Timer1_16bitAutoReload;
				AUXR |=  (1<<6);	//Timer1 set as 1T mode
				TH1 = (u8)(j>>8);
				TL1 = (u8)j;
				ET1 = 0;	//禁止中断
				TMOD &= ~0x40;	//定时
				INTCLKO &= ~0x02;	//不输出时钟
				TR1  = 1;
			}
			else if(COMx->UART_BRT_Use == BRT_Timer2)
			{
				AUXR &= ~(1<<4);	//Timer stop
				AUXR |= 0x01;		//S1 BRT Use Timer2;
				AUXR &= ~(1<<3);	//Timer2 set As Timer
				AUXR |=  (1<<2);	//Timer2 set as 1T mode
				T2H = (u8)(j>>8);
				T2L = (u8)j;
				IE2  &= ~(1<<2);	//禁止中断
				AUXR |=  (1<<4);	//Timer run enable
			}
			else return FAIL;	//错误
		}
		else if(COMx->UART_Mode == UART_ShiftRight)
		{
			if(COMx->BaudRateDouble == ENABLE)	AUXR |=  (1<<5);	//固定波特率SysClk/2
			else								AUXR &= ~(1<<5);	//固定波特率SysClk/12
		}
		else if(COMx->UART_Mode == UART_9bit)	//固定波特率SysClk*2^SMOD/64
		{
			if(COMx->BaudRateDouble == ENABLE)	PCON |=  (1<<7);	//固定波特率SysClk/32
			else								PCON &= ~(1<<7);	//固定波特率SysClk/64
		}
		UART1_RxEnable(COMx->UART_RxEnable);	//UART接收使能

		return SUCCESS;
	}
#endif
	return FAIL;	//错误
}

/*********************************************************/

/********************* UART1 函数 ************************/
#ifdef UART1
void TX1_write2buff(u8 dat)	//串口1发送函数
{
    #if(UART_QUEUE_MODE == 1)
	TX1_Buffer[COM1.TX_write] = dat;	//装发送缓冲，使用队列式数据发送，一次性发送数据长度不要超过缓冲区大小（COM_TXn_Lenth）
	if(++COM1.TX_write >= COM_TX1_Lenth)	COM1.TX_write = 0;

	if(COM1.B_TX_busy == 0)		//空闲
	{  
		COM1.B_TX_busy = 1;		//标志忙
		TI = 1;					//触发发送中断
	}
    #else
    //以下是阻塞方式发送方法
	SBUF = dat;
	COM1.B_TX_busy = 1;		//标志忙
	while(COM1.B_TX_busy);
    #endif
}

void PrintString1(u8 *puts)
{
    for (; *puts != 0;	puts++)  TX1_write2buff(*puts); 	//遇到停止符0结束
}

#endif

/********************* Printf 函数 ************************/
#if(PRINTF_SELECT == 1)

char putchar(char c)
{
	TX1_write2buff(c);
	return c;
}


#endif
