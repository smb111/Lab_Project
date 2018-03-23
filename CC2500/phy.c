#include "phy.h"

//ȫ�ֱ�����Ҫ����Ӧ�����ļ��н��г�ʼ��
PHY_PIB phyPIB;			//�����PIB����
PHY_RX_BUFFER PhyRxBuffer;	//���ջ�����
BOOL bRxEvent;			//����յ�һ����������0

const BYTE ChanTab[ConstRFChannelSize] =
    { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
0xC0, 0xD0, 0xE0, 0xF0 };
const BYTE PowerTab[ConstRFPowerSize] =
    { 0x00, 0x50, 0x44, 0xC0, 0x84, 0x81, 0x46, 0x93, 0x55, 0x8D, 0xC6, 0x97,
0x6E, 0x7F, 0xA9, 0xBB, 0xFE, 0xFF };

/*********************************************************************
 * ������:    void PHYInitSetup(void)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE *pTxBuffer��д��ֵ��ָ�룬BYTE sizeд����ֽ���
 * �������:  ��
 * ע����� ��
 * ��������:  ʵ��������һЩ�������ã�phyPIB���ŵ������ʣ�У�飬RF�ļ���
 ********************************************************************/
void PHYInitSetup(void)
{
	//PIB���Գ�ʼ��
	phyPIB.phyCurrentChannel = 5;
	phyPIB.phyChannelSuppoerted = 16;	//֧��16���ŵ�
	phyPIB.phyTransmitPower = 17;	//0-17
	phyPIB.phyCCAMode = 2;	//ȡֵ��Χ1-3
	phyPIB.phyBaudRate = 2;	//1-4
	//���ջ�������ʼ��
	PhyRxBuffer.Postion.cWrite = 0;
	PhyRxBuffer.Postion.cRead = 0;

	//zxy modified
	PHYSetBaud(phyPIB.phyBaudRate);
	PHYSetTxPwr(PowerTab[phyPIB.phyTransmitPower]);
	PHYSetChannel(phyPIB.phyCurrentChannel);
	PHYSetTRxState(RF_TRX_RX);
}


/*********************************************************************
 * ������:    void PHYSetTxPower(BYTE cPower)
 * ǰ������:  SPIInit()�Ѿ�����
 * �������:  BYTE cPower������Ĺ���ֵ
 * �������:  ��
 * ע����� ��
 * ��������:  �����趨���书��ֵ
 ********************************************************************/
//���÷��͹���
WORD PHYSetTxPower(BYTE Index)
{
	if ((Index < ConstRFPowerSize) && (Index > 0)) {
		PHYSetTxPwr(PowerTab[Index - 1]);
		return Index;
	}
	return InValid_Index;
}

//���ò�����
void PHYSetBaudRate(BYTE BaudRate)
{
	if ((BaudRate < ConstRFBaudRateSize) && (BaudRate > 0)) {
		PHYSetBaud(BaudRate);
	} else {
		PHYSetBaud(BaudRate);
	}
}

//����Ƶ�ʺ��ŵ�ɨ�������
/***********************************************************************
//����Ƶ��:2433MHZ
//ÿ�����ŵ���Ƶ����ȣ�200KHZ
//��256���ŵ����»��֣�ÿ��16��Ϊһ�����ŵ���Ƶ��Ϊ200*16=3200KHZ����16���ŵ�
***********************************************************************/

WORD PHYSetChannel(BYTE Index)
{
	if ((Index < ConstRFChannelSize) && (Index > 0)) {
		PHYSetChan(ChanTab[Index - 1]);
		return Index;
	}
	return InValid_Index;
}

BYTE PHYGetLinkQuality(BYTE rssi)
{
	BYTE cResult;
	INT8S cTemp;
	if (rssi >= 128)
		cTemp = (rssi - 256) / 2 - RSSI_OFFSET;
	else
		cTemp = rssi / 2 - RSSI_OFFSET;
	cResult = (BYTE) cTemp;
	return cResult;
}

void PHYPut(BYTE cVal)
{
	//�����ջ�����д������
	PhyRxBuffer.RxBuffer[PhyRxBuffer.Postion.cWrite] = cVal;
	//�޸�д��ָ��
	PhyRxBuffer.Postion.cWrite =
	    (PhyRxBuffer.Postion.cWrite + 1) % ConstPhyRxBufferSize;
}

BYTE PHYGet(void)
{
	BYTE cReturn;
	//�ӽ��ջ������ж�ȡ����
	cReturn = PhyRxBuffer.RxBuffer[PhyRxBuffer.Postion.cRead];
	//�޸Ķ���ָ��
	PhyRxBuffer.Postion.cRead =
	    (PhyRxBuffer.Postion.cRead + 1) % ConstPhyRxBufferSize;
	return cReturn;
}

