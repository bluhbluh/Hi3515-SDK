/*   extdrv/peripheral/dc/mt9v131.c
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
 * along with this program.
 *
 *
 * History:
 *     04-Apr-2006 create this file
 *
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
#include "mt9d131.h"


/* mt9d131 i2c slaver address micro-definition. */
#define I2C_MT9D131     0x90
static int out_mode = 1;
static int powerfreq = DC_VAL_50HZ;


static void mt9d131_default_init(void)
{
    /*========soft reset===============*/
    //page 0
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    
    gpio_i2c_write(I2C_MT9D131,0x65, 0xa0);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00); 
    //page 1
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);    
    
    gpio_i2c_write(I2C_MT9D131,0xc3, 0x05);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x01);  
    //page 0
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00); 
    
    gpio_i2c_write(I2C_MT9D131,0x0d, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x21);   
    
    msleep(10);  
    
    gpio_i2c_write(I2C_MT9D131,0x0d, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);   
    
    msleep(10); 
    //end 
      
    //pll control
    gpio_i2c_write(I2C_MT9D131,0x66, 0x10);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x04);
    
    gpio_i2c_write(I2C_MT9D131,0x67, 0x05);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);
    
    gpio_i2c_write(I2C_MT9D131,0x65, 0xe0);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);    
    
    msleep(10); 
        
    gpio_i2c_write(I2C_MT9D131,0x65, 0xA0);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);
    
    msleep(10);      
    
    gpio_i2c_write(I2C_MT9D131,0x65, 0x20);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);   
    
    msleep(10);      
    //end

/**************set mode*******************************************/   
    gpio_i2c_write(I2C_MT9D131,0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x01);   
    //contexa/b  bypass jpeg
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0B);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x30);

    //page 0
    gpio_i2c_write(I2C_MT9D131,0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00); 
    //Read Mode (A) 
    gpio_i2c_write(I2C_MT9D131, 0x21, 0x03);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    
    
/************set mode end******************************************/    
   
 /************************flicker detection****************************************/
 //  search_f1_50  Lower limit of period range  30       
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA4);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x08);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x1e);
 //   search_f1_50  upper limit of period range  32 
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA4);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x09);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x20);
   //  search_f1_60  Lower limit of period range  37    
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA4);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0a);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x25);
  //  search_f1_60  upper limit of period range  39  
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA4);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0b);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x27);
   // R9_Step_60   minimal shutter width step for 60hz ac  157
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x24);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x11);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x9d);
  // R9_Step_50   minimal shutter width step for 50hz ac  188
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x24);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x13);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xbc);
   
        /*====fix to 50HZ====*/
        gpio_i2c_write(I2C_MT9D131,0xf0, 0x00);
        gpio_i2c_write(I2C_MT9D131,0xf1, 0x01);
        gpio_i2c_write(I2C_MT9D131,0xC6, 0xa4);        
        gpio_i2c_write(I2C_MT9D131,0xf1, 0x04);
        gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);       
        gpio_i2c_write(I2C_MT9D131,0xf1, 0xc0);
   /************************************flicker detection  end ****************************/ 
    /***************************************************set auto exposure*********/
    //Max R12 (B)(Shutter Delay)  402
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x22);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0b);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x01);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x92);
    //IndexTH23  Zone number to start gain increase in low-light. 
    //Sets  frame rate at normal illumination.   3
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA2);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x17);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x03);
    //RowTime (msclk per)/4  Row time divided by 4 (in master clock periods)  527
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x22);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x28);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x02);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0f);
    //R9 Step   Integration time of one zone  156
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x22);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x2f);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x9c);
    //Maximum allowed zone number (that is maximumintegration time)  3
    gpio_i2c_write(I2C_MT9D131,0xC6, 0Xa2);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0e);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x03);
    /***************************************************set auto exposure  end*********/    
    /*======******************************************lens correcton***************************/
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x02);
    //lc control
    gpio_i2c_write(I2C_MT9D131, 0x80, 0x01);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xf8);
    //ZONE_BOUNDS_X1_X2 
    gpio_i2c_write(I2C_MT9D131, 0x81, 0x64);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x32);
    /* ZONE_BOUNDS_X0_X3 */
    gpio_i2c_write(I2C_MT9D131, 0x82, 0x32);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x96);
    /* ZONE_BOUNDS_X4_X5 */
    gpio_i2c_write(I2C_MT9D131, 0x83, 0x96);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x64);
    /* ZONE_BOUNDS_Y1_Y2 */
    gpio_i2c_write(I2C_MT9D131, 0x84, 0x50);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x28);
    /* ZONE_BOUNDS_Y0_Y3 */
    gpio_i2c_write(I2C_MT9D131, 0x85, 0x28);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x78);
    /* ZONE_BOUNDS_Y4_Y5 */
    gpio_i2c_write(I2C_MT9D131, 0x86, 0x78);      
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x50);
    /* CENTER_OFFSET */
    gpio_i2c_write(I2C_MT9D131, 0x87, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    /* FX_RED */
    gpio_i2c_write(I2C_MT9D131, 0x88, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x15);
    /* FY_RED */
    gpio_i2c_write(I2C_MT9D131, 0x8B, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2B);
    /* DF_DX_RED */
    gpio_i2c_write(I2C_MT9D131, 0x8E, 0x0F);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xEE);
    /* DF_DY_RED */
    gpio_i2c_write(I2C_MT9D131, 0x91, 0x0F);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x54);
    /* SECOND_DERIV_ZONE_0_RED */
    gpio_i2c_write(I2C_MT9D131, 0x94, 0xD4);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xFF);
    /* SECOND_DERIV_ZONE_1_RED */
    gpio_i2c_write(I2C_MT9D131, 0x97, 0x0B);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xA2);
    /* SECOND_DERIV_ZONE_2_RED */
    gpio_i2c_write(I2C_MT9D131, 0x9A, 0x13);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4C);
    /* SECOND_DERIV_ZONE_3_RED */
    gpio_i2c_write(I2C_MT9D131, 0x9D, 0x21);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x35);
     /* SECOND_DERIV_ZONE_4_RED */
    gpio_i2c_write(I2C_MT9D131, 0xA0, 0x10);   
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x41);
    /* SECOND_DERIV_ZONE_5_RED */
    gpio_i2c_write(I2C_MT9D131, 0xA3, 0x4F);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x5D);
    /* SECOND_DERIV_ZONE_6_RED */
    gpio_i2c_write(I2C_MT9D131, 0xA6, 0x94);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xC3);
    /* SECOND_DERIV_ZONE_7_RED */
    gpio_i2c_write(I2C_MT9D131, 0xA9, 0x2A);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xBE);
    /* FX_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x89, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x09);
    /* FY_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x8C, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x33);
    /* DF_DX_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x8F, 0x05);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x5E);
    /* DF_DY_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x92, 0x0F);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x86);
    /* SECOND_DERIV_ZONE_0_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x95, 0xBF);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x86);
    /* SECOND_DERIV_ZONE_1_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x98, 0x0E);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xB1);
    /* SECOND_DERIV_ZONE_2_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x9B, 0x17);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x45);
    /* SECOND_DERIV_ZONE_3_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0x9E, 0x22);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x43);
    /* SECOND_DERIV_ZONE_4_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0xA1, 0x0E);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x3F);
    /* SECOND_DERIV_ZONE_5_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0xA4, 0x59);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x5C);
    /* SECOND_DERIV_ZONE_6_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0xA7, 0x8B);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xD1);
    /* SECOND_DERIV_ZONE_7_GREEN */
    gpio_i2c_write(I2C_MT9D131, 0xAA, 0x01);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x9F);
    /* FX_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x8A, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x02);
     /* FY_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x8D, 0x00);   
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x31);
    /* DF_DX_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x90, 0x01);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xD0);
    /* DF_DY_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x93, 0x0F);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x88);
    /* SECOND_DERIV_ZONE_0_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x96, 0xDB);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xEB);
    /* SECOND_DERIV_ZONE_1_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x99, 0xF6);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xA2);
    /* SECOND_DERIV_ZONE_2_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x9C, 0x14);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x41);
    /* SECOND_DERIV_ZONE_3_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0x9F, 0x1B);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x20);
    /* SECOND_DERIV_ZONE_4_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0xA2, 0x0C);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2D);
    /* SECOND_DERIV_ZONE_5_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0xA5, 0x54);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x50);
    /* SECOND_DERIV_ZONE_6_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0xA8, 0x8D);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xC0);
    /* SECOND_DERIV_ZONE_7_BLUE */
    gpio_i2c_write(I2C_MT9D131, 0xAB, 0xFF);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xA3);
    /* X2_FACTORS */
    gpio_i2c_write(I2C_MT9D131, 0xAC, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xC1);
     /* GLOBAL_OFFSET_FXY_FUNCTION */
    gpio_i2c_write(I2C_MT9D131, 0xAD, 0x00);   
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    /* K_FACTOR_IN_K_FX_FY */
    gpio_i2c_write(I2C_MT9D131, 0xAE, 0x01);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x8E);
      
    //page 1
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);
    //enable  lc/gamma/color corretion
    gpio_i2c_write(I2C_MT9D131, 0x08, 0x01);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xFC);
    /*********************************************lens correction end***********/
    
   /*******************************Color Correction Matrices*****************/
     #if 0
    //page 1
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);
    //enable  lc/gamma/color corretion
    gpio_i2c_write(I2C_MT9D131, 0x08, 0x01);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xFC);
    
    //turn AWB on
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xa1);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x02);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0f);  
    
    //set digital WB gain to 1
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xa3);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 83);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x21);  
      
    //Set WB position to select the desired CCM.   
   gpio_i2c_write(I2C_MT9D131, 0xC6, 0xa3);
   gpio_i2c_write(I2C_MT9D131, 0xf1, 81);
   gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
   gpio_i2c_write(I2C_MT9D131, 0xf1, 127);
   
   ////ccml 9	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x18); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x20);
    ////ccml 10 	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x1A); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x35); 
    
    ////ccmr 9	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2E); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0a); 
     ////ccmr 10	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x30); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xff);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xf1); 
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);    /* Refresh Sequencer */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);    /*  = 5 */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x05);
    
    //ccml 0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x06);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x09);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xEA); 
    //ccml 1	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x08); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xFC);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xDE); 
    ////ccml 2	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0A); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xFA);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x82); 
    ////ccml 3	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0C); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xFD);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x6F);
    ////ccml 4 	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0E); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x0B);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4A); 	
    //ccml 5
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x10); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xF8);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xD3); 
    ////ccml 	6
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x12); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xFA);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x52); 
    ////ccml 7	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x14); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x04);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xCC); 
    ////ccml 8	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x16); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x02);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0D); 
    ////ccml 9	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x18); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x28);
    ////ccml 10 	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x1A); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x23); 
    ////ccmr 0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x1C); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xF8);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xD7); 
    ////ccmr 1	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x1E); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x01);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xCE); 
     ////ccmr 2	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x20); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x05);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x3D); 
     ////ccmr 3	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x22); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x01);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xED); 
     ////ccmr 4	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x24); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xF7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x1F); 
     ////ccmr 5	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x26); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x06);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xA0);
     ////ccmr 6 	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x28); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x05);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4A); 
     ////ccmr 7	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2A); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0xFA);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x62); 
     ////ccmr 8	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2C); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x54); 
     ////ccmr 9	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2E); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00); 
     ////ccmr 10	
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0x23);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x30); 	
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00); 	
    //refresh
    msleep(500);     /* DELAY = 500 */
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);    /* Refresh Sequencer */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);    /*  = 5 */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x05);
    #endif
    
    /*******************************Color Correction Matrices  end****************/
    
    /********************************gamma and contrast****************************/
    //page 1
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x43);  // MCU_ADDRESS [MODE_GAM_CONT_A]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x12);  // MCU_DATA_0   
    
 #if 0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x45);  // MCU_ADDRESS [MODE_GAM_TABLE_A_0]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x46);  // MCU_ADDRESS [MODE_GAM_TABLE_A_1]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x04);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x47);  // MCU_ADDRESS [MODE_GAM_TABLE_A_2]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x0E);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x48);  // MCU_ADDRESS [MODE_GAM_TABLE_A_3]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2A);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x49);  // MCU_ADDRESS [MODE_GAM_TABLE_A_4]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x54);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4A);  // MCU_ADDRESS [MODE_GAM_TABLE_A_5]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x76);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4B);  // MCU_ADDRESS [MODE_GAM_TABLE_A_6]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x94);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4C);  // MCU_ADDRESS [MODE_GAM_TABLE_A_7]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xAA);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4D);  // MCU_ADDRESS [MODE_GAM_TABLE_A_8]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xBA);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4E);  // MCU_ADDRESS [MODE_GAM_TABLE_A_9]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xC7);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x4F);  // MCU_ADDRESS [MODE_GAM_TABLE_A_10]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xD1);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x50);  // MCU_ADDRESS [MODE_GAM_TABLE_A_11]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xDA);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x51);  // MCU_ADDRESS [MODE_GAM_TABLE_A_12]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xE1);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x52);  // MCU_ADDRESS [MODE_GAM_TABLE_A_13]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xE8);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x53);  // MCU_ADDRESS [MODE_GAM_TABLE_A_14]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xED);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x54);  // MCU_ADDRESS [MODE_GAM_TABLE_A_15]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xF2);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x55);  // MCU_ADDRESS [MODE_GAM_TABLE_A_16]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xF7);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x56);  // MCU_ADDRESS [MODE_GAM_TABLE_A_17]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xFB);  // MCU_DATA_0
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA7);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x57);  // MCU_ADDRESS [MODE_GAM_TABLE_A_18]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xFF);  // MCU_DATA_0
 #endif
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);  // MCU_ADDRESS [SEQ_CMD]
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x05);  // MCU_DATA_0
     /**************************************gamma and contrast end***************************/


    /*--------use preview mode, not capture mode--------*/
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);
    //capture  clear
    gpio_i2c_write(I2C_MT9D131, 0xc6, 0xa1);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x20); 
    gpio_i2c_write(I2C_MT9D131, 0xc8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    //cmd do preview
    gpio_i2c_write(I2C_MT9D131, 0xc6, 0xa1);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03); 
    gpio_i2c_write(I2C_MT9D131, 0xc8, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);

}

static void mt9d131_vga_init(void)
{
    
    /************************************************set mode*********************************/
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    /* HBLANK (A) = 174 */
    gpio_i2c_write(I2C_MT9D131, 0x07, 0x00);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0xae);
     /* VBLANK (A) = 16 */
    gpio_i2c_write(I2C_MT9D131, 0x08, 0x00);   
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x10);
    
    //page 1
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);   

     //output_width 800 a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x03);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x20);
    //output_height 600 a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x05);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x02);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x58);
    //first sensor-readout row 28  context a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0F);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x1C);
    //first sensor-readout column 60 context a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x11);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x3C);
    //contexta number of sensor-readout rows 600 
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x13);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x02);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x58);
    //contexta number of sensor-readout columns 800
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x15);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x03);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x20);
    
    //extra sensor delay per frame context a 
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x17);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x03);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x18);
    //row-speed context a 17
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x19);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x11);
    //Crop_X0 (A)  0
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);
    //Crop_X1 (A)  800
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x29);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x03);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x20);
    //Crop_Y0 (A)  0
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x2b);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);
    //Crop_Y1 (A)  600
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x2d);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x02);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x58);
    //FIFO_Conf1 (A)   57570
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x6d);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0xE0);       
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xe2);
    //FIFO_Conf2 (A)   225
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA7);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x6f);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xe1);
    /***************************************************set mode end*************/

    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01); 

    msleep(500);     /* DELAY = 500 */
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);    /* Refresh Sequencer Mode */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);    /*  = 6 */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x06);

    msleep(500);     /* DELAY = 500 */
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);    /* Refresh Sequencer */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);    /*  = 5 */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x05);

}

static void mt9d131_uxga_init(void)
{
    mt9d131_default_init();
    /************************************************set mode*********************************/
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    /* HBLANK (A) = 516 */
    gpio_i2c_write(I2C_MT9D131, 0x07, 0x02);    
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x04);
     /* VBLANK (A) = 47 */
    gpio_i2c_write(I2C_MT9D131, 0x08, 0x00);   
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x2f);
    
    //page 1
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);   

     //output_width 1600 a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x06);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x40);
    //output_height 1200 a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x05);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x04);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xb0);
    //first sensor-readout row 28  context a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x0F);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x1C);
    //first sensor-readout column 60 context a
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x11);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x3C);
    //contexta number of sensor-readout rows 1200 
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x13);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x04);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xb0);
    //contexta number of sensor-readout columns 1600
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x15);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x06);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x40);
    
    //extra sensor delay per frame context a 1046
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x17);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x04);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x16);
    //row-speed context a 17
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x19);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x11);
    //Crop_X0 (A)  0
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x27);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);
    //Crop_X1 (A)  1600
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x29);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x06);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x40);
    //Crop_Y0 (A)  0
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x2b);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x00);
    //Crop_Y1 (A)  1200
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x2d);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x04);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xb0);
    //FIFO_Conf1 (A)   57569
    gpio_i2c_write(I2C_MT9D131,0xC6, 0x27);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x6d);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0xE0);       
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xe1);
    //FIFO_Conf2 (A)   225
    gpio_i2c_write(I2C_MT9D131,0xC6, 0xA7);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0x6f);
    gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
    gpio_i2c_write(I2C_MT9D131,0xf1, 0xe1);
    /***************************************************set mode end*************/

    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01); 

    msleep(500);     /* DELAY = 500 */
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);    /* Refresh Sequencer Mode */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);    /*  = 6 */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x06);

    msleep(500);     /* DELAY = 500 */
    
    gpio_i2c_write(I2C_MT9D131, 0xC6, 0xA1);    /* Refresh Sequencer */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x03);
    gpio_i2c_write(I2C_MT9D131, 0xC8, 0x00);    /*  = 5 */
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x05);
}

/*
 * mt9d131 open routine. 
 * do nothing.
 *
 */
int mt9d131_open(struct inode * inode, struct file * file)
{
    return 0;
}

/*
 * mt9d131 close routine. 
 * do nothing.
 *
 */
 
int mt9d131_close(struct inode * inode, struct file * file)
{
    return 0;
}


/*
 * mt9d131 ioctl routine. 
 * @param inode: pointer of the node;
 * @param file: pointer of the file;
 *
 * @param cmd: command from the app:
 * 
 * @param arg:arg from app layer.
 *
 * @return value:0-- set success; 1-- set error. 
 *
 */
 
int mt9d131_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    unsigned int val = 0;
    
    if (copy_from_user(&val, argp, sizeof(val))) 
    {
        return -EFAULT;
    }
    
    switch (cmd)
    {
        case DC_SET_IMAGESIZE:
        {
            unsigned int imagesize = val;
            if(imagesize == DC_VAL_VGA)
            {
                mt9d131_vga_init();
                printk("\nset mt9d131 VGA ok\n");
            }
            else if(imagesize == DC_VAL_UXGA)
            {
                mt9d131_uxga_init();
                printk("\nset mt9d131 UXGA ok\n");
            }
            else
            {
                printk("imagesize_set_error.\n");
                return -1;

            }   
            break;
        }
        
        case DC_SET_POWERFREQ:
        {
            unsigned int powerfreq_dy = val;
            if(powerfreq_dy == DC_VAL_50HZ)
            {
                /*====fix to 50HZ====*/
                gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
                gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);

                gpio_i2c_write(I2C_MT9D131,0xC6, 0xa4);        
                gpio_i2c_write(I2C_MT9D131,0xf1, 0x04);
                gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
                gpio_i2c_write(I2C_MT9D131,0xf1, 0xc0);
                powerfreq = DC_VAL_50HZ;
            }
            else if(powerfreq_dy == DC_VAL_60HZ)
            {
                /*====fix to 60HZ====*/
                gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
                gpio_i2c_write(I2C_MT9D131, 0xf1, 0x01);

                gpio_i2c_write(I2C_MT9D131,0xC6, 0xa4);        
                gpio_i2c_write(I2C_MT9D131,0xf1, 0x04);
                gpio_i2c_write(I2C_MT9D131,0xC8, 0x00);        
                gpio_i2c_write(I2C_MT9D131,0xf1, 0xa0);
                powerfreq = DC_VAL_60HZ;
            }
            else 
            {
                printk("powerfreq_set_error.\n");
                return -1;
                
            }
            break;
        }
         
        default:
            return -1;  
    }     
    return 0;
}






/*
 *  The various file operations we support.
 */
 
static struct file_operations mt9d131_fops = {
  .owner    = THIS_MODULE,
  .ioctl    = mt9d131_ioctl,
  .open   = mt9d131_open,
  .release  = mt9d131_close
};

static struct miscdevice mt9d131_dev = {
  MISC_DYNAMIC_MINOR,
  "mt9d131",
  &mt9d131_fops,
};

static int mt9d131_device_init(void)
{

    unsigned char regvalue;
    int loop1;
     
    /* read Chip version */
    gpio_i2c_write(I2C_MT9D131, 0xf0, 0x00);
    gpio_i2c_write(I2C_MT9D131, 0xf1, 0x00);
    regvalue = gpio_i2c_read(I2C_MT9D131, 0x00);
    loop1 = gpio_i2c_read(I2C_MT9D131, 0xf1); 
    if((regvalue != 0x15) || (loop1 != 0x19))
    {
        printk("read Prodect ID Number MSB is %x\n",regvalue);
        printk("read Prodect ID Number LSB is %x\n",loop1);
        printk("check mt9d131 ID error.\n");
        return -EFAULT;
    }
    if(out_mode == 1)
        mt9d131_uxga_init();
    else
        mt9d131_vga_init();  

    return 0;
}

static int __init mt9d131_init(void)
{  
    int ret = 0;
                
    ret = misc_register(&mt9d131_dev);
    if(ret)
    {
        printk("could not register mt9d131 devices. \n");
        return ret;
    }
    
    if(mt9d131_device_init()<0)
    {
        misc_deregister(&mt9d131_dev);
        printk("mt9d131 driver init fail for device init error!\n");
        return -1;
    } 
	   
    printk("mt9d131 driver init successful!\n");
    
    return ret;
}

static void __exit mt9d131_exit(void)
{
    misc_deregister(&mt9d131_dev);
}



module_init(mt9d131_init);
module_exit(mt9d131_exit);

#ifdef MODULE
#include <linux/compile.h>
#endif

module_param(out_mode, int, S_IRUGO);
module_param(powerfreq, int, S_IRUGO);

MODULE_INFO(build, UTS_VERSION);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hisilicon");





