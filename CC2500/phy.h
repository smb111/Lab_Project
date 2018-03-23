/************************************************************
文件描述：本文件主要是用来实现无限传感器网络的PHY层的基本功能，包括信道选择，
					激活射频状态，能量检测，发送和接收数据
版本信息：v1.1
修改者：朱晓勇
修改时间：2011-4-14 8:19:45
修改了某些变量不正确的名字
_RHY_RX_BUFFER 变为 _PHY_RX_BUFFER

文件描述：本文件主要是用来实现WSN的PHY层的基本功能，包括信道选择，
					激活射频状态，能量检测，发送和接收数据
版本信息：v1.0

修改时间：2008/03/
*************************************************************/

#ifndef _PHY_H
#define _PHY_H

#include "driver.h"

//定义PHY层缓冲区大小
#define ConstPhyRxBufferSize  256	//表示长度，而非数组下标
//PHY层的PIB属性结构
typedef struct _PHY_PIB {
	BYTE phyCurrentChannel;
	BYTE phyChannelSuppoerted;
	BYTE phyTransmitPower;
	BYTE phyCCAMode;
	BYTE phyBaudRate;
} PHY_PIB;
//定义信道的最大值
#define ConstRFChannelSize  16
//定义功率的最大等级数                
#define ConstRFPowerSize	18
//定义波特率的最大等级数
#define ConstRFBaudRateSize	3

#define PHY_RF_GDO0 	RF_GDO0

#define PHYSetTRxState(state)     RFSetTRxState(state)	//设置射频的收发状态
#define PHYGetTRxState() 	  RFGetTRxState	//读取射频的状态

#define PHYSetBaud(v)		RFSetBaudRate(v)
#define PHYSetTxPwr(v)		RFSetTxPower(v)
#define PHYSetChan(v)		RFSetChannel(v)
#define PHYGetChannel()	        phyPIB.phyCurrentChannel	//获取信道
#define PHYGetTxPower()		phyPIB.phyTransmitPower
#define PHYDetectStatus() 	RFDetectStatus()

#define PHYGetTxNumber()	RFGetStatus(REG_TXBYTES)&0x7F;
#define PHYGetRxStatus()	RFGetStatus(REG_RXBYTES);
#define PHYDetectChannels()	RFGetStatus(REG_PKTSTATUS)&0x10	//取出CCA位0001 0000

#define PHYClearTx()		RFClearTxBuffer()
#define PHYClearRx()		RFClearRxBuffer()	//RF FlushFIFO
#define PHYReadRx()		RFReadRxFIFO()	        //从RF接收缓冲区中读一个数

#define PHYPutTxBuffer(ptr,size) RFWriteTxFIFO(ptr,size)
#define PHYTransmitByCSMA()	   	 RFTransmitByCSMA()

#define BANDSPACE  256/16	//定义信道间的带宽
#define CHAN_BUSY   1		//定义信道忙碌标志

//PHY Constant
#define aMaxPHYPacketSize   64	//物理层最大的包长度

typedef RF_TRX_STATE PHY_TRX_STATE;

typedef struct _PHY_RX_BUFFER	//接收缓冲区
{
	BYTE RxBuffer[ConstPhyRxBufferSize];
	struct {
		WORD cWrite;	//记录写入的位置
		WORD cRead;	//记录读出的位置
	} Postion;		//记录接收缓冲区读写位置
} PHY_RX_BUFFER;

//全局变量，要在相应定义文件中进行初始化
extern PHY_PIB phyPIB;			//物理层PIB属性
extern PHY_RX_BUFFER PhyRxBuffer;	//接收缓冲区
extern BOOL bRxEvent;			//如果收到一个包，就置0

extern const BYTE ChanTab[ConstRFChannelSize];
extern const BYTE PowerTab[ConstRFPowerSize];

/*********************************************************************
 * 函数名:    void PHYInitSetup(void)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE *pTxBuffer，写入值的指针，BYTE size写入的字节数
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  实现物理层的一些基本设置，phyPIB，信道，速率，校验，RF的激活
 ********************************************************************/
void PHYInitSetup(void);


/*********************************************************************
 * 函数名:    void PHYSetTxPower(BYTE cPower)
 * 前提条件:  SPIInit()已经调用
 * 输入参数:  BYTE cPower，发射的功率值
 * 输出参数:  无
 * 注意事项： 无
 * 功能描述:  用于设定发射功率值
 ********************************************************************/
//设置发送功率
WORD PHYSetTxPower(BYTE Index);

//设置波特率
void PHYSetBaudRate(BYTE BaudRate);

//关于频率和信道扫描的事情
/***********************************************************************
//基带频率:2433MHZ
//每两个信道间频带宽度：200KHZ
//将256个信道重新划分，每隔16个为一个，信道间频宽为200*16=3200KHZ，共16个信道
***********************************************************************/

WORD PHYSetChannel(BYTE Index);

BYTE PHYGetLinkQuality(BYTE rssi);

void PHYPut(BYTE cVal);
BYTE PHYGet(void);

#endif
