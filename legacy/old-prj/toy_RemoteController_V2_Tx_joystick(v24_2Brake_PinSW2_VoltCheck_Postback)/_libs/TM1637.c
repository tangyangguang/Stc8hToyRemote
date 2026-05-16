#include "STC8H.h" 
#include "TM1637.h"
#include "intrins.h"
  
code unsigned char tab[] =
{
        0x3F,/*0*/
        0x06,/*1*/
        0x5B,/*2*/
        0x4F,/*3*/
        0x66,/*4*/
        0x6D,/*5*/
        0x7D,/*6*/
        0x07,/*7*/
        0x7F,/*8*/
        0x6F,/*9*/
        0x77,/*10 A*/
        0x7C,/*11 b*/
        0x58,/*12 c*/
        0x5E,/*13 d*/
        0x79,/*14 E*/
        0x71,/*15 F*/
        0x76,/*16 H*/
        0x38,/*17 L*/
        0x54,/*18 n*/
        0x73,/*19 P*/
        0x3E,/*20 U*/
        0x00,/*21 黑屏*/
		0x40,//22 -
		0x23,//23 上箭头
		0x1C,//24 下箭头
};
 
/*
void Delay_us(unsigned  int i)
{
    for(;i>0;i--)
        _nop_();;
}
 */

// 12MHz
void Delay_us(unsigned  int _us)
{
	unsigned char data i;
	for(_us;_us;_us--){
		i = 2;
		while (--i);
	}
}

//IIC开始
void TM1637_start(void)
{
     TM1637_CLK=1;
     TM1637_DIO=1;
     Delay_us(2);
     TM1637_DIO=0;
}
 
 
//IIC应答
void TM1637_ack(void)
{
    unsigned char i;
    TM1637_CLK=0;
    Delay_us(5);
     //TM1637_DIO=1;   
    while(TM1637_DIO==1&&(i<250))i++;
    TM1637_CLK=1;
    Delay_us(2);
    TM1637_CLK=0;
}
 
//IIC停止
void TM1637_stop(void)
{
     TM1637_CLK=0;
     Delay_us(2);
     TM1637_DIO=0;
     Delay_us(2);
     TM1637_CLK=1;
     Delay_us(2);
     TM1637_DIO=1;
     Delay_us(2);
}
 
//写数据函数
void TM1637_Write(unsigned char DATA)   
{
    unsigned char i;   
    for(i=0;i<8;i++)        
    {
        TM1637_CLK=0;     
        if(DATA & 0x01)
            TM1637_DIO=1;
        else TM1637_DIO=0;
         Delay_us(3);
        DATA=DATA>>1;      
        TM1637_CLK=1;
         Delay_us(3);
    }  
    //TM1637_ack();
}
 
 
/** 按从左到右的顺序显示, h 控制 ':'， 1显示，0不显示 */
void TM1637_display(unsigned char a,unsigned char b,unsigned char c,unsigned char d,unsigned char h)
{
	// if(1) return; // 使用的串口的Pin, P30和P31，关掉显示才能正常串口通信查看信息
	
	TM1637_start();
    TM1637_Write(0x40);//写数据+自动地址加1+普通模式
    TM1637_ack();
    TM1637_stop();
    TM1637_start();
    TM1637_Write(0xc0);//设置显示首地址即第一个LED
    TM1637_ack();
 
    TM1637_Write(tab[a]);
    TM1637_ack();
    TM1637_Write(tab[b]|h<<7);//h为1时显示时钟中间的两点
    TM1637_ack();
    TM1637_Write(tab[c]);
    TM1637_ack();
    TM1637_Write(tab[d]);
    TM1637_ack();
 
    TM1637_stop();
    TM1637_start();
    TM1637_Write(0x89);//开显示，2/16亮度
    TM1637_ack();
    TM1637_stop();
}
 
void TM1637_DisplayInt(unsigned int target)
{
	unsigned char one, ten, hund, thous;
	
	thous = target / 1000;
	hund = target / 100 % 10;
	ten = target / 10 % 10;
	one = target % 10;
	
	TM1637_Display(thous, hund, ten, one, 0);
}