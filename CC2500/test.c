/**
  *  该程序基于   S5Pv210  上面的cc2500 驱动使用多进程和多线程分别实现了
  *  读取cc2500  传过来的数据。发送数据在主进程随时都可以进行发送
  **/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>



//定义CC2500最大数量
#define CC2500_IOC_MAXNR 1
//定义CC2500的magic number
#define CC2500_IOC_MAGIC  'm'


#define CC2500_IOC_PHY_DETECT_STATUS _IO(CC2500_IOC_MAGIC, 1)
#define CC2500_IOC_PHY_GET_BAUDRATE _IOR(CC2500_IOC_MAGIC, 3, unsigned char)
#define CC2500_IOC_PHY_SET_BAUDRATE _IOW(CC2500_IOC_MAGIC, 4, unsigned char)
#define CC2500_IOC_PHY_GET_TXPOWER _IOR(CC2500_IOC_MAGIC, 5, unsigned char)
#define CC2500_IOC_PHY_SET_TXPOWER _IOW(CC2500_IOC_MAGIC, 6, unsigned char)
#define CC2500_IOC_PHY_GET_CHANNEL _IOR(CC2500_IOC_MAGIC, 7, unsigned char)
#define CC2500_IOC_PHY_SET_CHANNEL _IOW(CC2500_IOC_MAGIC, 8, unsigned char)

pthread_mutex_t mut;  //  多线程互斥锁

void *read_data(int *fd)
{
        int i;
	ssize_t read_size;
	char buf_read[100];
	while(1)
	{
	//	pthread_mutex_lock(&mut);
		
		read_size = read(*fd, buf_read, sizeof(buf_read)/sizeof(char));
		buf_read[read_size] = '\0';
	
		printf("接受到的数据为: \n");
	
		for(i = 0;i < read_size;i++)
			printf("%x   ",buf_read[i]);
		printf("\n");
		
	//	pthread_mutex_unlock(&mut);
	}
	
	pthread_exit(NULL);  
}

int main(int argc,char *argv[])
{
	int fd;
	int cmd;
	int ret;
	char buf[10] = {0x09,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09};
	char buf_read[100];
	int i;
	ssize_t read_size;
	pid_t pid;

	int baudrate=0;
	int txpower=0;
	int channel=0;
	
    fd = open("/dev/cc2500dev",O_RDWR);

    baudrate = 0x01;
    ioctl(fd,CC2500_IOC_PHY_SET_BAUDRATE,&baudrate);
    txpower = 0xff;
    ioctl(fd,CC2500_IOC_PHY_SET_TXPOWER,&txpower);
    channel = 0x01;
    ioctl(fd,CC2500_IOC_PHY_SET_CHANNEL,&channel);   // 设置 0x00 channel
    /*
    ioctl(fd,CC2500_IOC_PHY_GET_BAUDRATE,&baudrate);

    printf("获取波特率值:  %x\n",baudrate);

    ioctl(fd,CC2500_IOC_PHY_GET_TXPOWER,&txpower);

    printf("获取发送功率:  %x\n",txpower);

    ioctl(fd,CC2500_IOC_PHY_GET_CHANNEL,&channel);

    printf("获取信道:  %x\n",channel);
    */

    printf(" 需要写入的字节数: %d \n",sizeof(buf)/sizeof(char));  

    for(i=0;i<sizeof(buf)/sizeof(char);i++)
     
        printf("buf  %x   ",buf[i]);
     
    printf("\n");

	

	/* 在子线程中执行read  阻塞函数 
	pthread_mutex_init(&mut,NULL);
	pthread_t thread;  

	if(pthread_create(&thread, NULL, read_data, &fd)!=0)//创建子线程  
	{  
	     perror("pthread_create");  
	} */
	 
	while(1)
	{	
	    sleep(2);   
//      ret = write(fd, buf, sizeof(buf)/sizeof(char));   // 发送数据函数，随时可以运行
//      printf("ret is ...............%d\n",ret);

		/* 在子进程中执行  read  阻塞函数
		if((pid = fork()) == 0)
		{
			 read_size = read(fd, buf_read, sizeof(buf_read)/sizeof(char));
			 buf_read[read_size] = '\0';
			printf("接受到的数据为: \n");

			for(i = 0;i < read_size;i++)
				printf("%x   ",buf_read[i]);
			
			 exit(0);
		}else if(pid < 0)  
			printf("There is ana error in fork !\n");  
        */
		read_size = read(fd, buf_read, sizeof(buf_read)/sizeof(char));
		buf_read[read_size] = '\0';
	
		printf("接受到的数据为: \n");
	
		for(i = 0;i < read_size;i++)
			printf("%x   ",buf_read[i]);
		printf("\n");
	}
	
	close(fd);
		
	return 0;
}

/*
    Put_user
    
    信号量 
    
    cc2500 向信道发送的设置
    
    linux时间函数
    
    任务等待队列 （wait_queue。。。）
    
    arm - cc2500 收发一个字节（函数）

    多依赖文件的 Makefile 编写
    
    静态函数的作用域问题
    
    自动创建设备文件节点
    
    函数宏定义的使用
     
    内核变换带来的问题：  中断号不同  信号量初始化不同  file_operations 操作函数的原型变化编译出警告  
    
    程序的主要变化： 使用完全的的硬件 SPI  ，包括 cs 引脚也配置成 nss 端口复用功能  不使用师兄们写的配置成普通 gpio    cpu和cc2500传输数据函数略有变化（之前的使用手动延时，现在使用循环判断）
    
    连接情况：
        
        S5PV210         功能                    外设                cc2500
    -------------------------------------------------------------------------
    	GPH2_0   :   外部中断XEINT16    (CON14的21号管脚)   中断     GDO 0
	GPB0     :   SPI(0)时钟         (CON14的9号管脚)    时钟     CLK
	GPB3     :   SPI(0)MOSI    	(CON14的7号管脚)    MISI     SI
	GPB2     :   SPI(0)MISO    	(CON14的8号管脚)    MISO     SO
	GPB1     :   SPI(0)SSI        	(CON14的10号管脚)   输出     nSS   
    
    还需要添加：
    
    	LENA 和 另一个口的 cc2500 传输和接收时的功率放大

*/
