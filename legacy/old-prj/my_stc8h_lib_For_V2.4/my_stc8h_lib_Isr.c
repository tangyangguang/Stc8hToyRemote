


#include "STC8H.h"
#include "my_stc8h_lib.h"


//========================================================================
//                              UART1 中断处理函数
//========================================================================

// 串口1中断处理函数，必须有才能正常串口通信
void UART1_ISR_Handler (void) interrupt UART1_VECTOR
{
	if(RI)
	{
		RI = 0;

        if(COM1.RX_Cnt >= COM_RX1_Lenth)	COM1.RX_Cnt = 0;
        RX1_Buffer[COM1.RX_Cnt++] = SBUF;
        COM1.RX_TimeOut = TimeOutSet1;
	}

	if(TI)
	{
		TI = 0;
        COM1.B_TX_busy = 0;     //使用阻塞方式发送直接清除繁忙标志
    }
}



//========================================================================
//                              硬件SPI 中断处理函数
//========================================================================

void SPI_ISR_Handler() interrupt SPI_VECTOR
{
	B_SPI_Busy = 0;			// 清0 自定义的忙忙标志
	SPSTAT = 0xC0;			// 清0 SPIF和WCOL标志 (当发送/接收一个字节后，触发SPI的中断，需要软件方式写1进行清零)
}
