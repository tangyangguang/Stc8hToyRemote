#ifndef __NRF24L01_H
#define __NRF24L01_H


#define uchar unsigned char


#define TX_ADR_WIDTH    5   // 5  bytes TX(RX) address width

#define TX_PLOAD_WIDTH  32  // 32 bytes TX payload


#define READ_REG        0x00  // Define read command to register
#define WRITE_REG       0x20  // Define write command to register
#define RD_RX_PLOAD     0x61  // Define RX payload register address
#define WR_TX_PLOAD     0xA0  // Define TX payload register address
#define FLUSH_TX        0xE1  // Define flush TX register command
#define FLUSH_RX        0xE2  // Define flush RX register command
#define REUSE_TX_PL     0xE3  // Define reuse TX payload register command

// By TYG 注释掉下面这行，和STC8H.h中的定义有重复，且下面这个行也没有用到
// #define NOP             0xFF  // Define No Operation, might be used to read status register

//***************************************************//
// SPI(nRF24L01) registers(addresses)
#define CONFIG          0x00  // 'Config' register address
#define EN_AA           0x01  // 'Enable Auto Acknowledgment' register address
#define EN_RXADDR       0x02  // 'Enabled RX addresses' register address
#define SETUP_AW        0x03  // 'Setup address width' register address
#define SETUP_RETR      0x04  // 'Setup Auto. Retrans' register address
#define RF_CH           0x05  // 'RF channel' register address
#define RF_SETUP        0x06  // 'RF setup' register address
#define STATUS          0x07  // 'Status' register address
#define OBSERVE_TX      0x08  // 'Observe TX' register address
#define CD              0x09  // 'Carrier Detect' register address
#define RX_ADDR_P0      0x0A  // 'RX address pipe0' register address
#define RX_ADDR_P1      0x0B  // 'RX address pipe1' register address
#define RX_ADDR_P2      0x0C  // 'RX address pipe2' register address
#define RX_ADDR_P3      0x0D  // 'RX address pipe3' register address
#define RX_ADDR_P4      0x0E  // 'RX address pipe4' register address
#define RX_ADDR_P5      0x0F  // 'RX address pipe5' register address
#define TX_ADDR         0x10  // 'TX address' register address
#define RX_PW_P0        0x11  // 'RX payload width, pipe0' register address
#define RX_PW_P1        0x12  // 'RX payload width, pipe1' register address
#define RX_PW_P2        0x13  // 'RX payload width, pipe2' register address
#define RX_PW_P3        0x14  // 'RX payload width, pipe3' register address
#define RX_PW_P4        0x15  // 'RX payload width, pipe4' register address
#define RX_PW_P5        0x16  // 'RX payload width, pipe5' register address
#define FIFO_STATUS     0x17  // 'FIFO Status Register' register address



#define STA_MARK_RX     0X40
#define STA_MARK_TX     0X20
#define STA_MARK_MX     0X10	    



/***  By TYG 注释里是从SI124R1.h中拷贝过来的，和上面的定义一致，只是定义的名称不同
// STATUS Register 
#define RX_DR						0x40
#define TX_DS						0x20
#define MAX_RT					0x10
*/


//***************************************************************//
//                   FUNCTION's PROTOTYPES  //
/****************************************************************
 void SPI_Init(BYTE Mode);     // Init HW or SW SPI
 BYTE SPI_RW(BYTE byte);                                // Single SPI read/write
 BYTE SPI_Read(BYTE reg);                               // Read one byte from nRF24L01
 BYTE SPI_RW_Reg(BYTE reg, BYTE byte);                  // Write one byte to register 'reg'
 BYTE SPI_Write_Buf(BYTE reg, BYTE *pBuf, BYTE bytes);  // Writes multiply bytes to one register
 BYTE SPI_Read_Buf(BYTE reg, BYTE *pBuf, BYTE bytes);   // Read multiply bytes from one register
//*****************************************************************/
  void NRF24L01_SPI_Init(unsigned char Mode);     // Init HW or SW SPI
  unsigned char SPI_RW(unsigned char byte);                                // Single SPI read/write
  unsigned char SPI_Read(unsigned char reg);                               // Read one byte from nRF24L01
  unsigned char SPI_RW_Reg(unsigned char reg, unsigned char byte);                  // Write one byte to register 'reg'
  unsigned char SPI_Write_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes);  // Writes multiply bytes to one register
  unsigned char SPI_Read_Buf(unsigned char reg, unsigned char *pBuf, unsigned char bytes);   // Read multiply bytes from one register

  void init_nrf24l01_io(void);
  void ifnnrf_rx_mode(void);
  void ifnnrf_tx_mode(void);
  void ifnnrf_CLERN_ALL();



	/***  By TYG */
	unsigned char RxPacket(unsigned char *rxbuf);
	unsigned char TxPacket(unsigned char *txbuf);


  unsigned char tx_buf[];
  unsigned char rx_buf[];

  extern unsigned char	bdata sta;
  /**
  sbit IRQ = P3^2;
  sbit CE =  P1^0;
  sbit CSN=  P1^1;
  sbit SCK=  P4^0;
  sbit MOSI= P4^1;
  sbit MISO= P4^2;
  */

/**
  sbit IRQ = P1^1;

  sbit CE =  P1^0;

  sbit CSN=  P1^4;
  sbit SCK=  P1^5;
  sbit MOSI= P1^6;
  sbit MISO= P1^7;
*/


// By TYG 设置实际的引脚（使用的是STC8H1K08）
  sbit CE =  P1^6;  // nRF24L01的 PIN 3
  sbit CSN=  P1^2;  // nRF24L01的 PIN 4
	
  sbit SCK=  P1^5;  // nRF24L01的 PIN 5
  sbit MOSI= P1^3;  // nRF24L01的 PIN 6
  sbit MISO= P1^4;  // nRF24L01的 PIN 7
	
//  sbit IRQ = P1^7;  // nRF24L01的 PIN 8

  // P32支持中断
  sbit IRQ = P3^2;  // nRF24L01的 PIN 8





#endif
