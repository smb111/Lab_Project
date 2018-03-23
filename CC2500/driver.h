/************************************************************
driver.h
文件描述：本文件主要是用来实现对CC2500的基本操作，也是硬件驱动部分之一
版本信息：v1.0
修改时间：2011年11月2日
作者：朱晓勇
*************************************************************/
#ifndef _DRIVER_H
#define _DRIVER_H


#include "cc2500.h"
#include "common.h"

//RSSI偏移
#define RSSI_OFFSET 71

typedef enum _RF_TRX_STATE	//射频芯片的状态
{
	RF_TRX_RX,
	RF_TRX_OFF,
	RF_TRX_IDLE,
	RF_TRX_TX
} RF_TRX_STATE;

void SPIPut(BYTE cByte);		//发送一个字节
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
void RFWriteBurstReg(BYTE addr, BYTE * pWriteValue, BYTE size);	//连续写几个寄存器的值
void RFWriteReg(BYTE addr, BYTE value);
void RFDetectStatus(void);
BYTE RFReadRxFIFO(void);
void RFClearRxBuffer(void);
void RFInitSetup(void);
void RFReset(void);		//复位过后，内部寄存器和状态都回到默认


#endif
