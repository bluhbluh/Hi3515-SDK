#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mpi_vo.h"
#include "mkp_vd.h"
#include "mpi_sys.h"
#include "hi_comm_sys.h"
#include "vo_open.h"

HI_S32 MppSysInit(HI_VOID)
{
	HI_S32 i = 0;
	MPP_SYS_CONF_S stSysConf = {0};

	HI_MPI_SYS_Exit();
	
	stSysConf.u32AlignWidth = 16;
    if (HI_MPI_SYS_SetConf(&stSysConf))
    {
        printf("conf : system config failed!\n");
        return -1;
    }

    if (HI_MPI_SYS_Init())
    {
        printf("sys init failed!\n");
        return -1;
    }

	/*to ensure all vo device are disabled*/
	for(i=0; i<VOU_DEV_BUTT; i++)
	{
		(HI_VOID)HI_MPI_VO_Disable(i);
	}

    return HI_SUCCESS;
}


HI_VOID MppSysExit(HI_VOID)
{	
	HI_MPI_SYS_Exit();
}

HI_S32 EnableVoDev(HI_S32 DevId)
{
	static VO_PUB_ATTR_S s_stPubAttrDflt[3] = 
	{
		{
			.u32BgColor = 0x0000FF,
			.enIntfType = VO_INTF_VGA,
			.enIntfSync = VO_OUTPUT_1024x768_60,
			.stSyncInfo = {0},
		},
		{
			.u32BgColor = 0x0000FF,
			.enIntfType = VO_INTF_CVBS,
			.enIntfSync = VO_OUTPUT_PAL,
			.stSyncInfo = {0},
		},
		#ifdef hi3515
		{
			.u32BgColor = 0x0000FF,
			.enIntfType = VO_INTF_CVBS,
			.enIntfSync = VO_OUTPUT_PAL,
			.stSyncInfo = {0},
		}
		#else
		{
			.u32BgColor = 0x0000FF,
			.enIntfType = VO_INTF_CVBS,
			.enIntfSync = VO_OUTPUT_PAL,
			.stSyncInfo = {0},
		}
		#endif
	};
	
	HI_S32 s32Ret = 0;
	s32Ret = HI_MPI_VO_SetPubAttr(DevId,&s_stPubAttrDflt[DevId]);
	if(s32Ret != HI_SUCCESS)
	{
        printf("set vo attr on dev %d failed:%x!\n",DevId,s32Ret);
        return -1;
    }
    s32Ret = HI_MPI_VO_Enable(DevId);
	if(s32Ret != HI_SUCCESS)
    {
        printf("enable vo dev %d failed:%x!\n",DevId,s32Ret);
        return -1;
    }
	return HI_SUCCESS;
} 

HI_S32 DisableVoDev(HI_S32 DevId)
{
	HI_S32 s32Ret = 0;
    s32Ret = HI_MPI_VO_Disable(DevId);
	if(s32Ret != HI_SUCCESS)
    {
        printf("disable vo dev %d failed:%x!\n",DevId,s32Ret);
        return -1;
    }
	return HI_SUCCESS;
} 


