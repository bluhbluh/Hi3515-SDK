/* ./hi_dmac.h
 *
 * History: 
 *      17-August-2006 create this file
 */
 
 
#ifndef __HI_DMAC_H__
#define __HI_DMAC_H__

	/*the defination for the peripheral*/
	
#define __KCOM_HI_DMAC_INTER__
#include "kcom-hidmac.h"

int dmac_channelclose(unsigned int channel);
int dmac_register_isr(unsigned int channel,void *pisr);
int dmac_channel_free(unsigned int channel);
int free_dmalli_space(unsigned int *ppheadlli, unsigned int page_num);
int dmac_start_llim2p(unsigned int channel, unsigned int *pfirst_lli, unsigned int uwperipheralid);
int dmac_buildllim2m(unsigned int * ppheadlli, unsigned int pdest, unsigned int psource, unsigned int totaltransfersize, unsigned int uwnumtransfers);
int dmac_channelstart(unsigned int u32channel);
int dmac_start_llim2m(unsigned int channel, unsigned int *pfirst_lli);
int  dmac_channel_allocate(void *pisr);
int allocate_dmalli_space(unsigned int *ppheadlli, unsigned int page_num);
int  dmac_buildllim2p( unsigned int *ppheadlli, unsigned int *pmemaddr, 
                       unsigned int uwperipheralid, unsigned int totaltransfersize,
                       unsigned int uwnumtransfers ,unsigned int burstsize);
int dmac_start_m2p(unsigned int channel, unsigned int pmemaddr, unsigned int uwperipheralid, unsigned int  uwnumtransfers,unsigned int next_lli_addr);
int dmac_start_m2m(unsigned int channel, unsigned int psource, unsigned int pdest, unsigned int uwnumtransfers);
int dmac_wait(unsigned int channel);
int kcom_hidmac_register(void);
void kcom_hidmac_unregister(void);
#endif

