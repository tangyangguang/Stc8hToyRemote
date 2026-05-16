
#include "STC8H.h"
#include <intrins.h>
#include <stdio.h> 
#include "my_stc8h_lib.h"



//========================================================================
//                              UART1设置
//========================================================================

COMx_Define	COM1;
u8	xdata TX1_Buffer[COM_TX1_Lenth];	//发送缓冲
u8 	xdata RX1_Buffer[COM_RX1_Lenth];	//接收缓冲


void TX1_write2buff(u8 dat)	//串口1发送函数
{ 
    //以下是阻塞方式发送方法
	SBUF = dat;
	COM1.B_TX_busy = 1;		//标志忙
	while(COM1.B_TX_busy);
}

void PrintString1(u8 *puts)
{
    for (; *puts != 0;	puts++)  TX1_write2buff(*puts); 	//遇到停止符0结束
}

char putchar(char c)
{
	TX1_write2buff(c);
	return c;
}



//========================================================================
//                              SPI设置
//========================================================================

bit B_SPI_Busy = 0; //发送忙标志



	 
//========================================================================
//                              EEPROM 操作 @12MHz
//========================================================================

void IapIdle()
{
    IAP_CONTR = 0;                              //关闭IAP功能
    IAP_CMD = 0;                                //清除命令寄存器
    IAP_TRIG = 0;                               //清除触发寄存器
    IAP_ADDRH = 0x80;                           //将地址设置到非IAP区域
    IAP_ADDRL = 0;
}

char IapRead(int addr)
{
    char dat;

    IAP_CONTR = 0x80;                           //使能IAP
    IAP_TPS = 12;                               //设置等待参数12MHz
    IAP_CMD = 1;                                //设置IAP读命令
    IAP_ADDRL = addr;                           //设置IAP低地址
    IAP_ADDRH = addr >> 8;                      //设置IAP高地址
    IAP_TRIG = 0x5a;                            //写触发命令(0x5a)
    IAP_TRIG = 0xa5;                            //写触发命令(0xa5)
    _nop_();
    dat = IAP_DATA;                             //读IAP数据
    IapIdle();                                  //关闭IAP功能

    return dat;
}

void IapProgram(int addr, char dat)
{
    IAP_CONTR = 0x80;                           //使能IAP
    IAP_TPS = 12;                               //设置等待参数12MHz
    IAP_CMD = 2;                                //设置IAP写命令
    IAP_ADDRL = addr;                           //设置IAP低地址
    IAP_ADDRH = addr >> 8;                      //设置IAP高地址
    IAP_DATA = dat;                             //写IAP数据
    IAP_TRIG = 0x5a;                            //写触发命令(0x5a)
    IAP_TRIG = 0xa5;                            //写触发命令(0xa5)
    _nop_();
    IapIdle();                                  //关闭IAP功能
}

void IapErase(int addr)
{
    IAP_CONTR = 0x80;                           //使能IAP
    IAP_TPS = 12;                               //设置等待参数12MHz
    IAP_CMD = 3;                                //设置IAP擦除命令
    IAP_ADDRL = addr;                           //设置IAP低地址
    IAP_ADDRH = addr >> 8;                      //设置IAP高地址
    IAP_TRIG = 0x5a;                            //写触发命令(0x5a)
    IAP_TRIG = 0xa5;                            //写触发命令(0xa5)
    _nop_();                                    //
    IapIdle();                                  //关闭IAP功能
}



//========================================================================
//                              工具函数 @12MHz
//========================================================================


// 延时，单位1微秒
void _delay_us(unsigned int _us)  //@12MHz
{
	unsigned char data i;
	for(_us;_us;_us--){
		i = 2;
		while (--i);
	}
}



// 延时，单位1毫秒
void delay_ms(unsigned int ms)	 //@12MHz
{
	unsigned char data i, j;
	for(ms; ms; ms--){ 
		i = 16;
		j = 147;
		do
		{
			while (--j);
		} while (--i);
	}
}


//  从Arduino中拷贝过来的工具函数
long __run;
long __rise ;
long __delta;

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    __run = in_max - in_min;
    if(__run == 0){
      return -1; // AVR returns -1, SAM returns 0
    }
    __rise = out_max - out_min;
    __delta = x - in_min;
    return (__delta * __rise) / __run + out_min;
}




