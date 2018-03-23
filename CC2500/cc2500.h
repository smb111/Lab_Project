#ifndef CC2500_H
#define CC2500_H

#include "common.h"


//定义CC2500最大数量
#define CC2500_IOC_MAXNR 8
//定义CC2500的magic number
#define CC2500_IOC_MAGIC  'm'


#define CC2500_IOC_PHY_DETECT_STATUS _IO(CC2500_IOC_MAGIC, 1)
#define CC2500_IOC_PHY_GET_BAUDRATE _IOR(CC2500_IOC_MAGIC, 3, unsigned char)
#define CC2500_IOC_PHY_SET_BAUDRATE _IOW(CC2500_IOC_MAGIC, 4, unsigned char)
#define CC2500_IOC_PHY_GET_TXPOWER _IOR(CC2500_IOC_MAGIC, 5, unsigned char)
#define CC2500_IOC_PHY_SET_TXPOWER _IOW(CC2500_IOC_MAGIC, 6, unsigned char)
#define CC2500_IOC_PHY_GET_CHANNEL _IOR(CC2500_IOC_MAGIC, 7, unsigned char)
#define CC2500_IOC_PHY_SET_CHANNEL _IOW(CC2500_IOC_MAGIC, 8, unsigned char)

extern u32 *EINT_con_addr;
extern u32 *EINT_data_addr;
extern u32 *cc2500_con_addr;

extern u32 *spi_rcon_addr;
extern u32 *spi_rclk_addr;
extern u32 *spi_rfbclk_addr;
extern u32 *spi_rmode_addr;
extern u32 *spi_renINT_addr;
extern u32 *spi_rcscon_addr;
extern u32 *spi_rTx_data;
extern u32 *spi_rRx_data;
extern u32 *spi_rstatus_addr;
extern u32 *spi_rpending_clr_addr;
extern u32 *spi_rpacket_cnt_addr; 

bool spi_wait_TX_ready(void);     /* 静态函数在.c文件中不可见 ，被限定了作用域 */
bool spi_wait_TX_done(void);      
bool spi_wait_RX_ready(void);
void spi_flush_fifo(void);

#define RF_GDO0 (ioread8(EINT_data_addr)&0x02)  // 检测 中断口 是不是 1 
#define SetSendMode  iowrite8(( ioread8(cc2500_palan_dat) | (1<<3)) & ~(1<<2),cc2500_palan_dat)        /* reset GPH3_2 (LNA)  and  set  GPH3_3 (PAEN) */
#define SetRecvMode  iowrite8(( ioread8(cc2500_palan_dat) | (1<<2)) & ~(1<<3),cc2500_palan_dat)        /* set GPH3_2 (LNA)  and  reset  GPH3_3 (PAEN) */

/* 配置寄存器的宏定义 */
#define REG_IOCFG2       0x00	// GDO2 output pin configuration
#define REG_IOCFG1       0x01	// GDO1 output pin configuration
#define REG_IOCFG0       0x02	// GDO0 output pin configuration
#define REG_FIFOTHR      0x03	// RX FIFO and TX FIFO thresholds
#define REG_SYNC1        0x04	// Sync word, high byte
#define REG_SYNC0        0x05	// Sync word, low byte
#define REG_PKTLEN       0x06	// Packet length
#define REG_PKTCTRL1     0x07	// Packet automation control
#define REG_PKTCTRL0     0x08	// Packet automation control
#define REG_ADDR         0x09	// Device address
#define REG_CHANNR       0x0A	// Channel number
#define REG_FSCTRL1      0x0B	// Frequency synthesizer control
#define REG_FSCTRL0      0x0C	// Frequency synthesizer control
#define REG_FREQ2        0x0D	// Frequency control word, high byte
#define REG_FREQ1        0x0E	// Frequency control word, middle byte
#define REG_FREQ0        0x0F	// Frequency control word, low byte
#define REG_MDMCFG4      0x10	// Modem configuration
#define REG_MDMCFG3      0x11	// Modem configuration
#define REG_MDMCFG2      0x12	// Modem configuration
#define REG_MDMCFG1      0x13	// Modem configuration
#define REG_MDMCFG0      0x14	// Modem configuration
#define REG_DEVIATN      0x15	// Modem deviation setting
#define REG_MCSM2        0x16	// Main Radio Cntrl State Machine config
#define REG_MCSM1        0x17	// Main Radio Cntrl State Machine config
#define REG_MCSM0        0x18	// Main Radio Cntrl State Machine config
#define REG_FOCCFG       0x19	// Frequency Offset Compensation config
#define REG_BSCFG        0x1A	// Bit Synchronization configuration
#define REG_AGCCTRL2     0x1B	// AGC control
#define REG_AGCCTRL1     0x1C	// AGC control
#define REG_AGCCTRL0     0x1D	// AGC control
#define REG_WOREVT1      0x1E	// High byte Event 0 timeout
#define REG_WOREVT0      0x1F	// Low byte Event 0 timeout
#define REG_WORCTRL      0x20	// Wake On Radio control
#define REG_FREND1       0x21	// Front end RX configuration
#define REG_FREND0       0x22	// Front end TX configuration
#define REG_FSCAL3       0x23	// Frequency synthesizer calibration
#define REG_FSCAL2       0x24	// Frequency synthesizer calibration
#define REG_FSCAL1       0x25	// Frequency synthesizer calibration
#define REG_FSCAL0       0x26	// Frequency synthesizer calibration
#define REG_RCCTRL1      0x27	// RC oscillator configuration
#define REG_RCCTRL0      0x28	// RC oscillator configuration
#define REG_FSTEST       0x29	// Frequency synthesizer cal control
#define REG_PTEST        0x2A	// Production test
#define REG_AGCTEST      0x2B	// AGC test
#define REG_TEST2        0x2C	// Various test settings
#define REG_TEST1        0x2D	// Various test settings
#define REG_TEST0        0x2E	// Various test settings

#define REG_PATABLE      0x3E	//OutPower
#define REG_TRXFIFO      0x3F	// FIFO

/* 射频命令 */ 
#define STROBE_SRES         0x30	// Reset chip.
#define STROBE_SFSTXON      0x31	// Enable/calibrate freq synthesizer
#define STROBE_SXOFF        0x32	// Turn off crystal oscillator.
#define STROBE_SCAL         0x33	// Calibrate freq synthesizer & disable
#define STROBE_SRX          0x34	// Enable RX.
#define STROBE_STX          0x35	// Enable TX.
#define STROBE_SIDLE        0x36	// Exit RX / TX
#define STROBE_SAFC         0x37	// AFC adjustment of freq synthesizer
#define STROBE_SWOR         0x38	// Start automatic RX polling sequence
#define STROBE_SPWD         0x39	// Enter pwr down mode when CSn goes hi
#define STROBE_SFRX         0x3A	// Flush the RX FIFO buffer.
#define STROBE_SFTX         0x3B	// Flush the TX FIFO buffer.
#define STROBE_SWORRST      0x3C	// Reset real time clock.
#define STROBE_SNOP         0x3D	// No operation.

/* 状态寄存器的宏定义 */ 
#define REG_PARTNUM      0x30	// Part number
#define REG_VERSION      0x31	// Current version number
#define REG_FREQEST      0x32	// Frequency offset estimate
#define REG_LQI          0x33	// Demodulator estimate for link quality
#define REG_RSSI         0x34	// Received signal strength indication
#define REG_MARCSTATE    0x35	// Control state machine state
#define REG_WORTIME1     0x36	// High byte of WOR timer
#define REG_WORTIME0     0x37	// Low byte of WOR timer
#define REG_PKTSTATUS    0x38	// Current GDOx status and packet status
#define REG_VCO_VC_DAC   0x39	// Current setting from PLL cal module
#define REG_TXBYTES      0x3A	// Underflow and # of bytes in TXFIFO
#define REG_RXBYTES      0x3B	// Overflow and # of bytes in RXFIFO
#define REG_NUM_RXBYTES  0x7F	// Mask "# of bytes" field in _RXBYTES

/* 命令掩码 */
#define CMD_WRITE       (0x00)
#define CMD_BURST_WRITE (0x40)
#define CMD_READ        (0x80)
#define CMD_BURST_READ  (0xC0)


#endif