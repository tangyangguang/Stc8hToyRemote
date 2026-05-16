#ifndef __TM1637_H__
#define __TM1637_H__
 
#include "STC8H.h" // 我用的是stc8h系列单片机
 
// 随意两个I/O口
/*
#define TM1637_CLK P36 //定义模拟IIC总线的时钟线
#define TM1637_DIO P37 //定义模拟IIC总线的数据线
*/


// 占用P30，会影响下载程序，这样下载程序时需要先断开P30才能成功
// #define TM1637_CLK P30 //定义模拟IIC总线的时钟线
// #define TM1637_DIO P31 //定义模拟IIC总线的数据线
 
 
 
#define TM1637_CLK P17 //定义模拟IIC总线的时钟线
#define TM1637_DIO P31 //定义模拟IIC总线的数据线
 
 
// 显示整数
void TM1637_DisplayInt(unsigned int target);
 
// 显示小数: 使用 ':' 分隔, 仅限两位整数和两位小数, 否则截取
// void TM1637_DisplayFloat(unsigned float target);
 
 // 自定义显示. 按从左到右的顺序显示, h 控制 ':'， 1显示，0不显示
void TM1637_Display(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned char h);
 
#endif