/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : total.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/16
  Description   : total.c implement file
  History       :
  1.Date        : 2009/07/16
    Author      : x00100808
    Modification: Created file

******************************************************************************/

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
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vpp.h"

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vdec.h"
#include "mpi_venc.h"
#include "mpi_vpp.h"

#include "loadbmp.h"

#include "tw2864/tw2864.h"
#include "adv7441/adv7441.h"
#include "mt9d131/mt9d131.h"
#include "sample_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

VI_DEV ViDev = 0;
VO_DEV VoDev = 2;
PIC_SIZE_E PicSize = PIC_D1;

/* how many encoded frames will be saved. */
#define SAVE_ENC_FRAME_TOTAL 10000000

/* encode frame rate                                                        */
#define ENCODE_FARME_RATE   25

/* codec frame rate                                                         */
#define CODEC_FRAME_RATE    25


static HI_U32 g_u32TvMode = VIDEO_ENCODING_MODE_PAL;
static HI_U32 g_u32ScreenWidth = 704;
static HI_U32 g_u32ScreenHeight = 576;

#define CHECK(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d !\n", name, __FUNCTION__, __LINE__);\
            return HI_FAILURE;\
        }\
    }while(0)

#define CHECK_RET(express,name)\
                            do{\
                                HI_S32 s32Ret;\
                                s32Ret = express;\
                                if (HI_SUCCESS != s32Ret)\
                                {\
                                    printf("%s failed at %s: LINE: %d whit %#x!\n", name, __FUNCTION__, __LINE__, s32Ret);\
                                    return HI_FAILURE;\
                                }\
                            }while(0)

/* PRINT_COLOR     : 35(PURPLE) 34(BLUE) 33(YELLOW) 32(GREEN) 31(RED) */
#define PRINT_RED(s)        printf("\033[0;31m%s\033[0;39m", s);
#define PRINT_GREEN(s)      printf("\033[0;32m%s\033[0;39m", s);
#define PRINT_YELLOW(s)     printf("\033[0;33m%s\033[0;39m", s);
#define PRINT_BLUE(s)       printf("\033[0;34m%s\033[0;39m", s);
#define PRINT_PURPLE(s)     printf("\033[0;35m%s\033[0;39m", s);


typedef enum hiVDEC_SIZE_E
{
    VDEC_SIZE_D1 = 0,
    VDEC_SIZE_CIF,
    VDEC_SIZE_BUTT
} VDEC_SIZE_E;

typedef enum hiVO_MST_DEV_E
{
    HD  = 0,
    AD  = 1,
    SD  = 2,
    ALL = 012,
    BUTT
} VO_MST_DEV_E;

static HI_BOOL g_bVencStopFlag = HI_FALSE;
static HI_BOOL g_bVdecStartFlag = HI_FALSE;

/*****************************************************************************
 Prototype       : do_Set2815_2d1 & do_Set7179
 Description     : The following two functions are used
                    to set AD and DA for VI and VO.

*****************************************************************************/
HI_S32 SetVouDev(VO_DEV VoDev, HI_U32 u32BgColor, VO_INTF_TYPE_E mux, VO_INTF_SYNC_E mode)
{
    VO_PUB_ATTR_S stPubAttr;

	CHECK_RET(HI_MPI_VO_Disable(VoDev), "close");

    CHECK_RET(HI_MPI_VO_GetPubAttr(VoDev, &stPubAttr), "get");

    stPubAttr.u32BgColor = u32BgColor;
    stPubAttr.enIntfType = mux;
    stPubAttr.enIntfSync = mode;

    CHECK_RET(HI_MPI_VO_SetPubAttr(VoDev, &stPubAttr), "set");

    CHECK_RET(HI_MPI_VO_Enable(VoDev), "open");

    printf("Vou dev %d enabled!\n", VoDev);

    return HI_SUCCESS;
}

HI_S32 ResetDev(HI_S32 s32Dev)
{
    switch (s32Dev)
    {
        case HD:
            SetVouDev(HD, VO_BKGRD_BLUE, VO_INTF_VGA, VO_OUTPUT_1024x768_60);
            break;
        case AD:
            SetVouDev(AD, VO_BKGRD_BLUE, VO_INTF_CVBS, g_u32TvMode);
            break;
        case SD:
        {
            SetVouDev(SD, VO_BKGRD_BLUE, VO_INTF_CVBS, g_u32TvMode);
            break;
        }
        default:
            SetVouDev(HD, VO_BKGRD_BLUE, VO_INTF_VGA, VO_OUTPUT_1024x768_60);
            SetVouDev(AD, VO_BKGRD_BLUE, VO_INTF_CVBS, g_u32TvMode);
            SetVouDev(SD, VO_BKGRD_BLUE, VO_INTF_CVBS, g_u32TvMode);
            break;
    }

    return HI_SUCCESS;
}

HI_VOID SAMPLE_GetViCfg_SD(PIC_SIZE_E     enPicSize,
                           VI_PUB_ATTR_S* pstViDevAttr,
                           VI_CHN_ATTR_S* pstViChnAttr)
{
    pstViDevAttr->enInputMode = VI_MODE_BT656;
    pstViDevAttr->enWorkMode = VI_WORK_MODE_4D1;
    pstViDevAttr->enViNorm = g_u32TvMode;

    pstViChnAttr->stCapRect.u32Width  = 704;
    pstViChnAttr->stCapRect.u32Height =
        (VIDEO_ENCODING_MODE_PAL == g_u32TvMode) ? 288 : 240;
    pstViChnAttr->stCapRect.s32X = 8;
    pstViChnAttr->stCapRect.s32Y = 0;
    pstViChnAttr->enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstViChnAttr->bHighPri = HI_FALSE;
    pstViChnAttr->bChromaResample = HI_FALSE;

    /* different pic size has different capture method for BT656. */
    pstViChnAttr->enCapSel = (PIC_D1 == enPicSize || PIC_2CIF == enPicSize) ? \
                             VI_CAPSEL_BOTH : VI_CAPSEL_BOTTOM;
    pstViChnAttr->bDownScale = (PIC_D1 == enPicSize || PIC_HD1 == enPicSize) ? \
                               HI_FALSE : HI_TRUE;

    return;
}

HI_S32 MixerStartVi(HI_S32 s32ChnTotal)
{
    HI_S32 s32Ret;

    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;

    /* Config VI to input SD */
    SAMPLE_GetViCfg_SD(PicSize, &stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByDev(s32ChnTotal, ViDev, &stViDevAttr, &stViChnAttr);

    return s32Ret;
}

HI_S32 StartVideoLayer(VO_DEV VoDev, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
    HI_S32 s32Ret = 0;

    if (pstLayerAttr == NULL)
    {
        printf("Set video layer %d failed with null pointer!\n", VoDev);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoDev, pstLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        printf("Set Video Layer atter error!\t");
    }

    s32Ret = HI_MPI_VO_EnableVideoLayer(VoDev);
    if (s32Ret != HI_SUCCESS)
    {
        printf("Enable Video Layer atter error!\t");
    }

    return s32Ret;
}

HI_S32 MixerStartVo(VO_DEV VoDev, HI_S32 s32ChnTotal)
{
    HI_U32 i;
    HI_S32 s32Ret;
    HI_U32 u32Width, u32Height;

    VO_VIDEO_LAYER_ATTR_S stLayerAttrHd = {{0,0,1024,768},{720,576},25,PIXEL_FORMAT_YUV_SEMIPLANAR_420,VO_DEFAULT_CHN};;
    VO_VIDEO_LAYER_ATTR_S stLayerAttrAd = {{0,0,720,576},{720,576},25,PIXEL_FORMAT_YUV_SEMIPLANAR_420,VO_DEFAULT_CHN};;

    if (VoDev == 0)
    {
        u32Width = 720;
        u32Height = g_u32ScreenHeight;
    }
    else
    {
        u32Width = 720;
        u32Height = g_u32ScreenHeight;
    }

    stLayerAttrHd.stDispRect.u32Height = 768;
    stLayerAttrHd.stImageSize.u32Height = g_u32ScreenHeight;
    stLayerAttrAd.stImageSize.u32Height = g_u32ScreenHeight;
    stLayerAttrAd.stDispRect.u32Height = g_u32ScreenHeight;
    if (VoDev == 0)
    {
        StartVideoLayer(VoDev, &stLayerAttrHd);
    }
    else
    {
        StartVideoLayer(VoDev, &stLayerAttrAd);
    }

    s32Ret = SAMPLE_SetVoChnMScreen(VoDev, s32ChnTotal, u32Width, u32Height);

    for (i = 0; i < s32ChnTotal; i++)
    {
        s32Ret = HI_MPI_VO_EnableChn(VoDev, i);
        if (s32Ret)
        {
            printf("Enable vo channel %d failed with ec %#x\n", i, s32Ret);
        }
    }

    return s32Ret;
}

/*****************************************************************************
 Prototype       :  VPP operation

 Description     : The following seven functions are used
                    to set VI and VO, configure,enable,disable,etc.

*****************************************************************************/
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
	stRgnAttr.unAttr.stOverlay.stRect.s32X= 160;  /* start x must be multiply of 8 byte. */
	stRgnAttr.unAttr.stOverlay.stRect.s32Y= 160;
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

	if(1)   // load bmp picture
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
	}

	/*ctrl the region*/
	while(1)
	{
		/*change bitmap*/
        if(s32Cnt <= 30)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;

			unParam.stPoint.s32X = 160 + (s32Cnt - 20)*8;  /* start x must be multiply of 8 byte. */
			unParam.stPoint.s32Y = 160 + (s32Cnt - 20)*8;

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
			s32Cnt = 0;
            continue;
		}

		sleep(1);
		s32Cnt++;
	}

	return NULL;
}

HI_VOID* SampleCtrlCoverRegion1D1(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32Cnt = 0;
	REGION_CRTL_CODE_E enCtrl;
	REGION_CTRL_PARAM_U unParam;
	REGION_ATTR_S stRgnAttr;
	REGION_HANDLE handle[1];

	stRgnAttr.enType = COVER_REGION;
	stRgnAttr.unAttr.stCover.bIsPublic = HI_FALSE;
	stRgnAttr.unAttr.stCover.u32Color  = 0xff0000;
	stRgnAttr.unAttr.stCover.u32Layer  = 1;
	stRgnAttr.unAttr.stCover.stRect.s32X = 50;
	stRgnAttr.unAttr.stCover.stRect.s32Y = 50;
	stRgnAttr.unAttr.stCover.stRect.u32Height = 50;
	stRgnAttr.unAttr.stCover.stRect.u32Width  = 50;
	stRgnAttr.unAttr.stCover.ViChn = 1;
	stRgnAttr.unAttr.stCover.ViDevId = ViDev;

	s32Ret = HI_MPI_VPP_CreateRegion(&stRgnAttr, &handle[0]);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VPP_CreateRegion err 0x%x\n",s32Ret);
		return NULL;
	}

	/*show region*/
	enCtrl = REGION_SHOW;
	s32Ret = HI_MPI_VPP_ControlRegion(handle[0],enCtrl,&unParam);
	if(s32Ret != HI_SUCCESS)
	{
		printf("show faild 0x%x!!!\n",s32Ret);
		return NULL;
	}

	/*ctrl the region*/
	while(1)
	{
		if(s32Cnt <= 10)
		{
			/*change color*/
			enCtrl = REGION_SET_COLOR;

			if(0 == s32Cnt % 2)
			{
				unParam.u32Color = 0xff00;
			}
			else
			{
				unParam.u32Color = 0xff;
			}

			s32Ret = HI_MPI_VPP_ControlRegion(handle[0], enCtrl, &unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return NULL;
			}
		}
		else if(s32Cnt <= 20)
		{
			/*change position*/
			enCtrl = REGION_SET_POSTION;
			unParam.stPoint.s32X = 0 + ((s32Cnt - 2) * 2);
			unParam.stPoint.s32Y = 0 + ((s32Cnt - 2) * 5);

			s32Ret = HI_MPI_VPP_ControlRegion(handle[0] ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return NULL;
			}
		}
		else if(s32Cnt <= 30)
		{
			/*change size*/
			enCtrl = REGION_SET_SIZE;
			unParam.stDimension.s32Height = 50 + ((s32Cnt - 10) * 8);
			unParam.stDimension.s32Width  = 50 + ((s32Cnt - 10) * 8);

			s32Ret = HI_MPI_VPP_ControlRegion(handle[0] ,enCtrl,&unParam);

			if(s32Ret != HI_SUCCESS)
			{
				printf("set region position faild 0x%x!!!\n",s32Ret);
				return NULL;
			}
		}
		else
		{
            s32Cnt = 0;
            continue;
		}

		sleep(1);
		s32Cnt++;
	}

	return NULL ;
}


/*****************************************************************************
 Prototype       :
 Description     : The following function is used to encode.

*****************************************************************************/

static HI_S32 SampleEnableEncodeH264(VENC_GRP VeGroup, VENC_CHN VeChn,
                        VI_DEV ViDev, VI_CHN ViChn, VENC_CHN_ATTR_S *pstAttr)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
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

static HI_S32 SampleDisableEncodeH264(VENC_GRP VeGroup, VENC_CHN VeChn)
{
	HI_S32 s32Ret;

	s32Ret = HI_MPI_VENC_StopRecvPic(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyChn(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyChn err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	s32Ret = HI_MPI_VENC_DestroyGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_DestroyGroup err 0x%x\n",s32Ret);
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

static HI_S32 SendStreamToVdec(HI_S32 s32VencChn, VENC_STREAM_S *pstStream)
{
	HI_S32 VdChn = s32VencChn;
	VDEC_STREAM_S stStream;
	HI_S32 s32ret;
	int i;
	HI_U8 *pu8PackAddr = NULL;
	HI_S32 s32PackLen;

	if (HI_FALSE == g_bVdecStartFlag)
	{
	    printf("vdec doesn't start!\n");
		return HI_FAILURE;
	}

    /* Currently, we only support PT_H264  */
	if(1)
	{
    	for (i = 0; i< pstStream->u32PackCount; i ++)
    	{
    		if (0 == pstStream->pstPack[i].u32Len[1])
    		{
    			s32PackLen = pstStream->pstPack[i].u32Len[0];
    			pu8PackAddr = pstStream->pstPack[i].pu8Addr[0];
    		}
    		else
    		{
    			s32PackLen = pstStream->pstPack[i].u32Len[0] + pstStream->pstPack[i].u32Len[1];
    			pu8PackAddr = (HI_U8*)malloc(s32PackLen);
    			if (!pu8PackAddr)
    			{
    				return HI_FAILURE;
    			}
    			memcpy(pu8PackAddr, pstStream->pstPack[i].pu8Addr[0], pstStream->pstPack[i].u32Len[0]);
    			memcpy(pu8PackAddr+pstStream->pstPack[i].u32Len[0], pstStream->pstPack[i].pu8Addr[1],
    				pstStream->pstPack[i].u32Len[1]);
    		}

    		stStream.pu8Addr = pu8PackAddr;
    		stStream.u32Len = s32PackLen;
    		stStream.u64PTS = pstStream->pstPack[i].u64PTS;
    		s32ret = HI_MPI_VDEC_SendStream(VdChn, &stStream, HI_IO_BLOCK);
    		if (s32ret)
    		{
    			printf("send stream to vdec chn %d fail,err:%x\n", VdChn, s32ret);
    		}

    		if (0 != pstStream->pstPack[i].u32Len[1])
    		{
    			free(pu8PackAddr);
    			pu8PackAddr = NULL;
    		}
    	}
	}
    else
    {
        printf("Unsupport encode class!\n");
        return HI_FAILURE;
    }

	return HI_SUCCESS;
}

static HI_VOID* SampleGetH264StreamAndSend(HI_VOID *p)
{
	HI_S32 s32Ret;
	HI_S32 s32VencFd;
	VENC_CHN VeChn;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	FILE *pFile  = NULL;

    struct timeval TimeoutVal;

	VeChn = (HI_S32)p;
	pFile = fopen("stream.h264","wb");

	if(pFile == NULL)
	{
		HI_ASSERT(0);
		return NULL;
	}

	s32VencFd = HI_MPI_VENC_GetFd(VeChn);

    while (HI_TRUE != g_bVencStopFlag)
	{
		FD_ZERO(&read_fds);
		FD_SET(s32VencFd,&read_fds);

		TimeoutVal.tv_sec = 2;
		TimeoutVal.tv_usec = 0;
		s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);

		if (s32Ret < 0)
		{
			printf("select err\n");
			return NULL;
		}
		else if (0 == s32Ret)
		{
			printf("time out 2\n");
			return NULL;
		}
		else
		{
			if (FD_ISSET(s32VencFd, &read_fds))
			{
				s32Ret = HI_MPI_VENC_Query(VeChn, &stStat);

				if (s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_VENC_Query:0x%x err\n",s32Ret);
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

				s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_IO_NOBLOCK);
				if (HI_SUCCESS != s32Ret)
				{
					printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

                /* send stream to VDEC while encoding...                    */
                if(HI_SUCCESS != SendStreamToVdec(1, &stStream))
                {
                    printf("SendStreamToVdec failed!\n");
                    return NULL;
                }

				s32Ret = HI_MPI_VENC_ReleaseStream(VeChn,&stStream);
				if (s32Ret)
				{
					printf("HI_MPI_VENC_ReleaseStream:0x%x\n",s32Ret);
					free(stStream.pstPack);
					stStream.pstPack = NULL;
					return NULL;
				}

				free(stStream.pstPack);
				stStream.pstPack = NULL;
			}

            //printf(".");fflush(stdout);
		}
	}

	fclose(pFile);
	return NULL;
}


static HI_S32 SampleEncodeH264(VI_DEV ViDev, VI_CHN ViChn, VDEC_SIZE_E vdecSize)
{
    VENC_GRP VeGrpChn = 1;
    VENC_CHN VeChn = 1;
    VENC_CHN_ATTR_S stAttr;
    VENC_ATTR_H264_S stH264Attr;

    pthread_t VencH264Pid;
    pthread_t OverlayPid;

    switch (vdecSize)
    {
        case VDEC_SIZE_D1 :
        {
            stH264Attr.u32PicWidth  = g_u32ScreenWidth;
            stH264Attr.u32PicHeight = g_u32ScreenHeight;
            stH264Attr.bMainStream = HI_TRUE;
            stH264Attr.bByFrame = HI_TRUE;
            stH264Attr.enRcMode = RC_MODE_CBR;
            stH264Attr.bField     = HI_FALSE;
            stH264Attr.bVIField   = HI_FALSE;
            stH264Attr.u32Bitrate = 1024;
            stH264Attr.u32ViFramerate = 25;
            stH264Attr.u32TargetFramerate = CODEC_FRAME_RATE;
            stH264Attr.u32BufSize = g_u32ScreenWidth * g_u32ScreenHeight * 2;
            stH264Attr.u32Gop = 100;
            stH264Attr.u32MaxDelay = 100;
            stH264Attr.u32PicLevel = 0;
            break;
        }
        case VDEC_SIZE_CIF :
        {
            stH264Attr.u32PicWidth  = 352;
            stH264Attr.u32PicHeight = 288;
            stH264Attr.bMainStream = HI_TRUE;
            stH264Attr.bByFrame = HI_TRUE;
            stH264Attr.enRcMode = RC_MODE_CBR;
            stH264Attr.bField     = HI_FALSE;
            stH264Attr.bVIField   = HI_FALSE;
            stH264Attr.u32Bitrate = 512;
            stH264Attr.u32ViFramerate = 25;
            stH264Attr.u32TargetFramerate = CODEC_FRAME_RATE;
            stH264Attr.u32BufSize = 352 * 288 * 2;
            stH264Attr.u32Gop = 100;
            stH264Attr.u32MaxDelay = 100;
            stH264Attr.u32PicLevel = 0;
            break;

            break;
        }
        default:
        {
            printf("Unsupport decode method!\n");
            return HI_FAILURE;
        }
    }

    memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
    stAttr.enType = PT_H264;
    stAttr.pValue = (HI_VOID *)&stH264Attr;

    if(HI_SUCCESS != SampleEnableEncodeH264(VeGrpChn, VeChn, ViDev, ViChn, &stAttr))
    {
        printf(" SampleEnableEncode err\n");
        return HI_FAILURE;
    }

    if(HI_SUCCESS != pthread_create(&VencH264Pid, NULL, SampleGetH264StreamAndSend, (HI_VOID *)VeChn))
    {
        printf("create SampleGetH264Stream thread failed!\n");
        SampleDisableEncodeH264(VeGrpChn, VeChn);
        return HI_FAILURE;
    }

#if 1   // overlay

    pthread_create(&OverlayPid, 0, SampleCtrl1PublicOverlayRegion, NULL);

#endif


    return HI_SUCCESS;
}

HI_S32 SAMPLE_StartEncode(VENC_GRP VeGroup, VENC_CHN VeChn,
                          VI_DEV ViDev, VI_CHN ViChn, VENC_CHN_ATTR_S *pstAttr)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_CreateGroup(%d) err 0x%x\n", VeGroup, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_BindInput err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_CreateChn(%d) err 0x%x\n", VeChn, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_RegisterChn err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_StartRecvPic err 0x%x\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       :
 Description     : The following function is used to codec.

*****************************************************************************/
HI_S32 MxierStartH264Encode(VI_CHN ViChn, HI_S32 s32ChnTotal, PIC_SIZE_E enPicSize)
{
    VENC_GRP VeGroup = 0;
    VENC_CHN VeChn = 0;
    VENC_CHN_ATTR_S stAttr;
    VENC_ATTR_H264_S stH264Attr;
    HI_U32 u32Width;
    HI_U32 u32Height;
    HI_S32 s32Ret;

    switch ( enPicSize )
    {
        case PIC_QCIF:
            u32Width = 176;
            u32Height = (VIDEO_ENCODING_MODE_PAL == g_u32TvMode) ? 128 : 120;
            stH264Attr.u32PicWidth  = u32Width;
            stH264Attr.u32PicHeight = u32Height;
            stH264Attr.bMainStream = HI_TRUE;
            stH264Attr.bByFrame = HI_TRUE;
            stH264Attr.enRcMode = RC_MODE_CBR;
            stH264Attr.bField     = HI_FALSE;
            stH264Attr.bVIField   = HI_FALSE;
            stH264Attr.u32Bitrate = 256;
            stH264Attr.u32ViFramerate = 25;
            stH264Attr.u32TargetFramerate = ENCODE_FARME_RATE;
            stH264Attr.u32BufSize = u32Width * u32Height * 2;
            stH264Attr.u32Gop = 100;
            stH264Attr.u32MaxDelay = 100;
            stH264Attr.u32PicLevel = 0;
            break;
        case PIC_CIF:
            u32Width = 352;
            u32Height = (VIDEO_ENCODING_MODE_PAL == g_u32TvMode) ? 288 : 240;
            stH264Attr.u32PicWidth  = u32Width;
            stH264Attr.u32PicHeight = u32Height;
            stH264Attr.bMainStream = HI_TRUE;
            stH264Attr.bByFrame = HI_TRUE;
            stH264Attr.enRcMode = RC_MODE_CBR;
            stH264Attr.bField     = HI_FALSE;
            stH264Attr.bVIField   = HI_FALSE;
            stH264Attr.u32Bitrate = 512;
            stH264Attr.u32ViFramerate = 25;
            stH264Attr.u32TargetFramerate = ENCODE_FARME_RATE;
            stH264Attr.u32BufSize = u32Width * u32Height * 2;
            stH264Attr.u32Gop = 100;
            stH264Attr.u32MaxDelay = 100;
            stH264Attr.u32PicLevel = 0;
            break;
        case PIC_D1:
            u32Width = 704;
            u32Height = (VIDEO_ENCODING_MODE_PAL == g_u32TvMode) ? 576 : 480;
            stH264Attr.u32PicWidth  = u32Width;
            stH264Attr.u32PicHeight = u32Height;
            stH264Attr.bMainStream = HI_TRUE;
            stH264Attr.bByFrame = HI_TRUE;
            stH264Attr.enRcMode = RC_MODE_CBR;
            stH264Attr.bField     = HI_FALSE;
            stH264Attr.bVIField   = HI_FALSE;
            stH264Attr.u32Bitrate = 1024;
            stH264Attr.u32ViFramerate = 25;
            stH264Attr.u32TargetFramerate = ENCODE_FARME_RATE;
            stH264Attr.u32BufSize = u32Width * u32Height * 2;
            stH264Attr.u32Gop = 100;
            stH264Attr.u32MaxDelay = 100;
            stH264Attr.u32PicLevel = 0;
            break;
        default:
            printf("err pic size\n");
            return HI_FAILURE;
    }

    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
    stAttr.enType = PT_H264;
    stAttr.pValue = (HI_VOID *)&stH264Attr;

    /* Here, venc bind to vi in sequence. */
    s32Ret = SAMPLE_StartEncode(VeGroup, VeChn, ViDev, ViChn, &stAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 MixerStopEncode(VENC_GRP VeGroup, VENC_CHN VeChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_StopRecvPic(VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_StopRecvPic vechn %d err 0x%x\n", VeChn, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_UnRegisterChn(VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_UnRegisterChn vechn %d err 0x%x\n", VeChn, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_DestroyChn(VeChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_DestroyChn vechn %d err 0x%x\n", VeChn, s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VENC_DestroyGroup(VeGroup);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_DestroyGroup grp %d err 0x%x\n", VeGroup, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static HI_S32 MixerCreateVdecchn(HI_S32 s32ChnID, PAYLOAD_TYPE_E enType, void* pstAttr)
{
    VDEC_CHN_ATTR_S stAttr;
    HI_S32 s32ret;

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));

    switch (enType)
    {
    case PT_H264:
    {
        VDEC_ATTR_H264_S stH264Attr;

        memcpy(&stH264Attr, pstAttr, sizeof(VDEC_ATTR_H264_S));
        stAttr.enType = PT_H264;
        stAttr.u32BufSize = ((stH264Attr.u32PicWidth * stH264Attr.u32PicHeight) << 1);
        stAttr.pValue = (void*)&stH264Attr;
    }
        break;

    case PT_JPEG:
    {
        VDEC_ATTR_JPEG_S stJpegAttr;

        memcpy(&stJpegAttr, pstAttr, sizeof(VDEC_ATTR_JPEG_S));
        stAttr.enType = PT_JPEG;
        stAttr.u32BufSize = ((stJpegAttr.u32PicWidth * stJpegAttr.u32PicHeight) * 1.5);
        stAttr.pValue = (void*)&stJpegAttr;
    }
        break;
    default:
        return HI_FAILURE;
    }

    s32ret = HI_MPI_VDEC_CreateChn(s32ChnID, &stAttr, NULL);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32ret);
        return s32ret;
    }

    s32ret = HI_MPI_VDEC_StartRecvStream(s32ChnID);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32ret);
        return s32ret;
    }

    return HI_SUCCESS;
}

/*send h264 nalu to vdec pthread*/
#define MAX_READ_LEN 1024
static HI_CHAR gs_aBuf[MAX_READ_LEN];
void* thread_sendh264stream(void* p)
{
    VDEC_STREAM_S stStream;
    char *filename;
    FILE* file = NULL;
    HI_S32 s32ReadLen;
    HI_S32 s32Ret;

    filename = (char*)p;

    /*open the stream file*/
    file = fopen(filename, "r");
    if (!file)
    {
        printf("open file %s err\n", filename);
        return NULL;
    }

    while (1)
    {
        s32ReadLen = fread(gs_aBuf, 1, MAX_READ_LEN, file);
        if (s32ReadLen <= 0)
        {
            fseek(file, 0L, SEEK_SET);
            continue;
        }

        stStream.pu8Addr = gs_aBuf;
        stStream.u32Len = s32ReadLen;
        stStream.u64PTS = 0;

        s32Ret = HI_MPI_VDEC_SendStream(0, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32Ret)
        {
            printf("send to vdec 0x %x \n", s32Ret);
            break;
        }
    }

    fclose(file);
    return NULL;
}

void MixerWaitDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32ret;
    VDEC_CHN_STAT_S stStat;

    memset(&stStat, 0, sizeof(VDEC_CHN_STAT_S));

    s32ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (s32ret != HI_SUCCESS)
    {
        printf("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32ret);
        return;
    }

    while (1)
    {
        usleep(40000);
        s32ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
        if (s32ret != HI_SUCCESS)
        {
            printf("HI_MPI_VDEC_Query failed errno 0x%x \n", s32ret);
            return;
        }

        if ((stStat.u32LeftPics == 0) && (stStat.u32LeftStreamBytes == 0))
        {
            printf("had no stream and pic left\n");
            break;
        }
    }

    s32ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (s32ret != HI_SUCCESS)
    {
        printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32ret);
        return;
    }
}

static HI_S32 MixerStartVdecChn(HI_S32 s32VoChn, VDEC_SIZE_E vdecSize)
{
    HI_S32 s32ret;
    VDEC_CHN VdecChn = 1;
    VDEC_CHN_ATTR_S stAttr;
    VDEC_ATTR_H264_S stH264Attr;

	stH264Attr.u32Priority = 0;
	stH264Attr.enMode = H264D_MODE_STREAM;
	stH264Attr.u32RefFrameNum = 2;

    switch (vdecSize)
    {
        case VDEC_SIZE_D1 :
        {
            stH264Attr.u32PicWidth = g_u32ScreenWidth;
            stH264Attr.u32PicHeight = g_u32ScreenHeight;
            break;
        }
        case VDEC_SIZE_CIF :
        {
            stH264Attr.u32PicWidth = g_u32ScreenWidth/2;
            stH264Attr.u32PicHeight = g_u32ScreenHeight/2;
            break;
        }
        default:
        {
            printf("Unsupport decode method!\n");
            return HI_FAILURE;
        }
    }

    memset(&stAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    stAttr.enType = PT_H264;
    stAttr.u32BufSize = (((stH264Attr.u32PicWidth) * (stH264Attr.u32PicHeight))<<1);
    stAttr.pValue = (void*)&stH264Attr;

    s32ret = HI_MPI_VDEC_CreateChn(VdecChn, &stAttr, NULL);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32ret);
        return s32ret;
    }

    s32ret = HI_MPI_VDEC_StartRecvStream(VdecChn);
    if (HI_SUCCESS != s32ret)
    {
        printf("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32ret);
        return s32ret;
    }

	if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(VdecChn, VoDev, s32VoChn))
	{
        printf("bind vdec to vo failed\n");
		return HI_FAILURE;
	}

    g_bVdecStartFlag = HI_TRUE;


    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       :  MixerStartEncode
                    MixerStartDecode
                    MixerStartCodec

 Description     : The following three functions are used
                    to implement different function in main branch.

*****************************************************************************/
HI_S32 MixerStartEncode(HI_VOID)
{
    pthread_t CoverPid;
    MxierStartH264Encode(1, 1, PicSize);

    if (1)
    {
        GET_STREAM_S stGetStream;
        pthread_t VencGetStreamPid;

        stGetStream.enPayload = PT_H264;
        stGetStream.s32SaveTotal = SAVE_ENC_FRAME_TOTAL;
        stGetStream.VeChnStart = 0;
        stGetStream.s32ChnTotal = 1;
        pthread_create(&VencGetStreamPid, 0, SampleGetVencStreamProc, &stGetStream);
    }

#if 1   // overlay

    pthread_create(&CoverPid, 0, SampleCtrlCoverRegion1D1, NULL);

#endif

    return HI_SUCCESS;
}

HI_S32 MixerStartDecode(HI_VOID)
{
    pthread_t pidSendStream;
    VDEC_ATTR_H264_S stH264Attr;
    VDEC_CHN VdChn = 0;
    static HI_CHAR filename[40] = "d1-pal-seasiderun.h264";

    stH264Attr.u32Priority  = 0;
    stH264Attr.u32PicHeight = 576;
    stH264Attr.u32PicWidth = 720;
    stH264Attr.u32RefFrameNum = 3;
    stH264Attr.enMode = H264D_MODE_STREAM;

    /*create vdec chn and vo chn*/
    if (HI_SUCCESS != MixerCreateVdecchn(VdChn, PT_H264, &stH264Attr))
    {
        return HI_FAILURE;
    }

    /*bind vdec to vo*/
    if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(VdChn, VoDev, 2))
    {
        return HI_FAILURE;
    }

    pthread_create(&pidSendStream, NULL, thread_sendh264stream, filename);

    return HI_SUCCESS;
}

HI_S32 MixerStartCodec(HI_VOID)
{
    /* we use VOCHN 0 as decode destination                                 */
    CHECK(MixerStartVdecChn(3, VDEC_SIZE_D1),"SampleStartVdecChn D1 failed!");

    /* we use VIDEV 0, VICHN 0 as encode source                              */
    CHECK(SampleEncodeH264(ViDev, 0, VDEC_SIZE_D1),"SampleEncodeH264 D1 failed!");

    return HI_SUCCESS;
}


/*****************************************************************************
 Prototype       :  HandleSig & Usage & SysInit
 Description     : The following two functions are used
                    to process abnormal case and provide help respectively.

*****************************************************************************/

/* function to process abnormal case                                        */
HI_VOID HandleExcptSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
    	(HI_VOID)HI_MPI_SYS_Exit();
    	(HI_VOID)HI_MPI_VB_Exit();
    	printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }
    exit(0);
}

/* usage for user                                                           */
HI_VOID Usage(HI_VOID)
{
    puts(" ---- Usage ------------------------");
    puts(" append '0'     for Vo device HD!");
    puts(" append '1'     for Vo device AD!");
    puts(" append '2'     for Vo device SD!");
    puts(" append 'n'     for NTSC norm!");
    puts(" append 'reset' for system reset!");
    puts(" append 'about' for description!");
    puts(" append 'help'  for this menu!");
    puts(" type 'Ctl + c' for termination!");
    puts("------------------------------------");
}

HI_VOID About(HI_VOID)
{
    PRINT_YELLOW("\n ------ About -----------\n"
        " This function is used to display almost all function and performance of\n"
        " Hi3520 platform. 4 cif sub-screen layout, 0 channel for D1 preview, 1st channel for\n"
        " H264 D1 encode, 2nd channel for H264 D1 decode, the last channel for D1 encode-decode.\n"
        " Overlay is tapped on the last channel, Cover is tapped on 2nd channel with blue color.\n"
        " --------------------------------------------------------------------------------------\n\n");
}

HI_S32 MixerSysInit(HI_VOID)
{
    HI_S32 s32Ret;
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();

	MPP_SYS_CONF_S stSysConf = {0};
	VB_CONF_S stVbConf ={0};

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 1024*768*2;
	stVbConf.astCommPool[0].u32BlkCnt = 5;
	stVbConf.astCommPool[1].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[1].u32BlkCnt = 20;
	stVbConf.astCommPool[2].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[2].u32BlkCnt = 30;
	stSysConf.u32AlignWidth = 64;

    s32Ret = HI_MPI_VB_SetConf(&stVbConf);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VB_SetConf failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VB_Init();
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VB_Init failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_SYS_SetConf failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_SYS_Init();
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_SYS_Init failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : main
 Description     : main entry
 Input           : argc    : "reset", "N", "n" is optional.
                   argv[]  : 0 or 1 is optional
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
  History
  1.Date         : 2008/8/30
    Author       : x00100808
    Modification : Created function

*****************************************************************************/
int main(int argc, char *argv[])
{
	char ch = '\0';

    if (argc > 1)
    {
        /* if program can not run anymore, you may try 'reset' adhere command  */
        if (!strcmp(argv[1],"reset"))
        {
           	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
        	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");
        	return HI_SUCCESS;
        }
        else if ((!strcmp(argv[1],"N") || (!strcmp(argv[1],"n"))))
        {
            /* if you live in Japan or North America, you may need adhere 'N/n' */
            g_u32TvMode = VIDEO_ENCODING_MODE_NTSC;
            g_u32ScreenHeight = 480;
        }
        else if ((!strcmp(argv[1],"0")|| !strcmp(argv[1],"1") || !strcmp(argv[1],"2")))
        {
            VoDev = atoi(argv[1]);
            printf("Now display on VoDev %d\n", VoDev);
        }
        else if (!strcmp(argv[1],"about"))
        {
            About();
            return HI_SUCCESS;
        }
        else
        {
            /* you can open vi2vo preview only                              */
        	Usage();
            return HI_SUCCESS;
        }
    }

    Usage();

    /* process abnormal case                                                */
    signal(SIGINT, HandleExcptSig);
    signal(SIGTERM, HandleExcptSig);

    /* configure video buffer and initial system                            */
	CHECK(MixerSysInit(),"System init!");

    /* sys norm initialize, default settings */
    ResetDev(VoDev);

    /* set AD and DA for VI and VO                                          */
    SAMPLE_TW2865_CfgV(g_u32TvMode, VI_WORK_MODE_4D1);

    /* start vi                                                             */
    CHECK(MixerStartVi(4),"Start vi");

    /* start vo                                                             */
    CHECK(MixerStartVo(VoDev,4),"Start vo");

    /* entry                                                            */
    do
    {

    #if 1   /* preview                                                          */
            CHECK(HI_MPI_VI_BindOutput(ViDev, 0, VoDev, 0), "previewing...");
            PRINT_GREEN("Previewing...\n");
    #endif


    #if 1   /* encoder                                                          */
            CHECK(HI_MPI_VI_BindOutput(ViDev, 1, VoDev, 1), "prevewing2...");
            CHECK(MixerStartEncode(), "encodeing...");
            PRINT_GREEN("Encoding...\n");
    #endif


    #if 1   /* decoder                                                          */
            CHECK(MixerStartDecode(), "decodeing...");
            PRINT_GREEN("Decoding...\n");
    #endif


    #if 1   /* codecer                                                          */
            CHECK(MixerStartCodec(),  "codecing...");
            PRINT_GREEN("Codecing...\n");
    #endif

    }while (0);

    do
    {
        PRINT_PURPLE("type 'Ctrl + c' to exit!\n");

    } while((ch = getchar())!= 'q');

    g_bVencStopFlag = HI_TRUE;

    /* de-init sys and vb */
	CHECK(HI_MPI_SYS_Exit(),"HI_MPI_SYS_Exit");
	CHECK(HI_MPI_VB_Exit(),"HI_MPI_VB_Exit");

	return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

