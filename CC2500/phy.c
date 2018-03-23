#include "phy.h"

//全局变量，要在相应定义文件中进行初始化
PHY_PIB phyPIB;			//物理层PIB属性
PHY_RX_BUFFER PhyRxBuffer;	//接收缓冲区
BOOL bRxEvent;			//如果收到一个包，就置0

const BYTE ChanTab[ConstRFChannelSize] =
    { 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0,
0xC0, 0xD0, 0xE0, 0xF0 };
const BYTE PowerTab[ConstRFPowerSize] =
    { 0x00, 0x50, 0x44, 0xC0, 0x84, 0x81, 0x46, 0x93, 0x55, 0x8D, 0xC6, 0x97,
0x6E, 0x7F, 0xA9, 0xBB, 0xFE, 0xFF };

/*********************************************************************
 * 函数名:    void PHYInitSetup(void)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE *pTxBuffer，写入值的指针，BYTE size写入的字节数
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  实现物理层的一些基本设置，phyPIB，信道，速率，校验，RF的激活
 ********************************************************************/
void PHYInitSetup(void)
{
	//PIB属性初始化
	phyPIB.phyCurrentChannel = 5;
	phyPIB.phyChannelSuppoerted = 16;	//支持16个信道
	phyPIB.phyTransmitPower = 17;	//0-17
	phyPIB.phyCCAMode = 2;	//取值范围1-3
	phyPIB.phyBaudRate = 2;	//1-4
	//接收缓冲区初始化
	PhyRxBuffer.Postion.cWrite = 0;
	PhyRxBuffer.Postion.cRead = 0;

	//zxy modified
	PHYSetBaud(phyPIB.phyBaudRate);
	PHYSetTxPwr(PowerTab[phyPIB.phyTransmitPower]);
	PHYSetChannel(phyPIB.phyCurrentChannel);
	PHYSetTRxState(RF_TRX_RX);
}


/*********************************************************************
 * 函数名:    void PHYSetTxPower(BYTE cPower)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE cPower，发射的功率值
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  用于设定发射功率值
 ********************************************************************/
//设置发送功率
WORD PHYSetTxPower(BYTE Index)
{
	if ((Index < ConstRFPowerSize) && (Index > 0)) {
		PHYSetTxPwr(PowerTab[Index - 1]);
		return Index;
	}
	return InValid_Index;
}

//设置波特率
void PHYSetBaudRate(BYTE BaudRate)
{
	if ((BaudRate < ConstRFBaudRateSize) && (BaudRate > 0)) {
		PHYSetBaud(BaudRate);
	} else {
		PHYSetBaud(BaudRate);
	}
}

//关于频率和信道扫描的事情
/***********************************************************************
//基带频率:2433MHZ
//每两个信道间频带宽度：200KHZ
//将256个信道重新划分，每隔16个为一个，信道间频宽为200*16=3200KHZ，共16个信道
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
	//往接收缓冲区写入数据
	PhyRxBuffer.RxBuffer[PhyRxBuffer.Postion.cWrite] = cVal;
	//修改写入指针
	PhyRxBuffer.Postion.cWrite =
	    (PhyRxBuffer.Postion.cWrite + 1) % ConstPhyRxBufferSize;
}

BYTE PHYGet(void)
{
	BYTE cReturn;
	//从接收缓冲区中读取数据
	cReturn = PhyRxBuffer.RxBuffer[PhyRxBuffer.Postion.cRead];
	//修改读出指针
	PhyRxBuffer.Postion.cRead =
	    (PhyRxBuffer.Postion.cRead + 1) % ConstPhyRxBufferSize;
	return cReturn;
}

