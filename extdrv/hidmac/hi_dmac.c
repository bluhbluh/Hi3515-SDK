/* 
 * ./arch/arm/mach-hi3511_v100_f01/hi_dmac.c
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
 *      17-August-2006 create this file
 */



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
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/dma-mapping.h>
#include <linux/kcom.h>

#include "hi_dmac.h"
#include "dmac.h"


#define CLR_INT(i)	(*(volatile unsigned int *)IO_ADDRESS(DMAC_BASE_REG+0x008))=(1<<i)
#define CHANNEL_NUM     2
static int          dmac_channel[CHANNEL_NUM]={6,7};

static int    g_channel_status[DMAC_MAX_CHANNELS]; 

static int sio0_mode = 0; /* 0-i2s mode 1-pcm mode */
static int sio1_mode = 0; /* 0-i2s mode 1-pcm mode */
static int sio2_mode = 0; /* 0-i2s mode 1-pcm mode */


static unsigned int sio0_rx_fifo =  SIO0_RX_FIFO;
static unsigned int sio0_tx_fifo =  SIO0_TX_FIFO;
static unsigned int sio1_rx_fifo =  SIO1_RX_FIFO;
static unsigned int sio1_tx_fifo =  SIO1_TX_FIFO;
static unsigned int sio2_rx_fifo =  SIO2_RX_FIFO;

/*
 * 	Define Memory range 
 */
mem_addr mem_num[MEM_MAX_NUM]=
{
    	{DDRAM_ADRS,DDRAM_SIZE},
    	{FLASH_BASE,FLASH_SIZE},
    	{ITCM_BASE,ITCM_SIZE}
};

typedef void REG_ISR(int *p_dma_chn,int *p_dma_status) ;
REG_ISR *function[DMAC_MAX_CHANNELS];


/*
 *	DMA config array! 
 */
dmac_peripheral  g_peripheral[DMAC_MAX_PERIPHERALS]=
{

    	/*periphal 0,SIO0_RX*/
    	{ DMAC_SIO0_RX_REQ,(unsigned int*)SIO0_RX_FIFO,SIO0_RX_CONTROL,SIO0_RX_CONFIG},
    	/*periphal 1,SIO0_TX*/
    	{ DMAC_SIO0_TX_REQ,(unsigned int*)SIO0_TX_FIFO,SIO0_TX_CONTROL,SIO0_TX_CONFIG } ,

    	/*periphal 2,SIO1_RX*/
    	{ DMAC_SIO1_RX_REQ,(unsigned int*)SIO1_RX_FIFO,SIO1_RX_CONTROL,SIO1_RX_CONFIG} ,
    	/*periphal 3,SIO1_TX*/
    	{ DMAC_SIO1_TX_REQ,(unsigned int*)SIO1_TX_FIFO,SIO1_TX_CONTROL,SIO1_TX_CONFIG },

    	/*periphal 4,SIO2_RX*/
    	{ DMAC_SIO2_RX_REQ,(unsigned int*)SIO2_RX_FIFO,SIO2_RX_CONTROL,SIO2_RX_CONFIG},        
        /*periphal 5,SIO2_TX*/
        { DMAC_SIO2_TX_REQ,(unsigned int*)SIO2_TX_FIFO,SIO2_TX_CONTROL,SIO2_TX_CONFIG },    	
        /*periphal 6,SSP_RX*/
        { DMAC_SSP_RX_REQ,(unsigned int*)SSP_DATA_REG,SSP_RX_CONTROL,SSP_RX_CONFIG }, 
        /*periphal 7,SSP_TX*/
        { DMAC_SSP_TX_REQ,(unsigned int*)SSP_DATA_REG,SSP_TX_CONTROL,SSP_TX_CONFIG} ,
            
        /*periphal 8,MMC_RX*/
        { DMAC_MMC_RX_REQ,(unsigned int*)MMC_RX_REG,MMC_RX_CONTROL,MMC_RX_CONFIG},
        /*periphal 9,MMC_TX*/
        { DMAC_MMC_TX_REQ,(unsigned int*)MMC_TX_REG,MMC_TX_CONTROL,MMC_TX_CONFIG},
       
    	/*periphal 10,UART0_RX*/
    	{ DMAC_UART0_RX_REQ,(unsigned int*)UART0_DATA_REG,UART0_RX_CONTROL,UART0_RX_CONFIG} ,
    	/*periphal 11,UART0_TX*/
    	{ DMAC_UART0_TX_REQ,(unsigned int*)UART0_DATA_REG,UART0_TX_CONTROL,UART0_TX_CONFIG} ,

        /*periphal 12,UART1_RX*/
        { DMAC_UART1_RX_REQ,(unsigned int*)UART1_DATA_REG,UART1_RX_CONTROL,UART1_RX_CONFIG} ,
        /*periphal 13,UART1_TX*/
        { DMAC_UART1_TX_REQ,(unsigned int*)UART1_DATA_REG,UART1_TX_CONTROL,UART1_TX_CONFIG} ,

        /*periphal 14,UART2_RX*/
        { DMAC_UART2_RX_REQ,(unsigned int*)UART2_DATA_REG,UART2_RX_CONTROL,UART2_RX_CONFIG} ,
        /*periphal 15,UART2_TX*/
        { DMAC_UART2_TX_REQ,(unsigned int*)UART2_DATA_REG,UART2_TX_CONTROL,UART2_TX_CONFIG} 

};

/*
 *	dmac interrupt handle function
 */
static   irqreturn_t dmac_isr(int irq, void * dev_id)
{  
	unsigned int channel_status, tmp_channel_status[3], channel_tc_status, channel_err_status;
	unsigned int i,j,count = 0;
    
    /*read the status of current interrupt */
	dmac_readw(DMAC_INTSTATUS, channel_status);             
    
    /*decide which channel has trigger the interrupt*/
	for(i = 0; i < DMAC_MAX_CHANNELS; i++)          
    {
        count = 0;
        while(1)
        {
            for(j=0;j<3;j++)
            {
                    dmac_readw(DMAC_INTSTATUS, channel_status);
                    tmp_channel_status[j] = (channel_status >> i) & 0x01;
            }
            if((tmp_channel_status[0] == tmp_channel_status[1] == tmp_channel_status[2] == 0x1))
            {
                    break;
            }
            else if((tmp_channel_status[0] == tmp_channel_status[1] == tmp_channel_status[2] == 0x0))
            {
                    break;
            }
            count++;
            if(count > 10)
            {
                   printk("DMAC %d channel Int status is error.\n",i);
                   break;
            }
        }        
        
    	if((tmp_channel_status[0] == 0x01))
    	{
            CLR_INT(i);
            dmac_readw(DMAC_INTTCSTATUS, channel_tc_status);
            dmac_readw(DMAC_INTERRORSTATUS, channel_err_status);

            /*save the current channel transfer status to g_channel_status[i]*/
            if((0x01 == ((channel_tc_status >> i) & 0x01)))
            {
            	g_channel_status[i] = DMAC_CHN_SUCCESS;                 
            	dmac_writew(DMAC_INTTCCLEAR, (0x01 << i));              	
            }
            else if((0x01 == ((channel_err_status >> i)&0x01)))
            {
            	g_channel_status[i] = -DMAC_CHN_ERROR;                 
            	printk("Error in DMAC %d channel as finish!\n",i);   
            	dmac_writew(DMAC_INTERRCLR, (0x01 << i));  
            }
            else
            {
                /*	printk("Isr Error in DMAC_IntHandeler %d! channel\n" ,i); */ 
            }
            if((function[i])!=NULL)
            	 function[i](&i,&g_channel_status[i]);
    	}         
    }
	return IRQ_RETVAL(1);
} 

/*
 *	memory address validity check 
 */
static int mem_check_valid(unsigned int addr)
{
    	unsigned int cnt;
    
    	for(cnt=0;cnt < MEM_MAX_NUM;cnt++)
    	{
        	if((addr >= mem_num[cnt].addr_base) && (addr <= (mem_num[cnt].addr_base + mem_num[cnt].size)))
        	return 0;
    	}
    	return -1;
}

/*
 *	check the state of channels
 */
int dmac_check_over(unsigned int channel)
{     	   
    	if (-DMAC_CHN_ERROR == g_channel_status[channel])
    	{
        	/*printk( "The transfer of Channel %d has finished with errors!\n",channel);*/
        	dmac_writew(DMAC_CxCONFIG(channel),DMAC_CxDISABLE);          
        	g_channel_status[channel] = DMAC_CHN_VACANCY;
        	return -DMAC_CHN_ERROR;
    	}
    
    	else if (DMAC_NOT_FINISHED == g_channel_status[channel]) 
    	{
        	return DMAC_NOT_FINISHED;
    	}

    	else if(DMAC_CHN_ALLOCAT == g_channel_status[channel])
    	{
        	return DMAC_CHN_ALLOCAT;
    	}
    	
    	else if(DMAC_CHN_VACANCY == g_channel_status[channel])
    	{
        	return DMAC_CHN_VACANCY;
    	}
    	
    	else if(-DMAC_CHN_TIMEOUT == g_channel_status[channel])
    	{
        
    		printk("The transfer of Channel %d has timeout!\n",channel);
        	return -DMAC_CHN_TIMEOUT;
    	}
    	else if (DMAC_CHN_SUCCESS == g_channel_status[channel])
    	{      
    		/*The transfer of Channel %d has finished successfully!*/  	
        	return DMAC_CHN_SUCCESS;                                              
    	} 
     	else 
     	{
        	dmac_writew(DMAC_CxCONFIG(channel),DMAC_CxDISABLE);
        	g_channel_status[channel] =DMAC_CHN_VACANCY;
        	return -DMAC_CHN_ERROR;
     	}    
}
 
spinlock_t my_lcok = SPIN_LOCK_UNLOCKED;
unsigned long flags;
/*
 *	allocate channel. 
 */
int  dmac_channel_allocate(void *pisr)
{    
    	unsigned int i,channelinfo;    	
    	for(i=0;i<CHANNEL_NUM;i++)
    		dmac_check_over(dmac_channel[i]);
        spin_lock_irqsave(&my_lcok,flags);    		
        dmac_readw(DMAC_ENBLDCHNS, channelinfo);
        channelinfo= channelinfo&0x00ff;

    	for (i = 0; i < CHANNEL_NUM; i++)
    	{
//    	    printk("allocate channel status is %d......\n",g_channel_status[i]);
            if (g_channel_status[dmac_channel[i]]==DMAC_CHN_VACANCY)
        	{
        	    channelinfo = channelinfo >> dmac_channel[i];
                if (0x00 == (channelinfo & 0x01)) 
                {        	    
            		dmac_writew(DMAC_INTERRCLR, (0x01 << dmac_channel[i])); /*clear the interrupt in this channel */
            		dmac_writew(DMAC_INTTCCLEAR, (0x01 << dmac_channel[i])); 

            		function[dmac_channel[i]] =(void *)pisr;
            		g_channel_status[dmac_channel[i]] = DMAC_CHN_ALLOCAT;
//            		printk("allocate channel is %d......\n",i);
            		spin_unlock_irqrestore(&my_lcok,flags);
            		return dmac_channel[i];
            	}
        	} 
    	} 
    	spin_unlock_irqrestore(&my_lcok,flags);
    	printk("DMAC :no available channel can allocate!\n");
    	return -EINVAL;  
}


int dmac_register_isr(unsigned int channel,void *pisr)
{
	if((channel < 0) || (channel > 7))
	{
	    printk("channel which choosed %d is error !\n",channel);
		return -1;
	}
    if(g_channel_status[channel] != DMAC_CHN_VACANCY)
    {
        printk("dma chn %d is in used!\n",channel);
        return -1;
    }
	/*clear the interrupt in this channel */
	dmac_writew(DMAC_INTERRCLR, (0x01 << channel)); 
	dmac_writew(DMAC_INTTCCLEAR, (0x01 << channel));
	function[channel] = (void *)pisr;
	g_channel_status[channel] = DMAC_CHN_ALLOCAT;
	return 0;	
}


/*
 *	free channel
 */
int  dmac_channel_free(unsigned int channel)
{  
     	g_channel_status[channel] = DMAC_CHN_VACANCY;
     	function[channel]= NULL;
     	return 0;
}



/*
 *	init dmac register
 *	clear interupt flags
 *	called by dma_driver_init
 */
int  dmac_init(void)
{
    	unsigned int i,tempvalue;
        
        dmac_readw(DMAC_CONFIG, tempvalue);
        if(tempvalue == 0)
        {
    	    dmac_writew(DMAC_CONFIG,DMAC_CONFIG_VAL);
    	    dmac_writew(DMAC_SYNC,DMAC_SYNC_VAL);
    	    dmac_writew(DMAC_INTTCCLEAR,0xFF);
    	    dmac_writew(DMAC_INTERRCLR,0xFF);    
        	for(i=0;i< DMAC_MAX_CHANNELS;i++)
        	{
            	dmac_writew (DMAC_CxCONFIG(i), DMAC_CxDISABLE);  
        	}
        }
        
    	if(sio0_mode == 0)
    	{
    	    sio0_rx_fifo =  SIO0_RX_FIFO;
    	    sio0_tx_fifo =  SIO0_TX_FIFO;
    	}
    	else
    	{
    	    sio0_rx_fifo =  SIO0_RX_RIGHT_FIFO;
    	    sio0_tx_fifo =  SIO0_TX_RIGHT_FIFO;
    	}
    	g_peripheral[DMAC_SIO0_RX_REQ].pperi_addr = (unsigned int*)sio0_rx_fifo;
    	g_peripheral[DMAC_SIO0_TX_REQ].pperi_addr = (unsigned int*)sio0_tx_fifo;
        
    	if(sio1_mode == 0)
    	{
    	    sio1_rx_fifo =  SIO1_RX_FIFO;
            sio1_tx_fifo =  SIO1_TX_FIFO;
    	}
    	else
    	{
    	    sio1_rx_fifo =  SIO1_RX_RIGHT_FIFO;
            sio1_tx_fifo =  SIO1_TX_RIGHT_FIFO;
    	}
    	g_peripheral[DMAC_SIO1_RX_REQ].pperi_addr = (unsigned int*)sio1_rx_fifo;
        g_peripheral[DMAC_SIO1_TX_REQ].pperi_addr = (unsigned int*)sio1_tx_fifo;

        if(sio2_mode == 0)
    	{
    	    sio2_rx_fifo =  SIO2_RX_FIFO;
    	}
    	else
    	{
    	    sio2_rx_fifo =  SIO2_RX_RIGHT_FIFO;
    	}
    	g_peripheral[DMAC_SIO2_RX_REQ].pperi_addr = (unsigned int*)sio2_rx_fifo;
        
    	return 0;    
}

/*
 *	alloc_dma_lli_space
 *	output: 
 *             ppheadlli[0]: memory physics address
 *             ppheadlli[1]: virtual address
 *
 */
int allocate_dmalli_space(unsigned int *ppheadlli, unsigned int page_num)
{
    	dma_addr_t dma_phys;
    	void *dma_virt ;
    	unsigned int *address;
    	address =ppheadlli;

    	dma_virt = dma_alloc_coherent(NULL,page_num*PAGE_SIZE,&dma_phys,GFP_DMA|__GFP_WAIT);
    	if (NULL == dma_virt)
    	{
        	printk("can't get dma mem from system\n");;
        	return -1;
    	}
    	address[0] =(unsigned int)(dma_phys);
    	address[1] =(unsigned int)(dma_virt);
    	return 0;
}

/*
 *	free_dma_lli_space
 */
int free_dmalli_space(unsigned int *ppheadlli, unsigned int page_num)
{
    	dma_addr_t dma_phys;
    	unsigned int dma_virt ;
    	dma_phys = (dma_addr_t)(ppheadlli[0]);
    	dma_virt = ppheadlli[1];
    	dma_free_coherent(NULL,page_num*PAGE_SIZE,(void *)dma_virt,dma_phys);
    	return 0;
}

/*
 *	Apply DMA interrupt resource
 *	init channel state
 */
int dma_driver_init(void)
{   
    	unsigned int i;

    	dmac_init();
    
    	if(request_irq(DMAC_INT,&dmac_isr,IRQF_DISABLED,"Hisilicon Dmac",NULL))
    	{
        	printk("DMA Irq request failed\n");
        	return -1;
    	}
  
    	for(i=0;i<DMAC_MAX_CHANNELS;i++)
    		g_channel_status[i] = DMAC_CHN_VACANCY;
    	
    	return 0;        
}

/*
 *	config register for memory to memory DMA tranfer without LLI
 *	note:
 *             it is necessary to call dmac_channelstart for channel enable
 */
int dmac_start_m2m(unsigned int  channel, unsigned int psource, unsigned int pdest, unsigned int uwnumtransfers)
{
    	unsigned int uwchannel_num,tmp_trasnsfer,addtmp;                                                                                                
    	/*check input paramet*/
    	addtmp = psource;
    	if((mem_check_valid(addtmp) == -1)||(addtmp & 0x03))
    	{   
        	printk( "Invalidate source address,address=%x \n",(unsigned int)psource);                   
        	return -EINVAL;                              
    	}                                                           
                                                                   
    	addtmp = pdest;
    	if((mem_check_valid(addtmp) == -1)||(addtmp&0x03))
    	{   
        	printk(  "Invalidate destination address,address=%x \n",(unsigned int)pdest);                   
        	return -EINVAL;                         
    	}                                                                
    
    	if((uwnumtransfers> (MAXTRANSFERSIZE<<2))||(uwnumtransfers&0x3))              
    	{   
        	printk( "Invalidate transfer size,size=%x \n",uwnumtransfers);                   
        	return -EINVAL;                                  
    	}                                                                 
    
    	uwchannel_num = channel;           
    	if((uwchannel_num == DMAC_CHANNEL_INVALID)|| (uwchannel_num >7))
    	{   
        	printk( "failure of DMAC channel allocation in M2M function!\n ");                   
        	return -EFAULT;                                     
    	}

    	dmac_writew (DMAC_CxCONFIG(uwchannel_num), DMAC_CxDISABLE);          
    	dmac_writew (DMAC_CxSRCADDR(uwchannel_num), (unsigned int)psource);     
    	dmac_writew (DMAC_CxDESTADDR(uwchannel_num),(unsigned int)pdest);     
    	dmac_writew (DMAC_CxLLI(uwchannel_num), 0);              
    	tmp_trasnsfer = (uwnumtransfers >> 2) & 0xfff;                                        
    	tmp_trasnsfer = tmp_trasnsfer | (DMAC_CxCONTROL_M2M & (~0xfff));                               
    	dmac_writew (DMAC_CxCONTROL(uwchannel_num), tmp_trasnsfer);       
    	dmac_writew (DMAC_CxCONFIG(uwchannel_num), DMAC_CxCONFIG_M2M);


    	return 0;      
}

/*
 *	channel enable
 *	start a dma transfer immediately
 */
int dmac_channelstart(unsigned int u32channel)
{
   
    	unsigned int reg_value;
    	if(u32channel >= DMAC_MAX_CHANNELS)
    	{
        	printk(  "channel number is larger than or equal to DMAC_MAX_CHANNELS %d\n",DMAC_MAX_CHANNELS);
        	return -EINVAL;    
    	}
    	g_channel_status[u32channel] = DMAC_NOT_FINISHED;
    	dmac_readw(DMAC_CxCONFIG(u32channel),reg_value);
    	dmac_writew(DMAC_CxCONFIG(u32channel),(reg_value|DMAC_CHANNEL_ENABLE));
 

    	return 0;
}

    
/*
 *	wait for transfer end
 */    
int dmac_wait(unsigned int channel)
{
    	int ret_result;
    	ret_result = dmac_check_over(channel);   
    	while(1)
    	{   
        	if(ret_result == -DMAC_CHN_ERROR)
        	{
            		printk("DMAC Transfer Error.\n");
            		return -1;  
        	} 
        
        	else  if(ret_result == DMAC_NOT_FINISHED)
        	{
        	        udelay(100);
                    ret_result = dmac_check_over(channel);        	    

        	}
        
        	else if(ret_result == DMAC_CHN_SUCCESS)
        	{
            		return 0;
        	}
        	
           	else if  (ret_result == DMAC_CHN_VACANCY)
        	{
            		return 0;
        	}
        	
        	else if(ret_result ==-DMAC_CHN_TIMEOUT)
        	{
            		printk("DMAC Transfer Error.\n");
            		dmac_writew (DMAC_CxCONFIG(channel),DMAC_CxDISABLE);          
            		g_channel_status[channel] =DMAC_CHN_VACANCY;
            		return -1;
        	}
    	}
}
   
/*
 *	buile LLI for memory to memory DMA tranfer
 */ 
int dmac_buildllim2m(unsigned int *ppheadlli, unsigned int pdest, unsigned int psource, unsigned int totaltransfersize,unsigned int uwnumtransfers)
{
	unsigned int lli_num =0;
	unsigned int last_lli =0;
	unsigned int address , phy_address,srcaddr,denstaddr;
	unsigned int j;                                          

	lli_num =(totaltransfersize / uwnumtransfers);
	if((totaltransfersize % uwnumtransfers)!= 0)
		last_lli=1,++lli_num;        
	if(ppheadlli != NULL)
	{         
    	phy_address =(unsigned int)(ppheadlli[0]);
    	address =(unsigned int)(ppheadlli[1]);   
    	for (j=0; j<lli_num; j++)
    	{
    		srcaddr =(psource + (j*uwnumtransfers));
    		dmac_writew(address, srcaddr);
    		address += 4;phy_address+=4;
    		denstaddr=(pdest + (j*uwnumtransfers));
    		dmac_writew(address, denstaddr);
    		address += 4;phy_address+=4;
    		if(j==(lli_num -1))
    			dmac_writew(address, 0);
    		else
    			dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));
    		address += 4;phy_address+=4;
    
    		if((j==(lli_num -1)) && (last_lli == 0))
    			dmac_writew(address, ((DMAC_CxCONTROL_LLIM2M &(~0xfff))|(uwnumtransfers>>2)|0x80000000));
    		else if((j==(lli_num -1)) && (last_lli == 1))
    			dmac_writew(address, ((DMAC_CxCONTROL_LLIM2M&(~0xfff)) |((totaltransfersize % uwnumtransfers)>>2)|0x80000000));
    		else
    			dmac_writew(address, (((DMAC_CxCONTROL_LLIM2M&(~0xfff))|(uwnumtransfers>>2))&0x7fffffff));
    		address += 4;phy_address+=4;
    	}
	}
  	return 0; 
}

/*
 *	disable channel
 *	used before the operation of register configuration
 */
int dmac_channelclose(unsigned int channel)
{
    	unsigned int reg_value,count;
  
    	if (channel >=DMAC_MAX_CHANNELS)
    	{
        	printk("\nCLOSE :channel number is larger than or equal to DMAC_CHANNEL_NUM_TOTAL.\n");
        	return -EINVAL;    
    	}

    	dmac_readw(DMAC_CxCONFIG(channel),reg_value);

    	#define CHANNEL_CLOSE_IMMEDIATE
    #ifdef CHANNEL_CLOSE_IMMEDIATE  
        reg_value &= 0xFFFFFFFE;
        dmac_writew(DMAC_CxCONFIG(channel) , reg_value);
    #else                           
        reg_value |= DMAC_CONFIGURATIONx_HALT_DMA_ENABLE;
        dmac_writew(DMAC_CxCONFIG(channel) , reg_value);     /*ignore incoming dma request*/
        dmac_readw(DMAC_CxCONFIG(channel),reg_value);
        while ((reg_value & DMAC_CONFIGURATIONx_ACTIVE) == DMAC_CONFIGURATIONx_ACTIVE) /*if FIFO is empty*/
        {
            	dmac_readw(DMAC_CxCONFIG(channel),reg_value);
        }
        reg_value &= 0xFFFFFFFE;
        dmac_writew(DMAC_CxCONFIG(channel) , reg_value); 
    #endif

    	dmac_readw(DMAC_ENBLDCHNS,reg_value);
    	reg_value= reg_value&0x00ff;
        count = 0;
    	while (((reg_value >> channel) & 0x1) == 1 )
        {
    	    dmac_readw(DMAC_ENBLDCHNS,reg_value);
    	    reg_value= reg_value&0x00ff;
            if(count++ > 10000)
            {
                printk("channel close failure.\n");
                return -1;
            }    
        }
    	return 0;
}

/*
 *	load configuration from LLI for memory to memory
 */
int dmac_start_llim2m(unsigned int channel, unsigned int *pfirst_lli)
{
    	unsigned int uwchannel_num;
    	dmac_lli  plli;
    	unsigned int first_lli;
    	if(NULL == pfirst_lli)
    	{
        	printk("Invalidate LLI head!\n");                   
        	return -EFAULT;              
    	}

    	uwchannel_num = channel;
    	if((uwchannel_num == DMAC_CHANNEL_INVALID)|| (uwchannel_num >7))
    	{
        	printk("failure of DMAC channel allocation in LLIM2M function,channel=%x!\n ",uwchannel_num);                   
        	return -EINVAL;
    	}

    	memset(&plli,0,sizeof(plli));
    	first_lli =(unsigned int )pfirst_lli[1];
    	dmac_readw(first_lli,plli.src_addr);
    	dmac_readw(first_lli+4,plli.dst_addr);
    	dmac_readw(first_lli+8, plli.next_lli);
    	dmac_readw(first_lli+12,plli.lli_transfer_ctrl);
    
    	dmac_channelclose(uwchannel_num);
    	dmac_writew (DMAC_INTTCCLEAR, (0x1<<uwchannel_num));
    	dmac_writew (DMAC_INTERRCLR, (0x1<<uwchannel_num));
    	dmac_writew (DMAC_SYNC , 0x0);    
    
    	dmac_writew(DMAC_CxCONFIG(uwchannel_num),DMAC_CxDISABLE);     
    	dmac_writew (DMAC_CxSRCADDR(uwchannel_num), (unsigned int)(plli.src_addr));
    	dmac_writew (DMAC_CxDESTADDR (uwchannel_num),(unsigned int)(plli.dst_addr));
    	dmac_writew (DMAC_CxLLI (uwchannel_num),(unsigned int)(plli.next_lli));
    	dmac_writew (DMAC_CxCONTROL(uwchannel_num), (unsigned int)(plli.lli_transfer_ctrl));
    	dmac_writew(DMAC_CxCONFIG(uwchannel_num), DMAC_CxCONFIG_LLIM2M);  

    	return 0;
}    

/*
 *	load configuration from LLI for memory and peripheral
 */
int dmac_start_llim2p(unsigned int channel, unsigned int *pfirst_lli, unsigned int uwperipheralid)
{
    	unsigned int uwchannel_num;
    	dmac_lli   plli;
    	unsigned int first_lli;
        unsigned int temp = 0;
        
    	if(NULL == pfirst_lli)
    	{
        	printk("Invalidate LLI head!\n");                   
        	return -EINVAL;              
    	}
    	uwchannel_num = channel;
    	if((uwchannel_num == DMAC_CHANNEL_INVALID)|| (uwchannel_num >7))
    	{
        	printk(" failure of DMAC channel allocation in LLIM2P function,channel=%x!\n ",uwchannel_num);                   
        	return -EINVAL;
    	}
    	
    	memset(&plli,0,sizeof(plli));
    	first_lli =(unsigned int )pfirst_lli[1];
    	dmac_readw(first_lli,plli.src_addr);
    	dmac_readw(first_lli+4,plli.dst_addr);
    	dmac_readw(first_lli+8, plli.next_lli);
    	dmac_readw(first_lli+12,plli.lli_transfer_ctrl);
     
    	dmac_channelclose(uwchannel_num);
    	dmac_writew (DMAC_INTTCCLEAR, (0x1<<uwchannel_num));
    	dmac_writew (DMAC_INTERRCLR, (0x1<<uwchannel_num));
    	dmac_writew (DMAC_SYNC , 0x0);

        dmac_readw  (DMAC_CxCONFIG(uwchannel_num), temp);
    	dmac_writew (DMAC_CxCONFIG(uwchannel_num), temp|DMAC_CxDISABLE);
    	dmac_writew (DMAC_CxSRCADDR(uwchannel_num), plli.src_addr);
    	dmac_writew (DMAC_CxDESTADDR(uwchannel_num),plli.dst_addr);
    	dmac_writew (DMAC_CxLLI(uwchannel_num),plli.next_lli);
    	dmac_writew (DMAC_CxCONTROL(uwchannel_num),plli.lli_transfer_ctrl);
        
        dmac_readw  (DMAC_CxCONFIG(uwchannel_num), temp);
    	dmac_writew (DMAC_CxCONFIG(uwchannel_num), temp|((g_peripheral[uwperipheralid].transfer_cfg)&DMAC_CHANNEL_DISABLE));

    	return 0;
}

/*
 *	enable memory and peripheral dma transfer
 *	note:
 *	       it is necessary to call dmac_channelstart to enable channel
 */
int dmac_start_m2p(unsigned int channel, unsigned int pmemaddr, unsigned int uwperipheralid, unsigned int  uwnumtransfers ,unsigned int next_lli_addr)
{

    	unsigned int uwchannel_num,uwtrans_control =0;
    	unsigned int addtmp,tmp;
    	unsigned int uwdst_addr = 0,uwsrc_addr = 0;
    	unsigned int uwwidth;
    	addtmp = pmemaddr;
    	if((mem_check_valid(addtmp) == -1)||(addtmp&0x3))
    	{
        	printk("Invalidate source address,address=%x \n",(unsigned int)pmemaddr);                   
        	return -EINVAL;                              
    	} 

    	if((uwperipheralid>15))
    	{
        	printk("Invalidate peripheral id in M2P function, id=%x! \n",uwperipheralid);                   
        	return -EINVAL;
    	}

    	uwchannel_num = channel;
    	if((uwchannel_num == DMAC_CHANNEL_INVALID) || (uwchannel_num > 7) || ( uwchannel_num < 0))
    	{
        	printk("failure of DMAC channel allocation in M2P function\n");                   
        	return -EFAULT;
    	}
        
    	if((DMAC_UART0_TX_REQ == uwperipheralid)||(DMAC_UART0_RX_REQ == uwperipheralid)\
    	||(DMAC_SSP_TX_REQ==uwperipheralid)||(DMAC_SSP_RX_REQ==uwperipheralid))
        	uwwidth=0;
  #if 0
    	else if((DMAC_SIO0_TX_REQ==uwperipheralid)||(DMAC_SIO0_RX_REQ==uwperipheralid)\
    	|| (DMAC_SIO1_RX_REQ==uwperipheralid))
        	uwwidth=1;    
  #endif
    	else
        	uwwidth=2;
        	
        if(uwperipheralid & 0x01)        
        {  
        	uwsrc_addr = pmemaddr;
        	uwdst_addr = (unsigned int)(g_peripheral[uwperipheralid].pperi_addr);
        }
        else 
        {

        	uwsrc_addr=(unsigned int)(g_peripheral[uwperipheralid].pperi_addr);
        	uwdst_addr=pmemaddr;
        }
        
    	tmp= uwnumtransfers>>uwwidth;
    	if(tmp & (~0x0fff))
    	{
        	printk("Invalidate transfer size,size=%x! \n",uwnumtransfers);                   
        	return -EINVAL;
    	}
    	tmp = tmp &0xfff;
    	uwtrans_control = tmp|(g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff));
    	
    	dmac_writew(DMAC_INTTCCLEAR , (0x1<<uwchannel_num));
    	dmac_writew(DMAC_INTERRCLR , (0x1<<uwchannel_num));
    	dmac_writew(DMAC_CxSRCADDR(uwchannel_num), (unsigned int)uwsrc_addr);     
    	dmac_writew(DMAC_CxDESTADDR(uwchannel_num),(unsigned int)uwdst_addr);     
    	dmac_writew(DMAC_CxLLI(uwchannel_num), (unsigned int)next_lli_addr);              
    	dmac_writew(DMAC_CxCONTROL(uwchannel_num), (unsigned int)uwtrans_control ); 
    	dmac_writew (DMAC_CxCONFIG(uwchannel_num), ((g_peripheral[uwperipheralid].transfer_cfg)&DMAC_CHANNEL_DISABLE));

    	return 0;
}


/*
 *	build LLI for memory to sio0
 *      called by dmac_buildllim2p
 */
void buildlli4m2sio0(
                    unsigned int *ppheadlli, 
                    unsigned int *pmemaddr,
                    unsigned int uwperipheralid,
                    unsigned int lli_num, 
                    unsigned int totaltransfersize,
                    unsigned int uwnumtransfers,
                    unsigned int uwwidth,
                    unsigned int last_lli
                    )
{  
    	unsigned int srcaddr,address,phy_address ,j;
    
    	phy_address =(unsigned int)(ppheadlli[0]);
    	address =(unsigned int)(ppheadlli[1]);   
    
    	for (j=0; j<lli_num; j++) 
    	{
        	srcaddr =(pmemaddr[0] + (j*uwnumtransfers)); 
        	dmac_writew(address, srcaddr);

        	address += 4;phy_address+=4;
        	dmac_writew(address,SIO0_TX_LEFT_FIFO);
   
        	address += 4;phy_address+=4;
        	dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));
      
        	address += 4;phy_address+=4;
        	if((j==(lli_num -1)) && (last_lli == 1))
        	{
        	    dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    		   (((totaltransfersize % uwnumtransfers)>>uwwidth)&0x7fffffff))); 
		    }
		    else
		    {
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    	(uwnumtransfers>>uwwidth))&0x7fffffff); 
		    }


        	address += 4;phy_address+=4;
        	srcaddr =(pmemaddr[1] + (j*uwnumtransfers));
        	dmac_writew(address, srcaddr);

        	address += 4;phy_address+=4;  
        	dmac_writew(address, SIO0_TX_RIGHT_FIFO);     
      
        	address += 4;phy_address+=4;      
        	if(j==(lli_num -1))               
        		dmac_writew(address, 0); 
        	else
        		dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM)); 

        	address += 4;phy_address+=4;
        	if((j==(lli_num -1)) && (last_lli == 0))
        	{
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    		(uwnumtransfers>>uwwidth)|0x80000000));   
        	}
        	else if((j==(lli_num -1)) && (last_lli == 1)) 
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    		((totaltransfersize % uwnumtransfers)>>uwwidth)|0x80000000));
        	else
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    		(uwnumtransfers>>uwwidth))&0x7fffffff);
        	address += 4;phy_address+=4;
    	}
}


/*
 *	build LLI for sio0 to memory
 *      called by dmac_buildllim2p
 */
void buildlli4sio02m(unsigned int *ppheadlli, 
                     unsigned int *pmemaddr,
                     unsigned int uwperipheralid,
                     unsigned int lli_num, 
                     unsigned int totaltransfersize,
                     unsigned int uwnumtransfers,
                     unsigned int uwwidth,
                     unsigned int last_lli
                     )
{  
     	unsigned int srcaddr,address,phy_address ,j;                 
     	phy_address =(unsigned int)(ppheadlli[0]);            
     	address =(unsigned int)(ppheadlli[1]);                
     	srcaddr =(pmemaddr[0]);                               
                               
     	for (j=0; j<lli_num; j++)
    	{        
        	dmac_writew(address, SIO0_RX_LEFT_FIFO);             
        	address += 4;phy_address+=4; 
        	srcaddr =(pmemaddr[0] + (j*uwnumtransfers));
        	dmac_writew(address, srcaddr);
        	address += 4;phy_address+=4;
        	dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));
    		address += 4;phy_address+=4;
        	if((j==(lli_num -1)) && (last_lli == 1))
        	{    		
        	    dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))\
                    	|(((totaltransfersize % uwnumtransfers)>>uwwidth)&0x7fffffff)));
            } 
		    else
		    {
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    	(uwnumtransfers>>uwwidth))&0x7fffffff); 
		    }
        	address += 4;phy_address+=4;
        	dmac_writew(address, SIO0_RX_RIGHT_FIFO); 
        	address += 4;phy_address+=4; 
        	srcaddr =(pmemaddr[1] + (j*uwnumtransfers)); 
        	dmac_writew(address, srcaddr);
        	address += 4;phy_address+=4;
        	if(j==(lli_num -1))
        		dmac_writew(address, 0);
        	else
        		dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));  
        	address += 4;phy_address+=4;
        	
        	if((j==(lli_num -1)) && (last_lli == 0))  
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))\
                    		|(uwnumtransfers>>uwwidth)|0x80000000));
        	else if((j==(lli_num -1)) && (last_lli == 1))
           		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))\
                       		|((totaltransfersize % uwnumtransfers) >>uwwidth)|0x80000000));
        	else 
            		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                        	(uwnumtransfers>>uwwidth)));
        	address += 4;phy_address+=4;
     	}
}


/*
 *	build LLI for sio1 to memory
 *      called by dmac_buildllim2p
 */
void buildlli4sio12m(unsigned int *ppheadlli, 
                     unsigned int *pmemaddr,
                     unsigned int uwperipheralid,
                     unsigned int lli_num, 
                     unsigned int totaltransfersize,
                     unsigned int uwnumtransfers,
                     unsigned int uwwidth,
                     unsigned int last_lli
                     )
{  
     	unsigned int srcaddr,address,phy_address ,j;                 
     	phy_address =(unsigned int)(ppheadlli[0]);            
     	address =(unsigned int)(ppheadlli[1]);                
     	srcaddr =(pmemaddr[0]);                               
                               
     	for (j=0; j<lli_num; j++)
    	{        
        	dmac_writew(address, SIO1_RX_LEFT_FIFO);             
        	address += 4;phy_address+=4; 
        	srcaddr =(pmemaddr[0] + (j*uwnumtransfers));
        	dmac_writew(address, srcaddr);
        	address += 4;phy_address+=4;
        	dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));
    		address += 4;phy_address+=4;
        	if((j==(lli_num -1)) && (last_lli == 1))
        	{    		
        	    dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))\
                    	|(((totaltransfersize % uwnumtransfers)>>uwwidth)&0x7fffffff)));
            } 
		    else
		    {
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                    	(uwnumtransfers>>uwwidth))&0x7fffffff); 
		    }
        	address += 4;phy_address+=4;
        	dmac_writew(address, SIO1_RX_RIGHT_FIFO); 
        	address += 4;phy_address+=4; 
        	srcaddr =(pmemaddr[1] + (j*uwnumtransfers)); 
        	dmac_writew(address, srcaddr);
        	address += 4;phy_address+=4;
        	if(j==(lli_num -1))
        		dmac_writew(address, 0);
        	else
        		dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));  
        	address += 4;phy_address+=4;
        	
        	if((j==(lli_num -1)) && (last_lli == 0))  
        		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))\
                    		|(uwnumtransfers>>uwwidth)|0x80000000));
        	else if((j==(lli_num -1)) && (last_lli == 1))
           		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))\
                       		|((totaltransfersize % uwnumtransfers) >>uwwidth)|0x80000000));
        	else 
            		dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|\
                        	(uwnumtransfers>>uwwidth)));
        	address += 4;phy_address+=4;
     	}
}
/*
 *	build LLI for memory to peripheral
 */
int  dmac_buildllim2p( unsigned int *ppheadlli, unsigned int *pmemaddr, unsigned int uwperipheralid, unsigned int totaltransfersize,unsigned int uwnumtransfers ,unsigned int burstsize)
{
    
    	unsigned int addtmp,address=0,phy_address=0,srcaddr=0 ;
    	unsigned int uwwidth = 0, lli_num = 0,last_lli=0;
    	unsigned int  j=0; 
    	
    	addtmp   =  pmemaddr[0];	
    	if((mem_check_valid(addtmp) == -1)||(addtmp&0x3))
    	{
        	printk("Invalidate source address,address=%x \n",(unsigned int)pmemaddr[0]);
        	return -EINVAL;                              
    	} 
#if 0    	
    	if ((DMAC_SIO0_TX_REQ==uwperipheralid)  || (DMAC_SIO0_RX_REQ==uwperipheralid)\
    	        ||(DMAC_SIO1_RX_REQ==uwperipheralid))
       	{
        	addtmp = pmemaddr[1];
    		if((mem_check_valid(addtmp) == -1)||(addtmp&0x3))
    		{
        		printk("Invalidate source address,address=%x \n",(unsigned int)pmemaddr);                   
        		return -EINVAL;                              
    		} 
      	}
#endif    
    	if(uwperipheralid > 15)
    	{   
        	printk("Invalidate peripheral id in M2P LLI function, id=%x! \n",addtmp);
        	return -EINVAL;
    	}    
        
    	
    	if(((DMAC_SSP_TX_REQ==uwperipheralid)||(DMAC_SSP_RX_REQ==uwperipheralid)))
    	{
        	uwwidth=0;
    	}
 #if 0   
    	else  if((DMAC_SIO0_TX_REQ==uwperipheralid)||(DMAC_SIO0_RX_REQ==uwperipheralid)\
    	        ||(DMAC_SIO1_RX_REQ==uwperipheralid))
    	{
        	uwwidth=1;
    	}
 #endif   
    	else 
    	{
        	uwwidth=2;
    	}
    	
       	if((uwnumtransfers> (MAXTRANSFERSIZE<<uwwidth)))              
    	{   
        	printk("Invalidate transfer size,size=%x \n",uwnumtransfers);                   
        	return -EINVAL;                                  
    	}
    	lli_num =(totaltransfersize / uwnumtransfers);
    	if((totaltransfersize % uwnumtransfers)!= 0)
    		last_lli=1,++lli_num;
    	if(ppheadlli != NULL)
    	{
		    phy_address =(unsigned int)(ppheadlli[0]);
            address =(unsigned int)(ppheadlli[1]);    	    
    		/*memory to peripheral*/
        	if(uwperipheralid & 0x01)    
        	{       
        	#if 0
        		/*create lli for sio*/     	 
        		if(DMAC_SIO0_TX_REQ == uwperipheralid) 
        		{      
         			buildlli4m2sio0(ppheadlli, pmemaddr,uwperipheralid,lli_num ,\
                            	totaltransfersize,uwnumtransfers,uwwidth,last_lli);        
        		}
        		else
        	#endif
        		{        		    
            		for (j=0; j<lli_num; j++)
            		{
                			srcaddr =(pmemaddr[0] + (j*uwnumtransfers));
                			dmac_writew(address, srcaddr);
                			address += 4;phy_address+=4;
                
                			dmac_writew(address, (unsigned int)(g_peripheral[uwperipheralid].pperi_addr));
                			address += 4;phy_address+=4;

                			if(j==(lli_num -1))
                				dmac_writew(address, 0);
                			else
                				dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));
                			address += 4;phy_address+=4;

                			if((j==(lli_num -1)) && (last_lli == 0))
                				dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|(uwnumtransfers>>uwwidth)|0x80000000));
                			else if((j==(lli_num -1)) && (last_lli == 1))
                				dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|((totaltransfersize % uwnumtransfers) \
                					>>uwwidth)|0x80000000));
                			else
                				dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|(uwnumtransfers>>uwwidth))&0x7fffffff);
                			address += 4;phy_address+=4;
                
            		}
        		}
        	}
            /*peripheral to memory*/
    		else  
    		{
    		#if 0
    			/*create lli for sio*/
        		if(DMAC_SIO0_RX_REQ==uwperipheralid) 
        		{
             			buildlli4sio02m(ppheadlli, pmemaddr,uwperipheralid,lli_num ,\
                                	totaltransfersize,uwnumtransfers,uwwidth,last_lli);
        		}
        		else if(DMAC_SIO1_RX_REQ==uwperipheralid)
        		{
              			buildlli4sio12m(ppheadlli, pmemaddr,uwperipheralid,lli_num ,\
                                	totaltransfersize,uwnumtransfers,uwwidth,last_lli);       		
        		}
        		else
        	#endif
        		{        		    
        			for (j=0; j<lli_num; j++)
        			{
            			dmac_writew(address,(unsigned int)(g_peripheral[uwperipheralid].pperi_addr));
            			address += 4;phy_address+=4;
            
            			srcaddr =(pmemaddr[0] + (j*uwnumtransfers));
            			dmac_writew(address, srcaddr);
            			address += 4;phy_address+=4;

            			if(j==(lli_num -1))
            				dmac_writew(address, 0);
            			else
            				dmac_writew(address, (((phy_address+8)&(~0x03))|DMAC_CxLLI_LM));
            			address += 4;phy_address+=4;
           
            			if((j==(lli_num -1)) && (last_lli == 0))
            				dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|(uwnumtransfers>>uwwidth)|0x80000000));
            			else if((j==(lli_num -1)) && (last_lli == 1))
            				dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|((totaltransfersize % uwnumtransfers) \
               				 >>uwwidth)|0x80000000));
            			else
            				dmac_writew(address, ((g_peripheral[uwperipheralid].transfer_ctrl&(~0xfff))|(uwnumtransfers>>uwwidth))&0x7fffffff);
           			 	address += 4;phy_address+=4;
           
        			}
        		}
    		}
	    }

	    return 0;
}  

/*
 *	execute memory to memory dma transfer without LLI
 */
int dmac_m2m_transfer(unsigned int *psource, unsigned int *pdest, unsigned int  uwtransfersize)
{
    	unsigned int ulchnn, dma_size = 0;
    	unsigned int dma_count, left_size;
    	left_size = uwtransfersize;
    	dma_count = 0;     
    	ulchnn =  dmac_channel_allocate(NULL);
    	if(DMAC_CHANNEL_INVALID == ulchnn) 
    		return -1;
    
    	while((left_size >> 2) >= 0xffc)
    	{
        	dma_size = 0xffc;
        	left_size -= dma_size*4;
        	dmac_start_m2m(ulchnn, (unsigned int)(psource + dma_count * dma_size),(unsigned int)(pdest + dma_count * dma_size), (dma_size << 2));
        	if(dmac_channelstart(ulchnn)!=0)
        		return -1;
        	if(dmac_wait(ulchnn) !=  DMAC_CHN_SUCCESS) 
        		return -1;      
        	dma_count ++;    
    	}
    	dmac_start_m2m(ulchnn, (unsigned int)(psource + dma_count * dma_size), (unsigned int)(pdest + dma_count * dma_size), left_size);
    	if(dmac_channelstart(ulchnn)!=0) 
    		return -1;
    	if(dmac_wait(ulchnn) != DMAC_CHN_SUCCESS) 
    		return -1;     
    	return 0;
}

/*
 *	execute memory to peripheral dma transfer without LLI
 */
int dmac_m2p_transfer(unsigned int *pmemaddr, unsigned int uwperipheralid, unsigned int  uwtransfersize)
{
    	unsigned int ulchnn, dma_size = 0; 
    	unsigned int dma_count, left_size ,uwwidth;
    	left_size = uwtransfersize;
    	dma_count = 0;     
    	ulchnn =  dmac_channel_allocate(NULL);
    	if(DMAC_CHANNEL_INVALID == ulchnn) 
    		return -1;
                                                              

     	if((DMAC_UART0_TX_REQ == uwperipheralid)||(DMAC_UART0_RX_REQ == uwperipheralid) \
     	||(DMAC_SSP_TX_REQ==uwperipheralid)||(DMAC_SSP_RX_REQ==uwperipheralid))
    	{
        	uwwidth=0;
    	}
#if 0    
    	else if((DMAC_SIO0_TX_REQ == uwperipheralid)||(DMAC_SIO0_RX_REQ == uwperipheralid)\
    	        ||(DMAC_SIO1_RX_REQ==uwperipheralid))
    	
    	{
        	uwwidth=1;
    	}
#endif    
    	else 
    	{
        	uwwidth=2;
    	}
    	if(uwtransfersize > (MAXTRANSFERSIZE<<uwwidth))              
    	{   
        	printk("Invalidate transfer size,size=%x \n",uwtransfersize);                   
        	return -EINVAL;                                  
    	}
    	while((left_size >> uwwidth) >= 0xffc)
    	{
        	dma_size = 0xffc;
        	left_size -= dma_size*2*uwwidth;
        	dmac_start_m2p(ulchnn,(unsigned int)(pmemaddr + dma_count * dma_size), uwperipheralid, (dma_size << 2),0);
        	if(dmac_channelstart(ulchnn)!= 0) 
        		return -1;
        	if(dmac_wait(ulchnn) !=  DMAC_CHN_SUCCESS) 
        		return -1;
        	dma_count ++;    
    	}

    	dmac_start_m2p(ulchnn, (unsigned int)(pmemaddr + dma_count * dma_size), uwperipheralid, left_size,0);
    	if(dmac_channelstart(ulchnn)!= 0)   
    		return -1;
    	if(dmac_wait(ulchnn) != DMAC_CHN_SUCCESS) 
    		return -1;   
    	return 0;
}

static int __init dmac_module_init(void)
{
    	int ret;
    	ret = kcom_hidmac_register();
        if(0 != ret)
    	    return -1;
    	ret = dma_driver_init();
    	if(0 != ret)
    	    return -1;
    	//printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
    	return 0;
}


static void __exit dma_module_exit(void)
{
    	free_irq(DMAC_INT, NULL);
    	kcom_hidmac_unregister();  
}

module_init(dmac_module_init);
module_exit(dma_module_exit);

module_param(sio0_mode, int, S_IRUGO);
module_param(sio1_mode, int, S_IRUGO);
module_param(sio2_mode, int, S_IRUGO);


MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("hi_driver_group");
MODULE_VERSION("HI_VERSION=" OSDRV_MODULE_VERSION_STRING);



