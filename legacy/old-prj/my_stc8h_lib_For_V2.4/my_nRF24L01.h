#ifndef __NRF24L01_H
#define __NRF24L01_H


// nRF24L01模块的引脚定义（使用的是STC8H1K08）

sbit CE =  P1^6;  // nRF24L01的 PIN 3
sbit CSN=  P1^2;  // nRF24L01的 PIN 4
sbit SCK=  P1^5;  // nRF24L01的 PIN 5
sbit MOSI= P1^3;  // nRF24L01的 PIN 6
sbit MISO= P1^4;  // nRF24L01的 PIN 7
sbit IRQ = P3^2;  // nRF24L01的 PIN 8, // P32支持中断



#define uchar unsigned char


#define TX_ADR_WIDTH    5   // 5  bytes TX(RX) address width
// #define TX_PLOAD_WIDTH  32  // 32 bytes TX payload
#define TX_PLOAD_WIDTH  16 

// By TYG
// 参考： https://blog.csdn.net/weixin_42876465/article/details/87647836
// ACK_PAYLOAD的数据长度，越长就需要配置越长的ARD时间，对于250kbps, 8字节需要750us的自动重发时间间隔. 
// 1Mbps时，ARD在500us就可以时任何ACK有效负载长度
#define ACK_PLOAD_WIDTH  8  // ACK_PAYLOAD的数据长度



#define READ_REG        0x00  // Define read command to register
#define WRITE_REG       0x20  // Define write command to register
#define RD_RX_PLOAD     0x61  // Define RX payload register address
#define WR_TX_PLOAD     0xA0  // Define TX payload register address
#define FLUSH_TX        0xE1  // Define flush TX register command
#define FLUSH_RX        0xE2  // Define flush RX register command
#define REUSE_TX_PL     0xE3  // Define reuse TX payload register command
// #define NOP          0xFF  // Define No Operation, might be used to read status register   // By TYG 注释掉这行，和STC8H.h中的定义有重复，且这行也没有用到

// By TYG
#define R_RX_PL_WID		0x60	// 读取收到的数据字节数
#define W_ACK_PAYLOAD	0xA8	// 1 to 32 LSByte first 适用于接收方，通过PIPE PPP将数据通过ACK的形式发出去，最多允许三帧数据存于FIFO中。



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

// By TYG
#define DYNPD			0x1C    // 使能动态负载长度 寄存器
#define FEATURE			0x1D	// 特征寄存器
 
 

#define STA_MARK_RX     0X40
#define STA_MARK_TX     0X20
#define STA_MARK_MX     0X10	    


/***  By TYG 注释里是从SI124R1.h中拷贝过来的，和上面的定义一致，只是定义的名称不同
// STATUS Register 
#define RX_DR					0x40
#define TX_DS					0x20
#define MAX_RT					0x10
*/


//***************************************************************/
//                   FUNCTION's PROTOTYPES                      //
/****************************************************************/

// void NRF24L01_SPI_Init(uchar Mode);     // Init HW or SW SPI
uchar SPI_RW(uchar byte);                                // Single SPI read/write
uchar SPI_Read(uchar reg);                               // Read one byte from nRF24L01
uchar SPI_RW_Reg(uchar reg, uchar byte);                  // Write one byte to register 'reg'
uchar SPI_Write_Buf(uchar reg, uchar *pBuf, uchar bytes);  // Writes multiply bytes to one register
uchar SPI_Read_Buf(uchar reg, uchar *pBuf, uchar bytes);   // Read multiply bytes from one register

void init_nrf24l01_io(void);
void ifnnrf_rx_mode(void);
void ifnnrf_tx_mode(void);


/***  By TYG */
uchar RxPacket(uchar *rxbuf);
uchar TxPacket(uchar *txbuf);


uchar TxPacket_ACKPAYLOAD(uchar *txbuf, uchar *ackbuf); // 发送数据txbuf, 并接收ACK_PAYLOAD到ackbuf（ACKPAYLOAD模式必须是动态负载数据长度）
uchar RxPacket_ACKPAYLOAD(uchar *rxbuf, uchar *ackbuf); // 接收数据到rxbuf，并回传ackbuf数据(ACK_PAYLOAD)

uchar nRF24L01_Check(void); // 检测nRF24L01是否存在. 返回值1=存在, 0=不存在

void ifnnrf_tx_mode2_ACKPAYLOAD(void); // 设置为带ACK_PAYLOAD的发送模式
void ifnnrf_rx_mode2_ACKPAYLOAD(void); // 设置为带ACK_PAYLOAD的接收模式
uchar Ack_Payload_Width(void);	// 获取回传的ACK_PAYLOAD的数据长度时多少个字节，在发送端发送数据后调用


// 使用指定的子频道和地址值初始化模式，地址必须是5个字节的
void ifnnrf_tx_mode3_ACKPAYLOAD(uchar *tx_addr, uchar RF_CH_value); 			// 设置为带ACK_PAYLOAD的发送模式
void ifnnrf_rx_mode3_ACKPAYLOAD(uchar *tx_addr, uchar RF_CH_value);				// 设置为带ACK_PAYLOAD的接收模式

uchar ifnnrf_tx_mode3_ACKPAYLOAD_setAllAndTest(uchar *tx_addr, uchar RF_CH_value, uchar testCount); // 设置为带ACK_PAYLOAD的发送模式, 并尝试是否建立连接，1=已连接，0=未连接
uchar ifnnrf_tx_mode3_ACKPAYLOAD_setChannelAndTest(uchar RF_CH_value, uchar testCount); // 设置频道并尝试是否建立连接，频道0~125，在调用这个之前需要提前设置好发送模式的各项寄存器
uchar ifnnrf_tx_mode3_ACKPAYLOAD_Conn_CurrChannel(void);		// 占下当前的频道，在调用这个之前需要提前设置好发送模式的各项寄存器

void  ifnnrf_changeChannel(uchar RF_CH_value); // 只改变频道

//  By TYG
extern uchar tx_buf[]; // 发送的数据
extern uchar rx_buf[]; // 接收的数据
extern uchar ack_buf[]; // 接收时回传的数据ACK_PAYLOAD



// By TYG
#define NRF24L01_DATA_TX_TEST		0x11	// 发送数据包的第0位，表示还未建立连接，正在寻找频道，只在刚开机建立连接前才用
#define NRF24L01_DATA_TX_CONN		0x12	// 发送数据包的第0位，表示正在建立连接，但发送的还不是业务数据，只在刚找到频道时为了快速占下频道才用
#define NRF24L01_DATA_TX_BIZ		0x13	// 发送数据包的第0位，表示已经建立连接，并正式使用频道交流，发送的数据包从第1位开始是业务数据

#define NRF24L01_DATA_RX_ACK_TEST	0x21	// 回传数据包的第0位，表示还未建立连接，也未被占下来频道，只在刚开机建立连接前才用
#define NRF24L01_DATA_RX_ACK_CONN	0x22	// 回传数据包的第0位，表示正在建立连接，频道已经被占下来，只在刚开机建立连接时才用
#define NRF24L01_DATA_RX_ACK_BIZ	0x23	// 回传数据包的第0位，表示已经建立连接，并正式使用频道交流，回传的数据包从第1位开始是业务数据




#endif
