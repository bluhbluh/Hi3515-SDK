/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : sample_venc.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/04
  Description   :
  History       :
  1.Date        : 2009/07/04
    Author      : Hi3520MPP
    Modification: Created file

******************************************************************************/

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

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
#include "sample_common.h"

#define VIDEVID 0
#define VICHNID 0
#define VOCHNID 0
#define VENCCHNID 0
#define SNAPCHN 1

/* how many encoded frames will be saved. */
#define SAVE_ENC_FRAME_TOTAL 100

/* how many snap file will be save. */
#define SNAP_TOTAL_EACH_CHN  5

#define G_VODEV VO_DEV_SD

extern VIDEO_NORM_E gs_enViNorm;
extern VO_INTF_SYNC_E gs_enVoOutput;

typedef struct hiSNAP_MULTI_CHN_S
{
    HI_BOOL bThreadStart;
    pthread_t pid;
    VENC_GRP SnapGroup;   /*snap group*/
    VENC_CHN SnapChn;   /*snap venc chn*/
    VI_DEV ViDev;       /*vi device */
    HI_S32 s32ViChnCnt; /*how many vi channel to snap*/
    HI_S32 s32SnapTotal;/* how many frames to snap. It is sum of all channls. 
                               * invalid now. replaced by more interactive method. */
} SNAP_MULTI_CHN_S;

typedef struct hiSNAP_SINGLE_CHN_S
{
    HI_BOOL bThreadStart;
    pthread_t pid;
    VENC_GRP VeGroup;   /*snap group*/
    VENC_CHN SnapChn;   /*snap venc chn*/
    VI_DEV ViDev;       /*vi device,it has the vichn which snap group bind to*/
    VI_CHN ViChn;       /*vi channel which snap group binded to*/
    HI_S32 s32SnapTotal;/* how many frames to snap. 
                               * invalid now. replaced by more interactive method. */
} SNAP_SINGLE_CHN_S;

HI_S32 SAMPLE_GetSnapPic(VENC_CHN SnapChn, FILE *pFile)
{
    HI_S32 s32Ret;
    HI_S32 s32VencFd;
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
	fd_set read_fds;
    struct timeval TimeoutVal;

	s32VencFd = HI_MPI_VENC_GetFd(SnapChn);
	
	FD_ZERO(&read_fds);
	FD_SET(s32VencFd, &read_fds);
    
    TimeoutVal.tv_sec  = 2;
    TimeoutVal.tv_usec = 0;
	s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
	
	if (s32Ret < 0) 
	{
		printf("snap select err\n");
		return HI_FAILURE;
	}
	else if (0 == s32Ret) 
	{
		printf("snap time out\n");
		return HI_FAILURE;
	}
	else
	{
		if (FD_ISSET(s32VencFd, &read_fds))
		{
            s32Ret = HI_MPI_VENC_Query(SnapChn, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                printf("HI_MPI_VENC_Query:0x%x\n", s32Ret);
                fflush(stdout);
                return HI_FAILURE;
            }

            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack)
            {
                printf("malloc memory err!\n");
                return HI_FAILURE;
            }

            stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = HI_MPI_VENC_GetStream(SnapChn, &stStream, HI_TRUE);
            if (HI_SUCCESS != s32Ret)
            {
                printf("HI_MPI_VENC_GetStream:0x%x\n", s32Ret);
                fflush(stdout);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = SampleSaveJpegStream(pFile, &stStream);
            if (HI_SUCCESS != s32Ret)
            {
                printf("HI_MPI_VENC_GetStream:0x%x\n", s32Ret);
                fflush(stdout);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            s32Ret = HI_MPI_VENC_ReleaseStream(SnapChn, &stStream);
            if (s32Ret)
            {
                printf("HI_MPI_VENC_ReleaseStream:0x%x\n", s32Ret);
                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }

            free(stStream.pstPack);
            stStream.pstPack = NULL;
		}
	}
	
    return HI_SUCCESS;
}


/*****************************************************************************
                    snap by mode 1
 how to snap:
 1)only creat one snap group
 2)bind to a vichn to snap and then unbind
 3)repeat 2) to snap all vichn in turn

 features:
 1)save memory, because only one snap group and snap channel
 2)efficiency lower than mode 2, pictures snapped will not more than 8.
*****************************************************************************/
HI_VOID* thread_StartSnapByMode1(HI_VOID *p)
{
    HI_S32 s32Ret;
    VENC_GRP VeGroup = 0;
    VENC_CHN SnapChn = 0;
    VI_DEV ViDev = 0;
    VI_CHN ViChn = 0;
    FILE *pFile = NULL;
    HI_S32 s32SnapCnt;
    HI_S32 s32SnapTotal;
    SNAP_MULTI_CHN_S *pstSnapMultiChn = NULL;
    HI_S32 s32ViChnTotal;

    pstSnapMultiChn = (SNAP_MULTI_CHN_S*)p;
    VeGroup = pstSnapMultiChn->SnapGroup;
    SnapChn = pstSnapMultiChn->SnapChn;
    ViDev = pstSnapMultiChn->ViDev;
    s32ViChnTotal = pstSnapMultiChn->s32ViChnCnt;
    s32SnapTotal = pstSnapMultiChn->s32SnapTotal;
    
    /* snap all vi channels in sequence. 
      * step 1: bind vi
      * step 2: register snap channel to group.
      * step 3: start snap channel to receiver picture.
      * step 4: get one-picture stream and save as jpeg file.
      * step 5: undo step 3, step 2, step 1.
      * step 6: chose next vi to snap, then go to step 1
      */
    ViChn = 0;
    while (HI_TRUE == pstSnapMultiChn->bThreadStart)
    {
        s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_BindInput err 0x%x\n", s32Ret);
            return NULL;
        }

        s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_RegisterChn err 0x%x\n", s32Ret);
            return NULL;
        }

        s32Ret = HI_MPI_VENC_StartRecvPic(SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_StartRecvPic err 0x%x\n", s32Ret);
            return NULL;
        }

        {
            /*save jpeg picture*/
            char acFile[128]  = {0};   
            //struct timeval timenow;            
            //gettimeofday(&timenow, NULL);
            sprintf(acFile, "Vichn%d_num%d.jpg", ViChn, s32SnapCnt);
            pFile = fopen(acFile, "wb");
            if (pFile == NULL)
            {
                printf("open file err\n");
                return NULL;
            }

            s32Ret = SAMPLE_GetSnapPic(SnapChn, pFile);
            if (s32Ret != HI_SUCCESS)
            {
                printf("SAMPLE_GetSnapPic err 0x%x\n", s32Ret);
                fclose(pFile);
                return NULL;
            }
            
            fclose(pFile);
        }
        
        s32Ret = HI_MPI_VENC_StopRecvPic(SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_StopRecvPic err 0x%x\n", s32Ret);
            return NULL;
        }

        s32Ret = HI_MPI_VENC_UnRegisterChn(SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n", s32Ret);
            return NULL;
        }

        s32Ret = HI_MPI_VENC_UnbindInput(VeGroup);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_UnbindInput err 0x%x\n", s32Ret);
            return NULL;
        }

        /* continue to snap next vi channel */
        ViChn++;
        if(ViChn >= s32ViChnTotal)
        {
            ViChn = 0;
            s32SnapCnt++;
        }
    }

    return NULL;
}

/*****************************************************************************
                    snap by mode 2
 how to snap:
 1)create snap group for each vichn,and bind them (not unbind until program end),
   that is muti snap group exit, instead of only one
 2)create one snap channel for each snap group
 3)register snap chn to its corresponding group, snapping, and then unregister
 4)repeat 3) to snap muti pictures of the channel

 features:
 1)need more memory than mode 1, because muti snap group and snap channel exit
 2)higher efficiency, because all snap chn run simultaneity.
*****************************************************************************/
HI_VOID* thread_StartSnapByMode2(HI_VOID *p)
{
    HI_S32 s32Ret;
    VENC_GRP VeGroup;
    VENC_CHN SnapChn;
    VI_DEV ViDev;
    VI_CHN ViChn;
    FILE *pFile = NULL;
    HI_S32 s32SnapCnt;
    HI_S32 s32SnapTotal;
    SNAP_SINGLE_CHN_S *pstSnapSingleChn = NULL;

    pstSnapSingleChn = (SNAP_SINGLE_CHN_S*)p;
    VeGroup = pstSnapSingleChn->VeGroup;
    SnapChn = pstSnapSingleChn->SnapChn;
    ViDev = pstSnapSingleChn->ViDev;
    ViChn = pstSnapSingleChn->ViChn;
    s32SnapTotal = pstSnapSingleChn->s32SnapTotal;
    
    /*note: bind snap group to vichn, not unbind while snapping*/
    s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_BindInput err 0x%x\n", s32Ret);
        return NULL;
    }

    s32SnapCnt = 0;
    while (HI_TRUE == pstSnapSingleChn->bThreadStart)
    {
        s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_RegisterChn err 0x%x\n", s32Ret);
            return NULL;
        }

        s32Ret = HI_MPI_VENC_StartRecvPic(SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_StartRecvPic err 0x%x\n", s32Ret);
            return NULL;
        }

        {
            /*save jpeg picture*/
            char acFile[128]  = {0};
            //struct timeval timenow;
            sprintf(acFile, "Vichn%d_num%d.jpg", ViChn, s32SnapCnt);
            pFile = fopen(acFile, "wb");
            if (pFile == NULL)
            {
                printf("open file err\n");
                return NULL;
            }

            s32Ret = SAMPLE_GetSnapPic(SnapChn, pFile);
            if (s32Ret != HI_SUCCESS)
            {
                printf("SAMPLE_GetSnapPic err 0x%x\n", s32Ret);
                fclose(pFile);
                return NULL;
            }
            fclose(pFile);
        }
        
        s32Ret = HI_MPI_VENC_StopRecvPic(SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_StopRecvPic err 0x%x\n", s32Ret);
            return NULL;
        }

        s32Ret = HI_MPI_VENC_UnRegisterChn(SnapChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n", s32Ret);
            return NULL;
        }

        s32SnapCnt++;
    }

    s32Ret = HI_MPI_VENC_UnbindInput(VeGroup);
    if (s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_VENC_UnbindInput err 0x%x\n", s32Ret);
        return NULL;
    }

    return NULL;
}

HI_S32 SampleVencClipD1to4Cif()
{
    HI_S32 s32Ret, i;
    VENC_GRP VeGrp;
    VIDEO_PREPROC_CONF_S stVppCfg;
    
    for (i=0; i<4; i++)
    {
        VeGrp = i;
        s32Ret = HI_MPI_VPP_GetConf(VeGrp, &stVppCfg);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VPP_GetConf err 0x%x\n", s32Ret);
            return HI_FAILURE;
        }

        if (0==i || 1==i)
        {
            stVppCfg.stClipAttr[0].u32ClipMode  = VIDEO_FIELD_TOP;
        }
        else
        {
            stVppCfg.stClipAttr[0].u32ClipMode  = VIDEO_FIELD_BOTTOM;
        } 
        
        if (i & 1)
        {
            stVppCfg.stClipAttr[0].stClipRect.s32X = 0;
            stVppCfg.stClipAttr[0].stClipRect.s32Y = 0;
            stVppCfg.stClipAttr[0].stClipRect.u32Width  = 352;
            stVppCfg.stClipAttr[0].stClipRect.u32Height = 288;
        }
        else
        {
            stVppCfg.stClipAttr[0].stClipRect.s32X = 352;
            stVppCfg.stClipAttr[0].stClipRect.s32Y = 0;
            stVppCfg.stClipAttr[0].stClipRect.u32Width  = 352;
            stVppCfg.stClipAttr[0].stClipRect.u32Height = 288;
        }
        stVppCfg.stClipAttr[0].u32SrcWidth = 704;
        stVppCfg.stClipAttr[0].u32SrcHeight = 576; 
        
        s32Ret = HI_MPI_VPP_SetConf(VeGrp, &stVppCfg);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VPP_SetConf err 0x%x\n", s32Ret);
            return HI_FAILURE;
        }
        printf("set group %d clip ok, rect:(%d,%d,%d,%d) \n",VeGrp,
            stVppCfg.stClipAttr[0].stClipRect.s32X,stVppCfg.stClipAttr[0].stClipRect.s32Y,
            stVppCfg.stClipAttr[0].stClipRect.u32Width,stVppCfg.stClipAttr[0].stClipRect.u32Height);
    }

    return HI_SUCCESS;
}

HI_S32 SampleViVoClipD1to4Cif_SD(VI_DEV ViDev, VI_CHN ViChn, VO_DEV VoDev)
{
    HI_S32 s32Ret, i;

    SAMPLE_StartVi_SD(4, PIC_D1);
    SAMPLE_StartVo_SD(4, VoDev);
    
    for(i=0; i<4; i++)
    {
        VO_ZOOM_ATTR_S stZoom;
        stZoom.enField = (i <= 1) ? VIDEO_FIELD_TOP : VIDEO_FIELD_BOTTOM;
        stZoom.stZoomRect.s32X = ( i & 1) ? 0 : 352;
        stZoom.stZoomRect.s32Y = 0;
        stZoom.stZoomRect.u32Height = 288;
        stZoom.stZoomRect.u32Width  = 352;

        s32Ret = HI_MPI_VO_SetZoomInWindow(VoDev, i, &stZoom);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VO_SetZoomInWindow err 0x%x\n", s32Ret);
            return HI_FAILURE;
        }

        s32Ret = HI_MPI_VI_BindOutput(ViDev, ViChn, VoDev, i);
        if (s32Ret != HI_SUCCESS)
        {
            printf("HI_MPI_VI_BindOutput err 0x%x\n", s32Ret);
            return HI_FAILURE;
        }

    }

    return HI_SUCCESS;
}

SNAP_MULTI_CHN_S s_stSnapMultiChn;

HI_S32 SAMPLE_StartSnapThread(SNAP_MULTI_CHN_S *pstMultiSnap)
{
    memcpy(&s_stSnapMultiChn, pstMultiSnap, sizeof(SNAP_MULTI_CHN_S));
    
    s_stSnapMultiChn.bThreadStart = HI_TRUE;
    pthread_create(&s_stSnapMultiChn.pid, 0, thread_StartSnapByMode1, (HI_VOID*)&s_stSnapMultiChn);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_StopSnapThread()
{
    if (HI_TRUE == s_stSnapMultiChn.bThreadStart)
    {
        s_stSnapMultiChn.bThreadStart = HI_FALSE;
        pthread_join(s_stSnapMultiChn.pid, 0);
    }
    return HI_SUCCESS;
}


static SNAP_SINGLE_CHN_S s_stSnapSingleChn[VENC_MAX_CHN_NUM];

HI_S32 SAMPLE_StartSnapThread2(HI_S32 s32Idx, SNAP_SINGLE_CHN_S *pstSingleSnap)
{
    memcpy(&s_stSnapSingleChn[s32Idx], pstSingleSnap, sizeof(SNAP_SINGLE_CHN_S));
    
    s_stSnapSingleChn[s32Idx].bThreadStart = HI_TRUE;
    pthread_create(&s_stSnapSingleChn[s32Idx].pid, 0, thread_StartSnapByMode2, (HI_VOID*)&s_stSnapSingleChn[s32Idx]);

    return HI_SUCCESS;
}

HI_S32 SAMPLE_StopSnapThread2(HI_S32 s32Idx)
{
    if (HI_TRUE == s_stSnapSingleChn[s32Idx].bThreadStart)
    {
        s_stSnapSingleChn[s32Idx].bThreadStart = HI_FALSE;
        pthread_join(s_stSnapSingleChn[s32Idx].pid, 0);
    }
    return HI_SUCCESS;
}

HI_S32 SAMPLE_1D1H264(HI_VOID)
{
    HI_S32 s32Ret;
    VO_DEV VoDev = G_VODEV;    
    HI_S32 s32ChnTotal = 1;
    VB_CONF_S stVbConf = {0};
    GET_STREAM_S stGetStream;
    PIC_SIZE_E aenSize[2];
    PAYLOAD_TYPE_E aenType[2];
    
    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt  = 12;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = SAMPLE_StartViVo_SD(s32ChnTotal, PIC_D1, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    aenSize[0] = PIC_D1;
    aenType[0] = PT_H264;
    s32Ret = SAMPLE_StartVenc(s32ChnTotal, HI_FALSE, aenType, aenSize);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    stGetStream.enPayload = PT_H264;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = s32ChnTotal;
    SAMPLE_StartVencGetStream(&stGetStream);

    printf("press twice ENTER to exit\n");
    getchar();
    getchar();    
    
    SAMPLE_StopVencGetStream();

    SAMPLE_StopVenc(s32ChnTotal, HI_FALSE);

    SAMPLE_ExitMPP();
    return HI_SUCCESS;
}

HI_S32 SAMPLE_1D1Mjpeg(HI_VOID)
{
    HI_S32 s32Ret;
    VO_DEV VoDev = G_VODEV;        
    VB_CONF_S stVbConf = {0};
    HI_S32 s32ChnTotal = 1;
    GET_STREAM_S stGetStream;
    PIC_SIZE_E aenSize[2];
    PAYLOAD_TYPE_E aenType[2];
    
    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt  = 8;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = SAMPLE_StartViVo_SD(s32ChnTotal, PIC_D1, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    aenSize[0] = PIC_D1;
    aenType[0] = PT_MJPEG;
    s32Ret = SAMPLE_StartVenc(s32ChnTotal, HI_FALSE, aenType, aenSize);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    stGetStream.enPayload = PT_MJPEG;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = s32ChnTotal;
    SAMPLE_StartVencGetStream(&stGetStream);

    printf("press twice ENTER to exit\n");
    getchar();
    getchar();    
    
    SAMPLE_StopVencGetStream();
    
    SAMPLE_StopVenc(s32ChnTotal, HI_FALSE);

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}


HI_S32 SAMPLE_VENC_16CifH264(HI_VOID)
{
    HI_S32 s32Ret;
    GET_STREAM_S stGetStream;
    VO_DEV VoDev = G_VODEV; 
    VB_CONF_S stVbConf = {0};
    HI_S32 s32ChnTotal = 16; 
    HI_BOOL bHaveMinor = HI_TRUE;
    PIC_SIZE_E aenSize[2] = {PIC_CIF, PIC_QCIF};
    PAYLOAD_TYPE_E aenType[2] = {PT_H264, PT_H264};

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt  = 4;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 288 * 2;
    stVbConf.astCommPool[1].u32BlkCnt  = 8 * s32ChnTotal;
    /* init video buffer and mpp sys */
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    /* init vi and vo */
    s32Ret = SAMPLE_StartViVo_SD(s32ChnTotal, PIC_CIF, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    /* init group and venc */
    s32Ret = SAMPLE_StartVenc(s32ChnTotal, bHaveMinor, aenType, aenSize);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    stGetStream.enPayload = PT_H264;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = (bHaveMinor)?(s32ChnTotal*2):s32ChnTotal;
    SAMPLE_StartVencGetStream(&stGetStream);

    printf("press twice ENTER to exit\n");
    getchar();
    getchar();    
    
    SAMPLE_StopVencGetStream();

    SAMPLE_StopVenc(s32ChnTotal, bHaveMinor);

    SAMPLE_ExitMPP();
    return HI_SUCCESS;
}
HI_S32 SAMPLE_VENC_4D14CifH264(HI_VOID)
{
    HI_S32 s32Ret;
    GET_STREAM_S stGetStream;
    VO_DEV VoDev = G_VODEV; 
    VB_CONF_S stVbConf = {0};
    HI_S32 s32ChnTotal = 4; 
    HI_BOOL bHaveMinor = HI_TRUE;
    PIC_SIZE_E aenSize[2] = {PIC_D1, PIC_CIF};
    PAYLOAD_TYPE_E aenType[2] = {PT_H264, PT_H264};

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt  = 8 * s32ChnTotal;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 288 * 2;
    stVbConf.astCommPool[1].u32BlkCnt  = 8 * s32ChnTotal;
    /* init video buffer and mpp sys */
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    /* init vi and vo */
    s32Ret = SAMPLE_StartViVo_SD(s32ChnTotal, aenSize[0], VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    /* init group and venc */
    s32Ret = SAMPLE_StartVenc(s32ChnTotal, bHaveMinor, aenType, aenSize);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    stGetStream.enPayload = PT_H264;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = (bHaveMinor)?(s32ChnTotal*2):s32ChnTotal;
    SAMPLE_StartVencGetStream(&stGetStream);

    printf("press twice ENTER to exit\n");
    getchar();
    getchar();    
    
    SAMPLE_StopVencGetStream();

    SAMPLE_StopVenc(s32ChnTotal, bHaveMinor);

    SAMPLE_ExitMPP();
    return HI_SUCCESS;
}


HI_S32 SAMPLE_VENC_JpegSnap()
{
    VO_DEV VoDev = G_VODEV;    
    VB_CONF_S stVbConf = {0};    
    SNAP_MULTI_CHN_S stSnapMultiChn;
    VENC_GRP SnapGrp = 0;
    VENC_CHN SnapChn = 0;
    HI_S32 s32ViChnCnt = 4;

    /* config system*/
    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 30;
    stVbConf.astCommPool[1].u32BlkSize = 352 * 288 * 2;
    stVbConf.astCommPool[1].u32BlkCnt = 40;
    if (HI_SUCCESS != SAMPLE_InitMPP(&stVbConf))
    {
        return HI_FAILURE;
    }

    if (HI_SUCCESS != SAMPLE_StartViVo_SD(s32ViChnCnt, PIC_D1, VoDev))
    {
        return HI_FAILURE;
    }

    if (SAMPLE_CreateJpegChn(SnapGrp, SnapChn, PIC_D1) != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    /* creat 1 thread to snap 4 vichn's image */
    stSnapMultiChn.SnapGroup    = SnapGrp;
    stSnapMultiChn.SnapChn      = SnapChn;
    stSnapMultiChn.ViDev        = 0;
    stSnapMultiChn.s32ViChnCnt  = s32ViChnCnt;
    SAMPLE_StartSnapThread(&stSnapMultiChn);

    printf("start snap picture circularly for %d vi chn \n ", s32ViChnCnt);
    
    printf("press twice ENTER to exit\n");
    getchar();
    getchar();    

    SAMPLE_StopSnapThread();

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

/* snap mode 2 : 4 snap chn D1*/
HI_S32 SAMPLE_VENC_JpegSnap2(HI_VOID)
{    
    VO_DEV VoDev = G_VODEV;    
    HI_S32 s32ChnTotal = 4;
    SNAP_SINGLE_CHN_S astSnapSingleChn[s32ChnTotal];
    VB_CONF_S stVbConf = {0};
    HI_S32 i = 0;

    /*config system*/
    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 48;
    stVbConf.astCommPool[1].u32BlkSize = 352 * 288 * 2;
    stVbConf.astCommPool[1].u32BlkCnt = 48;
    if (HI_SUCCESS != SAMPLE_InitMPP(&stVbConf))
    {
        printf("SAMPLE_InitMPP fail\n");
        return HI_FAILURE;
    }

    if (HI_SUCCESS != SAMPLE_StartViVo_SD(s32ChnTotal, PIC_D1, VoDev))
    {
        printf("SAMPLE_StartViVo_SD fail\n");
        return HI_FAILURE;
    }

    for (i = 0; i < s32ChnTotal; i++)
    {
        VENC_GRP SnapGrp = i;
        VENC_CHN SnapChn = i;
        if (SAMPLE_CreateJpegChn(SnapGrp, SnapChn, PIC_D1))
        {
            return HI_FAILURE;
        }
        
        astSnapSingleChn[i].VeGroup = SnapGrp;
        astSnapSingleChn[i].SnapChn = SnapChn;
        astSnapSingleChn[i].ViDev = i / VIU_MAX_CHN_NUM_PER_DEV;
        astSnapSingleChn[i].ViChn = i % VIU_MAX_CHN_NUM_PER_DEV;
        SAMPLE_StartSnapThread2(i, &astSnapSingleChn[i]);
    }

    printf("press twice ENTER to exit\n");
    getchar();
    getchar();   
        
    for (i = 0; i < s32ChnTotal; i++)
    {  
        SAMPLE_StopSnapThread2(i);
    }
    
    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VencClipD1to4Cif(HI_VOID)
{
    HI_S32 s32Ret, i;
    VO_DEV VoDev = G_VODEV;    
    HI_S32 s32ChnTotal = 1;
    VB_CONF_S stVbConf = {0};
    GET_STREAM_S stGetStream;
    VI_DEV ViDev = 0;
    VI_CHN ViChn = 0;
    VENC_GRP VeGrp;
    
    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt  = 12;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = SampleViVoClipD1to4Cif_SD(ViDev, ViChn, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return s32Ret;
    }
    
    
    /* create group and venc, four group bind one vi */
    for (i=0; i<4; i++)
    {        
        VeGrp = i;        
        s32Ret = SAMPLE_StartOneVenc(VeGrp, ViDev, ViChn, PT_H264, PIC_CIF, 25);
        if (HI_SUCCESS != s32Ret)
        {
            return s32Ret;
        }
    }

    /* clip one D1 to 4 CIF */
    SampleVencClipD1to4Cif();
    
    stGetStream.enPayload = PT_H264;
    stGetStream.VeChnStart = 0;
    stGetStream.s32ChnTotal = 4;
    SAMPLE_StartVencGetStream(&stGetStream);

    printf("press twice ENTER to exit\n");
    getchar();
    getchar();    
    
    SAMPLE_StopVencGetStream();

    SAMPLE_ExitMPP();
    return HI_SUCCESS;
}

HI_S32 main(int argc, char *argv[])
{
    char ch;
    char ping[1024] = "\n/************************************/\n"
                      "1  --> SAMPLE_1D1H264\n"
                      "2  --> SAMPLE_1D1Mjpeg\n"
                      "3  --> SAMPLE_VENC_16CifH264\n"
                      "4  --> SAMPLE_VENC_4D1H264\n"
                      "5  --> SAMPLE_VENC_JpegSnap\n"
                      "6  --> SAMPLE_VENC_JpegSnap2\n"
                      "8  --> SAMPLE_VencClipD1to4Cif\n"
                      "q  --> quit\n"
                      "please enter order:\n";

    /* press Ctrl-C to quit. */
    signal(SIGINT, HandleSig);

    printf("%s", ping);
    while ((ch = getchar()) != 'q')
    {
        if ('\n' == ch)
        {
            continue;
        }

        switch (ch)
        {
            case '1':
            {
                SAMPLE_1D1H264();
                break;
            }
            case '2' :
            {
                SAMPLE_1D1Mjpeg();
                break;
            }
            case '3':
            {
                #ifdef hi3515
                printf("hi3515 demo board not support SAMPLE_VENC_16CifH264");
                #else
                SAMPLE_VENC_16CifH264();
                #endif
                break;
            }
            case '4':
            {
                SAMPLE_VENC_4D14CifH264();
                break;
            }
            case '5':
            {
                SAMPLE_VENC_JpegSnap();
                break;
            }
            case '6':
            {
                SAMPLE_VENC_JpegSnap2();
                break;
            }
            case '8':
            {
                SAMPLE_VencClipD1to4Cif();
                break;
            }
            default:
            {
                printf("no order@@@@@!\n");
                break;
            }
                
        }
        printf("%s", ping);
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */
