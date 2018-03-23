/************************************************************
�ļ����������ļ���Ҫ������ʵ�����޴����������PHY��Ļ������ܣ������ŵ�ѡ��
					������Ƶ״̬��������⣬���ͺͽ�������
�汾��Ϣ��v1.1
�޸��ߣ�������
�޸�ʱ�䣺2011-4-14 8:19:45
�޸���ĳЩ��������ȷ������
_RHY_RX_BUFFER ��Ϊ _PHY_RX_BUFFER

�ļ����������ļ���Ҫ������ʵ��WSN��PHY��Ļ������ܣ������ŵ�ѡ��
					������Ƶ״̬��������⣬���ͺͽ�������
�汾��Ϣ��v1.0

�޸�ʱ�䣺2008/03/
*************************************************************/

#ifndef _PHY_H
#define _PHY_H

#include "driver.h"

//����PHY�㻺������С
#define ConstPhyRxBufferSize  256	//��ʾ���ȣ����������±�
//PHY���PIB���Խṹ
typedef struct _PHY_PIB {
	BYTE phyCurrentChannel;
	BYTE phyChannelSuppoerted;
	BYTE phyTransmitPower;
	BYTE phyCCAMode;
	BYTE phyBaudRate;
} PHY_PIB;
//�����ŵ������ֵ
#define ConstRFChannelSize  16
//���幦�ʵ����ȼ���                
#define ConstRFPowerSize	18
//���岨���ʵ����ȼ���
#define ConstRFBaudRateSize	3

#define PHY_RF_GDO0 	RF_GDO0

#define PHYSetTRxState(state)     RFSetTRxState(state)	//������Ƶ���շ�״̬
#define PHYGetTRxState() 	  RFGetTRxState	//��ȡ��Ƶ��״̬

#define PHYSetBaud(v)		RFSetBaudRate(v)
#define PHYSetTxPwr(v)		RFSetTxPower(v)
#define PHYSetChan(v)		RFSetChannel(v)
#define PHYGetChannel()	        phyPIB.phyCurrentChannel	//��ȡ�ŵ�
#define PHYGetTxPower()		phyPIB.phyTransmitPower
#define PHYDetectStatus() 	RFDetectStatus()

#define PHYGetTxNumber()	RFGetStatus(REG_TXBYTES)&0x7F;
#define PHYGetRxStatus()	RFGetStatus(REG_RXBYTES);
#define PHYDetectChannels()	RFGetStatus(REG_PKTSTATUS)&0x10	//ȡ��CCAλ0001 0000

#define PHYClearTx()		RFClearTxBuffer()
#define PHYClearRx()		RFClearRxBuffer()	//RF FlushFIFO
#define PHYReadRx()		RFReadRxFIFO()	        //��RF���ջ������ж�һ����

#define PHYPutTxBuffer(ptr,size) RFWriteTxFIFO(ptr,size)
#define PHYTransmitByCSMA()	   	 RFTransmitByCSMA()

#define BANDSPACE  256/16	//�����ŵ���Ĵ���
#define CHAN_BUSY   1		//�����ŵ�æµ��־

//PHY Constant
#define aMaxPHYPacketSize   64	//��������İ�����

typedef RF_TRX_STATE PHY_TRX_STATE;

typedef struct _PHY_RX_BUFFER	//���ջ�����
{
	BYTE RxBuffer[ConstPhyRxBufferSize];
	struct {
		WORD cWrite;	//��¼д���λ��
		WORD cRead;	//��¼������λ��
	} Postion;		//��¼���ջ�������дλ��
} PHY_RX_BUFFER;

//ȫ�ֱ�����Ҫ����Ӧ�����ļ��н��г�ʼ��
extern PHY_PIB phyPIB;			//�����PIB����
extern PHY_RX_BUFFER PhyRxBuffer;	//���ջ�����
extern BOOL bRxEvent;			//����յ�һ����������0

extern const BYTE ChanTab[ConstRFChannelSize];
extern const BYTE PowerTab[ConstRFPowerSize];

/*********************************************************************
 * ������:    void PHYInitSetup(void)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE *pTxBuffer��д��ֵ��ָ�룬BYTE sizeд����ֽ���
 * �������:  ��
 * ע����� ��
 * ��������:  ʵ��������һЩ�������ã�phyPIB���ŵ������ʣ�У�飬RF�ļ���
 ********************************************************************/
void PHYInitSetup(void);


/*********************************************************************
 * ������:    void PHYSetTxPower(BYTE cPower)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE cPower������Ĺ���ֵ
 * �������:  ��
 * ע����� ��
 * ��������:  �����趨���书��ֵ
 ********************************************************************/
//���÷��͹���
WORD PHYSetTxPower(BYTE Index);

//���ò�����
void PHYSetBaudRate(BYTE BaudRate);

//����Ƶ�ʺ��ŵ�ɨ�������
/***********************************************************************
//����Ƶ��:2433MHZ
//ÿ�����ŵ���Ƶ����ȣ�200KHZ
//��256���ŵ����»��֣�ÿ��16��Ϊһ�����ŵ���Ƶ��Ϊ200*16=3200KHZ����16���ŵ�
***********************************************************************/

WORD PHYSetChannel(BYTE Index);

BYTE PHYGetLinkQuality(BYTE rssi);

void PHYPut(BYTE cVal);
BYTE PHYGet(void);

#endif
