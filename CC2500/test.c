/**
  *  �ó������   S5Pv210  �����cc2500 ����ʹ�ö���̺Ͷ��̷ֱ߳�ʵ����
  *  ��ȡcc2500  �����������ݡ�������������������ʱ�����Խ��з���
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



//����CC2500�������
#define CC2500_IOC_MAXNR 1
//����CC2500��magic number
#define CC2500_IOC_MAGIC  'm'


#define CC2500_IOC_PHY_DETECT_STATUS _IO(CC2500_IOC_MAGIC, 1)
#define CC2500_IOC_PHY_GET_BAUDRATE _IOR(CC2500_IOC_MAGIC, 3, unsigned char)
#define CC2500_IOC_PHY_SET_BAUDRATE _IOW(CC2500_IOC_MAGIC, 4, unsigned char)
#define CC2500_IOC_PHY_GET_TXPOWER _IOR(CC2500_IOC_MAGIC, 5, unsigned char)
#define CC2500_IOC_PHY_SET_TXPOWER _IOW(CC2500_IOC_MAGIC, 6, unsigned char)
#define CC2500_IOC_PHY_GET_CHANNEL _IOR(CC2500_IOC_MAGIC, 7, unsigned char)
#define CC2500_IOC_PHY_SET_CHANNEL _IOW(CC2500_IOC_MAGIC, 8, unsigned char)

pthread_mutex_t mut;  //  ���̻߳�����

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
	
		printf("���ܵ�������Ϊ: \n");
	
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
    ioctl(fd,CC2500_IOC_PHY_SET_CHANNEL,&channel);   // ���� 0x00 channel
    /*
    ioctl(fd,CC2500_IOC_PHY_GET_BAUDRATE,&baudrate);

    printf("��ȡ������ֵ:  %x\n",baudrate);

    ioctl(fd,CC2500_IOC_PHY_GET_TXPOWER,&txpower);

    printf("��ȡ���͹���:  %x\n",txpower);

    ioctl(fd,CC2500_IOC_PHY_GET_CHANNEL,&channel);

    printf("��ȡ�ŵ�:  %x\n",channel);
    */

    printf(" ��Ҫд����ֽ���: %d \n",sizeof(buf)/sizeof(char));  

    for(i=0;i<sizeof(buf)/sizeof(char);i++)
     
        printf("buf  %x   ",buf[i]);
     
    printf("\n");

	

	/* �����߳���ִ��read  �������� 
	pthread_mutex_init(&mut,NULL);
	pthread_t thread;  

	if(pthread_create(&thread, NULL, read_data, &fd)!=0)//�������߳�  
	{  
	     perror("pthread_create");  
	} */
	 
	while(1)
	{	
	    sleep(2);   
//      ret = write(fd, buf, sizeof(buf)/sizeof(char));   // �������ݺ�������ʱ��������
//      printf("ret is ...............%d\n",ret);

		/* ���ӽ�����ִ��  read  ��������
		if((pid = fork()) == 0)
		{
			 read_size = read(fd, buf_read, sizeof(buf_read)/sizeof(char));
			 buf_read[read_size] = '\0';
			printf("���ܵ�������Ϊ: \n");

			for(i = 0;i < read_size;i++)
				printf("%x   ",buf_read[i]);
			
			 exit(0);
		}else if(pid < 0)  
			printf("There is ana error in fork !\n");  
        */
		read_size = read(fd, buf_read, sizeof(buf_read)/sizeof(char));
		buf_read[read_size] = '\0';
	
		printf("���ܵ�������Ϊ: \n");
	
		for(i = 0;i < read_size;i++)
			printf("%x   ",buf_read[i]);
		printf("\n");
	}
	
	close(fd);
		
	return 0;
}

/*
    Put_user
    
    �ź��� 
    
    cc2500 ���ŵ����͵�����
    
    linuxʱ�亯��
    
    ����ȴ����� ��wait_queue��������
    
    arm - cc2500 �շ�һ���ֽڣ�������

    �������ļ��� Makefile ��д
    
    ��̬����������������
    
    �Զ������豸�ļ��ڵ�
    
    �����궨���ʹ��
     
    �ں˱任���������⣺  �жϺŲ�ͬ  �ź�����ʼ����ͬ  file_operations ����������ԭ�ͱ仯���������  
    
    �������Ҫ�仯�� ʹ����ȫ�ĵ�Ӳ�� SPI  ������ cs ����Ҳ���ó� nss �˿ڸ��ù���  ��ʹ��ʦ����д�����ó���ͨ gpio    cpu��cc2500�������ݺ������б仯��֮ǰ��ʹ���ֶ���ʱ������ʹ��ѭ���жϣ�
    
    ���������
        
        S5PV210         ����                    ����                cc2500
    -------------------------------------------------------------------------
    	GPH2_0   :   �ⲿ�ж�XEINT16    (CON14��21�Źܽ�)   �ж�     GDO 0
	GPB0     :   SPI(0)ʱ��         (CON14��9�Źܽ�)    ʱ��     CLK
	GPB3     :   SPI(0)MOSI    	(CON14��7�Źܽ�)    MISI     SI
	GPB2     :   SPI(0)MISO    	(CON14��8�Źܽ�)    MISO     SO
	GPB1     :   SPI(0)SSI        	(CON14��10�Źܽ�)   ���     nSS   
    
    ����Ҫ��ӣ�
    
    	LENA �� ��һ���ڵ� cc2500 ����ͽ���ʱ�Ĺ��ʷŴ�

*/
