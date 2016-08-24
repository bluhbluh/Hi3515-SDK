
//#define VXWORKS
#ifndef VXWORKS
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
#include <linux/moduleparam.h>

#include "gpio_i2c.h"
#endif

#include "adv7441.h"
#include "adv7441_def.h"

//#define printk printf

#define DEBUG_ADV7400 1

//static unsigned int ADV7400_dev_open_cnt =0;

#ifndef VXWORKS
#define gpio_i2c0_read gpio_i2c_read
#define gpio_i2c0_write gpio_i2c_write

#endif


unsigned char adv7400_cvbs_mode[]=
{
	DETECT_NTSC_M,				//NTSC_M
	DETECT_NTSC_443,			//NTSC_443
	DETECT_PAL_M,				//PAL_M
	DETECT_PAL_60,				//PAL_60
	DETECT_PAL_BGHID,			//PAL_BG
	DETECT_SECAM,				//SECAM
	DETECT_PAL_N,				//PAL_N
	DETECT_SECAM_525			//SECAM_525
};


MODEDETECTTB adv7400_ycbcr_mode[]=
{
	{DETECT_720_480_I60,		BL_480I,		SCF_480I_1,	SCF_480I_2},
	{DETECT_720_576_I50,		BL_576I,		SCF_576I_1,	SCF_576I_2},
	{DETECT_720_480_P60,	BL_480P,		SCF_480P_1,	SCF_480P_2},
	{DETECT_720_576_P50,	BL_576P,		SCF_576P_1,	SCF_576P_2},
	{DETECT_1280_720P,		BL_720P,		SCF_720P_1,	SCF_720P_2},
	{DETECT_1920_1080_I50,	BL_1080I50,	SCF_1080I50_1,	SCF_1080I50_2},
	{DETECT_1920_1080_I60,	BL_1080I60,	SCF_1080I60_1,	SCF_1080I60_2},
	{0xff,					0xffff,		0xffff},
};



#if 0
unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address);
void gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char value);
#endif




int ADV7441_read(unsigned char MAP_ADDR,unsigned char sub_add)
{
	int value;	
	value = gpio_i2c0_read(MAP_ADDR,sub_add);
	return value;
}

void ADV7441_write(unsigned char MAP_ADDR,unsigned char sub_add,unsigned char data_para)
{
	
	gpio_i2c0_write(MAP_ADDR,sub_add,data_para);
	return;
}
   



void Adv7400_Set_YcbcrMode(unsigned char videomode)
{

	unsigned char i;
	REGTABLE *reglink;
	i = 0;
	reglink = adi7401_initial_data;
	while(0xff != reglink[i].ucAddr || 0xff !=reglink[i].ucValue)
	{
		ADV7441_write(adv7441_I2C_ADDR_MAP,reglink[i].ucAddr,reglink[i].ucValue);
		i++;
	}

	switch(videomode)
	{
		case DETECT_720_576_I50:
			reglink = adi7401_YPbPr576i_data;	
			break;
		case DETECT_720_480_P60:
			reglink = adi7401_YPbPr480P_data;
		
			break;
		case DETECT_720_576_P50:
			reglink = adi7401_YPbPr576P_data;
	
			break;
		case DETECT_1280_720P:
		
			printk("ADV7400 driver select 720P successful!\n");

			reglink = adi7401_YPbPr720P_data;
		
			break;
		case DETECT_1920_1080_I50:
			reglink = adi7401_YPbPr1080i50_data;
			break;
		case DETECT_1920_1080_I60:
			reglink = adi7401_YPbPr1080i60_data;
			break;
		default:
			reglink = adi7401_YPbPr480i_data;
			break;
	}
	i = 0;
	while(0xff != reglink[i].ucAddr || 0xff !=reglink[i].ucValue)
	{
		ADV7441_write(adv7441_I2C_ADDR_MAP,reglink[i].ucAddr,reglink[i].ucValue);
		i++;
	}			

	return;
}




void Adc7400_Set_VgaMode(unsigned char video_mode)
{
	ADV7441_write(adv7441_I2C_ADDR_MAP,0x01,0xc8);
	//8BIT 656输出
	ADV7441_write(adv7441_I2C_ADDR_MAP,0x03,0x0c);
	//通道选择
	ADV7441_write(adv7441_I2C_ADDR_MAP,0x05,0x02);
	switch(video_mode)
	{
		case DETECT_60XGA:
		case DETECT_NONE:
			//设置信号输出属性
		 	ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x55);
			//设置信号格式为1024*768*60
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0c);
			//ADI控制
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x05);
			//PLL 电流控制为500UA
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5D);  //5B
			break;
		case DETECT_70XGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x55);
			//设置信号格式为1024*768*70
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0D);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x05);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5D);  
			break;					
		case DETECT_75XGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x55);
			//设置信号格式为1024*768*75
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0E);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x05);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5D);   //5B
			break;	
		case DETECT_85XGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x55);
			//设置信号格式为1024*768*85
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0F);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x05);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5D);  //5B
			break;			
		case DETECT_60SVGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x01);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5c);  
			break;
		case DETECT_72SVGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x02);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5c); 
 			break;
		case DETECT_75SVGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x03);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5c);  
		   	break;
		case DETECT_85SVGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x55);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x04);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x05);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5b);  
			break;
		case DETECT_60VGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x55);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x08);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x05);	
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5b);
			break;
		case DETECT_72VGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x09);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5b);
			break;
		case DETECT_75VGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0a);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5c);
			break;
		case DETECT_85VGA:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0b);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5b);
			break;
		case DETECT_DOS185:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0b);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5C);
			break;
		case DETECT_DOS285:
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x04,0x75);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x06,0x0b);//有问题
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x0f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3c,0x5C);//锁相环配置
			break;
		default:
			break;
	}		
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x17,0x01);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x27,0x58);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x31,0x12);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x38,0x80);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x39,0xc0);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3a,0x10);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3b,0x80);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x3d,0x33);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x41,0x41);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x4d,0xef);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x67,0x03);			
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x69,0x00);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x6a,0x00);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x6b,0xc2);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x73,0x90);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x77,0x3f);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x78,0xff);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x79,0xff);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x7a,0xff);			
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x7b,0x1c);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x85,0x42);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x86,0x02);
           	if(video_mode == DETECT_DOS185) //640x400
            {
        		ADV7441_write(adv7441_I2C_ADDR_MAP,0x87,0xE3);
    			ADV7441_write(adv7441_I2C_ADDR_MAP,0x88,0x40);
    			ADV7441_write(adv7441_I2C_ADDR_MAP,0x8A,0xB0);
            }
            else if(video_mode == DETECT_DOS285)//720x400
            {
        		ADV7441_write(adv7441_I2C_ADDR_MAP,0x87,0xE3);
    			ADV7441_write(adv7441_I2C_ADDR_MAP,0x88,0xA8);
    			ADV7441_write(adv7441_I2C_ADDR_MAP,0x8A,0xB0);
            }
            else
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x87,0x60);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x88,0x00);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0x89,0x08);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xb5,0x03);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xbf,0x02);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xc0,0x02);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xc2,0x02);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xc3,0x02);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xc4,0x02);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xc9,0x10);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xcd,0x10);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xce,0x10);			
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xd0,0x46);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xd4,0x01);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xd6,0x3e);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xdc,0x7b);		
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xe2,0x80);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xe3,0x80);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xe4,0x80);
			ADV7441_write(adv7441_I2C_ADDR_MAP,0xe8,0x65);
			if((video_mode == DETECT_85VGA)||(video_mode == DETECT_85SVGA)||(video_mode == DETECT_60XGA))
			{
				ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x80); 
				ADV7441_write(adv7441_I2C_ADDR_MAP,0x58,0xed); 
				ADV7441_write(adv7441_I2C_ADDR_MAP,0x0e,0x00); 
			}
}

void Adv7400_blue_screen(unsigned char enable)
{
    	unsigned char i, cp_i;
    	i = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,DEFAULT_VALUE_Y);
    	cp_i = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,CP_DEF_VAL_EN);
    	//
    	if(enable == 1)  //blue screen enable
    	{
        	ADV7441_write(adv7441_I2C_ADDR_MAP,0x36, 0x02);        
        	//设置Y，CB = 0xE0，CR=0x60
        	ADV7441_write(adv7441_I2C_ADDR_MAP,0x0d, 0x6E); 
        	//FORCE TO OUTPUT DEFAULT YCBCR
        	i = i | DEF_VAL_EN;
        	ADV7441_write(adv7441_I2C_ADDR_MAP,DEFAULT_VALUE_Y, i);     
		//设置输出单色颜色Y，CB，CR
       	ADV7441_write( adv7441_I2C_ADDR_MAP,0xc0, 35);        
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0xc1, 212);  
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0xc2, 114); 
        	//输出设置的单色颜色
        	cp_i = cp_i | CP_DEF_COL_MAIN_VAL;
        	ADV7441_write( adv7441_I2C_ADDR_MAP,CP_DEF_VAL_EN, cp_i);     
    	}
    	else if(enable == 0)   //raw data enable
    	{
    		//关闭FORCE输出
       	i = i & (~DEF_VAL_EN);
        	ADV7441_write( adv7441_I2C_ADDR_MAP,DEFAULT_VALUE_Y, i);   
        	cp_i = cp_i & (~CP_DEF_COL_MAIN_VAL);
        	ADV7441_write( adv7441_I2C_ADDR_MAP,CP_DEF_VAL_EN, cp_i);              
    	}
    	else if(enable == 2) //black screen enable
    	{
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0x36, 0x00);        
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0x0d, 0x88);          
        	i = i | DEF_VAL_EN;
        	ADV7441_write( adv7441_I2C_ADDR_MAP,DEFAULT_VALUE_Y, i);     
       
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0xc0, 0x30);        
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0xc1, 0x80);  
        	ADV7441_write( adv7441_I2C_ADDR_MAP,0xc2, 0x80);        
        	cp_i = cp_i |(CP_DEF_COL_FORCE+CP_DEF_COL_MAIN_VAL);
        	ADV7441_write( adv7441_I2C_ADDR_MAP,CP_DEF_VAL_EN, cp_i);     
    	}
}

void Adv7400_channel(unsigned char video_in)
{
	switch(video_in)
	{
		//AV INPUT FORM A11 
		case AV_IN:
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,PRIM_MODE,0x00);
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,SDM_SEL,0x01);
			//ENABLE 	ADC0
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,ADC_POWER_CONTROL,0x17);
			break;
		//YC INPUT FORM A10,A12 	
		case SVIDEO_IN:
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,PRIM_MODE,0x00);
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,SDM_SEL,0x02);		
			//ENABLE	ADC0,ADC1
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,ADC_POWER_CONTROL,0x13);
			break;
		//Y CB CR FORM A6,A4,A5
		case YCBCR_IN:
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,PRIM_MODE,0x01);
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,ADC_POWER_CONTROL,0x21);
//			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,0x85,0x18);  // Turn off SSPD as SYNC is embedded on Y
			break;
		//R G B FORM A3,A2,A1
		case VGA_IN:
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,PRIM_MODE,0x02);
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,ADC_POWER_CONTROL,0x11);
			gpio_i2c0_write(adv7441_I2C_ADDR_MAP,0x85,0x42);  
			break;
		default:
			break;
	}
}

#if 0
unsigned char Adv7400_detect(unsigned char video_in)
{	
	unsigned char status1,status3,SCVS;
	unsigned int BL,SCF;

	switch(video_in)
	{
		case AV_IN:
			 break;
		case SVIDEO_IN:
			status1 = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,STATUS1);
			status3 =  gpio_i2c0_read(adv7441_I2C_ADDR_MAP,STATUS3);
	 		if(((status3&STATUS3_INST_HLOCK)== 0x01)&&((status1&STATUS1_IN_LOCK)== 0x01)&&(ErrorId == 0xff))
            		{	
//            		status1 = status1 & STATUS1_DETECT_SYSTEM;
            			status1 = status1>>4;
            			return adv7400_cvbs_mode[status1&0x07];
			 }
			 else
   			    	return DETECT_NONE;
			break;
		case YCBCR_IN:  
			 return YCBCR_IN;
			 break;
		case VGA_IN :			
			status1 = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,RB_STANDARD_IDENTIFICATION_1);
    		if(status1 & SDTI_DVALID)
			{
				//读取BL
				BL = (status1 & 0x3f) <<8;
				status1 = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,RB_STANDARD_IDENTIFICATION_2);
				BL = BL + status1;
				//读取SCVS
				status1 = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,RB_STANDARD_IDENTIFICATION_3);
				SCVS = status1>>3;
				//读取SCF
				SCF = (status1&0x07)<<8;
				status1 = gpio_i2c0_read(adv7441_I2C_ADDR_MAP,RB_STANDARD_IDENTIFICATION_4);
				SCF = SCF + status1;
			}
    		if(video_in == YCBCR_IN)
    		{
    			status1 = 0;
    			while(adv7400_ycbcr_mode[status1].ucMode != 0xff)
    			{
    				if((BL >= (adv7400_ycbcr_mode[status1].bl-BL_DITHER))
    					&&(BL <= (adv7400_ycbcr_mode[status1].bl+BL_DITHER))
    					&&(SCF >= (adv7400_ycbcr_mode[status1].scf1))
    					&&(SCF <= (adv7400_ycbcr_mode[status1].scf2)))
    					return adv7400_ycbcr_mode[status1].ucMode;
    				status1++;
    			}
    			return DETECT_NONE;
    		}
			else
			{
				if(((BL > 6817)&&(BL < 6997))&&((SCF > 522)&&(SCF < 526))) 		return DETECT_DOS185;
	 			else if(((BL > 6817)&&(BL < 6997))&&((SCF > 522)&&(SCF < 526))) 	return DETECT_DOS285;        
	 			else if(((BL > 6817)&&(BL < 6997))&&((SCF > 522)&&(SCF < 526))) 	return DETECT_60VGA;
			    	else if(((BL > 5659)&&(BL < 5739))&&((SCF > 516)&&(SCF < 521)))	return DETECT_72VGA;	
				else if(((BL > 5720)&&(BL < 5800))&&((SCF > 496)&&(SCF < 501)))	return DETECT_75VGA;
				else if(((BL > 4948)&&(BL < 5028))&&((SCF > 505)&&(SCF < 510)))	return DETECT_85VGA;	
				else if(((BL > 5659)&&(BL < 5739))&&((SCF > 623)&&(SCF < 629)))	return DETECT_60SVGA;
				else if(((BL > 4451)&&(BL < 4531))&&((SCF > 659)&&(SCF < 667)))	return DETECT_72SVGA;	 
				else if(((BL > 4566)&&(BL < 4820))&&((SCF > 621)&&(SCF < 626)))	return DETECT_75SVGA;
				else if(((BL > 3982)&&(BL < 4062))&&((SCF > 627)&&(SCF < 632)))	return DETECT_85SVGA;	
				else if(((BL > 4323)&&(BL < 4503))&&((SCF > 799)&&(SCF < 807)))	return DETECT_60XGA;
				else if(((BL > 3783)&&(BL < 3963))&&((SCF > 799)&&(SCF < 807)))	return DETECT_70XGA;	
				else if(((BL > 3560)&&(BL < 3740))&&((SCF > 796)&&(SCF < 805)))	return DETECT_75XGA;
				else if(((BL > 3104)&&(BL < 3184))&&((SCF > 804)&&(SCF < 809)))	return DETECT_85XGA;	
		        	else															return DETECT_NONE;
			}
			break;
		default :
			return DETECT_NONE;
	  }
	  return -1;
}
#endif

int adv7441_720P_60HZ(void)
{
    ADV7441_write(0x42,0x03,0x0c);
	ADV7441_write(0x42,0x05,0x01);
	ADV7441_write(0x42,0x06,0x0a);
	ADV7441_write(0x42,0x1d,0x40);
	ADV7441_write(0x42,0x3c,0xa8);
	ADV7441_write(0x42,0x47,0x0a);
	
    //ADV7441_write(0x42,0x68,0xf0);  // bit1 0: output YUV  1: output RGB 
	ADV7441_write(0x42,0x6b,0xd3); // set cpop_sel: 0xd1: for 8 bit 4:2:2 mode  0xd6: 16bit 4:2:2
	
	ADV7441_write(0x42,0x7b,0x2f);
	
	ADV7441_write(0x42,0x85,0x19);
	ADV7441_write(0x42,0xba,0xa0);
	ADV7441_write(0x42,0xf4,0x3f);  // port highest driver.
	printk("720p/60Hz YPrPb In 1X1 30Bit 444 Out through DAC ok !\n");
    return 0;
}

int adv7441_1080I_60HZ(void)
{
    ADV7441_write(0x42,0x03,0x0c);
	ADV7441_write(0x42,0x05,0x01);     // Prim_Mode =001b COMP
	ADV7441_write(0x42,0x06,0x0c);    //VID_STD=1100b for 1125 1x1 
	ADV7441_write(0x42,0x1d,0x40);
	ADV7441_write(0x42,0x3c,0xa8);
	ADV7441_write(0x42,0x47,0x0a);
	ADV7441_write(0x42,0x6b,0xd3); // set cpop_sel: 0xd1: for 20 bit 4:2:2 mode  0xd3: 16bit 4:2:2
	ADV7441_write(0x42,0x7b,0x0f); // bit4  1: channel a/b have the same AV code. bit5 0: output progressive timing  bit6,7 0: F /V bit as default polarity.
	ADV7441_write(0x42,0x85,0x19);
	ADV7441_write(0x42,0xba,0xa0);
	ADV7441_write(0x42,0xf4,0x3f);  // port highest driver.
	printk("1080i 60Hz 1920x1080 YPrPb In 1x1 20Bit 422 Out through Encoder (AV Codes): is ok \n");	
    return 0;
}

int adv7441_set_video_format(ADV7441_VIDEO_FORMAT_E enFormat)
{
    if (VIDEO_FORMAT_720P_60HZ == enFormat)
    {
        adv7441_720P_60HZ();
    }
    else if (VIDEO_FORMAT_1080I_60HZ == enFormat)
    {
        adv7441_1080I_60HZ();
    }
    else
    {
        printk("not suppor this video format");
        return -1;
    }
    
    return 0;
}

int adv7441_device_init(void)
{
		int mode = DETECT_1920_1080_I60;	
        
		if(mode == DETECT_1280_720P)
		{
				ADV7441_write(0x42,0x03,0x0c);
				ADV7441_write(0x42,0x05,0x01);
				ADV7441_write(0x42,0x06,0x0a);
				ADV7441_write(0x42,0x1d,0x40);
				ADV7441_write(0x42,0x3c,0xa8);
				ADV7441_write(0x42,0x47,0x0a);
				
		//		ADV7441_write(0x42,0x68,0xf0);  // bit1 0: output YUV  1: output RGB 
				ADV7441_write(0x42,0x6b,0xd3); // set cpop_sel: 0xd1: for 8 bit 4:2:2 mode  0xd6: 16bit 4:2:2
				
				ADV7441_write(0x42,0x7b,0x2f);
				
				ADV7441_write(0x42,0x85,0x19);
				ADV7441_write(0x42,0xba,0xa0);
				ADV7441_write(0x42,0xf4,0x3f);  // port highest driver.

//				ADV7441_write(0x42,0x00,0xfd);
//				ADV7441_write(0x42,0x10,0x01);
				printk("720p/60Hz YPrPb In 1X1 30Bit 444 Out through DAC ok !\n");
	
		}
/*
42 03 0C ; Disable TOD
42 05 01 ; Prim_Mode =001b COMP
42 06 0C ; VID_STD=1100b for 1125 1x1
42 1D 40 ; Disable TRI_LLC
42 3C A8 ; SOG Sync level for atenuated sync, PLL Qpump to default
42 47 0A ; Enable Automatic PLL_Qpump and VCO Range
42 6B D1 ; Setup CPOP_SEL for 20 Bit.656 Enable
42 85 19 ; Turn off SSPD and force SOY. For Eval Board.
42 BA A0 ; Enable HDMI and Analog in
*/
		else if (mode == DETECT_1920_1080_I60)
		{
			ADV7441_write(0x42,0x03,0x0c);
			ADV7441_write(0x42,0x05,0x01);     // Prim_Mode =001b COMP
			ADV7441_write(0x42,0x06,0x0c);    //VID_STD=1100b for 1125 1x1 
			ADV7441_write(0x42,0x1d,0x40);
			ADV7441_write(0x42,0x3c,0xa8);
			ADV7441_write(0x42,0x47,0x0a);
			ADV7441_write(0x42,0x6b,0xd3); // set cpop_sel: 0xd1: for 20 bit 4:2:2 mode  0xd3: 16bit 4:2:2
			ADV7441_write(0x42,0x7b,0x0f); // bit4  1: channel a/b have the same AV code. bit5 0: output progressive timing  bit6,7 0: F /V bit as default polarity.
			ADV7441_write(0x42,0x85,0x19);
			ADV7441_write(0x42,0xba,0xa0);
			ADV7441_write(0x42,0xf4,0x3f);  // port highest driver.
						
			printk("1080i 60Hz 1920x1080 YPrPb In 1x1 20Bit 422 Out through Encoder (AV Codes): is ok \n");			
		}	

//#ifndef VXWORKS
#if 0
    int ret = 0;
    int temp0=0;
    int temp1=0;
    unsigned char inputmod = DETECT_NONE;
		unsigned char oldmod;
		unsigned char i,j,k;
		unsigned char noequal,noeuqalmode;
	unsigned char ucSource;
	
    int mode =DETECT_1280_720P;	  // DEFAULT IS 720P
  //  int mode =DETECT_1920_1080_I60;
    
    unsigned char video_in_7400 = YCBCR_IN;
     /*register adv7400_dev*/
    
    CONTROL tv_control;    
    INPUTMODEDETECT	 inputdetect;
    
    // ret = misc_register(&ADV7400_dev);
    printk("ADV7441 driver init start ... \n");
    if (ret)
    {
        printk("\tADV7400_ERROR: could not register ADV7400 devices. \n");
        return ret;
    }
    
    
   	tv_control.ucSource	= 	YCBCR_IN;
	tv_control.ucDigitalMode	=	0;
	tv_control.ucAnglogtMode  =	DETECT_BEGIN;
//  	VariableInitial();         // 初始化为YCBCR_IN 模式输入；
//	UIStatusInit(UI_NONE);
//	GPIO_Control_initial();
	ucSource=YCBCR_IN;
	tv_control.ucSource = ucSource;
	printk("tv_control regester= %x \n",tv_control.ucSource);

//	Set_Source(YCBCR_IN);
	Adv7400_channel(YCBCR_IN);       //input style and power control
//	Input_AnalogChannel(YCBCR_IN);
//	Input_DetectInitial();
    printk("111111111");
	inputdetect.ucSourceModifyDelay = 0;
	inputdetect.ucModeDetectDelay = 0;
	tv_control.ucAnglogtMode = DETECT_BEGIN;



/******************************************	
//#ifdef _DETECT_ENABLE_
	ucSource = tv_control.ucSource;
	printf("ADV7400 0x01 regester= %x \n",ucSource);
//	if(ucSource <DIGITAL_IN)		
//		display_screen(SCREEN_BLACK);
	Adv7400_channel(ucSource);                   //????????????????
*******************************************/



//	Input_AnalogChannel(ucSource);
//	Input_DetectInitial();
	inputdetect.ucSourceModifyDelay = 0;
	inputdetect.ucModeDetectDelay = 0;
	tv_control.ucAnglogtMode = DETECT_BEGIN;
	printk("UC ANGLOGTMODE regester= %x \n",tv_control.ucAnglogtMode);

//	Input_SourceModifyTimer(20);
//#endif
	
	for(k=0;k<1;k++)
	{
//		count = GetKeyValue();
//#ifdef	_DETECT_ENABLE_
//		Input_Detectmain();
		noequal = 0;
		noeuqalmode = DETECT_NONE;
		oldmod = tv_control.ucAnglogtMode;
		printk("ADV7400 oldmod = %x \n",oldmod);
		
		for(i = 0;i<6;i++)
		{
			inputmod = Adv7400_detect(tv_control.ucSource);
			printk("ADV7400 inputmod = %x \n",inputmod);
			if(inputmod != oldmod)
			{
				noequal ++;
				if(noeuqalmode != inputmod)
				{
					noeuqalmode = inputmod;
					noequal = 0;
				}
			}	
			for(j=0;j<10;j++);
		}
		
//#endif		
//		UIMain(count);
//#ifdef	_OSD_ENABLE_
//		OSDMain();
//#endif
//		TimerMain();
	} 
#if 0    
    printk("ADV7400 detect run !\n");
    Adv7400_Set_YcbcrMode(mode);            //select 720p mode!!!!
    
    printk("ADV7400 driver init successful!\n");

    Adv7400_channel(video_in_7400);
    printk("ADV7400 select channel run successful!\n");

    
    temp0=ADV7441_read(0x01);
    
    printk("ADV7400 0x01 regester=0x %x \n",temp0);
#endif    
    
//	ADV7441_write(0x01,0xc7);
	
//	temp1=ADV7441_read(0x05);
	//printk("ADV7400 temp1 0x05register= %d \n",temp1);
	
	//ADV7441_write(0x06,0xc7);
	
	temp1=ADV7441_read(0x05);
	printk("ADV7400 temp1 0x05 register=0x %x \n",temp1);
	
	temp1=ADV7441_read(0x06);
	printk("ADV7400 temp1 0x06 register=0x %x \n",temp1);
	
	temp1=ADV7441_read(0x3a);
	printk("ADV7400 temp1 0x3a register=0x %x \n",temp1);
	
	temp1=ADV7441_read(0x87);
	printk("ADV7400 temp1 0x87 register=0x %x \n",temp1);
	temp1=ADV7441_read(0x88);
	printk("ADV7400 temp1 0x88 register=0x %x \n",temp1);
	temp1=ADV7441_read(0x89);
	printk("ADV7400 temp1 0x89 register=0x %x \n",temp1);
	temp1=ADV7441_read(0x8a);
	printk("ADV7400 temp1 0x8a register=0x %x \n",temp1);
	ADV7441_write(0x8a,0x50);
	temp1=ADV7441_read(0x8a);
	printk("ADV7400 temp1 0x8a register=0x %x \n",temp1);
    
    return ret;
#endif 
	return 0;
}


#ifndef VXWORKS

int adv7441_open(struct inode * inode, struct file * file)
{
    return 0;
}
int adv7441_close(struct inode * inode, struct file * file)
{
    return 0;
}

int adv7441_ioctl(struct inode *inode, 
        struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
    	case ADV7441_SET_VIDEO_FORMAT:
        {
            ADV7441_VIDEO_FORMAT_E tmp = 0;
            if (copy_from_user(&tmp, (ADV7441_VIDEO_FORMAT_E*)arg, sizeof(tmp)))
            {
                return -ERESTARTSYS;
            }
            if (adv7441_set_video_format(tmp))
            {
                return -1;
            }
            break;
    	}
        default:
            break;
    }
    return 0;
}

static struct file_operations adv7441_fops = 
{
    .owner      = THIS_MODULE,
    .open       = adv7441_open,
    .release    = adv7441_close,
    .ioctl      = adv7441_ioctl,
};

static struct miscdevice adv7441_dev = 
{   
	.minor		= MISC_DYNAMIC_MINOR,
    .name		= "adv7441",
    .fops  		= &adv7441_fops,
};


static int __init adv7441_init(void)
{    
    if (misc_register(&adv7441_dev))
    {
        printk("ERROR: could not register adv7441 devices\n");
		return -1;
    }
    
    if (adv7441_device_init())
    {
        printk("adv7441_device_init fail \n");
        misc_deregister(&adv7441_dev);
        return -1;
    }
    
    printk("adv7441 driver init successful!\n");
    return 0;
}
static void __exit adv7441_exit(void)
{
    misc_deregister(&adv7441_dev);
}

module_init(adv7441_init);
module_exit(adv7441_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif
MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");
#endif
