/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_vpp.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/09
  Description   : this sample demo the cover region and the overlay region
  History       :
  1.Date        : 2009/07/09
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
#include "hi_comm_vpp.h"
#include "hi_comm_venc.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vpp.h"
#include "mpi_venc.h"
#include "loadbmp.h"
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

#if 0
HI_S32 SampleEnableViVo1D1(HI_VOID)
{
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;
	VO_PUB_ATTR_S VoAttr;
	VO_CHN_ATTR_S VoChnAttr;

	memset(&ViAttr, 0, sizeof(VI_PUB_ATTR_S));
	memset(&ViChnAttr, 0, sizeof(VI_CHN_ATTR_S));

	memset(&VoAttr, 0, sizeof(VO_PUB_ATTR_S));
	memset(&VoChnAttr, 0, sizeof(VO_CHN_ATTR_S));

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;
	ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	ViChnAttr.stCapRect.u32Height = 288;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 8;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_FALSE;
	ViChnAttr.bChromaResample = HI_FALSE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

	VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoAttr.u32BgColor = 10;
	VoChnAttr.bZoomEnable = HI_TRUE;
	VoChnAttr.u32Priority = 1;
	VoChnAttr.stRect.s32X = 0;
	VoChnAttr.stRect.s32Y = 0;
	VoChnAttr.stRect.u32Height = 576;
	VoChnAttr.stRect.u32Width = 720;

	if (HI_SUCCESS != HI_MPI_VI_SetPubAttr(ViDev, &ViAttr))
	{
		printf("set VI attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VI_SetChnAttr(ViDev, ViChn, &ViChnAttr))
	{
		printf("set VI Chn attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VI_Enable(ViDev))
	{
		printf("set VI  enable failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VI_EnableChn(ViDev, ViChn))
	{
		printf("set VI Chn enable failed !\n");
		return HI_FAILURE;
	}

 	if (HI_SUCCESS != HI_MPI_VO_SetPubAttr(&VoAttr))
	{
		printf("set VO attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VO_SetChnAttr(VoChn, &VoChnAttr))
	{
		printf("set VO Chn attribute failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VO_Enable())
	{
		printf("set VO  enable failed !\n");
		return HI_FAILURE;
	}

	if (HI_SUCCESS != HI_MPI_VO_EnableChn(VoChn))
	{
		printf("set VO Chn enable failed !\n");
		return HI_FAILURE;
	}

    if (HI_SUCCESS != HI_MPI_VI_BindOutput(ViDev, ViChn, VoChn))
	{
		printf("HI_MPI_VI_BindOutput failed !\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 SampleDisableViVo1D1(HI_VOID)
{
	VI_DEV ViDev = VIDEVID;
	VI_CHN ViChn = VICHNID;
	VO_CHN VoChn = VOCHNID;

	if(HI_SUCCESS != HI_MPI_VI_UnBindOutput(ViDev, ViChn, VoChn))
	{
		printf("HI_MPI_VI_UnBindOutput failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VI_DisableChn(ViDev, ViChn))
	{
		printf("HI_MPI_VI_DisableChn failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VI_Disable(ViDev))
	{
		printf("HI_MPI_VI_UnBindOutput failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VO_DisableChn(VoChn))
	{
		printf("HI_MPI_VO_DisableChn failed !\n");
		return HI_FAILURE;
	}

	if(HI_SUCCESS != HI_MPI_VO_Disable())
	{
		printf("HI_MPI_VO_Disable failed !\n");
		return HI_FAILURE;
	}

	return HI_SUCCESS;

}


HI_S32 SampleEnableViVo8Cif(HI_VOID)
{
	HI_S32 i;
	HI_S32 j;
	HI_S32 s32Ret;
	VI_PUB_ATTR_S ViAttr;
	VI_CHN_ATTR_S ViChnAttr;
	VO_PUB_ATTR_S VoAttr;
	VO_CHN_ATTR_S VoChnAttr[9];

	memset(&ViAttr, 0, sizeof(VI_PUB_ATTR_S));
	memset(&ViChnAttr, 0, sizeof(VI_CHN_ATTR_S));
	memset(&VoAttr, 0, sizeof(VO_PUB_ATTR_S));
	memset(VoChnAttr, 0, sizeof(VoChnAttr));

	ViAttr.enInputMode = VI_MODE_BT656;
	ViAttr.enWorkMode = VI_WORK_MODE_2D1;

	ViChnAttr.enCapSel = VI_CAPSEL_BOTTOM;
	ViChnAttr.stCapRect.u32Height = 288;
	ViChnAttr.stCapRect.u32Width = 704;
	ViChnAttr.stCapRect.s32X = 8;
	ViChnAttr.stCapRect.s32Y = 0;
	ViChnAttr.bDownScale = HI_TRUE;
	ViChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

	VoAttr.stTvConfig.stComposeMode = VIDEO_ENCODING_MODE_PAL;
	VoAttr.u32BgColor = 0x10;

	VoChnAttr[0].bZoomEnable = HI_TRUE;
	VoChnAttr[0].u32Priority = 1;
	VoChnAttr[0].stRect.s32X = 0;
	VoChnAttr[0].stRect.s32Y = 0;
	VoChnAttr[0].stRect.u32Height = 192;
	VoChnAttr[0].stRect.u32Width = 240;

	VoChnAttr[1].bZoomEnable = HI_TRUE;
	VoChnAttr[1].u32Priority = 1;
	VoChnAttr[1].stRect.s32X = 240;
	VoChnAttr[1].stRect.s32Y = 0;
	VoChnAttr[1].stRect.u32Height = 192;
	VoChnAttr[1].stRect.u32Width = 240;

	VoChnAttr[2].bZoomEnable = HI_TRUE;
	VoChnAttr[2].u32Priority = 1;
	VoChnAttr[2].stRect.s32X = 480;
	VoChnAttr[2].stRect.s32Y = 0;
	VoChnAttr[2].stRect.u32Height = 192;
	VoChnAttr[2].stRect.u32Width = 240;

	VoChnAttr[3].bZoomEnable = HI_TRUE;
	VoChnAttr[3].u32Priority = 1;
	VoChnAttr[3].stRect.s32X = 0;
	VoChnAttr[3].stRect.s32Y = 192;
	VoChnAttr[3].stRect.u32Height = 192;
	VoChnAttr[3].stRect.u32Width = 240;

	VoChnAttr[4].bZoomEnable = HI_TRUE;
	VoChnAttr[4].u32Priority = 1;
	VoChnAttr[4].stRect.s32X = 240;
	VoChnAttr[4].stRect.s32Y = 192;
	VoChnAttr[4].stRect.u32Height = 192;
	VoChnAttr[4].stRect.u32Width = 240;

	VoChnAttr[5].bZoomEnable = HI_TRUE;
	VoChnAttr[5].u32Priority = 1;
	VoChnAttr[5].stRect.s32X = 480;
	VoChnAttr[5].stRect.s32Y = 192;
	VoChnAttr[5].stRect.u32Height = 192;
	VoChnAttr[5].stRect.u32Width = 240;

	VoChnAttr[6].bZoomEnable = HI_TRUE;
	VoChnAttr[6].u32Priority = 1;
	VoChnAttr[6].stRect.s32X = 0;
	VoChnAttr[6].stRect.s32Y = 384;
	VoChnAttr[6].stRect.u32Height = 192;
	VoChnAttr[6].stRect.u32Width = 240;

	VoChnAttr[7].bZoomEnable = HI_TRUE;
	VoChnAttr[7].u32Priority = 1;
	VoChnAttr[7].stRect.s32X = 240;
	VoChnAttr[7].stRect.s32Y = 384;
	VoChnAttr[7].stRect.u32Height = 192;
	VoChnAttr[7].stRect.u32Width = 240;

	VoChnAttr[8].bZoomEnable = HI_TRUE;
	VoChnAttr[8].u32Priority = 1;
	VoChnAttr[8].stRect.s32X = 480;
	VoChnAttr[8].stRect.s32Y = 384;
	VoChnAttr[8].stRect.u32Height = 192;
	VoChnAttr[8].stRect.u32Width = 240;

    for(i=0; i<4; i++)
    {
		s32Ret = HI_MPI_VI_SetPubAttr(i, &ViAttr);
    	if (HI_SUCCESS != s32Ret)
    	{
    		printf("set VI attribute failed 0x%x!\n",s32Ret);
    		return HI_FAILURE;
    	}

        for(j=0; j<2; j++)
        {
			s32Ret = HI_MPI_VI_SetChnAttr(i, j, &ViChnAttr);
        	if (HI_SUCCESS != s32Ret)
        	{
        		printf("set VI Chn attribute failed 0x%x!\n",s32Ret);
        		return HI_FAILURE;
        	}
        }
    }

	s32Ret = HI_MPI_VO_SetPubAttr(&VoAttr);
 	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO attribute failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	for(i=0; i<8; i++)
	{
		s32Ret = HI_MPI_VO_SetChnAttr(i, &VoChnAttr[i]);
		if (HI_SUCCESS != s32Ret)
		{
			printf("set VO 0 Chn attribute failed 0x%x!\n",s32Ret);
			return HI_FAILURE;
		}
	}

	s32Ret = HI_MPI_VO_Enable();
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO  enable failed 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	for(i=0; i<8; i++)
	{
		s32Ret = HI_MPI_VO_EnableChn(i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("set VO 0 Chn attribute failed 0x%x!\n",s32Ret);
			return HI_FAILURE;
		}
	}

    for(i=0; i<4; i++)
    {
		s32Ret = HI_MPI_VI_Enable(i);
    	if (HI_SUCCESS != s32Ret)
    	{
    		printf("set VI  enable failed 0x%x!\n",s32Ret);
    		return HI_FAILURE;
    	}

        for(j=0; j<2; j++)
        {
			s32Ret = HI_MPI_VI_EnableChn(i, j);
        	if (HI_SUCCESS != s32Ret)
        	{
        		printf("set VI Chn enable failed 0x%x!\n",s32Ret);
        		return HI_FAILURE;
        	}
        }
    }

    for(i=0; i<8; i++)
    {
		s32Ret = HI_MPI_VI_BindOutput(i/2, i%2, i);
    	if (HI_SUCCESS != s32Ret)
    	{
    		printf("set VI Chn enable failed 0x%x!\n",s32Ret);
    		return HI_FAILURE;
    	}
    }

	return HI_SUCCESS;
}


HI_VOID SampleDisableViVo8Cif(HI_VOID)
{
	HI_U32 i;
	HI_S32 s32Ret;

	for(i=0; i<8; i++)
	{
		s32Ret = HI_MPI_VI_UnBindOutput(i/2, i%2, 0);
		if (HI_SUCCESS != s32Ret)
		{
			printf("set VO Chn enable failed 0x%x!\n",s32Ret);
			return;
		}

		s32Ret = HI_MPI_VO_DisableChn(i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("set VO Chn enable failed 0x%x!\n",s32Ret);
			return;
		}

		s32Ret = HI_MPI_VI_DisableChn(i/2, i%2);
		if (HI_SUCCESS != s32Ret)
		{
			printf("vi chn disable failed 0x%x!\n",s32Ret);
			return;
		}
	}

	for(i=0; i<4; i++)
	{
		s32Ret = HI_MPI_VI_Disable(i);
		if (HI_SUCCESS != s32Ret)
		{
			printf("vi disable failed 0x%x!\n",s32Ret);
			return;
		}
	}

	s32Ret = HI_MPI_VO_Disable();
	if (HI_SUCCESS != s32Ret)
	{
		printf("set VO  enable failed x%x!\n",s32Ret);
		return;
	}

	return;
}

#endif

HI_S32 SampleEnableVenc1D1(HI_VOID)
{
	HI_S32 s32Ret;
	VENC_GRP VeGroup = VENCCHNID;
	VENC_CHN VeChn = VENCCHNID;
	VENC_CHN_ATTR_S stAttr;
	VENC_ATTR_H264_S stH264Attr;

	stH264Attr.u32PicWidth = 704;
	stH264Attr.u32PicHeight = 576;
	stH264Attr.bMainStream = HI_TRUE;
	stH264Attr.bByFrame = HI_TRUE;
	stH264Attr.enRcMode = RC_MODE_CBR;
	stH264Attr.bField = HI_FALSE;
	stH264Attr.bVIField = HI_TRUE;
	stH264Attr.u32Bitrate = 1024;
	stH264Attr.u32ViFramerate = 25;
	stH264Attr.u32TargetFramerate = 25;
	stH264Attr.u32BufSize = 704*576*2;
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
	HI_S32 s32Ret;
	s32Ret = HI_MPI_VENC_StopRecvPic(VENCCHNID);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StopRecvPic err 0x%x\n",s32Ret);
	}

	s32Ret =HI_MPI_VENC_UnRegisterChn(VENCCHNID);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
	}

	s32Ret =HI_MPI_VENC_DestroyChn(VENCCHNID);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyChn err 0x%x\n",s32Ret);
	}

	s32Ret =HI_MPI_VENC_DestroyGroup(VENCCHNID);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyGroup err 0x%x\n",s32Ret);
	}
}


HI_VOID* SampleGetStream(HI_VOID *p)
{
	HI_S32 i;
	HI_S32 s32Ret;
	HI_S32 s32VencFd = 0;
	HI_U32 u32FrameIdx = 0;
	VENC_CHN VeChn = VENCCHNID;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	FILE *pFile  = NULL;

	pFile = fopen("stream.h264","wb");

	if(NULL == pFile)
	{
		printf("open file err!\n");
		return NULL;
	}

	s32VencFd = HI_MPI_VENC_GetFd(VeChn);

	do{
		FD_ZERO(&read_fds);
		FD_SET(s32VencFd,&read_fds);

		s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, NULL);

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
			if (FD_ISSET(s32VencFd, &read_fds))
			{
				s32Ret = HI_MPI_VENC_Query(VeChn, &stStat);

				if (s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_VENC_Query:0x%x\n",s32Ret);
					fflush(stdout);
					return NULL;
				}

				stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S)*stStat.u32CurPacks);

				if (NULL == stStream.pstPack)
				{
					printf("malloc memory err!\n");
					return NULL;
				}

				stStream.u32PackCount = stStat.u32CurPacks;

				s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_TRUE);

				if (s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
					fflush(stdout);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

				for (i=0; i< stStream.u32PackCount; i++)
				{
					fwrite(stStream.pstPack[i].pu8Addr[0],
							stStream.pstPack[i].u32Len[0], 1, pFile);

					fflush(pFile);

					if (stStream.pstPack[i].u32Len[1] > 0)
					{

						fwrite(stStream.pstPack[i].pu8Addr[1],
								stStream.pstPack[i].u32Len[1], 1, pFile);

						fflush(pFile);
					}
				}

				s32Ret = HI_MPI_VENC_ReleaseStream(VeChn,&stStream);
				if (s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_VENC_ReleaseStream:0x%x\n",s32Ret);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

				free(stStream.pstPack);
				stStream.pstPack = NULL;
			}

		}

		u32FrameIdx ++;
	}while (u32FrameIdx < 0x6ff);

	return HI_SUCCESS;
}


HI_S32 SampleLoadBmp(const char *filename, REGION_CTRL_PARAM_U *pParam)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
		printf("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;

    pParam->stBitmap.pData = malloc(2*(bmpInfo.bmiHeader.biWidth)*(bmpInfo.bmiHeader.biHeight));

    if(NULL == pParam->stBitmap.pData)
    {
        printf("malloc osd memroy err!\n");
        return HI_FAILURE;
    }

    CreateSurfaceByBitMap(filename,&Surface,(HI_U8*)(pParam->stBitmap.pData));

    pParam->stBitmap.u32Width = Surface.u16Width;
    pParam->stBitmap.u32Height = Surface.u16Height;
    pParam->stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;

    return HI_SUCCESS;
}

HI_S32 SampleLoadBmp2(const char *filename, BITMAP_S*pumBitMap)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
		printf("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;

    pumBitMap->pData = malloc(2*(bmpInfo.bmiHeader.biWidth)*(bmpInfo.bmiHeader.biHeight));

    if(NULL == pumBitMap->pData)
    {
        printf("malloc osd memroy err!\n");
        return HI_FAILURE;
    }

    CreateSurfaceByBitMap(filename,&Surface,(HI_U8*)(pumBitMap->pData));

    pumBitMap->u32Width = Surface.u16Width;
    pumBitMap->u32Height = Surface.u16Height;
    pumBitMap->enPixelFormat = PIXEL_FORMAT_RGB_1555;

    return HI_SUCCESS;
}


HI_S32 SampleUserGetRGB(PIXEL_FORMAT_E fmt,HI_VOID *pdata,HI_U32 index,
    				HI_U8 *pr, HI_U8 *pg, HI_U8 *pb,HI_U8 *pa)
{
    HI_U32 Bpp = 0;
    HI_U16 start;

    if(NULL == pdata)
    {
        return HI_FAILURE;
    }

    /*we just support aRGB1555 format*/
    if(PIXEL_FORMAT_RGB_1555 != fmt)
    {
        return HI_FAILURE;
    }
	else
	{
		Bpp = 2;
	}

	//printk("the a is %d\n",*pa);
    start = *((HI_U16*)((HI_U8 *)pdata + index * Bpp));
    *pr = (HI_U8)(((start >> 10) & 0x1f)<<3);
    *pg = (HI_U8)(((start >> 5) & 0x1f)<<3);
    *pb = (HI_U8)((start & 0x1f)<<3);

	/*value of a in every pix*/
	*pa = (HI_U8)((start >> 15)&0x1);

	//printk("the r(%d) g(%d) b(%d)\n",*pr>>3,*pg>>3,*pb>>3);
	//printk("the a is %d\n",*pa);
    return 0;
}

void SampleUserRGB2YUV(HI_U8 r, HI_U8 g, HI_U8 b, HI_U8 * py, HI_U8 * pcb, HI_U8 * pcr)
{
	//printk("r(%d) g(%d) b(%d)\n",r,g,b);
    /* Y */
    *py = (HI_U8)((r*66+g*129+b*25)/256 + 16);  /*Y*/

	/* Cr */
    *pcb = (HI_U8)(((b*112-r*38)-g*74)/256 + 128); /*U*/

	/* Cb */
    *pcr = (HI_U8)(((r*112-g*94)-b*18)/256 + 128); /*V*/
	//printf("y(%d) u(%d) v(%d)\n",*py,*pcb,*pcr);
}


/* NOTICE: you should free pstBitmap->pData after use if, now noly support SEMI_PLAN420/422*/

/*args:
 *pFile         filename      input
 *enPixelFormat PixelFormat   input
 *pumBitMap                   output
 */

HI_S32 SampleLoadBmp2YUV(const char * pFile,const PIXEL_FORMAT_E enPixelFormat,
									BITMAP_S*pumBitMap)
{
	HI_S32 i;
	HI_U32 row,col;
	HI_U32 u32Addr;
	HI_U8 *pu8StartAddr = NULL;

	/* to save one pixel's RGB data */
    HI_U8 pixel_r, pixel_g, pixel_b;

	/* save 2 pixels' rgb to YCbCr data temporarily */
    HI_U8 pixel_y[2], pixel_cb[2], pixel_cr[2],pixel_a[2];

    HI_U8 * pbitmap_data = NULL;/*bitamp data pointer*/

	HI_U8 * prow_start_y;
	HI_U8 * prow_start_vu;

	SampleLoadBmp2(pFile, pumBitMap);
	pu8StartAddr = (HI_U8*)pumBitMap->pData;

	if(PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixelFormat)
	{
		u32Addr = (HI_U32)malloc(pumBitMap->u32Height*pumBitMap->u32Width*2);

		for(row=0; row<pumBitMap->u32Height; row++)
	    {
			/* addres of bmp */
	        pbitmap_data = pu8StartAddr + row*pumBitMap->u32Width*2;

			/* U*/
	        prow_start_y = ((HI_U8*)u32Addr + row*pumBitMap->u32Width);

	        /* Cb*/
	        prow_start_vu = (HI_U8*)u32Addr + (pumBitMap->u32Width*pumBitMap->u32Height)
	        				+ row*(pumBitMap->u32Width);

	        for(col=0; col<pumBitMap->u32Width/2; col++)
	        {
				HI_U16 u16Tmp;

	            /*get 2 pixels' YCbCr every time*/
	            for(i = 0; i < 2; i++)
	            {
	                if(SampleUserGetRGB(pumBitMap->enPixelFormat, pbitmap_data, 2*col + i, &pixel_r, &pixel_g, &pixel_b,&pixel_a[i]) < 0)
	                {
	                    free((HI_VOID *)u32Addr);
						u32Addr = 0;
						free((HI_VOID *)pu8StartAddr);
						pu8StartAddr = 0;

						printf("soft osd bitmap fmt err!\n");
	                    return HI_FAILURE;
	                }


	                SampleUserRGB2YUV(pixel_r, pixel_g, pixel_b, &pixel_y[i], &pixel_cb[i], &pixel_cr[i]);
	            }

				/* process 2 pixel every time*/
				/* deal with Y */
				u16Tmp = pixel_y[0] + (pixel_y[1] << 8);
				*((HI_U16*)((HI_U16 *)prow_start_y + col)) = u16Tmp;

				u16Tmp = pixel_cr[0] + (pixel_cb[0] << 8);
				*((HI_U16*)((HI_U16 *)prow_start_vu + col)) = u16Tmp;
	        }

	    }

		pumBitMap->enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		pumBitMap->pData = (HI_VOID *)u32Addr;
	}
	else if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
	{
		/*YUV420*/
		u32Addr = (HI_U32)malloc(pumBitMap->u32Height*pumBitMap->u32Width*3/2);
		//memset(u32Addr,0xff,umParam.stBitmap.u32Height*umParam.stBitmap.u32Width*3/2);
		for(row=0; row<pumBitMap->u32Height; row++)
	    {
	        pbitmap_data = pu8StartAddr + row*pumBitMap->u32Width*2;

	        prow_start_y = ((HI_U8*)u32Addr + row*pumBitMap->u32Width);

	        prow_start_vu = (HI_U8*)u32Addr + (pumBitMap->u32Width*pumBitMap->u32Height)
	        				+ ((row>>1)*(pumBitMap->u32Width));


	        for(col=0; col<pumBitMap->u32Width/2; col++)
	        {
				HI_U16 u16Tmp;

	            /*get 2 pixels' YCbCr every time*/
	            for(i = 0; i < 2; i++)
	            {
	                if(SampleUserGetRGB(pumBitMap->enPixelFormat, pbitmap_data, 2*col + i, &pixel_r, &pixel_g, &pixel_b,&pixel_a[i]) < 0)
	                {
	                    free((HI_VOID *)u32Addr);
						u32Addr = 0;
						free((HI_VOID *)pu8StartAddr);
						pu8StartAddr = 0;

						printf("soft osd bitmap fmt err!\n");
	                    return HI_FAILURE;
	                }

	                SampleUserRGB2YUV(pixel_r, pixel_g, pixel_b, &pixel_y[i], &pixel_cb[i], &pixel_cr[i]);
	            }

				u16Tmp = pixel_y[0] + (pixel_y[1] << 8);
				*((HI_U16*)((HI_U16 *)prow_start_y + col)) = u16Tmp;

				if(0 == (row & 1))
				{
					u16Tmp = pixel_cr[0]+(pixel_cb[0] << 8);
					*((HI_U16*)((HI_U16 *)prow_start_vu + col)) = u16Tmp;
				}

	        }

	    }

		pumBitMap->enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		pumBitMap->pData = (HI_VOID *)u32Addr;
	}
	else
	{
		return HI_FAILURE;
	}

	free((HI_VOID *)pu8StartAddr);
	pu8StartAddr = NULL;

	return HI_SUCCESS;
}



HI_S32 SampleCtrlCoverRegion1D1(HI_VOID)
{
	HI_S32 i;
	HI_S32 s32Ret;
	HI_S32 s32Cnt = 0;
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_ATTR_S stRgnAttr;
	REGION_HANDLE handle[5];

	stRgnAttr.enType = COVER_REGION;
	stRgnAttr.unAttr.stCover.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stCover.u32Color  = 0;
	stRgnAttr.unAttr.stCover.u32Layer  = 1;
	stRgnAttr.unAttr.stCover.stRect.s32X = 100;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 200;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 50;
	stRgnAttr.unAttr.stCover.stRect.u32Width  = 50;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[0]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	stRgnAttr.enType = COVER_REGION;
	stRgnAttr.unAttr.stCover.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stCover.u32Color  = 0x0000ff00;
	stRgnAttr.unAttr.stCover.u32Layer  = 2;
	stRgnAttr.unAttr.stCover.stRect.s32X = 200;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 200;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 50;
	stRgnAttr.unAttr.stCover.stRect.u32Width  = 50;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[1]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	stRgnAttr.enType = COVER_REGION;
	stRgnAttr.unAttr.stCover.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stCover.u32Color  = 0x00ff0000;
	stRgnAttr.unAttr.stCover.u32Layer  = 3;
	stRgnAttr.unAttr.stCover.stRect.s32X = 300;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 200;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 50;
	stRgnAttr.unAttr.stCover.stRect.u32Width  = 50;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[2]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	stRgnAttr.enType = COVER_REGION;
	stRgnAttr.unAttr.stCover.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stCover.u32Color  = 0x00ff;
	stRgnAttr.unAttr.stCover.u32Layer  = 4;
	stRgnAttr.unAttr.stCover.stRect.s32X = 400;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 200;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 50;
	stRgnAttr.unAttr.stCover.stRect.u32Width  = 50;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[3]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	/*show region*/
	enCtrl = REGION_SHOW;
	s32Ret = HI_MPI_VPP_ControlRegion(handle[0],enCtrl,&unParam);
	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!!!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VPP_ControlRegion(handle[1],enCtrl,&unParam);
	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!!!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VPP_ControlRegion(handle[2],enCtrl,&unParam);
	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!!!\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VPP_ControlRegion(handle[3],enCtrl,&unParam);
	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!!!\n",s32Ret);
		return HI_FAILURE;
	}

	/*ctrl the region*/
	while(1)
	{
		sleep(1);
		s32Cnt++;

		if(s32Cnt <= 10)
		{
			/*change color*/
			enCtrl = REGION_SET_COLOR;

			if(0 == s32Cnt % 2)
			{
				unParam.u32Color = 0;
			}
			else
			{
				unParam.u32Color = 0xff;
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 20)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;
			unParam.stPoint.s32X = 200 + ((s32Cnt - 10) * 2);
			unParam.stPoint.s32Y = 200 + ((s32Cnt - 10) * 5);

			s32Ret = HI_MPI_VPP_ControlRegion(handle[1] ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 30)
		{
			/*change size*/
			enCtrl = REGION_SET_SIZE;
			unParam.stDimension.s32Height = 50 + ((s32Cnt - 20) * 8);
			unParam.stDimension.s32Width  = 50 + ((s32Cnt - 20) * 8);

			s32Ret = HI_MPI_VPP_ControlRegion(handle[2] ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 40)
		{
			/*change layer*/
			enCtrl = REGION_SET_LAYER;
			unParam.u32Layer = s32Cnt;

			if(0 == s32Cnt % 2)
			{
				handle[4] = handle[3];
			}
			else
			{
				handle[4] = handle[2];
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle[4] ,enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else
		{
			for(i=0; i<4; i++)
			{
				HI_MPI_VPP_DestroyRegion(handle[i]);
			}

			break;
		}

	}

	return HI_SUCCESS;
}


HI_S32 SampleCtrlCoverRegion8Cif(HI_VOID)
{
	HI_S32 s32Ret;
	HI_S32 s32Cnt = 0;
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_ATTR_S stRgnAttr;
	REGION_HANDLE handle;

	stRgnAttr.enType = COVER_REGION;
	stRgnAttr.unAttr.stCover.bIsPublic = HI_TRUE;
	stRgnAttr.unAttr.stCover.u32Color  = 0;
	stRgnAttr.unAttr.stCover.u32Layer  = 0;
	stRgnAttr.unAttr.stCover.stRect.s32X = 100;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 200;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 100;
	stRgnAttr.unAttr.stCover.stRect.u32Width  = 100;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle);

	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	/*show region*/
	enCtrl = REGION_SHOW;

	s32Ret = HI_MPI_VPP_ControlRegion(handle,enCtrl,&unParam);

	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	/*ctrl the region*/
	while(1)
	{
		sleep(1);
		s32Cnt++;

		if(s32Cnt <= 10)
		{
			/*change color*/
			enCtrl = REGION_SET_COLOR;

			if(0 == s32Cnt % 2)
			{
				unParam.u32Color = 0;
			}
			else
			{
				unParam.u32Color = 0xff;
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 20)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;
			unParam.stPoint.s32X = 100 + ((s32Cnt - 10) * 5);
			unParam.stPoint.s32Y = 200 + ((s32Cnt - 10) * 5);

			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 30)
		{
			/*change size*/
			enCtrl = REGION_SET_SIZE;
			unParam.stDimension.s32Height = 100 + ((s32Cnt - 20) * 10);
			unParam.stDimension.s32Width  = 100 + ((s32Cnt - 20) * 10);

			s32Ret = HI_MPI_VPP_ControlRegion(handle ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else
		{
			HI_MPI_VPP_DestroyRegion(handle);
			break;
		}

	}

	return HI_SUCCESS;
}

HI_VOID *SampleCtrl4OverlayRegion(HI_VOID *p)
{
	HI_S32 i = 0;
	HI_S32 s32Ret;
	HI_S32 s32Cnt = 0;
	char *pFilename;

	REGION_ATTR_S stRgnAttr;
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_HANDLE handle[4];

	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X= 104;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y= 100;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = 180;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 144;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 128;
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0;
	stRgnAttr.unAttr.stOverlay.VeGroup = 0;
	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[0]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x!\n",s32Ret);
		return NULL;
	}

	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X= 304;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y= 100;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = 180;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 144;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 128;
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x1f;
	stRgnAttr.unAttr.stOverlay.VeGroup = 0;
	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[1]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x!\n",s32Ret);
		return NULL;
	}

	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X= 104;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y= 300;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = 48;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 48;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 70;
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 70;
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x3e0;
	stRgnAttr.unAttr.stOverlay.VeGroup = 0;
	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[2]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x!\n",s32Ret);
		return NULL;
	}

	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X= 304;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y= 300;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = 48;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 48;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 30;
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 30;
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00;
	stRgnAttr.unAttr.stOverlay.VeGroup = 0;
	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[3]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x!\n",s32Ret);
		return NULL;
	}

	/*show all region*/
	enCtrl = REGION_SHOW;

	for(i=0; i<4; i++)
	{
		s32Ret = HI_MPI_VPP_ControlRegion(handle[i], enCtrl, &unParam);

		if(s32Ret != HI_SUCCESS)
		{
			printf("show faild 0x%x!\n",s32Ret);
			return NULL;
		}
	}

	/*ctrl the region*/
	while(1)
	{
		sleep(1);
		s32Cnt++;

		if(s32Cnt <= 10)
		{
			/*change bitmap*/
			if(10 == s32Cnt)
			{
				memset(&unParam, 0, sizeof(REGION_CTRL_PARAM_U));
				pFilename = "mm.bmp";

				SampleLoadBmp(pFilename, &unParam);

				enCtrl = REGION_SET_BITMAP;

				s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam);

				if(s32Ret != HI_SUCCESS)
				{
					if(unParam.stBitmap.pData != NULL)
					{
						free(unParam.stBitmap.pData);
						unParam.stBitmap.pData = NULL;
					}
					printf("set region bitmap faild 0x%x!\n",s32Ret);
					return NULL;
				}

				if(unParam.stBitmap.pData != NULL)
				{
					free(unParam.stBitmap.pData);
					unParam.stBitmap.pData = NULL;
				}

			}

		}
		else if(s32Cnt <= 20)
		{
			/*change bitmap*/
			if(20 == s32Cnt)
			{
				pFilename = "huawei.bmp";
				memset(&unParam, 0, sizeof(REGION_CTRL_PARAM_U));
				SampleLoadBmp(pFilename, &unParam);
				enCtrl = REGION_SET_BITMAP;

				s32Ret = HI_MPI_VPP_ControlRegion(handle[1], enCtrl, &unParam);

				if(s32Ret != HI_SUCCESS)
				{
					if(unParam.stBitmap.pData != NULL)
					{
						free(unParam.stBitmap.pData);
						unParam.stBitmap.pData = NULL;
					}
					printf("REGION_SET_BITMAP 0x%x!\n",s32Ret);
					return NULL;
				}

				if(unParam.stBitmap.pData != NULL)
				{
					free(unParam.stBitmap.pData);
					unParam.stBitmap.pData = NULL;
				}

			}

		}
		else if(s32Cnt <= 30)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;
			unParam.stPoint.s32X = 296 + (s32Cnt - 20)*8;
			unParam.stPoint.s32Y = 296 + (s32Cnt - 20)*8;

			s32Ret = HI_MPI_VPP_ControlRegion(handle[3], enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_POSTION faild 0x%x!\n",s32Ret);
				return NULL;
			}
		}
		else if(s32Cnt <=40)
		{
			enCtrl = REGION_SET_ALPHA0;
			unParam.u32Alpha = 128 - (s32Cnt - 30)*8;

			s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
				return NULL;
			}

			enCtrl = REGION_SET_ALPHA1;
			unParam.u32Alpha = 128 - (s32Cnt - 30)*8;
			s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
				return NULL;
			}
		}
		else
		{
			break;
		}
	}

	for(i=0; i<4; i++)
	{
		HI_MPI_VPP_DestroyRegion(handle[i]);
	}

	return NULL;
}



HI_VOID *SampleCtrl1PublicOverlayRegion(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32Cnt = 0;
	char *pFilename;

	REGION_ATTR_S stRgnAttr;
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_HANDLE handle;

	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_TRUE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X= 104;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y= 100;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = 180;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 144;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 128;
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x!\n",s32Ret);
		return NULL;
	}

	/*show all region*/
	enCtrl = REGION_SHOW;

	s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!\n",s32Ret);
		return NULL;
	}


	/*ctrl the region*/
	while(1)
	{
		sleep(1);
		s32Cnt++;

		if(s32Cnt <= 10)
		{
			/*change bitmap*/
			if(10 == s32Cnt)
			{
				memset(&unParam, 0, sizeof(REGION_CTRL_PARAM_U));
				pFilename = "mm.bmp";

				SampleLoadBmp(pFilename, &unParam);

				enCtrl = REGION_SET_BITMAP;

				s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

				if(s32Ret != HI_SUCCESS)
				{
					if(unParam.stBitmap.pData != NULL)
					{
						free(unParam.stBitmap.pData);
						unParam.stBitmap.pData = NULL;
					}

					printf("set region bitmap faild 0x%x!\n",s32Ret);
					return NULL;
				}

				if(unParam.stBitmap.pData != NULL)
				{
					free(unParam.stBitmap.pData);
					unParam.stBitmap.pData = NULL;
				}

			}

		}
		else if(s32Cnt <= 20)
		{
			/*change bitmap*/
			if(20 == s32Cnt)
			{
				pFilename = "huawei.bmp";
				memset(&unParam, 0, sizeof(REGION_CTRL_PARAM_U));

				SampleLoadBmp(pFilename, &unParam);

				enCtrl = REGION_SET_BITMAP;

				s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

				if(s32Ret != HI_SUCCESS)
				{
					if(unParam.stBitmap.pData != NULL)
					{
						free(unParam.stBitmap.pData);
						unParam.stBitmap.pData = NULL;
					}

					printf("REGION_SET_BITMAP 0x%x!\n",s32Ret);
					return NULL;
				}

				if(unParam.stBitmap.pData != NULL)
				{
					free(unParam.stBitmap.pData);
					unParam.stBitmap.pData = NULL;
				}

			}

		}
		else if(s32Cnt <= 30)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;

			unParam.stPoint.s32X = 200 + (s32Cnt - 20)*8;
			unParam.stPoint.s32Y = 200 + (s32Cnt - 20)*8;

			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_POSTION faild 0x%x!\n",s32Ret);
				return NULL;
			}
		}
		else if(s32Cnt <=40)
		{
			enCtrl = REGION_SET_ALPHA0;

			unParam.u32Alpha = 128 - (s32Cnt - 30)*8;

			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
				return NULL;
			}

			enCtrl = REGION_SET_ALPHA1;
			unParam.u32Alpha = 128 - (s32Cnt - 30)*8;
			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
				return NULL;
			}
		}
		else
		{
			HI_MPI_VPP_DestroyRegion(handle);
			break;
		}
	}

	return NULL;
}

HI_S32 SampleCtrlMosaicRegion1D1(HI_VOID)
{
	HI_S32 i = 0;
	HI_S32 s32Ret = 0;
	HI_S32 s32Cnt = 0;
    HI_CHAR au8FileString[128] = {0};
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_ATTR_S stRgnAttr;
	REGION_HANDLE handle[5] = {0};

	memset(&unParam,0,sizeof(REGION_CTRL_PARAM_U));
	memset(&stRgnAttr,0,sizeof(REGION_ATTR_S));

    sprintf(au8FileString,"mm2.bmp");
    s32Ret =
    SampleLoadBmp2YUV(au8FileString,PIXEL_FORMAT_YUV_SEMIPLANAR_420,&(unParam.stBitmap));
    if(HI_SUCCESS != s32Ret)
    {
        printf("Load bmp 2 YUV err 0x%x\n",s32Ret);
		return HI_FAILURE;
    }

	stRgnAttr.enType = MOSAIC_REGION;
	stRgnAttr.unAttr.stMosaic.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stMosaic.stRect.s32X = 50;
	stRgnAttr.unAttr.stMosaic.stRect.s32Y = 50;
	stRgnAttr.unAttr.stMosaic.stRect.u32Width = unParam.stBitmap.u32Width;
	stRgnAttr.unAttr.stMosaic.stRect.u32Height = unParam.stBitmap.u32Height;
	stRgnAttr.unAttr.stMosaic.u32Layer = 0;
	stRgnAttr.unAttr.stMosaic.u32BgColor = 0x00ff;
	stRgnAttr.unAttr.stMosaic.enPixelFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[0]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	stRgnAttr.enType = MOSAIC_REGION;
	stRgnAttr.unAttr.stMosaic.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stMosaic.stRect.s32X = 100;
	stRgnAttr.unAttr.stMosaic.stRect.s32Y = 100;
	stRgnAttr.unAttr.stMosaic.stRect.u32Width = unParam.stBitmap.u32Width;
	stRgnAttr.unAttr.stMosaic.stRect.u32Height = unParam.stBitmap.u32Height;
	stRgnAttr.unAttr.stMosaic.u32Layer = 1;
	stRgnAttr.unAttr.stMosaic.u32BgColor = 0x00ff;
	stRgnAttr.unAttr.stMosaic.enPixelFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[1]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	stRgnAttr.enType = MOSAIC_REGION;
	stRgnAttr.unAttr.stMosaic.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stMosaic.stRect.s32X = 200;
	stRgnAttr.unAttr.stMosaic.stRect.s32Y = 200;
	stRgnAttr.unAttr.stMosaic.stRect.u32Width = unParam.stBitmap.u32Width;
	stRgnAttr.unAttr.stMosaic.stRect.u32Height = unParam.stBitmap.u32Height;
	stRgnAttr.unAttr.stMosaic.u32Layer = 2;
	stRgnAttr.unAttr.stMosaic.u32BgColor = 0x00ff;
	stRgnAttr.unAttr.stMosaic.enPixelFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[2]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	stRgnAttr.enType = MOSAIC_REGION;
	stRgnAttr.unAttr.stMosaic.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stMosaic.stRect.s32X = 250;
	stRgnAttr.unAttr.stMosaic.stRect.s32Y = 250;
	stRgnAttr.unAttr.stMosaic.stRect.u32Width = unParam.stBitmap.u32Width;
	stRgnAttr.unAttr.stMosaic.stRect.u32Height = unParam.stBitmap.u32Height;
	stRgnAttr.unAttr.stMosaic.u32Layer = 3;
	stRgnAttr.unAttr.stMosaic.u32BgColor = 0x00ff;
	stRgnAttr.unAttr.stMosaic.enPixelFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[3]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	/*insert YUV to region and show region*/
	for(i = 0; i<4; i++)
	{
	    enCtrl = REGION_SET_BITMAP;
	    s32Ret = HI_MPI_VPP_ControlRegion(handle[i],enCtrl,&unParam);
    	if(s32Ret != HI_SUCCESS)
    	{
    	    if(unParam.stBitmap.pData != NULL)
			{
				free(unParam.stBitmap.pData);
				unParam.stBitmap.pData = NULL;
			}
    		printf("setbitmap faild 0x%x!!!\n",s32Ret);
    		return HI_FAILURE;
    	}

    	enCtrl = REGION_SHOW;
    	s32Ret = HI_MPI_VPP_ControlRegion(handle[i],enCtrl,&unParam);
    	if(s32Ret != HI_SUCCESS)
    	{
    		printf("show faild 0x%x!!!\n",s32Ret);
    		return HI_FAILURE;
    	}
    }

    if(unParam.stBitmap.pData != NULL)
	{
		free(unParam.stBitmap.pData);
		unParam.stBitmap.pData = NULL;
	}

	/*ctrl the region*/
	while(1)
	{
		sleep(1);
		s32Cnt++;

		if(s32Cnt <= 10)
		{
			/*change color*/
			enCtrl = REGION_SET_COLOR;

			if(0 == s32Cnt % 2)
			{
				unParam.u32Color = 0;
			}
			else
			{
				unParam.u32Color = 0xff;
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 20)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;
			unParam.stPoint.s32X = 100 + ((s32Cnt - 10) * 10);
			unParam.stPoint.s32Y = 100 + ((s32Cnt - 10) * 6);

			s32Ret = HI_MPI_VPP_ControlRegion(handle[1] ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 30)
		{
			/*change layer*/
			enCtrl = REGION_SET_LAYER;
			unParam.u32Layer = s32Cnt;

			if(0 == s32Cnt % 2)
			{
				handle[4] = handle[3];
			}
			else
			{
				handle[4] = handle[2];
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle[4] ,enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else
		{
			for(i=0; i<4; i++)
			{
				HI_MPI_VPP_DestroyRegion(handle[i]);
			}

			break;
		}

	}

	return HI_SUCCESS;
}


HI_S32 SampleCtrlMosaicRegion8Cif(HI_VOID)
{
	HI_S32 s32Ret = 0;
	HI_S32 s32Cnt = 0;
	HI_CHAR au8FileString[128] = {0};
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_ATTR_S stRgnAttr;
	REGION_HANDLE handle= 0;

	memset(&unParam,0,sizeof(REGION_CTRL_PARAM_U));
	memset(&stRgnAttr,0,sizeof(REGION_ATTR_S));

	stRgnAttr.enType = MOSAIC_REGION;
	stRgnAttr.unAttr.stMosaic.bIsPublic = HI_TRUE;
	stRgnAttr.unAttr.stMosaic.stRect.s32X = 50;
	stRgnAttr.unAttr.stMosaic.stRect.s32Y = 50;
	stRgnAttr.unAttr.stMosaic.stRect.u32Width = 76;
	stRgnAttr.unAttr.stMosaic.stRect.u32Height = 68;
	stRgnAttr.unAttr.stMosaic.u32Layer = 0;
	stRgnAttr.unAttr.stMosaic.u32BgColor = 0x00ff;
	stRgnAttr.unAttr.stMosaic.enPixelFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	stRgnAttr.unAttr.stCover.ViChn = VICHNID;
	stRgnAttr.unAttr.stCover.ViDevId = VIDEVID;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	/*show region*/
	enCtrl = REGION_SHOW;
	s32Ret = HI_MPI_VPP_ControlRegion(handle,enCtrl,&unParam);
	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!\n",s32Ret);
		return HI_FAILURE;
	}

	/*ctrl the region*/
	while(1)
	{
		sleep(1);
		s32Cnt++;

		if(s32Cnt <= 10)
		{
			/*change color*/
			enCtrl = REGION_SET_COLOR;

			if(0 == s32Cnt % 2)
			{
				unParam.u32Color = 0;
			}
			else
			{
				unParam.u32Color = 0xff;
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 20)
		{
		    if(11 == s32Cnt)
		    {
                /*loadbmp and change them to YUV */
                sprintf(au8FileString,"mm2.bmp");
                s32Ret = SampleLoadBmp2YUV(au8FileString,PIXEL_FORMAT_YUV_SEMIPLANAR_420,&(unParam.stBitmap));
                if(HI_SUCCESS != s32Ret)
                {
                    printf("Load bmp 2 YUV err 0x%x\n",s32Ret);
            		return HI_FAILURE;
                }

                /* insert picture */
    			enCtrl = REGION_SET_BITMAP;
        	    s32Ret = HI_MPI_VPP_ControlRegion(handle,enCtrl,&unParam);
            	if(s32Ret != HI_SUCCESS)
            	{
                    if(unParam.stBitmap.pData != NULL)
			        {
				        free(unParam.stBitmap.pData);
				        unParam.stBitmap.pData = NULL;
			        }
            		printf("setbitmap faild 0x%x!!!\n",s32Ret);
            		return HI_FAILURE;
            	}

                if(unParam.stBitmap.pData != NULL)
		        {
			        free(unParam.stBitmap.pData);
			        unParam.stBitmap.pData = NULL;
		        }

    	    }
		}
		else if(s32Cnt <= 30)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;
			unParam.stPoint.s32X = 48 + ((s32Cnt - 20) * 8);
			unParam.stPoint.s32Y = 50 + ((s32Cnt - 20) * 10);

			s32Ret = HI_MPI_VPP_ControlRegion(handle, enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else if(s32Cnt <= 40)
		{
			/*set layer*/
			enCtrl = REGION_SET_LAYER;
			if(0 == s32Cnt % 2)
			{
			    unParam.u32Layer = 0;
			}
			else
			{
			    unParam.u32Layer = 100;
            }

			s32Ret = HI_MPI_VPP_ControlRegion(handle ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
			    printf("set region position faild 0x%x!\n",s32Ret);
				return HI_FAILURE;
			}
		}
		else
		{
			HI_MPI_VPP_DestroyRegion(handle);
			break;
		}

	}

	return HI_SUCCESS;
}


/* cover of single chn (4 region, change clour,posion.size and layer,preview them)*/
HI_S32 SAMPLE_1D1_4CoverRegion()
{
    VO_DEV VoDev = G_VODEV;

	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SAMPLE_StartViVo_SD(1, PIC_D1, VoDev))
	{
		SampleSysExit();
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleCtrlCoverRegion1D1())
	{
		SampleSysExit();
		printf("ctrl cover region faild\n");
	    return HI_FAILURE;
	}

	SAMPLE_StopViVo_SD(1, VoDev);

	SAMPLE_ExitMPP();

	return HI_SUCCESS;
}


/* cover of some chn (1 public region, change clour,posion.size and layer,preview them)*/
HI_S32 SAMPLE_8Cif_1PublicCoverRegion()
{
    VO_DEV VoDev = G_VODEV;

	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SAMPLE_StartViVo_SD(8, PIC_CIF, VoDev))
	{
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleCtrlCoverRegion8Cif())
	{
		printf("ctrl cover region faild\n");
	    return HI_FAILURE;
	}

	SAMPLE_StopViVo_SD(9, VoDev);

	SampleSysExit();

	return HI_SUCCESS;
}


/* overlay of single chn (4 region, change picture,text,posionv and transparency, encode and save stream)*/
HI_S32 SAMPLE_1D1_4OverlayRegion(HI_VOID)
{
    VO_DEV VoDev = G_VODEV;
    pthread_t VencPid;
    GET_STREAM_S stGetStream;
    pthread_t OverlayPid;

	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SAMPLE_StartViVo_SD(1, PIC_D1, VoDev))
	{
		SampleSysExit();
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnableVenc1D1())
	{
		printf("enable vi/vo 1D1 faild\n");
	    return HI_FAILURE;
	}

    stGetStream.enPayload = PT_H264;
    stGetStream.s32SaveTotal = SAVE_ENC_FRAME_TOTAL;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = 1;
	SAMPLE_StartVencGetStream(&stGetStream);
    pthread_create(&OverlayPid, 0, SampleCtrl4OverlayRegion, NULL);

    printf("press twice ENTER to stop venc\n");
    getchar();
    getchar();

    SAMPLE_StopVencGetStream();
	pthread_join(OverlayPid, 0);

	SAMPLE_StopViVo_SD(1, VoDev);

	SampleDisableVenc1D1();

	SampleSysExit();
	return HI_SUCCESS;
}

/* overlay of multi chn (1 public region, change picture,text,posionv and transparency, encode and save stream)*/
HI_S32 SAMPLE_1D1_1PublicOverlayRegion(HI_VOID)
{
    VO_DEV VoDev = G_VODEV;

	pthread_t VencPid;
    GET_STREAM_S stGetStream;
    pthread_t OverlayPid;

	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SAMPLE_StartViVo_SD(1, PIC_D1, VoDev))
	{
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleEnableVenc1D1())
	{
	    return HI_FAILURE;
	}

    stGetStream.enPayload = PT_H264;
    stGetStream.s32SaveTotal = SAVE_ENC_FRAME_TOTAL;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = 1;
    SAMPLE_StartVencGetStream(&stGetStream);
    pthread_create(&OverlayPid, 0, SampleCtrl1PublicOverlayRegion, NULL);

    printf("press twice ENTER to stop venc\n");
    getchar();
    getchar();

    SAMPLE_StopVencGetStream();
	pthread_join(OverlayPid, 0);

	SAMPLE_StopViVo_SD(1, VoDev);

	SampleDisableVenc1D1();
	SampleSysExit();

	return HI_SUCCESS;
}

HI_S32 SAMPLE_1D1_4MosaicRegion()
{
    VO_DEV VoDev = G_VODEV;

	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SAMPLE_StartViVo_SD(1, PIC_D1, VoDev))
	{
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleCtrlMosaicRegion1D1())
	{
		printf("ctrl mosaic region faild\n");
	    return HI_FAILURE;
	}

	SAMPLE_StopViVo_SD(1, VoDev);

	SampleSysExit();

	return HI_SUCCESS;
}


HI_S32 SAMPLE_8Cif_1PublicMosaicRegion()
{
    VO_DEV VoDev = G_VODEV;

	if(HI_SUCCESS != SampleSysInit())
	{
	    printf("sys init err\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SAMPLE_StartViVo_SD(9, PIC_CIF, VoDev))
	{
		printf("enable vi/vo 8cif faild\n");
	    return HI_FAILURE;
	}

	if(HI_SUCCESS != SampleCtrlMosaicRegion8Cif())
	{
		printf("ctrl mosaic region faild\n");
	    return HI_FAILURE;
	}

    SAMPLE_StopViVo_SD(9, VoDev);

	SampleSysExit();

	return HI_SUCCESS;
}


HI_S32 main(int argc, char *argv[])
{
	char ch;
	char ping[1024] =   "\n/**************************************/\n"
						"1  --> SAMPLE_1D1_4CoverRegion\n"
						"2  --> SAMPLE_8Cif_1PublicCoverRegion\n"
						"3  --> SAMPLE_1D1_4OverlayRegion\n"
						"4  --> SAMPLE_1D1_1PublicOverlayRegion\n"
						"q  --> quit\n"
						"please enter order:\n";

	printf("%s",ping);
	while((ch = getchar())!= 'q')
	{

		if('\n' == ch)
		{
			continue;
		}

		switch(ch)
		{
			case '1':
			{
				SAMPLE_1D1_4CoverRegion();
				break;
			}

			case '2':
			{
				SAMPLE_8Cif_1PublicCoverRegion();
				break;
			}

			case '3':
			{
				SAMPLE_1D1_4OverlayRegion();
				break;
			}

			case '4':
			{
				SAMPLE_1D1_1PublicOverlayRegion();
				break;
			}
			default:
				printf("no order@@@@@!\n");
		}

		printf("%s",ping);
	}

	return HI_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

