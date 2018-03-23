/************************************************************
driver.h
�ļ����������ļ���Ҫ������ʵ�ֶ�CC2500�Ļ���������Ҳ��Ӳ����������֮һ
�汾��Ϣ��v1.0
�޸�ʱ�䣺2011��11��2��
���ߣ�������
*************************************************************/
#ifndef _DRIVER_H
#define _DRIVER_H


#include "cc2500.h"
#include "common.h"

//RSSIƫ��
#define RSSI_OFFSET 71

typedef enum _RF_TRX_STATE	//��ƵоƬ��״̬
{
	RF_TRX_RX,
	RF_TRX_OFF,
	RF_TRX_IDLE,
	RF_TRX_TX
} RF_TRX_STATE;

void SPIPut(BYTE cByte);		//����һ���ֽ�
void RFWriteTxFIFO(BYTE * pTxBuffer, BYTE size);
BOOL RFTransmitByCSMA(void);
void RFSetTRxState(RF_TRX_STATE state);
void RFClearTxBuffer(void);
BYTE RFGetStatus(BYTE addr);
void RFWriteStrobe(BYTE addr);
BYTE SPIGet(void);
void RFSetBaudRate(BYTE BaudRate);
void RFSetTxPower(BYTE power);
void RFSetChannel(BYTE channel);
void RFWriteBurstReg(BYTE addr, BYTE * pWriteValue, BYTE size);	//����д�����Ĵ�����ֵ
void RFWriteReg(BYTE addr, BYTE value);
void RFDetectStatus(void);
BYTE RFReadRxFIFO(void);
void RFClearRxBuffer(void);
void RFInitSetup(void);
void RFReset(void);		//��λ�����ڲ��Ĵ�����״̬���ص�Ĭ��


#endif
