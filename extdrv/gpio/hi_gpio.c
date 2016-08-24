#include <linux/module.h>
#include <asm/hardware.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include "hi_gpio.h"

#define  GPIO_0_BASE_ADDR  0x20150000
#define  GPIO_1_BASE_ADDR  0x20160000
#define  GPIO_2_BASE_ADDR  0x20170000
#define  GPIO_3_BASE_ADDR  0x20180000
#define  GPIO_4_BASE_ADDR  0x20190000
#define  GPIO_5_BASE_ADDR  0x201a0000
#define  GPIO_6_BASE_ADDR  0x201b0000
#define  GPIO_7_BASE_ADDR  0x201c0000

#define  GPIO_DIR_BASE   (groupbase+0x400)
#define  GPIO_INTR_MASK  (groupbase+0x410)
#define  GPIO_DATA_BASE   data_reg_base


#define WRITE_REG(Addr, Value) ((*(volatile unsigned int *)(Addr)) = (Value))
#define READ_REG(Addr)         (*(volatile unsigned int *)(Addr))

static DECLARE_MUTEX(gpio_sem);

unsigned int groupbase=-1;
unsigned int data_reg_base=0;
unsigned int gpio_0_base_addr_virtual=0;
unsigned int gpio_1_base_addr_virtual=0;
unsigned int gpio_2_base_addr_virtual=0;
unsigned int gpio_3_base_addr_virtual=0;
unsigned int gpio_4_base_addr_virtual=0;
unsigned int gpio_5_base_addr_virtual=0;
unsigned int gpio_6_base_addr_virtual=0;
unsigned int gpio_7_base_addr_virtual=0;


static void gpio_calculate_data_groupbase(unsigned int groupnum, unsigned int bitnum)
{
    switch(groupnum)
    {
	case 0: 
	     groupbase =gpio_0_base_addr_virtual;
	     break;
	case 1: 
	     groupbase =gpio_1_base_addr_virtual;
	     break;
	case 2: 
	     groupbase =gpio_2_base_addr_virtual;
	     break;
	case 3: 
	     groupbase =gpio_3_base_addr_virtual;
	     break;
	case 4: 
	     groupbase =gpio_4_base_addr_virtual;
	     break;
	case 5: 
	     groupbase =gpio_5_base_addr_virtual;
	     break;
	case 6: 
	     groupbase =gpio_6_base_addr_virtual;
	     break;
	case 7: 
	     groupbase =gpio_7_base_addr_virtual;
	     break;
	default:
	     break;
    }

    printk("groupbase:%x !\n",groupbase);
    data_reg_base=groupbase+(1<<(bitnum+2));
    printk("data_reg_base:%x !\n",data_reg_base);
}



static int gpio_open(struct inode *inode, struct file *filp)
{
   return 0;		
}

static int gpio_release(struct inode *inode, struct file *filp)
{
	return 0;	
}


static int gpio_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    gpio_groupbit_info group_bit_info;
    unsigned int reg_tmp=0;
    down_interruptible(&gpio_sem);
    switch(cmd)
    {
        case GPIO_SET_DIR:
            copy_from_user(&group_bit_info,(gpio_groupbit_info*)arg, sizeof(gpio_groupbit_info));

	    if((group_bit_info.groupnumber>7)||(group_bit_info.bitnumber>7))
	    {
		printk("group number or bitnum beyond extent!\n");
		up(&gpio_sem);
		return -1;
	    }
            gpio_calculate_data_groupbase(group_bit_info.groupnumber,group_bit_info.bitnumber);
	    reg_tmp=READ_REG(GPIO_DIR_BASE);

	    if(group_bit_info.value==0)
	    {
		reg_tmp &=~(1<<group_bit_info.bitnumber);
		WRITE_REG(GPIO_DIR_BASE,reg_tmp);
	    }
	    else if(group_bit_info.value==1)
	    {
		reg_tmp |=(1<<group_bit_info.bitnumber);
		WRITE_REG(GPIO_DIR_BASE,reg_tmp);
	    }
	    else
	    {
		printk("dir beyond of extent!\n");
		up(&gpio_sem);
		return -1;
	    }
	    

            break;
        case GPIO_GET_DIR:
            copy_from_user(&group_bit_info,(gpio_groupbit_info*)arg, sizeof(gpio_groupbit_info));
	    if((group_bit_info.groupnumber>7)||(group_bit_info.bitnumber>7))
	    {
		printk("group number or bitnum beyond extent!\n");
		up(&gpio_sem);
		return -1;
	    }
            gpio_calculate_data_groupbase(group_bit_info.groupnumber,group_bit_info.bitnumber);
	    reg_tmp=READ_REG(GPIO_DIR_BASE);

	    reg_tmp &=(1<<group_bit_info.bitnumber);

	    if(reg_tmp!=0)

	    {
		group_bit_info.value=1;
	    }
	    else
	    {
		group_bit_info.value=0;
	    }
            copy_to_user((void __user *)arg, &group_bit_info, sizeof(gpio_groupbit_info));

	    break;
	case GPIO_READ_BIT:
	    copy_from_user(&group_bit_info,(gpio_groupbit_info*)arg, sizeof(gpio_groupbit_info));
	    if((group_bit_info.groupnumber>7)||(group_bit_info.bitnumber>7))
	    {
		printk("group number or bitnum beyond extent!\n");
		up(&gpio_sem);
		return -1;
	    }
	    gpio_calculate_data_groupbase(group_bit_info.groupnumber,group_bit_info.bitnumber);
	    reg_tmp=READ_REG(GPIO_DATA_BASE);

	    reg_tmp &=(1<<group_bit_info.bitnumber);

	    if(reg_tmp!=0)
	    {
		group_bit_info.value=1;
	    }
	    else
	    {
		group_bit_info.value=0;
	    }
            copy_to_user((void __user *)arg, &group_bit_info, sizeof(gpio_groupbit_info));

            break; 
	case GPIO_WRITE_BIT:
	    copy_from_user(&group_bit_info,(gpio_groupbit_info*)arg, sizeof(gpio_groupbit_info));

	    if((group_bit_info.groupnumber>7)||(group_bit_info.bitnumber>7))
	    {
		printk("group number or bitnum beyond extent!\n");
		up(&gpio_sem);
		return -1;
	    }
	    gpio_calculate_data_groupbase(group_bit_info.groupnumber,group_bit_info.bitnumber);


	    if(group_bit_info.value==0)
	    {
		WRITE_REG(GPIO_DATA_BASE,0);
                printk("1addr:0x%x vaule:%x!\n",GPIO_DATA_BASE,*((unsigned int *)GPIO_DATA_BASE));		
	    }
	    else if(group_bit_info.value==1)
	    {
		WRITE_REG(GPIO_DATA_BASE,1<<group_bit_info.bitnumber);
                printk("2addr:0x%x vaule:%x!\n",GPIO_DATA_BASE,*((unsigned int *)GPIO_DATA_BASE));		
	    }
	    else
	    {
		printk("write bit beyond of extent!\n");
		up(&gpio_sem);
		return -1;
	    }

	    break;

        default:
	up(&gpio_sem);
            return -1;           
    }
    up(&gpio_sem);

    return 0;

}


static struct file_operations gpio_fops = {
  owner:THIS_MODULE,
  open:gpio_open,
  ioctl:gpio_ioctl,
  release:gpio_release,
};

static struct miscdevice gpio_dev = {
    MISC_DYNAMIC_MINOR,
    "hi_gpio",
    &gpio_fops,
};



//gpio的复用关系要放在uboot下面；
static int __init hi_gpio_init(void)
{
	signed int  ret=0;

        ret = misc_register(&gpio_dev);
        if (ret)
        {
                printk(KERN_ERR "register misc dev for i2c fail!\n");
		return ret;
	}
	
	gpio_0_base_addr_virtual=(unsigned int)ioremap_nocache(GPIO_0_BASE_ADDR,0x40000);
	if(!gpio_0_base_addr_virtual)
	{
	    printk("ioremap gpio group0 failed!\n");
	    return -1;
	}
	gpio_1_base_addr_virtual=gpio_0_base_addr_virtual+0x10000;
	gpio_2_base_addr_virtual=gpio_0_base_addr_virtual+0x20000;
	gpio_3_base_addr_virtual=gpio_0_base_addr_virtual+0x30000;
	gpio_4_base_addr_virtual=(unsigned int) ioremap_nocache(GPIO_4_BASE_ADDR,0x40000);
	if(!gpio_4_base_addr_virtual)
	{
	    printk("ioremap gpio group0 failed!\n");
	    iounmap((void*)gpio_0_base_addr_virtual);
	    return -1;
	}

	gpio_5_base_addr_virtual=gpio_4_base_addr_virtual+0x10000;
	gpio_6_base_addr_virtual=gpio_4_base_addr_virtual+0x20000;
	gpio_7_base_addr_virtual=gpio_4_base_addr_virtual+0x30000;

	return 0;         
}


static void __exit hi_gpio_exit(void)
{
    misc_deregister(&gpio_dev);
    iounmap((void*)gpio_0_base_addr_virtual);
    iounmap((void*)gpio_4_base_addr_virtual);

}

module_init(hi_gpio_init);
module_exit(hi_gpio_exit);

MODULE_AUTHOR("Digital Media Team ,Hisilicon crop ");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Real Time Clock interface for HI3511");




