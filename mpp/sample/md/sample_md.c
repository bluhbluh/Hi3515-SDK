/******************************************************************************

  Copyright (C), 2001-2011, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_md.c
  Version       : Initial Draft
  Author        : Hi3520MPP
  Created       : 2008/6/27
  Last Modified :
  Description   : this file demo that get MD date and print it to screen
  Function List :
              main
              SampleDisableMd
              SampleGetMdData
              SamplePrintfResult
              SAMPLE_Md_GetData
  History       :
  1.Date        : 2008/6/27
    Author      : Hi3520MPP
    Modification: Created file

******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_md.h"

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_md.h"
#include "tw2864/tw2864.h"
#include "adv7441/adv7441.h"
#include "mt9d131/mt9d131.h"
#include "sample_common.h"

#define VIDEVID 0
#define VICHNID 0
#define VOCHNID 0
#define VENCCHNID 0

/* how many encoded frames will be saved. */
#define SAVE_ENC_FRAME_TOTAL 100

#define G_VODEV VO_DEV_SD
extern VIDEO_NORM_E gs_enViNorm;
extern VO_INTF_SYNC_E gs_enSDTvMode;

HI_BOOL g_bIsQuit = HI_FALSE;

HI_S32 SampleSysInit(HI_VOID)
{
	HI_S32 s32Ret;
	VB_CONF_S stVbConf ={0};
    
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 20;	
	stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 40;	
    
	s32Ret = SAMPLE_InitMPP(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		return HI_FAILURE;
	}

    return HI_SUCCESS;
}


HI_VOID SampleSysExit(HI_VOID)
{	
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
}

HI_S32 SampleEnableVenc1Cif(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_GRP VeGroup = VENCCHNID;
	VENC_CHN VeChn = VENCCHNID;
	VENC_CHN_ATTR_S stAttr;
	VENC_ATTR_H264_S stH264Attr;
			
	stH264Attr.u32PicWidth = 352;
	stH264Attr.u32PicHeight = 288;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.enRcMode = RC_MODE_CBR;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_FALSE;
	stH264Attr.u32Bitrate = 512;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 352*288*2;
	stH264Attr.u32Gop = 100;
	stH264Attr.u32MaxDelay = 100;
	stH264Attr.u32PicLevel = 0;

	stAttr.enType = PT_H264;
	stAttr.pValue = (HI_VOID *)&stH264Attr;
	
	s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VENC_BindInput(VeGroup, VIDEVID, VICHNID);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VENC_CreateChn(VeChn, &stAttr, HI_NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}
	
	s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_VOID SampleDisableVenc1D1(HI_VOID)
{		
	HI_MPI_VENC_StopRecvPic(VENCCHNID);
	HI_MPI_VENC_UnRegisterChn(VENCCHNID);
	HI_MPI_VENC_DestroyChn(VENCCHNID);
	HI_MPI_VENC_DestroyGroup(VENCCHNID);
}


HI_S32 SampleEnableMd(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = VENCCHNID;
    MD_CHN_ATTR_S stMdAttr;
    MD_REF_ATTR_S  stRefAttr;

	/*set MD attribute*/
    stMdAttr.stMBMode.bMBSADMode =HI_TRUE;
    stMdAttr.stMBMode.bMBMVMode = HI_FALSE;
	stMdAttr.stMBMode.bMBPelNumMode = HI_FALSE;
	stMdAttr.stMBMode.bMBALARMMode = HI_FALSE;
	stMdAttr.u16MBALSADTh = 1000;
	stMdAttr.u8MBPelALTh = 20;
	stMdAttr.u8MBPerALNumTh = 20;
    stMdAttr.enSADBits = MD_SAD_8BIT;
    stMdAttr.stDlight.bEnable = HI_FALSE;
    stMdAttr.u32MDInternal = 0;
    stMdAttr.u32MDBufNum = 16;

	/*set MD frame*/
    stRefAttr.enRefFrameMode = MD_REF_AUTO;
	stRefAttr.enRefFrameStat = MD_REF_DYNAMIC;
    
    s32Ret =  HI_MPI_MD_SetChnAttr(VeChn, &stMdAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_MD_SetChnAttr Err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }
	
    s32Ret = HI_MPI_MD_SetRefFrame(VeChn, &stRefAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_MD_SetRefFrame Err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_MD_EnableChn(VeChn);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_MD_EnableChn Err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

	return HI_SUCCESS;
}



HI_S32 SampleDisableMd(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = VENCCHNID;
	
	s32Ret = HI_MPI_MD_DisableChn(VeChn);
	
    if(HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_MD_DisableChn Err 0x%x\n",s32Ret);
        return HI_FAILURE;
    } 

	return HI_SUCCESS;
}



HI_VOID SamplePrintfResult(const MD_DATA_S *pstMdData, FILE *pfd)
{
	/*get MD SAD data*/
	if(pstMdData->stMBMode.bMBSADMode)
	{
		HI_S32 i,j;
		HI_U16* pTmp = NULL;
		
		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U16 *)((HI_U32)pstMdData->stMBSAD.pu32Addr + 
										i*pstMdData->stMBSAD.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				fprintf(pfd, "%2d",*pTmp);
				pTmp++;
			}
			
			fprintf(pfd, "\n");
		}
	}

	/*get MD MV data*/
	if(pstMdData->stMBMode.bMBMVMode)
	{
		HI_S32 i,j;
		HI_U16* pTmp = NULL;
		
		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U16 *)((HI_U32)pstMdData->stMBMV.pu32Addr + 
										i*pstMdData->stMBMV.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				fprintf(pfd, "%2d",*pTmp);
				pTmp++;
			}
			
			fprintf(pfd, "\n");
		}
	}

	/*get MD MB alarm data*/
	if(pstMdData->stMBMode.bMBALARMMode)
	{
		HI_S32 i,j,k;
		HI_U8* pTmp = NULL;

		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U8 *)((HI_U32)pstMdData->stMBAlarm.pu32Addr + 
										i*pstMdData->stMBAlarm.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				k = j%8;

				if(j != 0 && k==0)
				{
					pTmp++;
				}
				
				fprintf(pfd, "%2d",((*pTmp)>>k)&0x1);		
			}
			
			fprintf(pfd, "\n");
		}
	}


	/*get MD MB alarm pels number data*/
	if(pstMdData->stMBMode.bMBPelNumMode)
	{
		HI_S32 i,j;
		HI_U8* pTmp = NULL;

		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U8 *)((HI_U32)pstMdData->stMBPelAlarmNum.pu32Addr + 
								(i*pstMdData->stMBPelAlarmNum.u32Stride));
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				fprintf(pfd, "%2d",*pTmp);
				pTmp++;
			}
			
			fprintf(pfd, "\n");
		}
	}
	
}



HI_VOID *SampleGetMdData(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32MdFd;
	MD_DATA_S stMdData;
	VENC_CHN VeChn = VENCCHNID;
	fd_set read_fds;
	struct timeval TimeoutVal; 
    FILE *pfd;

    pfd = fopen("md_result.data", "wb");
    if (!pfd)
    {
        return NULL;
    }

	s32MdFd = HI_MPI_MD_GetFd(VeChn);

	/*get 0xfff MD data*/
	do{
		FD_ZERO(&read_fds);
		FD_SET(s32MdFd,&read_fds);

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select(s32MdFd+1, &read_fds, NULL, NULL, &TimeoutVal);

		if (s32Ret < 0) 
		{
			printf("select err\n");
			return NULL;
		}
		else if (0 == s32Ret) 
		{
			printf("time out\n");
			return NULL;
		}
		else
		{
			memset(&stMdData, 0, sizeof(MD_DATA_S));
			
			if (FD_ISSET(s32MdFd, &read_fds))
			{
				s32Ret = HI_MPI_MD_GetData(VeChn, &stMdData, HI_IO_NOBLOCK);
				if(s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_MD_GetData err 0x%x\n",s32Ret);
					return NULL;
				}
			}
	
			SamplePrintfResult(&stMdData, pfd);

			s32Ret = HI_MPI_MD_ReleaseData(VeChn, &stMdData);
			if(s32Ret != HI_SUCCESS)
			{
		    	printf("Md Release Data Err 0x%x\n",s32Ret);
				return NULL;
			}

		}
	}while (HI_FALSE == g_bIsQuit);

    if (pfd)
    {
        fclose(pfd);
    }
	return NULL;
}



/* one chn h264 venc (cif), get SAD ,output them to terminal*/
HI_S32 SAMPLE_Md_GetData(HI_VOID)
{
    VO_DEV VoDev = G_VODEV;
    pthread_t MdPid;
	pthread_t VencPid;
	GET_STREAM_S stGetStream;

	/*system init*/
	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	/*enable vi vo*/
	if(HI_SUCCESS != SAMPLE_StartViVo_SD(1, PIC_D1, VoDev))
	{
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	/*enable 1 cif coding*/
	if(HI_SUCCESS != SampleEnableVenc1Cif())
	{
		SAMPLE_StopViVo_SD(1, VoDev);
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}


	/*enable MD*/
	if(HI_SUCCESS != SampleEnableMd())
	{
		SAMPLE_StopViVo_SD(1, VoDev);
		SampleDisableVenc1D1();
		SampleSysExit();
	    return HI_FAILURE;
	}

	g_bIsQuit = HI_FALSE;

	/*enable get H264 stream thread*/
	stGetStream.enPayload = PT_H264;
	stGetStream.s32SaveTotal = SAVE_ENC_FRAME_TOTAL;
	stGetStream.VeChnStart = VENCCHNID;
	stGetStream.s32ChnTotal = 1;
	//pthread_create(&VencPid, 0, thread_GetStream, (HI_VOID *)&stGetStream);
	SAMPLE_StartVencGetStream(&stGetStream);
	
	/*enable get MD data thread*/
    pthread_create(&MdPid, 0, SampleGetMdData, NULL);
    
    printf("start get md data , input twice Enter to stop sample ... ... \n");
	getchar();
	getchar();

	g_bIsQuit = HI_TRUE;
	sleep(1);

    SAMPLE_StopVencGetStream();
	SampleDisableMd();
	SAMPLE_StopViVo_SD(1, VoDev);
	SampleDisableVenc1D1();
	SampleSysExit();
	
	return HI_SUCCESS;
}

#define SAMPLE_MD_HELP(void)\
{\
    printf("usage : %s 1 \n", argv[0]);\
    printf("1:  SAMPLE_Md_GetData\n");\
}

HI_S32 main(int argc, char *argv[])
{    
    int index;

    if (2 != argc)
    {
        SAMPLE_MD_HELP();
        return HI_FAILURE;
    }
    
    index = atoi(argv[1]);
    
    if (1 != index)
    {
        SAMPLE_MD_HELP();
        return HI_FAILURE;
    }
    
    if (1 == index)
    {
        SAMPLE_Md_GetData();
    }

    return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

