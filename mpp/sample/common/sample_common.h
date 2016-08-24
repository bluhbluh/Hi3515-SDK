/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_common_new.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/07
  Description   : sample_common.c header file
  History       :
  1.Date        : 2009/07/07
    Author      : Hi3520MPP
    Modification: Created file

******************************************************************************/

#ifndef __SAMPLE_COMMON_H__
#define __SAMPLE_COMMON_H__

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vpp.h"
#include "hi_comm_md.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vdec.h"
#include "mpi_vpp.h"
#include "mpi_md.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_aenc.h"
#include "mpi_adec.h"

#include "tw2864/tw2864.h"
#include "tw2865/tw2865.h"
#include "adv7441/adv7441.h"
#include "mt9d131/mt9d131.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


/* RGB format is 1888. */
#define VO_BKGRD_RED      0xFF0000    /* red back groud color */
#define VO_BKGRD_GREEN    0x00FF00    /* green back groud color */
#define VO_BKGRD_BLUE     0x0000FF    /* blue back groud color */
#define VO_BKGRD_BLACK    0x000000    /* black back groud color */

#define CHECK_MPI_CALL(ret, mpi_name) \
do\
{\
    if (HI_SUCCESS != ret)\
    {\
        printf("%s fail! return 0x%08x\n", mpi_name, ret);\
        return HI_FAILURE;\
    }\
}while(0)

#define RET_IF_FAIL(ret) \
do\
{\
    if (HI_SUCCESS != ret)\
    {\
        return HI_FAILURE;\
    }\
}while(0)

/* vou device enumeration */
typedef enum hiVO_DEV_E
{
    VO_DEV_HD  = 0,                 /* high definition device */
    VO_DEV_AD  = 1,                 /* assistant device */
    VO_DEV_SD  = 2,                 /* spot device */
    VO_DEV_BUTT
} VO_DEV_E;

extern HI_S32 SAMPLE_ADV7441_CfgV(ADV7441_VIDEO_FORMAT_E enFormat);
extern HI_S32 SAMPLE_MT9D131_CfgV(HI_S32 val);
extern HI_S32 SAMPLE_TW2864_CfgV(VI_WORK_MODE_E enViWorkMode);
extern HI_S32 SAMPLE_TW2865_CfgV(VIDEO_NORM_E enVideoMode,VI_WORK_MODE_E enWorkMode);
extern void SAMPLE_StartVLossDet(int chn_cnt);
extern void SAMPLE_StopVLossDet();


extern HI_S32 SAMPLE_InitMPP(VB_CONF_S *pstVbConf);
extern HI_S32 SAMPLE_ExitMPP(HI_VOID);

/* level 1 capsulation: base method of VI */
extern HI_S32 SAMPLE_GetViChnPerDev(VI_DEV ViDev);
extern HI_S32 SAMPLE_StartViByChn(HI_S32 s32ChnTotal, VI_PUB_ATTR_S* pstViDevAttr, VI_CHN_ATTR_S* pstViChnAttr);
extern HI_S32 SAMPLE_StartViByDev(HI_S32 s32ChnTotal, VI_DEV ViDev, VI_PUB_ATTR_S* pstViDevAttr, VI_CHN_ATTR_S* pstViChnAttr);
extern HI_S32 SAMPLE_StopViByChn(HI_S32 s32ChnTotal);
extern HI_S32 SAMPLE_StopViByDev(HI_S32 s32ChnTotal, VI_DEV ViDev);

extern HI_S32 SAMPLE_StartViDev(VI_INPUT_MODE_E enInputMode);
extern HI_S32 SAMPLE_StartViChn(VI_DEV ViDev, VI_CHN ViChn, PIC_SIZE_E enPicSize);
extern HI_S32 SAMPLE_StopViChn(VI_DEV ViDev, VI_CHN ViChn);

/*  level 1 capsulation: base method of VO */
extern HI_S32 SAMPLE_SetVoChnMScreen(VO_DEV VoDev, HI_U32 u32ChnCnt, HI_U32 u32Width, HI_U32 u32Height);
extern HI_S32 SampleStartVoDevice(VO_DEV VoDev, VO_PUB_ATTR_S* pstDevAttr);
extern HI_S32 SAMPLE_StartVoVideoLayer(VO_DEV VoDev, VO_VIDEO_LAYER_ATTR_S* pstVideoLayerAttr);
extern HI_S32 SAMPLE_StartVo(HI_S32 s32ChnTotal, VO_DEV VoDev, VO_PUB_ATTR_S* pstVoDevAttr, VO_VIDEO_LAYER_ATTR_S* pstVideoLayerAttr);
extern HI_S32 SAMPLE_StopVo( HI_S32 s32ChnTotal, VO_DEV VoDev);
extern HI_S32 SAMPLE_VoPicSwitch(VO_DEV VoDev, HI_U32 u32VoPicDiv);


/* levle 2 capsulation: special VI/VO method */
extern HI_S32 SAMPLE_StartVi_SD(HI_S32 s32ViChnTotal, PIC_SIZE_E enPicSize);
extern HI_S32 SAMPLE_StartVo_SD(HI_S32 s32ChnTotal, VO_DEV VoDev);

/* level 3 capsulation: preview. */
extern HI_S32 SAMPLE_ViBindVo(HI_S32 s32ViChnTotal, VO_DEV VoDev);
HI_S32 SAMPLE_StartViVo_SD(HI_S32 s32ChnTotal, PIC_SIZE_E enViSize, VO_DEV VoDev);
HI_S32 SAMPLE_StopViVo_SD(HI_S32 s32ChnTotal, VO_DEV VoDev);

/* */
extern HI_S32 SAMPLE_StopAllVi(HI_VOID);
extern HI_S32 SAMPLE_StopAllVo(HI_VOID);

/***************************** venc ************************************/
typedef struct hiGET_STREAM_S
{
    HI_BOOL         bThreadStart;
    pthread_t       pid;
    PAYLOAD_TYPE_E enPayload;   /* ven channel is H.264? MJPEG? or JPEG? */
    VENC_CHN VeChnStart;        /* From this channel to get stream. */
    HI_S32 s32ChnTotal;         /* how many channels to get stream from. 
                                         * channel index is VeChnStart, VeChnStart+1, ..., VeChnStart+(s32ChnTotal-1).
                                         * Used for SampleGetVencStreamProc. */
    HI_S32 s32SaveTotal;        /* how many frames will be get each channel. */
                                /* Note: JPEG snap, it must be 1. */
    HI_U8 aszFileNoPostfix[32]; /* complete file name will add postfix automaticly: (not finish yet)
                                         * _chnX.h264.  -- for h.264
                                         * _chnX.mjp.   -- for mjpeg
                                         * _chnX_Y.jpg  -- for jpeg
                                         * */
} GET_STREAM_S;


HI_S32 SAMPLE_StartVenc(HI_U32 u32GrpCnt, HI_BOOL bHaveMinor, 
                                PAYLOAD_TYPE_E aenType[2], PIC_SIZE_E aenSize[2]);
HI_S32 SAMPLE_StopVenc(HI_U32 u32GrpCnt, HI_BOOL bHaveMinor);

HI_S32 SAMPLE_StartOneVenc(VENC_GRP VeGrp, VI_DEV ViDev, VI_CHN ViChn,
    PAYLOAD_TYPE_E enType, PIC_SIZE_E enSize, HI_S32 s32FrmRate);

HI_S32 SampleSaveH264Stream(FILE* fpH264File, VENC_STREAM_S *pstStream);
HI_S32 SampleSaveJpegStream(FILE* fpJpegFile, VENC_STREAM_S *pstStream);
HI_VOID* SampleGetVencStreamProc(HI_VOID *p);
HI_S32 SAMPLE_StartVencGetStream(GET_STREAM_S *pstGetVeStream);
HI_S32 SAMPLE_StopVencGetStream();
HI_S32 SAMPLE_CreateJpegChn(VENC_GRP VeGroup, VENC_CHN SnapChn, PIC_SIZE_E enPicSize);


HI_VOID HandleSig(HI_S32 signo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __SAMPLE_COMMON_H__ */
