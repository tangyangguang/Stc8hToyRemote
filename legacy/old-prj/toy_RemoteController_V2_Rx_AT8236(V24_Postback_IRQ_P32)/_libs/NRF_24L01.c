#include "Config.h"
#include "STC8H.h"
#include <intrins.h>
#include <string.h>
#include <stdio.h>

#include "NRF_24L01.h"



/************************ 配置信息 **************************/

/***
原文链接：https://blog.csdn.net/m0_51388102/article/details/127001860

两个NRF24L01,收发双方需要满足4个条件：
1.发射接收频道相同（设置频道寄存器RF_CH 0-125）
2.发射接收地址相同（设置TX_ADDR和RX_ADDR_P0相同 5个8位地址 ）
3.发射接收数据宽度相同（n<=32）
4.发射接收速率相同（250K 1M 2M）
*/                      



// 地址
// uchar const TX_ADDRESS[TX_ADR_WIDTH]  = {0x34,0x43,0x10,0x10,0x01}; // Define a static TX address

// 遥控器V1
// uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '0', '1'};	// 2通道：前后方向 + 速度。				自用
// uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '0', '2'};	// 4通道：前后方向 + 速度 + 灯光 + 喇叭。	自用
// uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '0', '3'};	// 4通道：前后方向 + 速度 + 灯光 + 喇叭。	给星屹

// 遥控器V2
// uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '1', '1'};		// 7通道：前后方向 + 速度 + 刹车 + 转向舵机 + 灯光 + 喇叭 + MOSFET调速开关

// uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '1', '2'};	
 
 uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '1', '3'};	
 


// RF_CH 寄存器设置值，设置射频通道（选择通信频率）
// RF通道频率指的是nRF24LO1所使用的中心频率，该频率范围从2400GHz到2.525GHz，以IMHz区分一个频点，故有125个频点可使用。
// 由参数RF_CH确定，公式为：Fo-2400+RF_CHL（MIZ）
#define RF_CH_VALUE 0x01


// RF_SETUP 寄存器设置值; 0x07（0000 0111） 表示 TX_PWR:0dBm, 1Mbps, LNA:HCURR
// 速率要一致，否则Tx方就显示发送失败;
#define RF_SETUP_VALUE 0x07




/**************************************************/

uchar rx_buf[TX_PLOAD_WIDTH] = {0};

uchar tx_buf[TX_PLOAD_WIDTH];

uchar flag;


/**************************************************/
unsigned char	bdata sta;

sbit	RX_DR	=sta^6;

sbit	TX_DS	=sta^5;

sbit	MAX_RT	=sta^4;
/**************************************************/


// By TYG 声明函数，具体实现在main.c中
void _delay_us(unsigned int _us);
void delay_ms(unsigned int _ms);



/**************************************************
Function: init_io();
Description:
  flash led one time,chip enable(ready to TX or RX Mode),
  Spi disable,Spi clock line init high
/**************************************************/
#define KEY 0xaa
void init_nrf24l01_io(void)
{
	CE=0;			// chip enable
	CSN=1;			// Spi disable	
	SCK=0;			// Spi clock line init high
	
	// By TYG 
	IRQ=1; 
}
/**************************************************/

/**************************************************
Function: Inituart();

Description:
  set uart working mode 
/**************************************************/

/**
void delay_ms(unsigned int _ms)
{
  for(_ms;_ms;_ms--)
  {
    _delay_us(1000);
  }
}
*/

/**************************************************/

/**************************************************
Function: SPI_RW();

Description:
  Writes one byte to nRF24L01, and return the byte read
  from nRF24L01 during write, according to SPI protocol
/**************************************************/
bdata unsigned char st=0;
sbit st_1=st^0;
sbit st_2=st^1;
sbit st_3=st^2;
sbit st_4=st^3;
sbit st_5=st^4;
sbit st_6=st^5;
sbit st_7=st^6;
sbit st_8=st^7;
bdata unsigned char st1=0;
sbit st_11=st1^0;
sbit st_12=st1^1;
sbit st_13=st1^2;
sbit st_14=st1^3;
sbit st_15=st1^4;
sbit st_16=st1^5;
sbit st_17=st1^6;
sbit st_18=st1^7;
/*
uchar SPI_RW(uchar byte)
{
	uchar bit_ctr;
   	for(bit_ctr=0;bit_ctr<8;bit_ctr++)   // output 8-bit
   	{
   		MOSI = (byte & 0x80);         // output 'byte', MSB to MOSI
   		byte = (byte << 1);           // shift next bit into MSB..
   		SCK = 1;                      // Set SCK high..
		MISO=1;
   		byte |= MISO;       		  // capture current MISO bit
   		SCK = 0;            		  // ..then set SCK low again
   	}
    return(byte);           		  // return read byte
}
*/

uchar SPI_RW(uchar byte)
{
	//uchar bit_ctr;

    st=byte;

    MOSI=st_8;
    SCK = 1;
    st_18=MISO;
    SCK = 0;

    MOSI=st_7;
    SCK = 1;
    st_17=MISO;
    SCK = 0;

    MOSI=st_6;
    SCK = 1;
    st_16=MISO;
    SCK = 0;

    MOSI=st_5;
    SCK = 1;
    st_15=MISO;
    SCK = 0;

    MOSI=st_4;
    SCK = 1;
    st_14=MISO;
    SCK = 0;

    MOSI=st_3;
    SCK = 1;
    st_13=MISO;
    SCK = 0;

    MOSI=st_2;
    SCK = 1;
    st_12=MISO;
    SCK = 0;

    MOSI=st_1;
    SCK = 1;
    st_11=MISO;
    SCK = 0;


    return(st1);           		  // return read byte
}
/**************************************************/

/**************************************************
Function: SPI_RW_Reg();

Description:
  Writes value 'value' to register 'reg'
/**************************************************/
uchar SPI_RW_Reg(uchar reg, uchar value)
{
	uchar status;
  	CSN = 0;                   // CSN low, init SPI transaction
  	status = SPI_RW(reg);      // select register
  	SPI_RW(value);             // ..and write value to it..
  	CSN = 1;                   // CSN high again
  	return(status);            // return nRF24L01 status byte
}
/**************************************************/

/**************************************************
Function: SPI_Read();

Description:
  Read one byte from nRF24L01 register, 'reg'
/**************************************************/
uchar SPI_Read(uchar reg)
{
	uchar reg_val;

  	CSN = 0;                // CSN low, initialize SPI communication...
  	SPI_RW(reg);            // Select register to read from..
  	reg_val = SPI_RW(0);    // ..then read registervalue
  	CSN = 1;                // CSN high, terminate SPI communication

  	return(reg_val);        // return register value
}
/**************************************************/

/**************************************************
Function: SPI_Read_Buf();

Description:
  Reads 'bytes' #of bytes from register 'reg'
  Typically used to read RX payload, Rx/Tx address
/**************************************************/
uchar SPI_Read_Buf(uchar reg, uchar *pBuf, uchar bytes)
{
	uchar status,byte_ctr;

  	CSN = 0;                    		// Set CSN low, init SPI tranaction
  	status = SPI_RW(reg);       		// Select register to write to and read status byte

  	for(byte_ctr=0;byte_ctr<bytes;byte_ctr++)
    	pBuf[byte_ctr] = SPI_RW(0);    // Perform SPI_RW to read byte from nRF24L01

  	CSN = 1;                           // Set CSN high again

  	return(status);                    // return nRF24L01 status byte
}
/**************************************************/

/**************************************************
Function: SPI_Write_Buf();

Description:
  Writes contents of buffer '*pBuf' to nRF24L01
  Typically used to write TX payload, Rx/Tx address
/**************************************************/
uchar SPI_Write_Buf(uchar reg, uchar *pBuf, uchar bytes)
{
	uchar status,byte_ctr;

  	CSN = 0;                   // Set CSN low, init SPI tranaction
  	status = SPI_RW(reg);    // Select register to write to and read status byte
  	for(byte_ctr=0; byte_ctr<bytes; byte_ctr++) // then write all byte in buffer(*pBuf)
    	SPI_RW(*pBuf++);
  	CSN = 1;                 // Set CSN high again
  	return(status);          // return nRF24L01 status byte
}
/**************************************************/

/**************************************************
Function: RX_Mode();

Description:
  This function initializes one nRF24L01 device to
  RX Mode, set RX address, writes RX payload width,
  select RF channel, datarate & LNA HCURR.
  After init, CE is toggled high, which means that
  this device is now ready to receive a datapacket.
/**************************************************/
void power_off()
{
	CE=0;
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0D); 
	CE=1;
	_delay_us(20);
}
void ifnnrf_rx_mode(void)
{
    power_off();
	CE=0;
  	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // Use the same address on the RX device as the TX device

  	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0
  	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0
	SPI_RW_Reg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);     // Set PWR_UP bit, enable CRC(2 bytes) & Prim:RX. RX_DR enabled..

  	// SPI_RW_Reg(WRITE_REG + RF_CH, 40);        // Select RF channel 40
	// SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   // TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_VALUE);      // By TYG
  	SPI_RW_Reg(WRITE_REG + RF_SETUP, RF_SETUP_VALUE);   // By TYG
		
  	CE = 1; // Set CE pin high to enable RX device

  //  This device is now ready to receive one packet of 16 bytes payload from a TX device sending to address
  //  '3443101001', with auto acknowledgment, retransmit count of 10, RF channel 40 and datarate = 2Mbps.

}
/**************************************************/

/**************************************************
Function: TX_Mode();

Description:
  This function initializes one nRF24L01 device to
  TX mode, set TX address, set RX address for auto.ack,
  fill TX payload, select RF channel, datarate & TX pwr.
  PWR_UP is set, CRC(2 bytes) is enabled, & PRIM:TX.

  ToDo: One high pulse(>10us) on CE will now send this
  packet and expext an acknowledgment from the RX device.
/**************************************************/
void ifnnrf_tx_mode(void)
{
    power_off();
	CE=0;
	
  	SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // Writes TX_Address to nRF24L01
  	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // RX_Addr0 same as TX_Adr for Auto.Ack
  	SPI_Write_Buf(WR_TX_PLOAD, tx_buf, TX_PLOAD_WIDTH); // Writes data to TX payload

  	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0
  	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0
  	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x1a); // 500us + 86us, 10 retrans...
		//	SPI_RW_Reg(WRITE_REG + RF_CH, 40);        // Select RF channel 40
		//  SPI_RW_Reg(WRITE_REG + RF_SETUP, 0x07);   // TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR

		SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_VALUE);      // By TYG
  	SPI_RW_Reg(WRITE_REG + RF_SETUP, RF_SETUP_VALUE);   // By TYG
	
  	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);     // Set PWR_UP bit, enable CRC(2 bytes) & Prim:TX. MAX_RT & TX_DS enabled..
	CE=1;

}

void SPI_CLR_Reg(uchar R_T)
{
  	CSN = 0; 
	if(R_T==1)                  // CSN low, init SPI transaction
  	SPI_RW(FLUSH_TX);             // ..and write value to it..
	else
	SPI_RW(FLUSH_RX);             // ..and write value to it..
  	CSN = 1;                   // CSN high again
}


// By TYG 在loop()中如果调用这个，会出Tx方显示发送成功，但接收的数据为空的情况
void ifnnrf_CLERN_ALL()
{
  SPI_CLR_Reg(0);
  SPI_CLR_Reg(1);
  SPI_RW_Reg(WRITE_REG+STATUS,0xff);
  IRQ=1;
}




/************************************************************** 从SI124R1.c中拷贝过来的 */

/**注释里是从SI124R1.h中拷贝过来的，和下面的定义一致，只是定义的名称不同
	// SI124R1.h文件
	#define RX_DR						0x40
	#define TX_DS						0x20
	#define MAX_RT					0x10

	// NRF_2401.h文件
	#define STA_MARK_RX     0X40
	#define STA_MARK_TX     0X20
	#define STA_MARK_MX     0X10	
*/


/********************************************************
函数功能：读取接收数据                       
入口参数：rxbuf:接收数据存放首地址
返回  值：1:接收到数据 （改为1是有新数据，0是没有新数据）
          0:没有接收到数据
*********************************************************/
uchar RxPacket(uchar *rxbuf)
{
	uchar state;
	state = SPI_Read(STATUS);  			                 //读取状态寄存器的值    	  
	SPI_RW_Reg(WRITE_REG+STATUS,state);               //清除RX_DS中断标志

	
	// ！！ 当没有接NRF24L01模块时，执行程序，这里会返回1 ？？ 不知为什么
	// BY TYG 2024.04.06
	// TODO 经测试，当没有接nRF24L01模块时，这里state是0xFF，如果不加处理，会返回1（有新数据），所以需要处理一下
	if(state == 0xFF){
		return 0;
	}
	
	
	if(state & STA_MARK_RX)								                           //接收到数据
	{
		SPI_Read_Buf(RD_RX_PLOAD,rxbuf,TX_PLOAD_WIDTH);     //读取数据
		SPI_RW_Reg(FLUSH_RX,0xff);					              //清除RX FIFO寄存器
		// return 0; 
		return 1;  // By TYG
	}	   
	// return 1;                                                    //没收到任何数据
	return 0; // By TYG
}




/********************************************************
函数功能：发送一个数据包                      
入口参数：txbuf:要发送的数据
返回  值：0x10:达到最大重发次数，发送失败 
          0x20:发送成功            
          0xff:发送失败                  
*********************************************************/
uchar TxPacket(uchar *txbuf)
{
	uchar state;
	CE=0;																										  //CE拉低，使能SI24R1配置
  SPI_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);	    //写数据到TX FIFO,32个字节
 	CE=1;																										  //CE置高，使能发送	   
	
	while(IRQ == 1);																				  //等待发送完成
	state = SPI_Read(STATUS);  											  //读取状态寄存器的值	   
	SPI_RW_Reg(WRITE_REG+STATUS, state); 								//清除TX_DS或MAX_RT中断标志
		
	if(state&STA_MARK_MX)																			    //达到最大重发次数
	{
		SPI_RW_Reg(FLUSH_TX,0xff);										    //清除TX FIFO寄存器 
		return STA_MARK_MX; 
	}
	if(state&STA_MARK_TX)																			      //发送完成
	{
		return STA_MARK_TX;
	}
	
	return 0XFF;																						  //发送失败
}  


/**注释里是从SI124R1.h中拷贝过来的，和下面的定义一致，只是定义的名称不同
	// SI124R1.h文件
	#define RX_DR						0x40
	#define TX_DS						0x20
	#define MAX_RT					0x10

	// NRF_2401.h文件
	#define STA_MARK_RX     0X40
	#define STA_MARK_TX     0X20
	#define STA_MARK_MX     0X10	
*/

