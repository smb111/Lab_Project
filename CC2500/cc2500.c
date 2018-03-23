#include "cc2500.h"
#include "phy.h"
#include "common.h"

//#define DEBUG_MODE
#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)
#define CC2500_NAME "cc2500dev"

struct cdev cc2500_cdev; 
dev_t cc2500_devno;
static struct class *CC2500_class;  //创建一个类，申请/dev下设备的时候使用。
struct semaphore sem;		//define a semaphore
int RxDataReadyFlag = 0;
static DECLARE_WAIT_QUEUE_HEAD(Rx_waitq);	//define a wait queue


/* 定义中断口的配置寄存器和数据寄存器 */
#define GPH2CON 0xe0200c40
#define GPH2DAT 0xe0200c44
u32 *EINT_con_addr;
u32 *EINT_data_addr;      /// 用于检测中断口的状态的 RF_GDO1

/* 定义硬件SPI  IO  口复用 的配置寄存器 */
#define GPBCON 0xE0200040
u32 *cc2500_con_addr;

/* SPI各配置寄存器定义 */
#define SPI_BASE     0xE1300000
#define S5PV210_SPI(x)   (SPI_BASE + x)
#define S5PV210_SPICON     S5PV210_SPI(0x00)   /* CH_CFG0          : SPI Port 0 Configuration Register */
#define S5PV210_SPIFBCLK   S5PV210_SPI(0x2c)   /* FB_CLK_SEL0 	   : SPI Port 0 Feedback Clock Selection Register */
#define S5PV210_SPICLK     S5PV210_SPI(0x04)   /* CLK_CFG0    	   : SPI Port 0 Clock Configuration Register */
#define S5PV210_SPIMOD     S5PV210_SPI(0x08)   /* MODE_CFG0        : SPI Port 0 FIFO Control Register */
#define S5PV210_SPIENINT   S5PV210_SPI(0x10)   /* SPI_INT_EN0      : SPI Port 0 Interrupt Enable Register */
#define S5PV210_SPICSCON   S5PV210_SPI(0x0c)   /* CS_REG0          : SPI Port 0 Slave Selection Control register */
#define S5PV210_PACKETCNT  S5PV210_SPI(0x20)   /* PACKET_CNT_REG0  : SPI Port 0 Packet Count Register */
#define S5PV210_SPIPENDCLR S5PV210_SPI(0x24)   /* PENDING_CLR_REG0 : SPI Port 0 Interrupt Pending Clear Register */
#define S5PV210_SPISTATUS  S5PV210_SPI(0x14)   /* SPI_STATUS0      : SPI Port 0 Status register */
#define S5PV210_SPITXDAT   S5PV210_SPI(0x18)   /* SPI_TX_DATA0     : SPI Port 0 TX Data Register */
#define S5PV210_SPIRXDAT   S5PV210_SPI(0x1c)   /* SPI_RX_DATA0     : SPI Port 0 RX Data Register */
u32 *spi_rcon_addr;
u32 *spi_rclk_addr;
u32 *spi_rfbclk_addr;
u32 *spi_rmode_addr;
u32 *spi_renINT_addr;
u32 *spi_rcscon_addr;
u32 *spi_rTx_data;
u32 *spi_rRx_data;
u32 *spi_rstatus_addr;
u32 *spi_rpending_clr_addr;
u32 *spi_rpacket_cnt_addr;

/* LAN 和PAEN 寄存器定义 */
#define CC2500_PALAN_CON  0xE0200C60                 /* GPH3CON, R/W, Address = 0xE020_0C60  */
#define CC2500_PALAN_DAT  0xE0200C64	            /* GPH3DAT, R/W, Address = 0xE020_0C64  */
u32 *cc2500_palan_con;
u32 *cc2500_palan_dat;

/** handle the interrupt
  * 注意事项： 如果接收到一个错误的数据包后，进行错误处理是关键中的关键
  * 功能描述:  把接收到的数据包都放到自定义的接收缓冲区中去
  */
static irqreturn_t CC2500_interrupt(int irq, void *dev_id)
{
	WORD i;
	WORD cRest = 0;
	BYTE cTemp;
	BYTE cSize;
	BYTE cValue, cResult;

	cValue = PHYGetRxStatus();
	cResult = cValue;
#ifdef DEBUG_MODE	
       printk(" Get into the interrupt !!! \n");
	printk("Value is : %x ",cValue);
#endif
	
	cValue = cValue & 0x80;
	cResult = cResult & 0x7F;

	if (cValue == 0x80)	//接收溢出
	{
		  PHYClearRx();
		  return FALSE;
	}
	//如果rx位空
/*	if (!cResult)
	{
		PHYClearRx();
		printk("CC2500 RX error, rx fifo is empty\n");
		return FALSE;
	}
*/
	cSize = PHYReadRx();
	printk("The cSize is : %x\n",cSize);
	cSize = cSize + 2;	//长度包含2个字节的CRC和RSSI                   
	if ((cSize >= 64) || (cSize < 7))
	{
		  PHYClearRx();
		  return FALSE;
	}
	//接收缓冲区内，是否有空
	if (PhyRxBuffer.Postion.cWrite <= PhyRxBuffer.Postion.cRead)  // 缓冲区满了
		// cRest = PhyRxBuffer.Postion.cWrite - PhyRxBuffer.Postion.cRead - 1;	//减一的目的是去掉长度字节
		cRest = -1;
	else
		cRest = ConstPhyRxBufferSize - PhyRxBuffer.Postion.cWrite + PhyRxBuffer.Postion.cRead - 1;

	if (cRest >= cSize + 1)	//有足够的空间,这是个循环队列，留一个空的用来判断是满还是空
	  {
		  PHYPut(cSize);	//写入长度
		  for (i = 0; i < cSize; i++)
		    {
			    cTemp = PHYReadRx();
			    PHYPut(cTemp);

			    #ifdef DEBUG_MODE
			    printk("cTemp is : %x\n",cTemp);
			    #endif
				
		    }
		  //data ready
		  RxDataReadyFlag++;
		  wake_up_interruptible(&Rx_waitq);
		  return TRUE;
	} else			//若缓冲区满，则清除缓冲区，和消息
	  {
		  PHYClearRx();
		  PhyRxBuffer.Postion.cWrite = 0;
		  PhyRxBuffer.Postion.cRead = 0;
		  return FALSE;
	  }

	return IRQ_RETVAL(IRQ_HANDLED);
}

static ssize_t CC2500_write(struct file *filp, const char *buf, size_t count,loff_t * f_ops)
{
    __u8 buffTemp[ConstPhyRxBufferSize];
    __u8 ret;
    u32  value;
	
    SetSendMode;  //  设置发送模式	
    
#ifdef DEBUG_MODE
    if (access_ok(VERIFY_READ, buf, count))  
        printk("this addr is valid !\n") ;
    else /* security hole - plug it */  
       printk("this addr is not valid !\n") ;
#endif

    if (count > ConstPhyRxBufferSize)
    {
        printk("Write failed because data too much\n");
        SetRecvMode;
        return -EFAULT;
    }
	  
    /*这里注意
      数据结构的交互很重要
      我们这样规定：传递给下层的数据，在数据的最开始，需要包含数据的数量
      比如，需要传送0x11,0x22,0x33三个数据
      
      那么，上层需要传递过来的应该是0x03,0x11,0x22,0x33四个字节
      
      因为CC2500里面规定，长度字节不包括本身
      但是，作为write函数来说，需要写入的，实际上是4个字节。
      因此，这里write的函数的参数，最后是count，比buf[0]要大1。
      从用户空间复制count字节的数据，放入缓冲区
      */
    value = __copy_from_user(&buffTemp[0], buf, count);

#ifdef DEBUG_MODE
    int i;

    for(i = 0;i < count;i++)
         printk("%x   ",buffTemp[i]);
    printk("\n");
#endif
	
    if(count != buffTemp[0]+1)
    {
        printk("value is : %d\n",count);
	 printk("Write failed. Data num doesn't match,count is %d,buffer[0] is %d\n",count,buffTemp[0]);
	 SetRecvMode;
	 return -EFAULT;
    }
	
    /* 信号量减去1*/
    down(&sem);

    PHYPutTxBuffer(&buffTemp[0],count);       // 写入CC2500  的发送缓冲区 
    
    ret = PHYTransmitByCSMA();
    
    up(&sem); 
	
    SetRecvMode; 
	
    return ret;
}

static ssize_t CC2500_read(struct file *filp, char *buff, size_t count, loff_t * offp)
{
	unsigned long err;
	__u8 buffTemp[ConstPhyRxBufferSize];
	long cTemp;
	__u8 i;
	//如果没有接收到数据，或者数据出错
	if (RxDataReadyFlag <= 0)
	  {
	  		//如果是非阻塞的方式访问，返回错误
	  		//如果阻塞方式访问，放入等待队列
		  if (filp->f_flags & O_NONBLOCK)
			  return -EAGAIN;
		  else
		    {
			    //原型是wait_event_interruptible(queue, condition)	
			    //进程睡眠，其醒来的条件是当RxDataFlag大于0		    
			    wait_event_interruptible(Rx_waitq, (RxDataReadyFlag > 0));
		    }
	  }
	  
	  //如果接收到了cc2500的数据，才往上层返回
	  //同时，这里之前检测过了wait_event_interruptible事件。
	 if(RxDataReadyFlag > 0)
	 {
	     //标志减去1，代表已经读取了数据
	     RxDataReadyFlag--;
	     //读取长度
	     cTemp = PHYGet();   /* 读取接收缓冲区里面的第一个字节，表示接收的数据字节数 ( 但是改长度不包括本身 )每执行一次 PHYGet 读指针自增一次*/
	     for (i = 0; i < cTemp; i++)
	     {
		  buffTemp[i] = PHYGet();
	     }
	     //传送回用户空间
	     err = copy_to_user((void *)buff, buffTemp, cTemp);   /* 如果数据拷贝成功，则返回零；否则，返回没有拷贝成功的数据字节数。 */
	     //返回读取到的数目
	     return err ? -EFAULT : min((int)cTemp, (int)count);  /* cTemp 比count 小1 */
	}
	return -1;
}

static long CC2500_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{

	int err = 0;
	int ret = 0;
	int ioctlarg = 0;
	//如果不是给CC2500的命令
	if (_IOC_TYPE(cmd) != CC2500_IOC_MAGIC)
	{
		    printk("This cmd is not for CC2500 ! \n"); 
		    return -ENOTTY;
	}	
	//如果命令数太大，超出了所有的命令
	if (_IOC_NR(cmd) > CC2500_IOC_MAXNR)
	{
                   printk("  \n");
	            return -ENOTTY;
	}
	//使用int access_ok(int type, const void *addr, unsigned long size)来检查一个指针地址是否可以访问
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	//if err, return false
	if (err)
		return -EFAULT;
	
	//看看命令
	switch (cmd)
	  {
	  case CC2500_IOC_PHY_DETECT_STATUS:
		  PHYDetectStatus();
		  return 0;
		  break;
	  case CC2500_IOC_PHY_GET_BAUDRATE:  /* 获取cc2500向信道发送数据波特率 */
		  ret = put_user(phyPIB.phyBaudRate, (int *)arg);
		  return ret;
		  break;
	  case CC2500_IOC_PHY_SET_BAUDRATE:  /* 设置cc2500向信道发送数据波特率 */
		  get_user(ioctlarg, (int *)arg);
		  phyPIB.phyBaudRate = ioctlarg;
		  printk("CC2500_IOC_PHY_SET_BAUDRATE: %d\n",phyPIB.phyBaudRate);
		  PHYSetBaud(phyPIB.phyBaudRate);
		  break;
	  case CC2500_IOC_PHY_GET_TXPOWER:   /* 获取 cc2500 的发送功率 */
		  ret = put_user(PHYGetTxPower(), (int *)arg);
		  return ret;
		  break;
	  case CC2500_IOC_PHY_SET_TXPOWER:   /* 设置发送功率 */
		  get_user(ioctlarg, (int *)arg);
		  phyPIB.phyTransmitPower = ioctlarg;
		  printk("CC2500_IOC_PHY_SET_TXPOWER: %d\n",phyPIB.phyTransmitPower);
		  PHYSetTxPwr(PowerTab[phyPIB.phyTransmitPower]);
		  break;
	  case CC2500_IOC_PHY_GET_CHANNEL:  /* 获取信道 */
		  ret = put_user(PHYGetChannel(), (int *)arg);
		  return ret;
		  break;
	  case CC2500_IOC_PHY_SET_CHANNEL:  /* 设置通信信道 */
		  get_user(ioctlarg, (int *)arg);
		  phyPIB.phyCurrentChannel = ioctlarg;
		  printk("CC2500_IOC_PHY_SET_CHANNEL: %d\n",phyPIB.phyCurrentChannel);
		  PHYSetChannel(phyPIB.phyCurrentChannel);
		  break;
	  default:
		  return -ENOTTY;
	  }
	return ret;
}

static int CC2500_close(struct inode *inode, struct file *file)
{
	iounmap(spi_rcon_addr);
	iounmap(spi_rfbclk_addr);
	iounmap(spi_rclk_addr);
	iounmap(spi_rmode_addr);
	iounmap(spi_renINT_addr);
	iounmap(spi_rpending_clr_addr);
	iounmap(spi_rpacket_cnt_addr);
	iounmap(spi_rcscon_addr);
	iounmap(spi_rstatus_addr);
	iounmap(spi_rRx_data);
	iounmap(spi_rTx_data);
	
	iounmap(EINT_con_addr);
	iounmap(EINT_data_addr);
	iounmap(cc2500_con_addr);  
	
	free_irq(gpio_to_irq(S5PV210_GPH2(1)), 0);  /* 注销中断处理（中断号） */
	
	return 0;
}

int CC2500_open (struct inode *inode, struct file *file)
{    
    u32 spi_modecfg = 0;
    u32 spi_packet = 0;     
    
    spi_rcon_addr = ioremap(S5PV210_SPICON,4);
    spi_rfbclk_addr = ioremap(S5PV210_SPIFBCLK,4);
    spi_rclk_addr = ioremap(S5PV210_SPICLK,4);
    spi_rmode_addr = ioremap(S5PV210_SPIMOD,4);
    spi_renINT_addr = ioremap(S5PV210_SPIENINT,4);
    spi_rpending_clr_addr = ioremap(S5PV210_SPIPENDCLR,4);
    spi_rpacket_cnt_addr = ioremap(S5PV210_PACKETCNT,4);
    spi_rcscon_addr = ioremap(S5PV210_SPICSCON,4); 
    spi_rstatus_addr = ioremap(S5PV210_SPISTATUS,4);     
    spi_rRx_data   = ioremap(S5PV210_SPIRXDAT,4);
    spi_rTx_data = ioremap(S5PV210_SPITXDAT,4);
    
    EINT_con_addr  = ioremap(GPH2CON,4);
    EINT_data_addr = ioremap(GPH2DAT,4);
    cc2500_con_addr = ioremap(GPBCON,4);
    
    /* 1. 端口功能配置初始化 */
      
    // iowrite32(ioread32(EINT_con_addr)|0xF0,EINT_con_addr);      /* 配置GPH2_1为中断模式 */
    
    iowrite32(ioread32(EINT_con_addr) & (~0xF0),EINT_con_addr);    /* 配置GPH2_1为输入模式 */
    
    /* 2. 配置SPI0的4个端口模式 */
    
    iowrite32( (ioread32(cc2500_con_addr)|0x00002222) & 0xFFFF2222 ,cc2500_con_addr);   /* 配置 GPB0:SPI0->CLK GPB1:SPI0->nSS GPB2:SPI0->MISO GPB3:SPI0->MOSI*/
    
    /* 3. 设置传输类型 */
    
    iowrite8(ioread8(spi_rcon_addr) & ~(1<<4) & ~(1<<3) & ~(1<<2),spi_rcon_addr);  /* 1. Disable High Speed */ /*   2. Master  3. CPOL : Active High 4. CPHA_Format A */
    
    /* 4. 禁用 Feedback Clock */
    
    iowrite8(0x0,spi_rfbclk_addr);
       
    /* 5. 使能SPI时钟 */
    
    iowrite16(0x01ff,spi_rclk_addr);
    
    /* 6. 设置 SPI MODE Configuration register */
    
    spi_modecfg = (0<<29) | (0<<17);          /* 17、29位清零 */

    spi_modecfg |= (0<<1) | (0<<0) | (0<<2);  /* 0-2位清零 */

    spi_modecfg &= ~( 0x3f << 5);             /* 6-10位清零 */

    spi_modecfg |= ( 0x1 << 5);               /* set bit 5 : Tx FIFO trigger level in INT mode 1 Byte */ 

    spi_modecfg &= ~( 0x3f << 11);            /* 11-16位清零 */ 

    spi_modecfg |= ( 0x1 << 11);              /* set bit 11 */

    spi_modecfg &= ~( 0x3ff << 19);           /* 19-28位清零 */    

    spi_modecfg |= ( 0x1 << 19);              /* set bit 19 Counting of Tailing Bytes 1 Byte */ 
    
    iowrite32(spi_modecfg,spi_rmode_addr);
    
    /* 7. 设置 SPI INT_EN 中断模式 */
    
    iowrite8(0x00,spi_renINT_addr);
    
    iowrite8(0x1f,spi_rpending_clr_addr);
    
    /* 8. 设置 PACKET_CNT_REG register if necessary */
   
    spi_packet |= (1<<16);  

    spi_packet |= 0xffff; 
    
    iowrite32(spi_packet,spi_rpacket_cnt_addr);
    
    /* 9. 设置 Tx / RX Channel on */
    
    iowrite8(ioread8(spi_rcon_addr) | (1<<0) | (1<<1),spi_rcon_addr);
    
    /* 10. CC2500 的初始化,以及物理层的一些设置 */

    RFInitSetup();

    PHYInitSetup();  /* 实现物理层的一些基本设置，PhyRxBuffer , phyPIB，信道，速率，校验，RF的激活 */

    /* 11. initialize the GPH2_1 here because when CC2500 is reset, GDO0 pin is configured as CLK output at a frequency of 135k-141k, which will cause too many interrupts */
    
    iowrite32(ioread32(EINT_con_addr)|0xF0,EINT_con_addr);      /* 配置GPH2_1为中断模式 */

    /* 12. 注册中断 */
    
    if( request_irq(gpio_to_irq(S5PV210_GPH2(1)),CC2500_interrupt, IRQF_TRIGGER_FALLING,"cc2500", 0) != 0 )   /* 按键下降沿中断 IRQ_EINT16_31 */

    	printk(KERN_ALERT "request GPH2_1 IRQ failed\n");

    /* 13. 初始化信号量 */
    sema_init(&sem,1);

    /* 14. set state to RX 设置cc2500 状态寄存器 */
    RFSetTRxState(RF_TRX_RX);

    /* 15. cs = 1 */ 

    iowrite16(ioread16(spi_rcscon_addr) | (1<<0) ,spi_rcscon_addr);  /* cs = 1 */ 

   /* 16. 初始化端口 LAN  和PAEN  为输出模式GPH3_2 (LNA)& GPH3_3  */

   cc2500_palan_con = ioremap(CC2500_PALAN_CON,4);
   cc2500_palan_dat = ioremap(CC2500_PALAN_DAT,4) ;

   iowrite32( (ioread32(cc2500_palan_con)&0xffff00ff) | (1<<12) | (1<<8),cc2500_palan_con );
   
    SetRecvMode;  //  设置接收模

   printk("open complete !\n");
    
   return 0;
}

static struct file_operations cc2500_ops = 
{  
    .owner              = THIS_MODULE,
    .open		= CC2500_open,
    .unlocked_ioctl     = CC2500_ioctl,
    .write              = CC2500_write,
    .read               = CC2500_read,
    .release            = CC2500_close,
};

static int cc2500_init(void)
{  
    
    /* 1. 注册字符设备 */
    
    cdev_init(&cc2500_cdev, &cc2500_ops);
    
    alloc_chrdev_region(&cc2500_devno,0,1,"cc2500");
    
    if(!cc2500_devno)
    
        printk(KERN_ALERT "alloc_char_dev_regin failed !\n");
        
    if ( cdev_add(&cc2500_cdev, cc2500_devno, 1) < 0 )
    
        printk(KERN_ALERT "Register cdev failed !\n");
        
    /* 自动在/dev 目录下面创建节点 */
    CC2500_class= class_create(THIS_MODULE, CC2500_NAME);
     
    if(IS_ERR(CC2500_class)) 
    {
        printk("ERR:cannot create a CC2500_class");
        cdev_del(&cc2500_cdev);
    }
               
    device_create(CC2500_class,NULL,MKDEV(MAJOR(cc2500_devno), 0), NULL, CC2500_NAME);	
    
    return 0;
}

void cc2500_exit(void)
{
    /* 1. 删除 device_create() 创建的设备 */
    
    device_destroy(CC2500_class,cc2500_devno); 
    
    /* 2. 删除 类CC2500_class */
    
    class_destroy(CC2500_class); 
	
    /* 3. 注销字符设备 */
    
    cdev_del(&cc2500_cdev);
    
    /* 4.释放宝贵的主设备号 */
    
    unregister_chrdev_region(cc2500_devno,1);
    
    /* 注销中断处理（中断号） 放到了关闭cc2500设备文件里面 */
    // free_irq(gpio_to_irq(S5PV210_GPH2(1)), 0);
 
    printk(KERN_ALERT "CC2500 bus released\n");
}

/**
 * ready to Transmit Data ?
 **/
bool spi_wait_TX_ready(void)  
{  
    u32 loops = msecs_to_loops(10);
    u32 val = 0;
    do{  
          val = ioread32(spi_rstatus_addr);
      }while(!((val & (1<<25)) && (val & (1<<0))) && loops--);  /* 21 bit and 0 bit statue is 1   or   loops == 0  break the while */
          
    if(loops == 0)  
        return false;  
    else  
        return true;  
}

/**
 *  Transmit Data Complete ?
 **/
bool spi_wait_TX_done(void)  
{
    u32 loops = msecs_to_loops(10);
    u32 val = 0;
    do{
          val= readl(spi_rstatus_addr);
      }while(!(val & (1<<25))  && loops--);
          
    if(loops == 0)  
        return false;  
    else  
        return true;  
}

/**
 *  Receive Data Ready ?
 **/
bool spi_wait_RX_ready(void)  
{  
    u32 loops = msecs_to_loops(10);  
    u32 val = 0;  
    do{  
          val= readl(spi_rstatus_addr);  
      }while(!(val & (1<<24)) && loops--);  
          
     if(loops == 0)  
         return false;  
     else  
         return true;  
}

bool spi_sendbyte( Byte data)  
{   
    spi_rcscon_addr = ioremap(S5PV210_SPICSCON,4);
    
    iowrite16(ioread16(spi_rcscon_addr) & ~(1<<0) ,spi_rcscon_addr); 
        
    spi_rTx_data = ioremap(S5PV210_SPITXDAT,4);
    
    spi_rstatus_addr = ioremap(S5PV210_SPISTATUS,4);
         
    spi_rRx_data = ioremap(S5PV210_SPIRXDAT,4);  
         
    printk("Start send data ;\n");
         
    if(!spi_wait_TX_ready())  
    {  
         printk("failed to get tx channel.\n");  
         return false ;  
    }  
         
    writel(data, spi_rTx_data);
         
    while(!spi_wait_RX_ready());
         
    readl(spi_rRx_data);
         
    return true ; 
}  
   
/**
 *
 * spi_flush_fifo()- Clear the TxFIFO , RxFIFO and TX/RX shift register 
 *
 **/
 
void spi_flush_fifo(void)  
{  
    /* soft rest the spi controller, flush theFIFO */  
    if(spi_wait_TX_done())  
    {  
        iowrite8(ioread8(spi_rcon_addr) | (1<<5), spi_rcon_addr);   /* set bit 5 : Software reset Rx/Tx FIFO Data, SPI_STATUS Once reset, this bit must be clear manually. */
        iowrite8(ioread8(spi_rcon_addr) & ~(1<<5), spi_rcon_addr);  /* reset bit 5 */
    }  
}  
   
/* spi_readbyte()- Read a byte received on SPI0 */
Byte spi_readbyte(void)  
{  
    u32 tmp;
    Byte ret;  
   
    if(!spi_wait_TX_ready()) 
     
        return false;  

    spi_flush_fifo(); 
    
    spi_rTx_data = ioremap(S5PV210_SPITXDAT,4);
    
    spi_rRx_data = ioremap(S5PV210_SPIRXDAT,4);
    
    iowrite32(0xFF, spi_rTx_data);
    
    while(!spi_wait_RX_ready());
    
    tmp = ioread32(spi_rRx_data);

    ret = tmp & 0xff;
    
    return ret;
    
}  
module_init(cc2500_init);
module_exit(cc2500_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZXY & smb111.github.io");
