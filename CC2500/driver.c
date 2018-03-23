/************************************************************
driver.h
�ļ����������ļ���Ҫ������ʵ�ֶ�CC2500�Ļ���������Ҳ��Ӳ����������֮һ
�汾��Ϣ��v1.0
�޸�ʱ�䣺2008/03/
*************************************************************/
//#define DEBUG_MODE
#include "driver.h"
#include "cc2500.h"
#include "common.h"

void SPIPut(BYTE cByte)		//����һ���ֽ�
{	
       u32 tmp ;
	while(!spi_wait_TX_ready());     //wait for the transfer buffer ready

	iowrite8(cByte,spi_rTx_data);
#ifdef DEBUG_MODE
	printk("SPIPut : %x\n",cByte);
#endif
	while(!spi_wait_RX_ready());
        
        tmp = readl(spi_rRx_data);
        
        
//	udelay(20);		//IMPORTANT!! DELAY TO LET CC2500 RECEIVE DATA   ����ʹ�� ����Ĵ���
}

BYTE SPIGet(void)		//����һ���ֽ�
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
          
    spi_flush_fifo();   /* �����λ */

    iowrite8(0xFF, spi_rTx_data);
    
    while(!spi_wait_RX_ready());
    
    tmp = ioread8(spi_rRx_data);

    ret = tmp & 0xff;
    
    return ret;
}  


/*********************************************************************
 * ������:    void RFWriteTxFIFO(BYTE *pTxBuffer,BYTE size)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE *pTxBuffer��д��ֵ��ָ�룬BYTE sizeд����ֽ���
 * �������:  ��
 * ע����� ��
 * ��������:  ��TXFIFO�Ĵ�����д������
 ********************************************************************/
void RFWriteTxFIFO(BYTE * pTxBuffer, BYTE size)
{
	BYTE i;
	
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(REG_TRXFIFO | CMD_BURST_WRITE);	/* ��Ҫ���㷢�����ˣ������� CMD_BURST_WRITE   REG_TRXFIFO */
  
	for (i = 0; i < size; i++) 
	{
	    SPIPut(pTxBuffer[i]);
	}
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 
}


/** cc2500 �������ݵ��ŵ��к���
 * �������˼·���£�
 * ����while (!status)����ŵ��Ƿ�Ϊ�գ�������ǣ��ȴ�һ��ʱ�䡣���ﲢû�в���ָ���˱ܡ�
 * ���⣬�����ֵ����ƾ�о��趨�ģ�û�о����ϸ���㡣
 * ��Σ�while (!RF_GDO0)����Ƿ�����˷���״̬�������û�н��뷢��״̬����ʱ
 * ���while (bSucceed && RF_GDO0)����Ƿ�����ɣ��������һ��ʱ�仹û�з�����ɣ���Ϊ����ʧ��
 **/
BOOL RFTransmitByCSMA(void)
{
	BYTE status;
	BOOL bSucceed = TRUE;
	struct timeval tpstart, tpend;
	unsigned long timeuse = 0;
	
       
	iowrite32(ioread32(EINT_con_addr) & 0xFFFFFF0F,EINT_con_addr);     /* ���� GPH2_1 Ϊ input ģʽ �������ж�ģʽ */  
	RFSetTRxState(RF_TRX_RX);  /* ���� cc2500 Ϊ����״̬ */

	status = RFGetStatus(REG_PKTSTATUS) & 0x10;	//ȡ��CCAλ0001 0000 

	do_gettimeofday(&tpstart);	/* ��ȡLinux�ں�ʱ����Ϊ��ʼʱ�� */ 
	
	while (!status)		//�ȴ��ŵ��Ƿ�Ϊ�գ�Ϊ��ʱ���Ų�ִ�����while    ����Ϊ��ִ�����while
	{
		if (timeuse > 80000)	//��ʱ80ms������//ZXY MODIFIED
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

	do_gettimeofday(&tpstart);	// ��ʼʱ��
	RFSetTRxState(RF_TRX_TX);
	
	while (!RF_GDO0)	/* ����жϿڵ�״ֵ̬����ʱ�жϿ�������״̬ */  //û�м�⵽ͬ����sync wordʱ��ִ�����while
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
	do_gettimeofday(&tpstart);	// ��ʼʱ��
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
		iowrite32(ioread32(EINT_con_addr) | 0xF0,EINT_con_addr);     /* ���� GPH2_1 Ϊ�ж�ģʽ */
		return TRUE;
	} else {
		RFClearTxBuffer();
		printk("One packet of data is failed to transmit\n");
		iowrite32(ioread32(EINT_con_addr) | 0xF0,EINT_con_addr);     /* ���� GPH2_1 Ϊ�ж�ģʽ */
		return FALSE;
	}
}

/* ���� cc2500 ״̬�Ĵ��� */ 
void RFSetTRxState(RF_TRX_STATE state)
{
	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	switch (state) {
	case RF_TRX_RX:
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_SRX);	//����״̬
		break;
	case RF_TRX_OFF:
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_SXOFF);	//��Ƶ�ر�
		break;
	case RF_TRX_IDLE:
		RFWriteStrobe(STROBE_SIDLE);	//����״̬
		break;
	case RF_TRX_TX:
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_STX);	//����״̬
		break;
	default:
		break;
	}
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
}

/*********************************************************************
 * ������:    void RFWriteStrobe(BYTE cmd)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE cmd����Ҫд�������
 * �������:  ��
 * ע����� �ޡ�
 * ��������:  ��CC2500����������
 ********************************************************************/
void RFWriteStrobe(BYTE addr)	//дCC2500����
{
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(addr | CMD_WRITE);	//Strobe
	
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 
}

/*********************************************************************
 * ������:    BYTE RFGetStatus(BYTE addr);
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE addr���Ĵ�����ַ
 * �������:  ����CC2500��ǰ��״̬
 * ע����� ��
 * ��������:  ��ȡCC2500״̬
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
	RFWriteBurstReg(REG_PATABLE, &power, 1);	//��������
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
}

void RFSetChannel(BYTE channel)
{

	disable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	RFWriteStrobe(STROBE_SIDLE);	                //��ת�ɽ���״̬
	RFWriteReg(CMD_WRITE | REG_CHANNR, channel);	//�ŵ�����
	//����״̬��
	RFWriteStrobe(STROBE_SIDLE);
	RFWriteStrobe(STROBE_SFRX);
	RFWriteStrobe(STROBE_SRX);

	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
}

/*********************************************************************
 * ������:    void RFWriteBurstReg(BYTE addr,BYTE *pWriteValue,BYTE size);
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE addr���Ĵ����ĳ�ʼ��ַ��BYTE *pWriteValue�洢д������ֵָ��,
 *						BYTE size����д��Ĵ����ĸ���
 * �������:  ��
 * ע����� ��
 * ��������:  ����д��CC2500���üĴ���
 ********************************************************************/
void RFWriteBurstReg(BYTE addr, BYTE * pWriteValue, BYTE size)	//����д�����Ĵ�����ֵ
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
 * ������:    void RFWriteReg(BYTE addr, BYTE value);
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE addr���Ĵ�����ַ��BYTE addr����Ҫ���õ�ֵ
 * �������:  ��
 * ע����� ��
 * ��������:  ��CC2500���üĴ���
 ********************************************************************/
void RFWriteReg(BYTE addr, BYTE value)
{
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */ 

	SPIPut(addr | CMD_WRITE);	//Address

	SPIPut(value);		//Value
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */
}

/*********************************************************************
 * ������:    BYTE RFReadReg(BYTE addr);
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE addr���Ĵ�����ַ
 * �������:  ���ؼĴ�����ֵ
 * ע����� ��
 * ��������:  ��ȡCC2500���üĴ���
 ********************************************************************/
BYTE RFReadReg(BYTE addr)	//���Ĵ�����ֵ
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
	//ι������ֹ����������븴λ
	if (cState == 0x11)	//�������
	{
		RFWriteStrobe(STROBE_SFRX);
		RFWriteStrobe(STROBE_SRX);
	} else if (cState == 0x16)	//�������
	{
		RFWriteStrobe(STROBE_SFTX);
		RFWriteStrobe(STROBE_SRX);
	} else if (cState == 0x01)	//����״̬
	{
		RFWriteStrobe(STROBE_SRX);	//�������״̬ת������״̬
	} else if (cState == 0x00)	//˯��״̬
	{
		RFWriteStrobe(STROBE_SIDLE);
		RFWriteStrobe(STROBE_SRX);
	}
	//remove the state of RX, modified by zhuxiaoyong
}


/*********************************************************************
 * ������:    BYTE RFReadRxFIFO(void)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  ��
 * �������:  BYTE����RXFIFO�ж�ȡһ���ֽ�
 * ע����� ��
 * ��������:  ��RXFIFO�Ĵ����ж�ȡһ���ֽ�
 ********************************************************************/
BYTE RFReadRxFIFO(void)		//�ѽ��յ������ݶ�����ջ�����
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
	//����Ҫ��λ
	RFReset();
	//��ʱһ��
	udelay(20);
	//�ı�һ�·�ʽ��GDO0ֻ�������գ�GDO2��������ز�CS
	RFWriteReg(REG_IOCFG2, 0x0E);	// �ߵ�ƽ˵�������ز�
	RFWriteReg(REG_IOCFG0, 0x06);	// GDO0 ��������.
	RFWriteReg(REG_PKTLEN, 0x3F);	// Packet length,�����64�ֽڼ��ϳ��ȹ�64�ֽ�
	RFWriteReg(REG_PKTCTRL1, 0x0C);	// CRCУ��ʧ�ܣ��Զ����������������RSSI��CRCУ����
	//��ַУ�飬0x00��0xFF�ǹ㲥��ַ
	RFWriteReg(REG_PKTCTRL0, 0x05);	// �ɱ䳤�ȵ����ݱ�
	printk("REG_IOCFG0 is %d\n",RFReadReg(REG_IOCFG0));
	printk("REG_PKTCTRL0 is %d\n",RFReadReg(REG_PKTCTRL0));
	enable_irq(gpio_to_irq(S5PV210_GPH2(1)));
	
}

/*********************************************************************
 * ������:    void RFReset(void)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  ��
 * �������:  ��
 * ע����� ��
 * ��������:  ��ʼ������
 ********************************************************************/
void RFReset(void)		//��λ�����ڲ��Ĵ�����״̬���ص�Ĭ��
{
	//SCLK=1;
	//SO=0; //��������ָ��������ֹǱ�ڵĴ������modified by zhuxiaoyong
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */
	udelay(60);
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */    //CS�ȵͺ��
	udelay(60);
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */     //�߱��ִ�Լ40us
	udelay(90);
	iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr);  /* cs = 0 */    //���ͣ��ȴ������ȶ����͸�λ����
	//�ȶ�
	SPIPut(CMD_WRITE | STROBE_SRES);	// ���͸�λ����
	//�ٴεȴ������ȶ�
	iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */     //�����˴β���
}

void RFSetBaudRate(BYTE BaudRate)
{
	switch (BaudRate) {
	case 0x01:		//ͨѶ����Ϊ2.4K
		//Ƶ��У��
		RFWriteReg(REG_FSCTRL1, 0x08);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		//ͨ��Ƶ���趨���ز�Ƶ�� 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		RFWriteReg(REG_MDMCFG4, 0x86);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x83);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x03);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x22);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//����ͨ�����ʺ͵��Ʒ�ʽ
		RFWriteReg(REG_DEVIATN, 0x44);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//ʼ�ճ��ڽ���״̬
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
	case 0x02:		//ͨѶ����Ϊ10K
		//Ƶ��У��
		RFWriteReg(REG_FSCTRL1, 0x06);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		//ͨ��Ƶ���趨���ز�Ƶ�� 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		RFWriteReg(REG_MDMCFG4, 0x78);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x93);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x03);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x22);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//����ͨ�����ʺ͵��Ʒ�ʽ
		RFWriteReg(REG_DEVIATN, 0x44);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//ʼ�ճ��ڽ���״̬
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
	case 0x03:		//ͨ������Ϊ250K
		//ͨ��Ƶ���趨���ز�Ƶ�� 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		//Ƶ��У��
		RFWriteReg(REG_FSCTRL1, 0x07);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		RFWriteReg(REG_MDMCFG4, 0x2D);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x3B);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x73);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x22);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//����ͨ�����ʺ͵��Ʒ�ʽ
		RFWriteReg(REG_DEVIATN, 0x01);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//ʼ�ճ��ڽ���״̬
		RFWriteReg(REG_MCSM0, 0x18);	//MainRadio Cntrl State Machine

		RFWriteReg(REG_AGCCTRL2, 0xC7);	// AGC control.
		RFWriteReg(REG_AGCCTRL1, 0x00);	// AGC control.
		RFWriteReg(REG_AGCCTRL0, 0xB2);	// AGC control.

		RFWriteReg(REG_FREND1, 0xB6);	// Front end RX configuration.
		RFWriteReg(REG_FREND0, 0x10);	// Front end RX configuration.

		RFWriteReg(REG_FOCCFG, 0x1D);	// Ƶ��ƫ�Ʋ���
		RFWriteReg(REG_BSCFG, 0x1C);	// λͬ������

		RFWriteReg(REG_FSCAL3, 0xEA);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL2, 0x0A);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL1, 0x00);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSCAL0, 0x11);	// Frequency synthesizer cal.
		RFWriteReg(REG_FSTEST, 0x59);	// Frequency synthesizer cal.

		RFWriteReg(REG_TEST2, 0x88);	// Various test settings.
		RFWriteReg(REG_TEST1, 0x31);	// Various test settings.
		RFWriteReg(REG_TEST0, 0x0B);	// Various test settings.

		break;
	case 0x04:		//ͨѶ����Ϊ500K
		//ͨ��Ƶ���趨���ز�Ƶ�� 2433 MHz
		RFWriteReg(REG_FREQ2, 0x5D);	// Freq control word, high byte
		RFWriteReg(REG_FREQ1, 0x93);	// Freq control word, mid byte.
		RFWriteReg(REG_FREQ0, 0xB1);	// Freq control word, low byte.

		//Ƶ��У��
		RFWriteReg(REG_FSCTRL1, 0x10);	// Freq synthesizer control.
		RFWriteReg(REG_FSCTRL0, 0x00);	// Freq synthesizer control.

		RFWriteReg(REG_MDMCFG4, 0x0E);	// Modem configuration.
		RFWriteReg(REG_MDMCFG3, 0x3B);	// Modem configuration.
		RFWriteReg(REG_MDMCFG2, 0x73);	// Modem configuration.
		RFWriteReg(REG_MDMCFG1, 0x42);	// Modem configuration.
		RFWriteReg(REG_MDMCFG0, 0xF8);	// Modem configuration.

		//����ͨ�����ʺ͵��Ʒ�ʽ
		RFWriteReg(REG_DEVIATN, 0x00);	// Modem dev (when FSK mod en)
		RFWriteReg(REG_MCSM1, 0x3F);	//ʼ�ճ��ڽ���״̬
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



