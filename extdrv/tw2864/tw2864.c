
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <linux/string.h>
#include <linux/list.h>
#include <asm/semaphore.h>
#include <asm/delay.h>
#include <linux/timer.h>
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
#include "gpio_i2c1.h"

#include "tw2864_def.h"
#include "tw2864.h"

/************************************************************************************
 * notes:
 *     1) the default value is  2 way d1, vd1: ch1,ch2; vd2:ch2,ch4; vd3:ch3,ch1; vd4:ch4,ch2
 *     2) we set the audio and video initialization value in one function ,not 2.
 *         so, we changed the calling relationship.
 *     3) you must specify the  video mode if you want to set the video mode such as: PAL,NTSC.
 *     4)  
 *************************************************************************************/


static unsigned int cascad_judge = 0;


unsigned char channel_array[8] = {0x20,0x64,0xa8,0xec,0x31,0x75,0xb9,0xfd};
unsigned char channel_array_default[8] = {0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe};
unsigned char channel_array_our[8] = {0x10,0x32,0x98,0xba,0x54,0x76,0xdc,0xfe};
/*正常的情况下，4通道排序*/
unsigned char channel_array_4chn[8] = {0x10,0x54,0x98,0xdc,0x32,0x76,0xba,0xfe};
/*16cif板的4通道排序*/
unsigned char channel_array_anlian16[8] = {0x03,0x21,0xa9,0xcd,0x21,0x87,0xb9,0xdb};

unsigned char ch_map[4]={3,3,3,3};
unsigned char video_ch_map[8]={0,2,1,3,0,2,1,3};
unsigned char chip_address_map[]={0x50,0x52,0x54,0x56};

#if 0
unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address);
void gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char value);
#endif


static unsigned char tw2864_byte_write(unsigned char chip_addr,
        unsigned char addr,unsigned char data)
{
#ifndef HI_FPGA
    gpio_i2c_write(chip_addr,addr,data);
#else
    if (TW2864A_I2C_ADDR == chip_addr || TW2864B_I2C_ADDR == chip_addr)
    {
        gpio_i2c_write(chip_addr,addr,data);
    }
    else
    {
        gpio_i2c1_write(chip_addr,addr,data);
    }
#endif
        
    return 0;
}

static unsigned char tw2864_byte_read(unsigned char chip_addr,unsigned char addr)
{
#ifndef HI_FPGA 
    return gpio_i2c_read(chip_addr,addr);
#else
    if (TW2864A_I2C_ADDR == chip_addr || TW2864B_I2C_ADDR == chip_addr)
    {
        return gpio_i2c_read(chip_addr,addr);
    }
    else
    {
        return gpio_i2c1_read(chip_addr,addr);
    }
#endif
}


static void tw2864_write_table(unsigned char chip_addr,unsigned char addr,
        unsigned char *tbl_ptr,unsigned char tbl_cnt)
{
    unsigned char i = 0;
#ifndef HI_FPGA   
    for(i = 0;i<tbl_cnt;i++)
    {
		gpio_i2c_write(chip_addr,(addr+i),*(tbl_ptr+i));
    }
#else
    if (TW2864A_I2C_ADDR == chip_addr || TW2864B_I2C_ADDR == chip_addr)
    {
        for(i = 0;i<tbl_cnt;i++)
        {
    		gpio_i2c_write(chip_addr,(addr+i),*(tbl_ptr+i));
        }
    }
    else
    {
        for(i = 0;i<tbl_cnt;i++)
        {
    		gpio_i2c1_write(chip_addr,(addr+i),*(tbl_ptr+i));
        }
    }
#endif
}

static void tw2864_read_table(unsigned char chip_addr,
        unsigned char addr,unsigned char reg_num)
{
    unsigned char i = 0,temp = 0;
    for(i =0; i < reg_num;i++ )
    {
		temp = tw2864_byte_read(chip_addr,addr+i);
		printk("reg 0x%x=0x%x,",addr+i,temp);
		if(((i+1)%4)==0)
		{
		    printk("\n");
		}
    }    
}

static void tw2864_video_mode_init(unsigned chip_addr,
        unsigned char video_mode, unsigned char ch)
{
    unsigned char mode_temp = 0;
    mode_temp = video_mode;

    udelay(50);

    if(mode_temp == NTSC)
    {
	    tw2864_write_table(chip_addr,0x00+0x10*ch,tbl_ntsc_tw2864_common,16);
    }
    else
    {
	    tw2864_write_table(chip_addr,0x00+0x10*ch,tbl_pal_tw2864_common,16);
    }

    /* 2864 has no register like this to set  miscellaneous control,zkun
       temp = tw2864_byte_read(chip_addr,0x43);
       SET_BIT(chip_addr,0x80);
      tw2864_byte_write(chip_addr,0x43,temp);
     */ 
}

static void tw2864_audio_cascade()
{
    int i;
    unsigned char temp;
    
    /* cascade */
    tw2864_byte_write(TW2864A_I2C_ADDR, 0x82, 0x00);
    tw2864_byte_write(TW2864B_I2C_ADDR, 0x82, 0x00);
    tw2864_byte_write(TW2864C_I2C_ADDR, 0x82, 0x00);
    tw2864_byte_write(TW2864D_I2C_ADDR, 0x82, 0x00);
    
    for (i=0; i<4; i++)
    {
        /* the number of audio for record on the ADATR pin */
        temp = tw2864_byte_read(tw2864_i2c_addr[i], 0xd2);
        temp &= 0xfc;/* [1:0]*/
        temp |= 0x3;/* 0x3 16chn */
        tw2864_byte_write(tw2864_i2c_addr[i], 0xd2, temp);
        
        /* SMD = b'01 */
        temp = tw2864_byte_read(tw2864_i2c_addr[i], 0xcf);
        temp &= 0x3f;/* [7:6]*/
        temp |= 0x40;/* 0x1 */
        tw2864_byte_write(tw2864_i2c_addr[i], 0xcf, temp);
        
        /* set FIRSTCNUM */
        temp = tw2864_byte_read(tw2864_i2c_addr[i], 0xcf);
        temp &= 0xf0;/* [3:0]*/
        temp |= ((3==i)?(0x00):(0x04));
        tw2864_byte_write(tw2864_i2c_addr[i], 0xcf, temp);
        
        /* set PB_MASTER */
        temp = tw2864_byte_read(tw2864_i2c_addr[i], 0xdb);
        temp &= 0xdc;
        temp |= 0x01;    
        tw2864_byte_write(tw2864_i2c_addr[i], 0xdb, temp); 
    }
    
    for (i=0; i<4; i++)
    {
        /* Sequence of Audio to be Recorded */
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD3, 0x10);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD4, 0x32);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD5, 0x54);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD6, 0x76);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD7, 0x98);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD8, 0xBA);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xD9, 0xDC);
        tw2864_byte_write(tw2864_i2c_addr[i], 0xDA, 0xFE); 
        /* Mix Ratio Value */
        tw2864_byte_write(tw2864_i2c_addr[i], 0xDD, 0x33); 
        tw2864_byte_write(tw2864_i2c_addr[i], 0xDE, 0x33); 
        tw2864_byte_write(tw2864_i2c_addr[i], 0xDF, 0x33); 
    }    
}

static void tw2864_audio_init(unsigned char chip_addr)
{
    tw2864_byte_write(chip_addr, 0xCE, 0x40 );
    //tw2864_byte_write(chip_addr, 0xCF, 0x44); 

    /*  */                      
    tw2864_byte_write(chip_addr, 0xD0, 0x33);
    tw2864_byte_write(chip_addr, 0xD1, 0x33);

    /* recode: I2S, 4 chn */
    tw2864_byte_write(chip_addr, 0xD2, 0x01);        
    
    tw2864_byte_write(chip_addr, 0xD3, 0x10);/* 1 0*/
    tw2864_byte_write(chip_addr, 0xD7, 0x32);/* 9 8*/

    /* playback: I2S, master, 16bit, ACLKR pin is output */
    tw2864_byte_write(chip_addr, 0xDB, 0xE1);

    /* PCM output */
    tw2864_byte_write(chip_addr, 0xDC, 0x00);

    /* Enable Video and Audio Detection*/
    tw2864_byte_write(chip_addr, 0xFC, 0xFF);

    /* Audio Clock Increment  8K */
    tw2864_byte_write(chip_addr, 0xF0, 0x83);
    tw2864_byte_write(chip_addr, 0xF1, 0xb5);
    tw2864_byte_write(chip_addr, 0xF2, 0x09);
    /* Audio Detection Threshold 8K */
    tw2864_byte_write(chip_addr, 0xE1, 0xc0);
    tw2864_byte_write(chip_addr, 0xE2, 0xaa);
    tw2864_byte_write(chip_addr, 0xE3, 0xaa);

    tw2864_byte_write(chip_addr, 0xf8, 0xc4); 
    tw2864_byte_write(chip_addr, 0xfa, 0x4a); 
    //tw2864_byte_write(chip_addr, 0xfb, 0x0f);    

    /* 根据工作时钟不同而设置 */
    //tw2864_byte_write(chip_addr, 0x89, 0x02);
    //tw2864_byte_write(chip_addr, 0xca ,0xaa);    
}

static void tw2864_device_video_init(unsigned char chip_addr,unsigned char video_mode)
{
    unsigned char tw2864_id =0;
    unsigned char tw2864_id2 =0;

    tw2864_id  = tw2864_byte_read(chip_addr,TW2864_ID);
    tw2864_id2 = tw2864_byte_read(chip_addr,0xfe);
    
    if (0xff == tw2864_id)
    {
        printk("warning: tw2864 0x%x i2c_read err !!!\n",chip_addr);
        return;
    }

    /*
     * init the channel of some chip with the value of NTSC/PAL,zkun
     */
    tw2864_video_mode_init(chip_addr,video_mode,0);
    tw2864_video_mode_init(chip_addr,video_mode,1);
    tw2864_video_mode_init(chip_addr,video_mode,2);
    tw2864_video_mode_init(chip_addr,video_mode,3);

    /*
     * here we init the video and audio registers with the parameters.so the special
     * function which inits the audio  below should not be called. zkun
     */            
    tw2864_write_table(chip_addr,0x80,tbl_tw2864_0x80_0x8f,16);
    tw2864_write_table(chip_addr,0x90,tbl_tw2864_0x90,16);
    tw2864_write_table(chip_addr,0xa4,tbl_tw2864_0xa4,11);
    tw2864_write_table(chip_addr,0xb0,tbl_tw2864_0xb0,1);
    tw2864_write_table(chip_addr,0xc4,tbl_tw2864_0xc4,12);

    tw2864_byte_write(chip_addr, 0xfa, 0x4a);
    
    //tw2864_write_table(chip_addr,0xd0,tbl_tw2864_0xd0,16);
    //tw2864_write_table(chip_addr,0xe0,tbl_tw2864_0xe0,16);
    //tw2864_write_table(chip_addr,0xf0,tbl_tw2864_0xf0,16);
    
    printk("tw2864 0x%x init ok !\n",chip_addr);
}

static void tw2864_reg_dump(unsigned char chip_addr)
{
    tw2864_read_table(chip_addr,0x0,0xff);
    printk("tw2864_reg_dump ok\n");
}

/*
*      restart alloc channel
*      default: 0
*      right and left alloc:  1
*      our:2
*/
static void channel_alloc(unsigned char chip_addr,unsigned char ch)
{
    unsigned char i = 0;
    if(ch == 0)
    {
		for(i =0 ;i < 8;i++)
		{
		    tw2864_byte_write(chip_addr,0xd3+i,0);
		    tw2864_byte_write(chip_addr,0xd3+i,channel_array_default[i]);
		}
    }
    else if(ch == 1)
    {
		for(i =0 ;i < 8;i++)
		{
		    tw2864_byte_write(chip_addr,0xd3+i,0);
		    tw2864_byte_write(chip_addr,0xd3+i,channel_array[i]);
		}

    }
    else if(ch == 2)
    {
		for(i =0 ;i < 8;i++)
		{
		    tw2864_byte_write(chip_addr,0xd3+i,0);
		    tw2864_byte_write(chip_addr,0xd3+i,channel_array_our[i]);
		}
    }
    else if(ch == 3)
    {
		for(i = 0; i < 8; i++)
		{
		    tw2864_byte_write(chip_addr,0xd3+i,0);
		    tw2864_byte_write(chip_addr,0xd3+i,channel_array_4chn[i]);
		}
    }
    else if(ch == 4)
    {
		for(i = 0; i < 8; i++)
		{
		    tw2864_byte_write(chip_addr,0xd3+i,0);
		    tw2864_byte_write(chip_addr,0xd3+i,channel_array_anlian16[i]);
		}
    }
}

static void set_audio_output(unsigned char chip_addr,unsigned char num_path)
{
    unsigned char temp = 0;
    unsigned char help = 0;

    if(0 < num_path && num_path <=2)
	help = 0;
    else if(2 < num_path && num_path <= 4)
	help = 1;
    else if(4 < num_path && num_path <= 8)
	help = 2;
    else if(8 <num_path && num_path <=16)
	help = 3;
    else
    {
		printk("tw2864a audio path choice error\n");
		return;
    }
    temp = tw2864_byte_read(chip_addr,0xd2);

    CLEAR_BIT(temp,0x3);

    SET_BIT(temp,help);

    tw2864_byte_write(chip_addr,0xd2,temp);

    printk("tw2864 set output path=%d  ok\n",num_path);	

    return;
}
/*set cascad*/
static void set_audio_cascad(unsigned char chip_addr,int firstcnnum)
{
    unsigned char temp = 0;
    temp = tw2864_byte_read(chip_addr,0xcf);
    CLEAR_BIT(temp,0xc0);
    SET_BIT(temp,0x80);
    tw2864_byte_write(chip_addr,0xcf,temp);

    temp = tw2864_byte_read(chip_addr,0xcf);
    CLEAR_BIT(temp,0x0f);
    SET_BIT(temp,firstcnnum );
    tw2864_byte_write(chip_addr,0xcf,temp);
}

static void set_audio_mix_out(unsigned char chip_addr,unsigned char ch)
{
    unsigned char temp = 0;
    temp = tw2864_byte_read(chip_addr,0xe0);
    CLEAR_BIT(temp,0x1f);
    SET_BIT(temp,ch);
    tw2864_byte_write(chip_addr,0xe0,temp);
    if(ch == 16)
    {
		printk("tw2864 select playback audio out\n");
    }
    if(ch == 17)
    {
		printk("tw2864 select mix digital and analog audio data\n");
    }
    printk("set_audio_mix_out ok\n"); 

    return;
}
/*single chip*/
static void set_single_chip(void)
{
}

static void set_audio_record_m(unsigned char chip_addr,unsigned char num_path)
{
	set_audio_output(chip_addr,num_path);
}

static void set_audio_mix_mute(unsigned char chip_addr,unsigned char ch) 
{
    unsigned char temp = 0 ;
    temp = tw2864_byte_read(chip_addr,0xdc);
	CLEAR_BIT(temp, 0x1f);
    SET_BIT(temp,1<<ch);
    tw2864_byte_write(chip_addr,0xdc,temp);
    return;
}

static void clear_audio_mix_mute(unsigned char chip_addr,unsigned char ch)
{
    unsigned char temp = 0;
    temp = tw2864_byte_read(chip_addr,0xdc);
    CLEAR_BIT(temp,1<<ch);
    tw2864_byte_write(chip_addr,0xdc,temp);
}

static void hue_control(unsigned char chip_addr,unsigned char ch ,unsigned char hue_value)
{   
    tw2864_byte_write(chip_addr,ch*0x10+0x6,hue_value);

}
static void hue_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *hue_value)
{
    *hue_value = tw2864_byte_read(chip_addr,ch*0x10+0x6);
}


static void saturation_control(unsigned char chip_addr,unsigned char ch ,unsigned char saturation_value)
{
    tw2864_byte_write(chip_addr,ch*0x10+0x4,saturation_value);
    tw2864_byte_write(chip_addr,ch*0x10+0x5,saturation_value);
}
static int saturation_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *saturation_value)
{
    unsigned char char_tmp=tw2864_byte_read(chip_addr,ch*0x10+0x4);
    *saturation_value = tw2864_byte_read(chip_addr,ch*0x10+0x5);
    if(char_tmp==(*saturation_value))
    {
		printk("U and V is not the same !\n");
		return -1 ;
    }
    return 0;
}


static void contrast_control(unsigned char chip_addr,unsigned char ch ,unsigned char contrast_value)
{
    tw2864_byte_write(chip_addr,ch*0x10+0x2,contrast_value);
}
static void contrast_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *contrast_value)
{
    *contrast_value = tw2864_byte_read(chip_addr,ch*0x10+0x2);
}

static void brightness_control(unsigned char chip_addr,unsigned char ch ,unsigned char brightness_value)
{
    tw2864_byte_write(chip_addr,ch*0x10+0x1,brightness_value);
}
static void brightness_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *brightness_value)
{
    *brightness_value = tw2864_byte_read(chip_addr,ch*0x10+0x1);
}
static void video_lost_check(unsigned char chip_addr,unsigned char ch ,unsigned int *videolost_info)
{
    *videolost_info=tw2864_byte_read(chip_addr,ch*0x10) >> 7;
}


static int luminance_peaking_control(unsigned char chip_addr,unsigned char ch ,unsigned char luminance_peaking_value)
{
    unsigned char temp = 0;
    if(luminance_peaking_value > 15)
    {
		return -1;
    }
    temp = tw2864_byte_read(chip_addr,ch*0x10+0xb);
    CLEAR_BIT(temp,0xf);
    SET_BIT(temp,luminance_peaking_value);
    tw2864_byte_write(chip_addr,ch*0x10+0xb,temp);
    return 0;
}
static void luminance_peaking_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *luminance_peaking_value)
{
    *luminance_peaking_value = (tw2864_byte_read(chip_addr,ch*0x10+0xb)&0x0f);
}

static int cti_control(unsigned char chip_addr,unsigned char ch ,unsigned char cti_value)
{
    unsigned char temp = 0;
    if(cti_value > 15)
    {
		return -1;
    }
    temp = tw2864_byte_read(chip_addr,ch*0x10+0xc);
    CLEAR_BIT(temp,0xf);
    SET_BIT(temp,cti_value);
    tw2864_byte_write(chip_addr,ch*0x10+0xc,temp);
    return 0;
}
static void cti_control_get(unsigned char chip_addr,unsigned char ch ,unsigned int *cti_value)
{
    *cti_value = (tw2864_byte_read(chip_addr,ch*0x10+0xc)&0x0f);
}

/*
 * tw2834 open routine.
 * do nothing.
 */

int tw2864_open(unsigned char chip_addr,struct inode * inode, struct file * file)
{
    return 0;
} 
/*
 * tw2864 close routine.
 * do nothing.
 */
int tw2864_close(unsigned char chip_addr,struct inode * inode, struct file * file)
{
    return 0;
}

Tw2864Ret tw2864_ioctl(unsigned char chip_addr,struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int __user *argp = (unsigned int __user *)arg;
    unsigned int tmp,tmp_help;
    tw2864_w_reg tw2864reg;
    tw2864_set_videomode tw2864_videomode;
    unsigned int tmp_reg=0;

    tw2864_set_controlvalue tw2864_controlvalue;
    switch(cmd)
    {
	case TW2864_READ_REG:
	    if (copy_from_user(&tmp_help, argp, sizeof(tmp_help)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_READ_REG_FAIL;
	    }
	    tmp = tw2864_byte_read(chip_addr,(u8)tmp_help);
	    if (copy_to_user(argp, &tmp, sizeof(tmp)))
	    {
            return -1;
	    }
	    break;
	case TW2864_WRITE_REG:
	    if (copy_from_user(&tw2864reg, argp, sizeof(tw2864reg)))
	    {
			printk("ttw2864a_ERROR");
			return TW2864_WRITE_REG_FAIL;
	    }
	    tw2864_byte_write(chip_addr,(u8)tw2864reg.addr,(u8)tw2864reg.value);
	    break;
	case TW2864_SET_ADA_PLAYBACK_SAMPLERATE :	    
	    break;
	case TW2864_GET_ADA_PLAYBACK_SAMPLERATE :	    
	    break;
	case TW2864_SET_ADA_PLAYBACK_BITWIDTH:	       
	    break;
	case TW2864_GET_ADA_PLAYBACK_BITWIDTH :	    
	    break;
	case TW2864_SET_ADA_PLAYBACK_BITRATE:	    
	    break;
	case TW2864_GET_ADA_PLAYBACK_BITRATE:
	    break;          
	case TW2864_SET_ADA_SAMPLERATE :
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_SET_ADA_SAMPLERATE_FAIL;
	    }        
	    if((tmp !=SET_8K_SAMPLERATE)&&(tmp !=SET_16K_SAMPLERATE))
	    {
			return 	TW2864_SET_ADA_SAMPLERATE_FAIL;
	    }
	    //tw2864_set_ada_playback_samplerate(chip_addr,SERIAL_CONTROL,samplerate);
	    break;
	case TW2864_GET_ADA_SAMPLERATE:
		{
		    int ada_samplerate= tw2864_byte_read(chip_addr,SERIAL_CONTROL);
		    if((ada_samplerate & 0x04)==0x04)
		    {
			ada_samplerate = SET_16K_SAMPLERATE;
		    }
		    else
		    {
			ada_samplerate = SET_8K_SAMPLERATE;
		    }
		    if (copy_to_user(argp,&ada_samplerate,sizeof(ada_samplerate)))
		    {
                return -1;
		    }
		}
	    break;               
	case TW2864_SET_ADA_BITWIDTH:
	    break;
	case TW2864_GET_ADA_BITWIDTH:
	    break;      
	case TW2864_SET_ADA_BITRATE:
	    break;
	case TW2864_GET_ADA_BITRATE:
	    break;     
	case TW2864_SET_D1:	    
	    break;
	case TW2864_SET_2_D1:	   
	    break;
	case TW2864_SET_4HALF_D1:	    
	    break;
	case TW2864_SET_4_CIF:	    
	    break;
	case  TW2864_SET_AUDIO_OUTPUT:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_SET_AUDIO_OUTPUT_FAIL;
	    }
	    if((tmp<=0)||(tmp>16))
	    {
			return TW2864_SET_AUDIO_OUTPUT_FAIL;
	    }
	    set_audio_output(chip_addr,tmp);
	    break;
	case TW2864_GET_AUDIO_OUTPUT:
	    tmp = tw2864_byte_read(chip_addr,0xd2);
	    if((tmp & 0x03)==0x0)
	    {
			tmp_help = 2;
	    }
	    else if((tmp & 0x03)==0x1)
	    {
			tmp_help =4;
	    }
	    else if((tmp & 0x03)==0x2)
	    {
			tmp_help =8;
	    }
	    else if((tmp & 0x03)==0x3)
	    {
			tmp_help =16;
	    }
	    if (copy_to_user(argp,&tmp_help,sizeof(tmp_help)))
	    {
            return -1;
	    }
	    break; 
	case  TW2864_SET_AUDIO_MIX_OUT:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_SET_AUDIO_MIX_OUT_FAIL;
	    }
	    set_audio_mix_out(chip_addr,tmp);
	    break;

	case TW2864_SET_AUDIO_RECORD_M:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_SET_AUDIO_RECORD_M_FAIL;
	    }
	    set_audio_record_m(chip_addr,tmp);
	    break;
	case TW2864_SET_MIX_MUTE:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_SET_MIX_MUTE_FAIL;
	    }
	    if(tmp<0||tmp>4)
	    {
			printk("TW2864A_SET_MIX_MUTE_FAIL\n");
			return TW2864_SET_MIX_MUTE_FAIL;
	    }  
	    set_audio_mix_mute(chip_addr,tmp);
	    break;
	case TW2864_CLEAR_MIX_MUTE:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_CLEAR_MIX_MUTE_FAIL;
	    }
	    if(tmp<0||tmp>4)
	    {
			printk("TW2864A_CLEAR_MIX_MUTE_FAIL\n");
			return TW2864_CLEAR_MIX_MUTE_FAIL;
	    }

	    clear_audio_mix_mute(chip_addr,tmp);
	    break;

	case TW2864_SET_VIDEO_MODE:
	    if (copy_from_user(&tw2864_videomode, argp, sizeof(tw2864_videomode)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tw2864_videomode.mode);
			return TW2864_SET_VIDEO_MODE_FAIL;
	    }
	    if((tw2864_videomode.mode != NTSC)&&(tw2864_videomode.mode !=PAL)&&(tw2864_videomode.mode !=AUTOMATICALLY))
	    {
			printk("set video mode %d error\n ",tw2864_videomode.mode);
			return TW2864_SET_VIDEO_MODE_FAIL;
	    }
            
	    tw2864_video_mode_init(chip_address_map[tw2864_videomode.ch/4],tw2864_videomode.mode,video_ch_map[tw2864_videomode.ch]);
	    break; 
	case TW2864_GET_VIDEO_MODE:
        if (copy_from_user(&tmp, argp, sizeof(tmp)))
        {
            printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
            return TW2864_GET_VIDEO_MODE_FAIL;
        }
	     
		tmp_help = (tw2864_byte_read(chip_address_map[tmp/4],video_ch_map[tmp]*0x10+0xe))& 0x7;

		if((tmp_help == 3) ||(tmp_help == 0))
		{
		    tmp = NTSC;
		}
		else if((tmp_help == 1) ||(tmp_help == 4)||(tmp_help == 5)||(tmp_help == 6))
		{
		    tmp = PAL;
		}
		if (copy_to_user(argp,&tmp,sizeof(tmp)))
		{
            return -1;
		}
		break;
	case TW2864_REG_DUMP:
		tw2864_reg_dump(chip_addr);
	    break;
	case TW2864_SET_CHANNEL_SEQUENCE:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
		printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
		return TW2864_SET_CHANNEL_SEQUENCE_FAIL;
	    }
	    if((tmp>3)||(tmp<0))
		return TW2864_SET_CHANNEL_SEQUENCE_FAIL;
	    channel_alloc(chip_addr,tmp);
	    break;
	case TW2864_SET_AUDIO_CASCAD:
	    if (copy_from_user(&tmp, argp, sizeof(tmp)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_SET_AUDIO_CASCAD_FAIL;
	    }
		set_audio_cascad(chip_addr,tmp);

	    break;
	case TW2864_HUE_CONTROL:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_HUE_CONTROL_FAIL;
	    } 
	    hue_control(chip_address_map[tw2864_controlvalue.ch/4],video_ch_map[tw2864_controlvalue.ch],tw2864_controlvalue.controlvalue);
	    break;
	case TW2864_GET_HUE_SET:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_HUE_SET_FAIL;
	    }
	    hue_control_get(chip_address_map[tw2864_controlvalue.ch/4],video_ch_map[tw2864_controlvalue.ch],&tw2864_controlvalue.controlvalue);
	    if (copy_to_user(argp,&tw2864_controlvalue, sizeof(tw2864_controlvalue)))
	    {
            return -1;
	    }
	    break;


	case TW2864_CONTRAST_CONTROL:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_CONTRAST_CONTROL_FAIL;
	    }

	    contrast_control(chip_address_map[tw2864_controlvalue.ch/4],video_ch_map[tw2864_controlvalue.ch],tw2864_controlvalue.controlvalue);

	    break;
	case TW2864_GET_CONTRAST_SET:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_CONTRAST_SET_FAIL;
	    }
	    contrast_control_get(chip_address_map[tw2864_controlvalue.ch/4],
            video_ch_map[tw2864_controlvalue.ch],&tw2864_controlvalue.controlvalue);
	    if (copy_to_user(argp,&tw2864_controlvalue, sizeof(tw2864_controlvalue)))
	    {
            return -1;
	    }
	    break;
	case TW2864_BRIGHTNESS_CONTROL:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_BRIGHTNESS_CONTROL_FAIL;
	    }
	    brightness_control(chip_address_map[tw2864_controlvalue.ch/4],video_ch_map[tw2864_controlvalue.ch],tw2864_controlvalue.controlvalue);
	    break;
	case TW2864_GET_BRIGHTNESS_SET:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_BRIGHTNESS_SET_FAIL;
	    }
	    brightness_control_get(chip_address_map[tw2864_controlvalue.ch/4],
            video_ch_map[tw2864_controlvalue.ch],&tw2864_controlvalue.controlvalue);
	    if (copy_to_user(argp,&tw2864_controlvalue, sizeof(tw2864_controlvalue)))
	    {
            return -1;
	    }
	    break; 
	case TW2864_GET_VEDIO_LOST_INFORMATION:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return  -1;
	    }
	    video_lost_check(chip_address_map[tw2864_controlvalue.ch/4],
            video_ch_map[tw2864_controlvalue.ch],&tw2864_controlvalue.controlvalue);
	    if (copy_to_user(argp,&tw2864_controlvalue, sizeof(tw2864_controlvalue)))
	    {
            return -1;
	    }
	    break;
	case TW2864_SATURATION_CONTROL:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return -1;
	    }
	    saturation_control(chip_address_map[tw2864_controlvalue.ch/4],
            video_ch_map[tw2864_controlvalue.ch],tw2864_controlvalue.controlvalue);
	    break;

	case TW2864_GET_SATURATION_SET:
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_BRIGHTNESS_SET_FAIL;
	    }
        if(0!=saturation_control_get(chip_address_map[tw2864_controlvalue.ch/4],
            video_ch_map[tw2864_controlvalue.ch],&tw2864_controlvalue.controlvalue))
	    {
			printk(" get saturation error!\n");
			return -1;
	    }
	    if (copy_to_user(argp,&tw2864_controlvalue, sizeof(tw2864_controlvalue)))
	    {
            return -1;
	    }
	    break;
	case TW2864_LUMINANCE_PEAKING_CONTROL:
	    break;
	case TW2864_GET_LUMINANCE_PEAKING_SET:
	    break;
	case TW2864_CTI_CONTROL:
	    break;
	case TW2864_GET_CTI_SET: 
	    break; 
	case TW2864_SET_PLAYBACK_MODE:
	    if (copy_from_user(&tmp, argp, sizeof(tmp))) 
	    {
    		printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
    		return TW2864_SET_PLAYBACK_MODE_FAIL;
	    }
	    tmp &= 0x1;
	    tmp_help = (tw2864_byte_read(chip_addr,SERIAL_PLAYBACK_CONTROL)&0xbf); 
	    tmp_help |=(tmp << 6);
	    tw2864_byte_write(chip_addr,SERIAL_PLAYBACK_CONTROL,tmp_help); 
	     break;
	case TW2864_SET_CLOCK_OUTPUT_DELAY:
	    if (copy_from_user(&tmp, argp, sizeof(tmp))) 
	    {
    		printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
    		return TW2864_SET_CLOCK_OUTPUT_DELAY_FAIL;
	    }
	    tw2864_byte_write(chip_addr,0x4d,tmp);
	     break;
	case TW2864_LOCAL_AUDIO_LOOP: 
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_CTI_SET_FAIL;
	    }

	    // select mix_output channel;
	    tmp_help = tw2864_byte_read(chip_addr,0xe0)&0xe0; 
	    if(tw2864_controlvalue.ch>3)
	    {
			printk(" this boad only support 1 audio input!\n");
			return -1;
	    }

	    tmp_help +=ch_map[tw2864_controlvalue.ch];
	    tw2864_byte_write(chip_addr,0xe0,tmp_help);

	    //select mute or not 
	    tmp_help = tw2864_byte_read(chip_addr,0xdc)&0xe0; 
	    if(tw2864_controlvalue.controlvalue==0)
	    {
		    tw2864_byte_write(chip_addr,0xe0,0x14);
			tmp_help += 0x1f;
	    }
	    tw2864_byte_write(chip_addr,0xdc,tmp_help);
	    break; 

	case TW2864_PLAYBACK_AUDIO_LOOP: 
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_CTI_SET_FAIL;
	    }

	    // select mix_output channel;
	    tmp_help = tw2864_byte_read(chip_addr,0xe0)&0xe0; 
	    tmp_help += 16;
	    tw2864_byte_write(chip_addr,0xe0,tmp_help);

	    //select mute or not 
	    tmp_help = tw2864_byte_read(chip_addr,0xdc)&0xe0; 
	    if(tw2864_controlvalue.controlvalue==0)
	    {
		    tw2864_byte_write(chip_addr,0xe0,0x14);    
			tmp_help += 0x1f;
	    }
	    tw2864_byte_write(chip_addr,0xdc,tmp_help);
	    break; 
	case TW2864_AUDIO_OUTPUT_GAIN: 
	    if (copy_from_user(&tmp_reg, argp, sizeof(unsigned int)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return -1;
	    }

	    if(tmp_reg > 15)
	    {
			printk("ao output gain out of gain!\n");
			return -1;
	    }

	    tmp_help = tw2864_byte_read(chip_addr,SERIAL_AUDIO_OUTPUT_GAIN_REG)&0x0f; 
	    tmp_help += tmp_reg<<4;
	    tw2864_byte_write(chip_addr,SERIAL_AUDIO_OUTPUT_GAIN_REG,tmp_help);
	    break; 

	case TW2864_AUDIO_INPUT_GAIN: 
	    if (copy_from_user(&tw2864_controlvalue, argp, sizeof(tw2864_controlvalue)))
	    {
			printk("\ttw2864a_ERROR: WRONG cpy tmp is %x\n",tmp);
			return TW2864_GET_CTI_SET_FAIL;
	    }

	    if((tw2864_controlvalue.ch>3)&&(tw2864_controlvalue.ch<0))
	    {
			printk("channel num out of range!\n");
			return -1;
	    }

	    if((tw2864_controlvalue.controlvalue>15)&&(tw2864_controlvalue.controlvalue<0))
	    {
			printk(" audio input gain  out of range!\n");
			return -1;
	    }

	    if((ch_map[tw2864_controlvalue.ch]%2)==0)
	    {
			if((ch_map[tw2864_controlvalue.ch]/2)<1)
			{
			    tmp_help = tw2864_byte_read(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG1)&0xf0; 
			    tmp_help +=tw2864_controlvalue.controlvalue;
			    tw2864_byte_write(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG1,tmp_help);
			}
			else
			{
			    tmp_help = tw2864_byte_read(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG2)&0xf0; 
			    tmp_help +=tw2864_controlvalue.controlvalue;
			    tw2864_byte_write(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG2,tmp_help);
			}
	    }
	    else
	    {
			if((ch_map[tw2864_controlvalue.ch]/2)<1)
			{
			    tmp_help = tw2864_byte_read(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG1)&0x0f; 
			    tmp_help +=tw2864_controlvalue.controlvalue <<4;
			    tw2864_byte_write(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG1,tmp_help);
			}
			else
			{
			    tmp_help = tw2864_byte_read(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG2)&0x0f; 
			    tmp_help +=tw2864_controlvalue.controlvalue <<4;
			    tw2864_byte_write(chip_addr,SERIAL_AUDIO_INPUT_GAIN_REG2,tmp_help);
			}
	    }

	    break; 
	default:
	    break;
    }

    return TW2864_IOCTL_OK;
}

int tw2864a_open(struct inode * inode, struct file * file)
{
    return tw2864_open(TW2864A_I2C_ADDR,inode,file);
} 

int tw2864a_close(struct inode * inode, struct file * file)
{
    return tw2864_close(TW2864A_I2C_ADDR,inode,file);
}

int tw2864a_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	return tw2864_ioctl(TW2864A_I2C_ADDR,inode, file, cmd, arg);
}
/*-------------------------------------------*/
int tw2864b_open(struct inode * inode, struct file * file)
{
    return tw2864_open(TW2864B_I2C_ADDR,inode,file);
} 

int tw2864b_close(struct inode * inode, struct file * file)
{
    return tw2864_close(TW2864B_I2C_ADDR,inode,file);
}

int tw2864b_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	return tw2864_ioctl(TW2864B_I2C_ADDR,inode, file, cmd, arg);
}

/*-------------------------------------------*/
int tw2864c_open(struct inode * inode, struct file * file)
{
    return tw2864_open(TW2864C_I2C_ADDR,inode,file);
} 

int tw2864c_close(struct inode * inode, struct file * file)
{
    return tw2864_close(TW2864C_I2C_ADDR,inode,file);
}

int tw2864c_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	return tw2864_ioctl(TW2864C_I2C_ADDR,inode, file, cmd, arg);
}
/*---------------------------------------------*/
int tw2864d_open(struct inode * inode, struct file * file)
{
    return tw2864_open(TW2864D_I2C_ADDR,inode,file);
} 

int tw2864d_close(struct inode * inode, struct file * file)
{
    return tw2864_close(TW2864D_I2C_ADDR,inode,file);
}

int tw2864d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	return tw2864_ioctl(TW2864D_I2C_ADDR,inode, file, cmd, arg);
}




/*
 *      The various file operations we support.
 */
static struct file_operations tw2864_fops[] = {
	{
	    .owner      = THIS_MODULE,
	    .ioctl      = tw2864a_ioctl,
	    .open       = tw2864a_open,
	    .release    = tw2864a_close
	},
	{
	    .owner      = THIS_MODULE,
	    .ioctl      = tw2864b_ioctl,
	    .open       = tw2864b_open,
	    .release    = tw2864b_close
	},
	{
	    .owner      = THIS_MODULE,
	    .ioctl      = tw2864c_ioctl,
	    .open       = tw2864c_open,
	    .release    = tw2864c_close
	},
	{
	    .owner      = THIS_MODULE,
	    .ioctl      = tw2864d_ioctl,
	    .open       = tw2864d_open,
	    .release    = tw2864d_close
	}
};

static struct miscdevice tw2864_dev[] = {
	{   
		.minor		= MISC_DYNAMIC_MINOR,
	    .name		= "tw2864adev",
	    .fops  		= &tw2864_fops[0],
	},
	{   
		.minor		= MISC_DYNAMIC_MINOR,
	    .name		= "tw2864bdev",
	    .fops  		= &tw2864_fops[1],
	},
	{   
		.minor		= MISC_DYNAMIC_MINOR,
	    .name		= "tw2864cdev",
	    .fops  		= &tw2864_fops[2],
	},
	{   
		.minor		= MISC_DYNAMIC_MINOR,
	    .name		= "tw2864ddev",
	    .fops  		= &tw2864_fops[3],
	},
};

/* 1--ntsc , 2 -- pal */
static int norm = PAL;
/*chip nums on board*/
static int chips = 4;

module_param(norm, uint, S_IRUGO);
module_param(chips, uint, S_IRUGO);


static int __init tw2864_init(void)
{
    int ret = 0;
	int i = 0;
	int j = 0;

	printk("norm:%d,chips:%d\n",norm,chips);
	
	if (norm != NTSC && norm != PAL)
	{
		printk("module param norm must be PAL(%d) or NTSC(%d)\n",PAL,NTSC);
		return -1;
	}
	if (chips <= 0||chips > (sizeof(tw2864_dev)/sizeof(tw2864_dev[0])))
	{
		printk("chips must be 1~%d\n",(sizeof(tw2864_dev)/sizeof(tw2864_dev[0])) );
		return -1;
	}    

    /*register tw2864_dev*/
	for (i = 0 ;i < chips; i ++)
	{
	    ret = misc_register(&tw2864_dev[i]);
	    if (ret)
	    {
			printk("ERROR: could not register tw2864 devices:%d. \n",i);
			goto FAILED;
	    }
		
	    tw2864_device_video_init(tw2864_i2c_addr[i], norm);
        
        tw2864_audio_init(tw2864_i2c_addr[i]);
	}
    
    tw2864_audio_cascade();       
	
    printk("tw2864 driver init successful!\n");
    return 0;
	
FAILED:
	for(j = 0; j < i; j ++)
	{
	    misc_deregister(&tw2864_dev[j]);
	}
	return ret;
	
}



static void __exit tw2864_exit(void)
{
	int i = 0;
	
	for(i = 0 ;i < chips; i ++)
	{
	    misc_deregister(&tw2864_dev[i]);
	}
}

module_init(tw2864_init);
module_exit(tw2864_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");

