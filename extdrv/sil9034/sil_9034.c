/*
 *
 * Copyright (c) 2006 Hisilicon Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 *
 * History:
 *      10-April-2006 create this file
 */


#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/poll.h>

#include <asm/hardware.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

#include "gpio_i2c.h"

#include "sil9034.h"
#include "sil9034_def.h"

#define DEV_NAME "sil9034"
#define DEBUG_LEVEL 1
#define DPRINTK(level,fmt,args...) do{ if(level < DEBUG_LEVEL)\
    printk(KERN_INFO "%s [%s ,%d]: " fmt "\n",DEV_NAME,__FUNCTION__,__LINE__,##args);\
}while(0)

static unsigned int open_cnt = 0;
static unsigned int norm = 0;

#if 0
static void sil9034_write(unsigned char chip_addr,unsigned char reg_addr,unsigned char value)
{
    gpio_i2c_write(chip_addr,reg_addr,value);
}

static int sil9034_read(unsigned char chip_addr,unsigned char reg_addr)
{
    return gpio_i2c_read(chip_addr,reg_addr);
}
#endif

static void sil9034_write_table(unsigned char chip_addr,unsigned char addr,unsigned char *tbl_ptr,unsigned tbl_cnt)
{
	unsigned int i;

	for(i = 0;i<tbl_cnt;i++)
	{
		 gpio_i2c_write(chip_addr,(addr+i),*(tbl_ptr+i));
	}
}

static void sil9034_reset(void)
{
    gpio_i2c_write(SIL9034_I2C_ADDR_A, 0x5, 0x1);

    /* hold reset low level : 50 us */
    udelay(100);
}

static void sil9034_720p_init(void)
{
	sil9034_write_table(SIL9034_I2C_ADDR_A,0x0,tbl_sil9034_720p_0x72,256);
	sil9034_write_table(SIL9034_I2C_ADDR_B,0x0,tbl_sil9034_720p_0x7a,256);
}

static void sil9034_1080i50_init(void)
{
	sil9034_write_table(SIL9034_I2C_ADDR_A,0x0,tbl_sil9034_1080i50_0x72,256);
	sil9034_write_table(SIL9034_I2C_ADDR_B,0x0,tbl_sil9034_1080i50_0x7a,256);
}

static void sil9034_1080i60_init(void)
{
	sil9034_write_table(SIL9034_I2C_ADDR_A,0x0,tbl_sil9034_1080i60_0x72,256);
	sil9034_write_table(SIL9034_I2C_ADDR_B,0x0,tbl_sil9034_1080i60_0x7a,256);
}

static void sil9034_clear_reg(void)
{
	unsigned int i;

	for(i = 0;i<256;i++)
	{
		 gpio_i2c_write(SIL9034_I2C_ADDR_A,i,0x00);
	}

	for(i = 0;i<256;i++)
	{
		 gpio_i2c_write(SIL9034_I2C_ADDR_B,i,0x00);
	}
}

/*
 *	device open. set counter (unsupport re-open)
 */
static int sil9034_open(struct inode * inode, struct file * file)
{
	if(0 == open_cnt++)
		return 0;

    DPRINTK(0, "you should close the device first!");
	return -1 ;
}

/*
 *	Close device, Do nothing!
 */
static int sil9034_close(struct inode *inode ,struct file *file)
{
    open_cnt--;
	return 0;
}

static int sil9034_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

/*
 *  The various file operations we support.
 */

static struct file_operations sil9034_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= sil9034_ioctl,
	.open		= sil9034_open,
	.release	= sil9034_close
};

static struct miscdevice sil9034_dev = {
	MISC_DYNAMIC_MINOR,
	DEV_NAME,
	&sil9034_fops,
};

static int sil9034_device_init(void)
{
    switch (norm)
    {
        case 5 :
            printk("sil9034 init as 1080i@50\n");
            sil9034_1080i50_init();
            break;
        case 6 :
            printk("sil9034 init as 1080i@60\n");
            sil9034_1080i60_init();
            break;
        case 7 :
            printk("sil9034 init as 720p@60\n");
            sil9034_720p_init();
            break;
        default:
            printk("sil9034 init as 1080i@50\n");
            sil9034_1080i50_init();
    }

	return 0;
}

static int __init sil9034_init(void)
{
	unsigned int ret;

	ret = misc_register(&sil9034_dev);
	if(ret)
	{
		DPRINTK(0,"could not register sil9034 device");
		return -1;
	}

    /* chip reset first! */
    sil9034_reset();

    if(sil9034_device_init() < 0)
    {
        misc_deregister(&sil9034_dev);
        DPRINTK(0, "sil9034 device init fail,deregister it!");
        return -1;
    }

	DPRINTK(1, "sil9034 driver init successful!");
	return ret;
}

static void __exit sil9034_exit(void)
{
    sil9034_clear_reg();

    misc_deregister(&sil9034_dev);

    DPRINTK(1, "deregister sil9034!");

    return;
}

module_param(norm, uint, S_IRUGO);
MODULE_PARM_DESC(norm,"5:1080i@50(default), 6:1080i@60, 7:720p@60");

module_init(sil9034_init);
module_exit(sil9034_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif

MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hisilicon");

