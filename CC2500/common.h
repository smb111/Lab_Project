#ifndef _COMMON_H_
#define _COMMON_H_

#include <linux/module.h>
#include<linux/uaccess.h>// copy from/to user
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>		// file_opration
#include <linux/io.h> 
#include<linux/interrupt.h>// request_irq
#include <mach/gpio.h>  
#include <mach/regs-gpio.h>  
#include <linux/irq.h>
#include <linux/delay.h>	// mdelay/udelay/ndelay
#include <linux/device.h>         /* device_create, class_create */ 

#include <linux/string.h>
#include <linux/types.h>
#include <linux/slab.h>		// kmalloc
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <mach/regs-gpio.h>	// extern interrupts
#include <mach/regs-irq.h>
#include <mach/regs-clock.h>
#include <mach/hardware.h>
#include <asm/system.h>

#define InValid_Index 0xFFFF
#define InValidVal    0xFFFF

typedef __u8 BYTE;
typedef __u16 WORD;
typedef __u32 DWORD;
typedef __u8 BOOL;
typedef __s16 INT16S;
typedef __s8 INT8S;
typedef unsigned char  Byte;

typedef union _DWORD_VAL {
	BYTE cVal[4];
	WORD nVal[2];
	DWORD dwVal;
} DWORD_VAL;

typedef union _WORD_VAL {
	BYTE cVal[2];
	WORD nVal;
} WORD_VAL;


#define TRUE  1
#define FALSE 0
#define true  1
#define false 0

#endif
