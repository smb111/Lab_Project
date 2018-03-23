/************************************************************
driver.h
文件描述：本文件主要是用来实现对CC2500的基本操作，也是硬件驱动部分之一
版本信息：v1.0
修改时间：2008/03/
*************************************************************/
//#define DEBUG_MODE
#include "driver.h"
#include "cc2500.h"
#include "common.h"

void SPIPut(BYTE cByte)		//发送一个字节
{	
       u32 tmp ;
	while(!spi_wait_TX_ready());     //wait for the transfer buffer ready

	iowrite8(cByte,spi_rTx_data);
#ifdef DEBUG_MODE
	printk("SPIPut : %x\n",cByte);
#endif
	while(!spi_wait_RX_ready());
        
        tmp = readl(spi_rRx_data);
        
        
//	udelay(20);		//IMPORTANT!! DELAY TO LET CC2500 RECEIVE DATA   或者使用 上面的代码
}

BYTE SPIGet(void)		//接收一个字节
{
	/*
	while (~ioread8(rSPSTA1) & 0x01) ;	//wait for the transfer buffer ready
	iowrite8(0xFF, rSPTDAT1);	//transfer dummy data because we want to use the CLK
	while (~ioread8(rSPSTA1) & 0x01) ;	//wait for the transfer buffer ready
	return ioread8(rSPRDAT1);
	*/
    BYTE tmp;
    BYTE ret; 
    
    while(!spi_wait_TX_ready()) 
    {
        return false;	
    }
          
    spi_flush_fifo();   /* 软件复位 */

    iowrite8(0xFF, spi_rTx_data);
    
    while(!spi_wait_RX_ready());
    
    tmp = ioread8(spi_rRx_data);

    ret = tmp & 0xff;
    
    return ret;
}  


/*********************************************************************
 * 函数名:    void RFWriteTxFIFO(BYTE *pTxBuffer,BYTE size)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE *pTxBuffer，写入值的指针，BYTE size写入的字节数
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  往TXFIFO寄存器中写入数据
 ********************************************************************/
void RFWriteTxFIFO(BYTE * pTxBuffer, BYTE size)
{
	BYTE i;
	
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(REG_TRXFIFO | CMD_BURST_WRITE);	/* 我要给你发数据了，命令是 CMD_BURST_WRITE   REG_TRXFIFO */
  
	for (i = 0; i < size; i++) 
	{
	    SPIPut(pTxBuffer[i]);
	}
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 
}


/** cc2500 发送数据到信道中函数
 * 这里面的思路如下：
 * 首先while (!status)检测信道是否为空，如果不是，等待一段时间。这里并没有采用指数退避。
 * 另外，这里的值都是凭感觉设定的，没有经过严格计算。
 * 其次，while (!RF_GDO0)检测是否进入了发送状态，如果还没有进入发送状态，延时
 * 最后，while (bSucceed && RF_GDO0)检测是否发送完成，如果超过一定时间还没有发送完成，认为发送失败
 **/
BOOL RFTransmitByCSMA(void)
{
	BYTE status;
	BOOL bSucceed = TRUE;
	struct timeval tpstart, tpend;
	unsigned long timeuse = 0;
	
       
	iowrite32(ioread32(EINT_con_addr) & 0xFFFFFF0F,EINT_con_addr);     /* 设置 GPH2_1 为 input 模式 ，不是中断模式 */  
	RFSetTRxState(RF_TRX_RX);  /* 设置 cc2500 为接收状态 */

	status = RFGetStatus(REG_PKTSTATUS) & 0x10;	//取出CCA位0001 0000 

	do_gettimeofday(&tpstart);	/* 获取Linux内核时间作为开始时间 */ 
	
	while (!status)		//等待信道是否为空，为空时，才不执行这个while    ，不为空执行这个while
	{
		if (timeuse > 80000)	//超时80ms，跳出//ZXY MODIFIED
		{
			RFSetTRxState(RF_TRX_RX);
			break;
		}
		status = RFGetStatus(REG_PKTSTATUS) & 0x10;
		udelay(100);
		do_gettimeofday(&tpend);	/* get the end time */ 
		
		timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;     /* calculate the time in usecond */
		
		#ifdef DEBUG_MODE
		printk("time use is %d, status is 0x%.2X\n", timeuse, status);
		#endif
	}

	do_gettimeofday(&tpstart);	// 开始时间
	RFSetTRxState(RF_TRX_TX);
	
	while (!RF_GDO0)	/* 检测中断口的状态值，此时中断口是输入状态 */  //没有检测到同步字sync word时，执行这个while
	{
		if (timeuse > 100000)	//100ms ZXY MODIFIED
		{
			RFSetTRxState(RF_TRX_RX);
			bSucceed = FALSE;
			break;
		}
		do_gettimeofday(&tpend);	//get the end time 
		//calculate the time in usecond
		timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
		#ifdef DEBUG_MODE
		printk("time use is %d, GDO0 is %d\n", timeuse, RF_GDO0);
		#endif
	}
	do_gettimeofday(&tpstart);	// 开始时间
	while (bSucceed && RF_GDO0) {
		if (timeuse > 1000000)	//1s,ZXY MODIFIED
		{
			RFSetTRxState(RF_TRX_RX);
			bSucceed = FALSE;
			break;
		}
		do_gettimeofday(&tpend);	//get the end time 
		//calculate the time in usecond
		timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
		#ifdef DEBUG_MODE
		printk("time use is %d, GDO0 is %d, bSucceed is %d\n", timeuse, RF_GDO0, bSucceed);
		#endif
	}

	if (bSucceed) {
		printk("One packet of data is transmitted\n");
		iowrite32(ioread32(EINT_con_addr) | 0xF0,EINT_con_addr);     /* 设置 GPH2_1 为中断模式 */
		return TRUE;
	} else {
		RFClearTxBuffer();
		printk("One packet of data is failed to transmit\n");
		iowrite32(ioread32(EINT_con_addr) | 0xF0,EINT_con_addr);     /* 设置 GPH2_1 为中断模式 */
		return FALSE;
	}
}

/* 设置 cc2500 状态寄存器 */ 
void RFSetTRxState(RF_TRX_STATE state)
{
	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	switch (state) {
	case RF_TRX_RX:
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_SRX);	//接收状态
		break;
	case RF_TRX_OFF:
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_SXOFF);	//射频关闭
		break;
	case RF_TRX_IDLE:
		RFWriteStrobe(STROBE_SIDLE);	//空闲状态
		break;
	case RF_TRX_TX:
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_STX);	//发射状态
		break;
	default:
		break;
	}
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
}

/*********************************************************************
 * 函数名:    void RFWriteStrobe(BYTE cmd)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE cmd，需要写入的命令
 * 输出参数:  无
 * 注意事项： 无。
 * 功能描述:  对CC2500设置射命令
 ********************************************************************/
void RFWriteStrobe(BYTE addr)	//写CC2500命令
{
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(addr | CMD_WRITE);	//Strobe
	
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 
}

/*********************************************************************
 * 函数名:    BYTE RFGetStatus(BYTE addr);
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE addr，寄存器地址
 * 输出参数:  返回CC2500当前的状态
 * 注意事项： 无
 * 功能描述:  读取CC2500状态
 ********************************************************************/
BYTE RFGetStatus(BYTE addr)
{
	BYTE value;
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(addr | CMD_BURST_READ);
	
	value = SPIGet();
	
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 
	
	return value;
}

void RFClearTxBuffer(void)
{
	RFWriteStrobe(STROBE_SIDLE);
	RFWriteStrobe(STROBE_SFTX);
	RFWriteStrobe(STROBE_SRX);
}

void RFSetTxPower(BYTE power)
{
	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	RFWriteBurstReg(REG_PATABLE, &power, 1);	//功率设置
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
}

void RFSetChannel(BYTE channel)
{

	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	RFWriteStrobe(STROBE_SIDLE);	                //先转成接收状态
	RFWriteReg(CMD_WRITE | REG_CHANNR, channel);	//信道设置
	//接收状态啊
	RFWriteStrobe(STROBE_SIDLE);
	RFWriteStrobe(STROBE_SFRX);
	RFWriteStrobe(STROBE_SRX);

	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
}

/*********************************************************************
 * 函数名:    void RFWriteBurstReg(BYTE addr,BYTE *pWriteValue,BYTE size);
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE addr，寄存器的初始地址；BYTE *pWriteValue存储写入数据值指针,
 *						BYTE size，是写入寄存器的个数
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  连续写入CC2500配置寄存器
 ********************************************************************/
void RFWriteBurstReg(BYTE addr, BYTE * pWriteValue, BYTE size)	//连续写几个寄存器的值
{
	BYTE i;
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */

	SPIPut(addr | CMD_BURST_WRITE);	//Address

	for (i = 0; i < size; i++) {
		SPIPut(*pWriteValue);
		pWriteValue++;

	}
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 
}

/*********************************************************************
 * 函数名:    void RFWriteReg(BYTE addr, BYTE value);
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE addr，寄存器地址，BYTE addr，需要配置的值
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  对CC2500配置寄存器
 ********************************************************************/
void RFWriteReg(BYTE addr, BYTE value)
{
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(addr | CMD_WRITE);	//Address

	SPIPut(value);		//Value
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */
}

/*********************************************************************
 * 函数名:    BYTE RFReadReg(BYTE addr);
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE addr，寄存器地址
 * 输出参数:  返回寄存器的值
 * 注意事项： 无
 * 功能描述:  读取CC2500配置寄存器
 ********************************************************************/
BYTE RFReadReg(BYTE addr)	//读寄存器的值
{
	BYTE value, cCmd;
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	cCmd = addr | CMD_READ;
	SPIPut(cCmd);
	value = SPIGet();
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */
	return value;
}

void RFDetectStatus(void)
{
	BYTE cState;

	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	cState = RFGetStatus(REG_MARCSTATE) & 0x1F;
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	//喂狗，防止正常情况进入复位
	if (cState == 0x11)	//接收溢出
	{
		RFWriteStrobe(STROBE_SFRX);
		RFWriteStrobe(STROBE_SRX);
	} else if (cState == 0x16)	//发送溢出
	{
		RFWriteStrobe(STROBE_SFTX);
		RFWriteStrobe(STROBE_SRX);
	} else if (cState == 0x01)	//空闲状态
	{
		RFWriteStrobe(STROBE_SRX);	//如果空闲状态转到接收状态
	} else if (cState == 0x00)	//睡眠状态
	{
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_SRX);
	}
	//remove the state of RX, modified by zhuxiaoyong
}


/*********************************************************************
 * 函数名:    BYTE RFReadRxFIFO(void)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  无
 * 输出参数:  BYTE，从RXFIFO中读取一个字节
 * 注意事项： 无
 * 功能描述:  从RXFIFO寄存器中读取一个字节
 ********************************************************************/
BYTE RFReadRxFIFO(void)		//把接收到的数据读入接收缓冲区
{
	BYTE value;
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(REG_TRXFIFO | CMD_BURST_READ);	//Address
	value = SPIGet();
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */
	return value;
}

void RFClearRxBuffer(void)
{
	RFWriteStrobe(STROBE_SIDLE);
	RFWriteStrobe(STROBE_SFRX);
	RFWriteStrobe(STROBE_SRX);
}

void RFInitSetup(void)
{

	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	//首先要复位
	RFReset();
	//延时一下
	udelay(20);
	//改变一下方式，GDO0只用作接收，GDO2用作检测载波CS
	RFWriteReg(REG_IOCFG2, 0x0E);	// 高电平说明是有载波
	RFWriteReg(REG_IOCFG0, 0x06);	// GDO0 用作接收.
	RFWriteReg(REG_PKTLEN, 0x3F);	// Packet length,最大是64字节加上长度共64字节
	RFWriteReg(REG_PKTCTRL1, 0x0C);	// CRC校验失败，自动清除缓冲区，包含RSSI和CRC校验码
	//地址校验，0x00和0xFF是广播地址
	RFWriteReg(REG_PKTCTRL0, 0x05);	// 可变长度的数据报
	printk("REG_IOCFG0 is %d\n",RFReadReg(REG_IOCFG0));
	printk("REG_PKTCTRL0 is %d\n",RFReadReg(REG_PKTCTRL0));
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	
}

/*********************************************************************
 * 函数名:    void RFReset(void)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  无
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  初始化变量
 ********************************************************************/
void RFReset(void)		//复位过后，内部寄存器和状态都回到默认
{
	//SCLK=1;
	//SO=0; //以上两条指令用作防止潜在的错误出现modified by zhuxiaoyong
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */
	udelay(60);
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */    //CS先低后高
	udelay(60);
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */     //高保持大约40us
	udelay(90);
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */    //拉低，等待晶振稳定后发送复位命令
	//稳定
	SPIPut(CMD_WRITE | STROBE_SRES);	// 发送复位命令
	//再次等待晶振稳定
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */     //结束此次操作
}

void RFSetBaudRate(BYTE BaudRate)
{
	switch (BaudRate) {
	case 0x01:		//通讯速率为2.4K
		//频率校正
		RFWriteReg(REG_FSCTRL1, 0x08);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		//通信频率设定，载波频率 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		RFWriteReg(REG_MDMCFG4, 0x86);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x83);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x03);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x22);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//设置通信速率和调制方式
		RFWriteReg(REG_DEVIATN, 0x44);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//始终出于接收状态
		RFWriteReg(REG_MCSM0, 0x18);	//MainRadio Cntrl State Machine

		RFWriteReg(REG_AGCCTRL2, 0x03);	// AGC control.
		RFWriteReg(REG_AGCCTRL1, 0x40);	// AGC control.
		RFWriteReg(REG_AGCCTRL0, 0x91);	// AGC control.

		RFWriteReg(REG_FREND1, 0x56);	// Front end RX configuration.
		RFWriteReg(REG_FREND0, 0x10);	// Front end RX configuration.

		RFWriteReg(REG_FOCCFG, 0x16);	// Freq Offset Compens. Config
		RFWriteReg(REG_BSCFG, 0x6C);	//  Bit synchronization config.

		RFWriteReg(REG_FSCAL3, 0xA9);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL2, 0x0A);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL1, 0x00);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL0, 0x11);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSTEST, 0x59);	// Frequency synthesizer cal.

		RFWriteReg(REG_TEST2, 0x88);	// Various test settings.
		RFWriteReg(REG_TEST1, 0x31);	// Various test settings.
		RFWriteReg(REG_TEST0, 0x0B);	// Various test settings.

		break;
	case 0x02:		//通讯速率为10K
		//频率校正
		RFWriteReg(REG_FSCTRL1, 0x06);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		//通信频率设定，载波频率 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		RFWriteReg(REG_MDMCFG4, 0x78);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x93);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x03);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x22);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//设置通信速率和调制方式
		RFWriteReg(REG_DEVIATN, 0x44);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//始终出于接收状态
		RFWriteReg(REG_MCSM0, 0x18);	//MainRadio Cntrl State Machine

		RFWriteReg(REG_AGCCTRL2, 0x43);	// AGC control.
		RFWriteReg(REG_AGCCTRL1, 0x40);	// AGC control.
		RFWriteReg(REG_AGCCTRL0, 0x91);	// AGC control.

		RFWriteReg(REG_FREND1, 0x56);	// Front end RX configuration.
		RFWriteReg(REG_FREND0, 0x10);	// Front end RX configuration.

		RFWriteReg(REG_FOCCFG, 0x16);	// Freq Offset Compens. Config
		RFWriteReg(REG_BSCFG, 0x6C);	//  Bit synchronization config.

		RFWriteReg(REG_FSCAL3, 0xA9);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL2, 0x0A);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL1, 0x00);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL0, 0x11);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSTEST, 0x59);	// Frequency synthesizer cal.

		RFWriteReg(REG_TEST2, 0x88);	// Various test settings.
		RFWriteReg(REG_TEST1, 0x31);	// Various test settings.
		RFWriteReg(REG_TEST0, 0x0B);	// Various test settings.

		break;
	case 0x03:		//通信速率为250K
		//通信频率设定，载波频率 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		//频率校正
		RFWriteReg(REG_FSCTRL1, 0x07);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		RFWriteReg(REG_MDMCFG4, 0x2D);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x3B);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x73);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x22);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//设置通信速率和调制方式
		RFWriteReg(REG_DEVIATN, 0x01);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//始终出于接收状态
		RFWriteReg(REG_MCSM0, 0x18);	//MainRadio Cntrl State Machine

		RFWriteReg(REG_AGCCTRL2, 0xC7);	// AGC control.
		RFWriteReg(REG_AGCCTRL1, 0x00);	// AGC control.
		RFWriteReg(REG_AGCCTRL0, 0xB2);	// AGC control.

		RFWriteReg(REG_FREND1, 0xB6);	// Front end RX configuration.
		RFWriteReg(REG_FREND0, 0x10);	// Front end RX configuration.

		RFWriteReg(REG_FOCCFG, 0x1D);	// 频率偏移补偿
		RFWriteReg(REG_BSCFG, 0x1C);	// 位同步设置

		RFWriteReg(REG_FSCAL3, 0xEA);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL2, 0x0A);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL1, 0x00);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL0, 0x11);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSTEST, 0x59);	// Frequency synthesizer cal.

		RFWriteReg(REG_TEST2, 0x88);	// Various test settings.
		RFWriteReg(REG_TEST1, 0x31);	// Various test settings.
		RFWriteReg(REG_TEST0, 0x0B);	// Various test settings.

		break;
	case 0x04:		//通讯速率为500K
		//通信频率设定，载波频率 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		//频率校正
		RFWriteReg(REG_FSCTRL1, 0x10);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		RFWriteReg(REG_MDMCFG4, 0x0E);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x3B);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x73);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x42);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//设置通信速率和调制方式
		RFWriteReg(REG_DEVIATN, 0x00);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//始终出于接收状态
		RFWriteReg(REG_MCSM0, 0x18);	//MainRadio Cntrl State Machine

		RFWriteReg(REG_FOCCFG, 0x1D);	// Freq Offset Compens. Config
		RFWriteReg(REG_BSCFG, 0x1C);	//  Bit synchronization config.

		RFWriteReg(REG_AGCCTRL2, 0xC7);	// AGC control.
		RFWriteReg(REG_AGCCTRL1, 0x40);	// AGC control.
		RFWriteReg(REG_AGCCTRL0, 0xB0);	// AGC control.

		RFWriteReg(REG_FREND1, 0xB6);	// Front end RX configuration.
		RFWriteReg(REG_FREND0, 0x10);	// Front end RX configuration.

		RFWriteReg(REG_FSCAL3, 0xEA);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL2, 0x0A);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL1, 0x00);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL0, 0x19);	// Frequency synthesizer cal.

		RFWriteReg(REG_FSTEST, 0x59);	// Frequency synthesizer cal.
		RFWriteReg(REG_TEST2, 0x88);	// Various test settings.
		RFWriteReg(REG_TEST1, 0x31);	// Various test settings.
		RFWriteReg(REG_TEST0, 0x0B);	// Various test settings.

		break;
	default:
		break;
	}
}



