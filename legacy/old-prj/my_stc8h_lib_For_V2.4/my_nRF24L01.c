#include "STC8H.h"
#include <intrins.h>
#include <string.h>
#include <stdio.h>

#include "my_nRF24L01.h"

#include "my_stc8h_lib.h"


/************************ 配置信息 **************************/
/*
原文链接：https://blog.csdn.net/m0_51388102/article/details/127001860

两个NRF24L01,收发双方需要满足4个条件：
1.发射接收频道相同（设置频道寄存器RF_CH 0-125）
2.发射接收地址相同（设置TX_ADDR和RX_ADDR_P0相同 5个8位地址 ）
3.发射接收数据宽度相同（n<=32）
4.发射接收速率相同（250K 1M 2M）
*/                      

// 1. 地址
//uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {0x20, 0x24, 0x06, 0x12, 0x00};
uchar const  TX_ADDRESS[TX_ADR_WIDTH] = {'T', '0', '0', '1', '3'};

// 2. 频道
// RF_CH 寄存器设置值，设置芯片工作时的信道，分别对应第0~125个信道；信道间隔为1MHz，默认为02即2402MHz
#define RF_CH_VALUE 0x11


// 3. 射频数据率
// RF_SETUP 寄存器设置值，数据率越低，遥控距离越远
// 速率要一致，否则Tx方就显示发送失败;
// #define RF_SETUP_VALUE 0x07 		// 24L01+中，0x07表示射频数据率为 1Mbps, TX发射功率 0dBm 

#define RF_SETUP_VALUE 0x27 		// 24L01+中，0x27表示射频数据率为 250kbps，TX发射功率 0dBm 


/**************************************************/

uchar rx_buf[TX_PLOAD_WIDTH] = {0};
uchar tx_buf[TX_PLOAD_WIDTH] = {0};
uchar ack_buf[ACK_PLOAD_WIDTH] = {0};
uchar flag;

/**************************************************/


// By TYG 声明函数，具体实现在main.c中
void _delay_us(unsigned int _us);

/**************************************************
Function: init_io();
Description:
  flash led one time,chip enable(ready to TX or RX Mode),
  Spi disable,Spi clock line init high
/**************************************************/
void init_nrf24l01_io(void)
{
	CE=0;			// chip enable
	CSN=1;			// Spi disable	
	SCK=0;			// Spi clock line init high
}

// ========================================================= 基础操作 =====================================

/**************************************************

SPI基本读写操作，一次读写一个Byte(8Bit) 

* 注意，操作nRF24L01模块时，执行一个命令不能直接使用SPI_RW(cmd);而是SPI_RW(cmd); SPI_RW(0xff);才行！！（注意CSN需要设置） 
/**************************************************/
/* 
// 模拟SPI
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

/*  */
// 硬件SPI
uchar SPI_RW(uchar byte)
{
	SPDAT = byte;
	B_SPI_Busy = 1;
	while(B_SPI_Busy);
	return SPDAT;
}



/**************************************************

读写nRF24L01的一个寄存器, 写入值或返回值是1字节的

/**************************************************/

// Writes value 'value' to register 'reg'
uchar SPI_RW_Reg(uchar reg, uchar value)
{
	uchar status;
  	CSN = 0;                   // CSN low, init SPI transaction
  	status = SPI_RW(reg);      // select register
  	SPI_RW(value);             // ..and write value to it..
  	CSN = 1;                   // CSN high again
  	return(status);            // return nRF24L01 status byte
}


// Read one byte from nRF24L01 register, 'reg'
uchar SPI_Read(uchar reg)
{
	uchar reg_val;
  	CSN = 0;                // CSN low, initialize SPI communication...
  	SPI_RW(reg);            // Select register to read from..
  //reg_val = SPI_RW(0);    // ..then read registervalue
  	reg_val = SPI_RW(0xff); // By TYG
	CSN = 1;                // CSN high, terminate SPI communication
  	return(reg_val);        // return register value
}




/**************************************************

读写nRF24L01的一个寄存器, 写入值或返回值是多字节的

/**************************************************/

// Description:
//   Reads 'bytes' #of bytes from register 'reg'
//   Typically used to read RX payload, Rx/Tx address
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



// Description:
//   Writes contents of buffer '*pBuf' to nRF24L01
//   Typically used to write TX payload, Rx/Tx address
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



// ========================================================= 基础操作 End =====================================





/*********************************************************************/
/*                                                                   */
/*  检测nRF24L01是否存在. 返回值1=存在, 0=不存在                       */
/*                                                                   */
/*********************************************************************/
u8 nRF24L01_Check(void)
{
	u8 check_in_buf[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
	u8 check_out_buf[20] = {0};
	
	CE = 0;
	SPI_Write_Buf(WRITE_REG + TX_ADDR, check_in_buf, 5);
	SPI_Read_Buf (READ_REG  + TX_ADDR, check_out_buf, 5);
	
	if( (check_out_buf[0] == 0x11) && (check_out_buf[1] == 0x22) && (check_out_buf[2] == 0x33) && (check_out_buf[3] == 0x44) && (check_out_buf[4] == 0x55) )
	{
		return 1;
	}
	
	return 0;
}





// ========================================================= 设置nRF24L01模块 =====================================



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



// 有用的参考：NRF24L01+设置为带数据的ACK功能
// https://home.eeworld.com.cn/my/space-uid-527961-blogid-237126.html

// 设置为带ACK_PAYLOAD的发送模式
// 参考：https://blog.csdn.net/JACK__Q/article/details/109294451
void ifnnrf_tx_mode2_ACKPAYLOAD(void)
{
    power_off();
	CE=0;
  	
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);   // Set PWR_UP bit, enable CRC(2 bytes) & Prim:TX. MAX_RT & TX_DS enabled..	// 寄存器0x00: 配置为发射模式、CRC、可屏蔽中断
	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0  		// 寄存器0x01: 通道0自动应答
  	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0					// 寄存器0x02: 使能接收通道0
  	SPI_RW_Reg(WRITE_REG + SETUP_AW, 0x03);										// 寄存器0x03: 设置地址长度为5字节（默认值5字节）
//	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x15); 									// 寄存器0x04: 设置自动重传：ARD(B7:B4)=1自动重发延时500us, ARC(B3:B0)=5最大自动重发次数5次
//	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x25); 									// 寄存器0x04: 设置自动重传：ARD(B7:B4)=2自动重发延时750us, ARC(B3:B0)=5最大自动重发次数5次
	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x3f); 									// 寄存器0x04: 设置自动重传：ARD 1000us, ARC 15 // 设置重发多一点，隔着东西时增加点ACK回传的成功率
	
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_VALUE);      	// By TYG				// 寄存器0x05: 设置RF频道
	SPI_RW_Reg(WRITE_REG + RF_SETUP, RF_SETUP_VALUE);   // By TYG				// 寄存器0x06: 设置发射功率	

  	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // RX_Addr0 same as TX_Adr for Auto.Ack	// 寄存器0x0A: 设置接收地址
	SPI_Write_Buf(WRITE_REG + TX_ADDR,    TX_ADDRESS, TX_ADR_WIDTH); // Writes TX_Address to nRF24L01			// 寄存器0x10: 设置发送地址
 	SPI_RW_Reg(WRITE_REG + RX_PW_P0, 0x20);																		// 寄存器0x11: 设置通道0接收负载的字节数, 0x20是32Byte
	
	SPI_RW_Reg(WRITE_REG + DYNPD, 0x01);	 // NRF24L01+特有，动态负载长度. (B0=1) DPL_P0 使能接收管道0动态负载长度(需EN_DPL及ENAA_P0)
	SPI_RW_Reg(WRITE_REG + FEATURE, 0x06);   // NRF24L01+特有.              (B2=1) EN_DPL 使能动态负载长度; (B1=1) EN_ACK_PAY 使能ACK负载(带负载数据的ACK包); (B0=0) EN_DYN_ACK 使能命令W_TX_PAYLOAD_NOACK

	CE=1;
}



// 设置为带ACK_PAYLOAD的接收模式
// 参考：https://blog.csdn.net/JACK__Q/article/details/109294451
void ifnnrf_rx_mode2_ACKPAYLOAD(void)
{
    power_off();
	CE=0;
	
  	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);     // Set PWR_UP bit, enable CRC(2 bytes) & Prim:RX. RX_DR enabled..				// 寄存器0x00: 配置为接收模式、CRC、可屏蔽中断
	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0  		// 寄存器0x01: 通道0自动应答
  	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0					// 寄存器0x02: 使能接收通道0
  	SPI_RW_Reg(WRITE_REG + SETUP_AW, 0x03);										// 寄存器0x03: 设置地址长度为5字节（默认值5字节）
//	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x15); 									// 寄存器0x04: 设置自动重传：ARD(B7:B4)=1自动重发延时500us, ARC(B3:B0)=5最大自动重发次数5次
//	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x25); 									// 寄存器0x04: 设置自动重传：ARD(B7:B4)=2自动重发延时750us, ARC(B3:B0)=5最大自动重发次数5次
	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x3f); 									// 寄存器0x04: 设置自动重传：ARD 1000us, ARC 15 // 设置重发多一点，隔着东西时增加点ACK回传的成功率
	
	
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_VALUE);      	// By TYG				// 寄存器0x05: 设置RF频道
	SPI_RW_Reg(WRITE_REG + RF_SETUP, RF_SETUP_VALUE);   // By TYG				// 寄存器0x06: 设置发射功率	

	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH); // Use the same address on the RX device as the TX device	// 寄存器0x0A: 设置接收地址
	SPI_Write_Buf(WRITE_REG + TX_ADDR,    TX_ADDRESS, TX_ADR_WIDTH); // Writes TX_Address to nRF24L01							// 寄存器0x10: 设置发送地址																		
	SPI_RW_Reg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width						// 寄存器0x11: 设置通道0接收负载的字节数, 0x20是32Byte
	
	SPI_RW_Reg(WRITE_REG + DYNPD, 0x01);	 // NRF24L01+特有，动态负载长度. (B0=1) DPL_P0 使能接收管道0动态负载长度(需EN_DPL及ENAA_P0)
	SPI_RW_Reg(WRITE_REG + FEATURE, 0x06);   // NRF24L01+特有.              (B2=1) EN_DPL 使能动态负载长度; (B1=1) EN_ACK_PAY 使能ACK负载(带负载数据的ACK包); (B0=0) EN_DYN_ACK 使能命令W_TX_PAYLOAD_NOACK
	
  	CE = 1; // Set CE pin high to enable RX device
}




// 设置为带ACK_PAYLOAD的发送模式，使用指定的子频道和地址值初始化模式，地址必须是5个字节的
void ifnnrf_tx_mode3_ACKPAYLOAD(uchar *tx_addr, uchar RF_CH_value)
{
 //   power_off();
	CE=0;
  	
	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0e);   // Set PWR_UP bit, enable CRC(2 bytes) & Prim:TX. MAX_RT & TX_DS enabled..	// 寄存器0x00: 配置为发射模式、CRC、可屏蔽中断
	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0  		// 寄存器0x01: 通道0自动应答
  	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0					// 寄存器0x02: 使能接收通道0
  	SPI_RW_Reg(WRITE_REG + SETUP_AW, 0x03);										// 寄存器0x03: 设置地址长度为5字节（默认值5字节）
	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x3f); 									// 寄存器0x04: 设置自动重传：ARD 1000us, ARC 15 // 设置重发多一点，隔着东西时增加点ACK回传的成功率
	
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_value);      	// By TYG				// 寄存器0x05: 设置RF频道
	SPI_RW_Reg(WRITE_REG + RF_SETUP, RF_SETUP_VALUE);   // By TYG				// 寄存器0x06: 设置发射功率	

  	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, tx_addr, 5); // RX_Addr0 same as TX_Adr for Auto.Ack	// 寄存器0x0A: 设置接收地址
	SPI_Write_Buf(WRITE_REG + TX_ADDR,    tx_addr, 5); // Writes TX_Address to nRF24L01			// 寄存器0x10: 设置发送地址
 	SPI_RW_Reg(WRITE_REG + RX_PW_P0, 0x20);																		// 寄存器0x11: 设置通道0接收负载的字节数, 0x20是32Byte
	
	SPI_RW_Reg(WRITE_REG + DYNPD, 0x01);	 // NRF24L01+特有，动态负载长度. (B0=1) DPL_P0 使能接收管道0动态负载长度(需EN_DPL及ENAA_P0)
	SPI_RW_Reg(WRITE_REG + FEATURE, 0x06);   // NRF24L01+特有.              (B2=1) EN_DPL 使能动态负载长度; (B1=1) EN_ACK_PAY 使能ACK负载(带负载数据的ACK包); (B0=0) EN_DYN_ACK 使能命令W_TX_PAYLOAD_NOACK

	CE=1;
}



// 设置为带ACK_PAYLOAD的接收模式，使用指定的子频道和地址值初始化模式，地址必须是5个字节的
void ifnnrf_rx_mode3_ACKPAYLOAD(uchar *tx_addr, uchar RF_CH_value)
{
//    power_off();
	CE=0;
	
  	SPI_RW_Reg(WRITE_REG + CONFIG, 0x0f);     // Set PWR_UP bit, enable CRC(2 bytes) & Prim:RX. RX_DR enabled..				// 寄存器0x00: 配置为接收模式、CRC、可屏蔽中断
	SPI_RW_Reg(WRITE_REG + EN_AA, 0x01);      // Enable Auto.Ack:Pipe0  		// 寄存器0x01: 通道0自动应答
  	SPI_RW_Reg(WRITE_REG + EN_RXADDR, 0x01);  // Enable Pipe0					// 寄存器0x02: 使能接收通道0
  	SPI_RW_Reg(WRITE_REG + SETUP_AW, 0x03);										// 寄存器0x03: 设置地址长度为5字节（默认值5字节）
	SPI_RW_Reg(WRITE_REG + SETUP_RETR, 0x3f); 									// 寄存器0x04: 设置自动重传：ARD 1000us, ARC 15 // 设置重发多一点，隔着东西时增加点ACK回传的成功率
	
	
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_value);      	// By TYG				// 寄存器0x05: 设置RF频道
	SPI_RW_Reg(WRITE_REG + RF_SETUP, RF_SETUP_VALUE);   // By TYG				// 寄存器0x06: 设置发射功率	

	SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, tx_addr, 5); // Use the same address on the RX device as the TX device	// 寄存器0x0A: 设置接收地址
	SPI_Write_Buf(WRITE_REG + TX_ADDR,    tx_addr, 5); // Writes TX_Address to nRF24L01							// 寄存器0x10: 设置发送地址																		
	SPI_RW_Reg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH); // Select same RX payload width as TX Payload width						// 寄存器0x11: 设置通道0接收负载的字节数, 0x20是32Byte
	
	SPI_RW_Reg(WRITE_REG + DYNPD, 0x01);	 // NRF24L01+特有，动态负载长度. (B0=1) DPL_P0 使能接收管道0动态负载长度(需EN_DPL及ENAA_P0)
	SPI_RW_Reg(WRITE_REG + FEATURE, 0x06);   // NRF24L01+特有.              (B2=1) EN_DPL 使能动态负载长度; (B1=1) EN_ACK_PAY 使能ACK负载(带负载数据的ACK包); (B0=0) EN_DYN_ACK 使能命令W_TX_PAYLOAD_NOACK
	
  	CE = 1; // Set CE pin high to enable RX device
}




// 设置频道并尝试是否建立连接，频道0~125，在调用这个之前需要提前设置好发送模式的各项寄存器
uchar ifnnrf_tx_mode3_ACKPAYLOAD_setChannelAndTest(uchar RF_CH_value, uchar testCount)
{
	u8 cnt, txResult;
	
	CE=0;
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_value);      	// 寄存器0x05: 设置RF频道
	CE=1;
	
	// 准备数据
	tx_buf[0] = NRF24L01_DATA_TX_TEST; // 表示寻找频道
	
	// 发送数据测试是否联通
	for(cnt = 0; cnt < testCount; cnt++)
	{
		// 发送数据, 并接收ACKPAYLOAD
		txResult = TxPacket_ACKPAYLOAD(tx_buf, ack_buf); 
		
		// 发送成功
		if(txResult & 0x20)
		{ 
			if(ack_buf[0] == NRF24L01_DATA_RX_ACK_TEST)
			{
				return 1;
			}
		}
	}
	return 0;
}



// 设置为带ACK_PAYLOAD的发送模式, 并尝试是否建立连接，1=已连接，0=未连接
uchar ifnnrf_tx_mode3_ACKPAYLOAD_setAllAndTest(uchar *tx_addr, uchar RF_CH_value, uchar testCount)	
{
	// 指定频道，设为发送模式
	ifnnrf_tx_mode3_ACKPAYLOAD(tx_addr, RF_CH_value); 
		
	// 尝试连接
	return ifnnrf_tx_mode3_ACKPAYLOAD_setChannelAndTest(RF_CH_value, testCount);
}


// 占下当前的频道，在调用这个之前需要提前设置好发送模式的各项寄存器
uchar ifnnrf_tx_mode3_ACKPAYLOAD_Conn_CurrChannel(void)
{
	u8 txResult;
	
	// 准备数据
	tx_buf[0] = NRF24L01_DATA_TX_CONN;  // 表示进行连接
	
	// 发送数据, 并接收ACKPAYLOAD
	txResult = TxPacket_ACKPAYLOAD(tx_buf, ack_buf); 
		
	// 发送成功
	if(txResult & 0x20)
	{ 
		return 1;
	}
	return 0;
}


// 只改变频道
void  ifnnrf_changeChannel(uchar RF_CH_value)
{
	CE=0;
	SPI_RW_Reg(WRITE_REG + RF_CH, RF_CH_value);      	// 寄存器0x05: 设置RF频道
	CE=1;
}





// 发送一个数据包，并接收ACKPAYLOAD的数据
// 返回  值： 
//	  0x10:达到最大重发次数，发送失败 
//	  0x20:发送成功            
//    0xff:发送失败                  
uchar TxPacket_ACKPAYLOAD(uchar *txbuf, uchar *ackbuf)
{
	uchar state;
	CE=0;												//CE拉低，使能SI24R1配置
	SPI_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);	//写数据到TX FIFO,32个字节
 	CE=1;												//CE置高，使能发送	   
	
	while(IRQ == 1);									//等待发送完成
	state = SPI_Read(STATUS);  							//读取状态寄存器的值	   
	SPI_RW_Reg(WRITE_REG+STATUS, state); 				//清除TX_DS或MAX_RT中断标志
	
	
	if(state & STA_MARK_RX)			// 接收到ACK_PAYLOAD的标志
	{
		SPI_Read_Buf(RD_RX_PLOAD, ackbuf, ACK_PLOAD_WIDTH);     //读取ACKPAYLOAD数据
	}
	
	if(state & STA_MARK_MX)			//达到最大重发次数
	{
		SPI_RW_Reg(FLUSH_TX,0xff);	//清除TX FIFO寄存器 
		// return STA_MARK_MX; //0x10
	}
	if(state & STA_MARK_TX)			//发送完成
	{
		// return STA_MARK_TX; //0x20
	}
	
	return state;
	// return 0xFF;					//发送失败
}  


// 接收数据包，并回传ACKPAYLOAD的数据   
uchar RxPacket_ACKPAYLOAD(uchar *rxbuf, uchar *ackbuf)
{
	uchar state;

	// 设置ACK_PAYLOAD前先清空TX_FIFO，否则收到的数据可能会错位或多出数据(测试时未先清空TX_FIFO，有时收到的ACK_PAYLOAD的第一个字节是0xFF,然后才是准确的数据）
	// 参考：https://bbs.elecfans.com/jishu_1969592_1_1.html
	// SPI_RW_Reg(FLUSH_TX, 0xff);          
  	// SPI_Read(FLUSH_TX);
	CSN = 0;
	SPI_RW(FLUSH_TX);
	SPI_RW(0xff);	// 不加这个，只是SPI_RW(FLUSH_TX)会频繁出现发送失败的情况！
	CSN = 1;
	
	// 设置ACK_PAYLOAD
	SPI_Write_Buf(W_ACK_PAYLOAD, ackbuf, ACK_PLOAD_WIDTH);

	state = SPI_Read(STATUS);  			      //读取状态寄存器的值    	  
	SPI_RW_Reg(WRITE_REG+STATUS,state);       //清除RX_DS中断标志

	// TODO 经测试，当没有接nRF24L01模块时，这里state是0xFF，如果不加处理，会返回1（有新数据），所以需要处理一下
	if(state == 0xFF){
		return 0;
	}
	

	if(state & STA_MARK_RX)								      //接收到数据
	{
		SPI_Read_Buf(RD_RX_PLOAD, rxbuf, TX_PLOAD_WIDTH);     //读取数据
		SPI_RW_Reg(FLUSH_RX, 0xff);					          //清除RX FIFO寄存器
		//_delay_us(130);
		
		return 1;  // By TYG
	}	   
	// return 1;                                              //没收到任何数据
	return 0; // By TYG
}



// 获取ACKPAYLOAD的负载数据长度
uchar Ack_Payload_Width(void)	// 获取回传的ACK_PAYLOAD的数据长度时多少个字节，在发送端发送数据后调用
{
	return SPI_Read(R_RX_PL_WID);	// 获取ACKPAYLOAD的负载数据长度
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
		SPI_RW_Reg(FLUSH_RX, 0xff);					        //清除RX FIFO寄存器
		
		// return 0; 
		return 1;  // By TYG
	}	   
	// return 1;                                                    //没收到任何数据
	return 0; // By TYG
}




/********************************************************
函数功能：发送一个数据包                      
入口参数：txbuf:要发送的数据
返回  值： 
	0x10:达到最大重发次数，发送失败 
	0x20:发送成功            
	0xff:发送失败                  
*********************************************************/
uchar TxPacket(uchar *txbuf)
{
	uchar state;
	CE=0;												//CE拉低，使能SI24R1配置
	SPI_Write_Buf(WR_TX_PLOAD, txbuf, TX_PLOAD_WIDTH);	//写数据到TX FIFO,32个字节
 	CE=1;												//CE置高，使能发送	   
	
	while(IRQ == 1);									//等待发送完成
	state = SPI_Read(STATUS);  							//读取状态寄存器的值	   
	SPI_RW_Reg(WRITE_REG+STATUS, state); 				//清除TX_DS或MAX_RT中断标志
		
	if(state & STA_MARK_MX)			//达到最大重发次数
	{
		SPI_RW_Reg(FLUSH_TX,0xff);	//清除TX FIFO寄存器 
		return STA_MARK_MX; //0x10
	}
	if(state & STA_MARK_TX)			//发送完成
	{
		return STA_MARK_TX; //0x20
	}
	
	return 0xFF;					//发送失败
}  


/**注释里是从SI124R1.h中拷贝过来的，和下面的定义一致，只是定义的名称不同
	// SI124R1.h文件
	#define RX_DR			0x40
	#define TX_DS			0x20
	#define MAX_RT			0x10

	// NRF_2401.h文件
	#define STA_MARK_RX     0X40
	#define STA_MARK_TX     0X20
	#define STA_MARK_MX     0X10	
*/

