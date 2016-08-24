/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_vio.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/09
  Description   : this sample demo the vi to vo live display
  History       :
  1.Date        : 2009/07/09
    Author      : Hi3520MPP
    Modification: Created file
  2.Date        : 2010/02/12
    Author      : Hi3520MPP
    Modification: Add video loss detect demo 
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

typedef enum hiSAMPLE_VO_DIV_MODE
{
    DIV_MODE_1  = 1,    /* 1-screen display */
    DIV_MODE_4  = 4,    /* 4-screen display */
    DIV_MODE_9  = 9,    /* 9-screen display */
    DIV_MODE_16 = 16,   /* 16-screen display */
    DIV_MODE_BUTT
} SAMPLE_VO_DIV_MODE;


#define BT656_WORKMODE VI_WORK_MODE_4D1

/***********************************************************************************
 * User may want to display video on TV, CRT, LCD or other facilities
 * when they run some samples. So, user can chose display vo device by macro G_VODEV.
 *
 * For example:
 *
 * display_facility   G_VODEV
 *  PAL  TV           VO_DEV_SD
 *  NTSC TV           VO_DEV_SD
 *  computer CRT      VO_DEV_HD
 *  HD TV             VO_DEV_HD
 *
 * Have fun.
 *
 * Note: some vio samples are not support user to do this.
 *
 */
#define G_VODEV VO_DEV_SD

/* For PAL or NTSC input and output. */
extern VIDEO_NORM_E gs_enViNorm;
extern VO_INTF_SYNC_E gs_enSDTvMode;


HI_VOID SAMPLE_ReadFrame(FILE * fp, HI_U8 * pY, HI_U8 * pU, HI_U8 * pV,
                                              HI_U32 width, HI_U32 height, HI_U32 stride, HI_U32 stride2)
{
    HI_U8 * pDst;

    HI_U32 u32Row;

    pDst = pY;
    for ( u32Row = 0; u32Row < height; u32Row++ )
    {
        fread( pDst, width, 1, fp );
        pDst += stride;
    }
    
    pDst = pU;
    for ( u32Row = 0; u32Row < height/2; u32Row++ )
    {
        fread( pDst, width/2, 1, fp );
        pDst += stride2;
    }
    
    pDst = pV;
    for ( u32Row = 0; u32Row < height/2; u32Row++ )
    {
        fread( pDst, width/2, 1, fp );
        pDst += stride2;
    }
}

HI_S32 SAMPLE_PlanToSemi(HI_U8 *pY, HI_S32 yStride, 
                       HI_U8 *pU, HI_S32 uStride,
                       HI_U8 *pV, HI_S32 vStride, 
                       HI_S32 picWidth, HI_S32 picHeight)
{
    HI_S32 i;
    HI_U8* pTmpU, *ptu;
    HI_U8* pTmpV, *ptv;
    HI_S32 s32HafW = uStride >>1 ;
    HI_S32 s32HafH = picHeight >>1 ;
    HI_S32 s32Size = s32HafW*s32HafH;
        
    pTmpU = malloc( s32Size ); ptu = pTmpU;
    pTmpV = malloc( s32Size ); ptv = pTmpV;
    
    memcpy(pTmpU,pU,s32Size);
    memcpy(pTmpV,pV,s32Size);
    
    for(i = 0;i<s32Size>>1;i++)
    {
        *pU++ = *pTmpV++;
        *pU++ = *pTmpU++;
        
    }
    for(i = 0;i<s32Size>>1;i++)
    {
        *pV++ = *pTmpV++;
        *pV++ = *pTmpU++;        
    }

    free( ptu );
    free( ptv );

    return HI_SUCCESS;
}

HI_S32 SAMPLE_GetVFrame_FromYUV(FILE *pYUVFile, 
    HI_U32 u32Width, HI_U32 u32Height,HI_U32 u32Stride, VIDEO_FRAME_INFO_S *pstVFrameInfo)
{
    HI_U32             u32LStride;
    HI_U32             u32CStride;
    HI_U32             u32LumaSize;
    HI_U32             u32ChrmSize;
    HI_U32             u32Size;
    VB_BLK VbBlk;
    HI_U32 u32PhyAddr;
    HI_U8 *pVirAddr;

    u32LStride  = u32Stride;
    u32CStride  = u32Stride;
    
    u32LumaSize = (u32LStride * u32Height);
    u32ChrmSize = (u32CStride * u32Height) >> 2;/* YUV 420 */
    u32Size = u32LumaSize + (u32ChrmSize << 1);

    /* alloc video buffer block ---------------------------------------------------------- */
    VbBlk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, u32Size);
    if (VB_INVALID_HANDLE == VbBlk)
    {
        printf("HI_MPI_VB_GetBlock err! size:%d\n",u32Size);
        return -1;
    }
    u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(VbBlk);
    if (0 == u32PhyAddr)
    {
        return -1;
    }
    pVirAddr = (HI_U8 *) HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
    if (NULL == pVirAddr)
    {
        return -1;
    }

    pstVFrameInfo->u32PoolId = HI_MPI_VB_Handle2PoolId(VbBlk);
    if (VB_INVALID_POOLID == pstVFrameInfo->u32PoolId)
    {
        return -1;
    }
    printf("pool id :%d, phyAddr:%x,virAddr:%x\n" ,pstVFrameInfo->u32PoolId,u32PhyAddr,(int)pVirAddr);
    
    pstVFrameInfo->stVFrame.u32PhyAddr[0] = u32PhyAddr;
    pstVFrameInfo->stVFrame.u32PhyAddr[1] = pstVFrameInfo->stVFrame.u32PhyAddr[0] + u32LumaSize;
    pstVFrameInfo->stVFrame.u32PhyAddr[2] = pstVFrameInfo->stVFrame.u32PhyAddr[1] + u32ChrmSize;

    pstVFrameInfo->stVFrame.pVirAddr[0] = pVirAddr;
    pstVFrameInfo->stVFrame.pVirAddr[1] = pstVFrameInfo->stVFrame.pVirAddr[0] + u32LumaSize;
    pstVFrameInfo->stVFrame.pVirAddr[2] = pstVFrameInfo->stVFrame.pVirAddr[1] + u32ChrmSize;

    pstVFrameInfo->stVFrame.u32Width  = u32Width;
    pstVFrameInfo->stVFrame.u32Height = u32Height;
    pstVFrameInfo->stVFrame.u32Stride[0] = u32LStride;
    pstVFrameInfo->stVFrame.u32Stride[1] = u32CStride;
    pstVFrameInfo->stVFrame.u32Stride[2] = u32CStride;  
    pstVFrameInfo->stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;        
    pstVFrameInfo->stVFrame.u32Field = VIDEO_FIELD_FRAME;  

    /* read Y U V data from file to the addr ----------------------------------------------*/
    SAMPLE_ReadFrame(pYUVFile, pstVFrameInfo->stVFrame.pVirAddr[0], 
                               pstVFrameInfo->stVFrame.pVirAddr[1], pstVFrameInfo->stVFrame.pVirAddr[2],
                               pstVFrameInfo->stVFrame.u32Width, pstVFrameInfo->stVFrame.u32Height, 
                               pstVFrameInfo->stVFrame.u32Stride[0], pstVFrameInfo->stVFrame.u32Stride[1] >> 1 );

    /* convert planar YUV420 to sem-planar YUV420 -----------------------------------------*/
    SAMPLE_PlanToSemi(pstVFrameInfo->stVFrame.pVirAddr[0], pstVFrameInfo->stVFrame.u32Stride[0],
                pstVFrameInfo->stVFrame.pVirAddr[1], pstVFrameInfo->stVFrame.u32Stride[1],
                pstVFrameInfo->stVFrame.pVirAddr[2], pstVFrameInfo->stVFrame.u32Stride[1],
                pstVFrameInfo->stVFrame.u32Width, pstVFrameInfo->stVFrame.u32Height);
    
    HI_MPI_SYS_Mmap(u32PhyAddr, u32Size);
    return 0;
}

HI_S32 SAMPLE_SetViUserPic(HI_CHAR *pszYuvFile, HI_U32 u32Width, HI_U32 u32Height, 
        HI_U32 u32Stride, VIDEO_FRAME_INFO_S *pstFrame)
{
    FILE *pfd;

    /* 打开YUV文件 */
    pfd = fopen(pszYuvFile, "rb");
    if (!pfd)
    {
        printf("open file -> %s fail \n", pszYuvFile);
        return -1;
    }

    /* 从YUV文件读取视频帧信息 (注意只支持planar 420) */
    if (SAMPLE_GetVFrame_FromYUV(pfd, u32Width, u32Height, u32Stride, pstFrame))
    {
        return -1;
    }
    fclose(pfd);

    if (HI_MPI_VI_SetUserPic(0, 0, pstFrame))
    {
        return -1;
    }

    printf("set vi user pic ok, yuvfile:%s\n", pszYuvFile);
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype       : SAMPLE_GetViCfg_SD
 Description     : vi configs to input standard-definition video
 Input           : enPicSize     **
                   pstViDevAttr  **
                   pstViChnAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
*****************************************************************************/
HI_VOID SAMPLE_GetViCfg_SD(PIC_SIZE_E     enPicSize,
                           VI_PUB_ATTR_S* pstViDevAttr,
                           VI_CHN_ATTR_S* pstViChnAttr)
{
    pstViDevAttr->enInputMode = VI_MODE_BT656;
    pstViDevAttr->enWorkMode = BT656_WORKMODE;
    pstViDevAttr->enViNorm = gs_enViNorm;

    pstViChnAttr->stCapRect.u32Width  = 704;
    pstViChnAttr->stCapRect.u32Height =
        (VIDEO_ENCODING_MODE_PAL == gs_enViNorm) ? 288 : 240;
    pstViChnAttr->stCapRect.s32X = 
        (704 == pstViChnAttr->stCapRect.u32Width) ? 8 : 0;
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

/*****************************************************************************
 Prototype       : SAMPLE_GetViCfg_HD_720P
 Description     : vi configs to input high-definition 720P video.
 Input           : pstViDevAttr  **
                   pstViChnAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetViCfg_HD_720P(VI_PUB_ATTR_S* pstViDevAttr,
                                VI_CHN_ATTR_S* pstViChnAttr)
{
    pstViDevAttr->enInputMode   = VI_MODE_BT1120_PROGRESSIVE;
    pstViDevAttr->bIsChromaChn  = HI_FALSE;
    pstViDevAttr->bChromaSwap   = HI_TRUE;

    pstViChnAttr->stCapRect.s32X = 0;
    pstViChnAttr->stCapRect.s32Y = 0;
    pstViChnAttr->stCapRect.u32Width  = 1280;
    pstViChnAttr->stCapRect.u32Height = 720;
    pstViChnAttr->enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstViChnAttr->bHighPri = HI_FALSE;
    pstViChnAttr->bChromaResample = HI_FALSE;
    pstViChnAttr->enCapSel   = VI_CAPSEL_BOTH;
    pstViChnAttr->bDownScale = HI_FALSE;

    return;
}

/*****************************************************************************
 Prototype       : SAMPLE_GetViCfg_HD_1080I
 Description     : vi configs to input high-definition 1080I video.
 Input           : pstViDevAttr  **
                   pstViChnAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetViCfg_HD_1080I(VI_PUB_ATTR_S* pstViDevAttr,
                                VI_CHN_ATTR_S* pstViChnAttr)
{
    pstViDevAttr->enInputMode   = VI_MODE_BT1120_INTERLACED;
    pstViDevAttr->bIsChromaChn  = HI_FALSE;
    pstViDevAttr->bChromaSwap   = HI_TRUE;

    pstViChnAttr->stCapRect.s32X = 0;
    pstViChnAttr->stCapRect.s32Y = 0;
    pstViChnAttr->stCapRect.u32Width  = 1920;
    pstViChnAttr->stCapRect.u32Height = 540;
    pstViChnAttr->enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstViChnAttr->bHighPri = HI_FALSE;
    pstViChnAttr->bChromaResample = HI_FALSE;
    pstViChnAttr->enCapSel   = VI_CAPSEL_BOTH;
    pstViChnAttr->bDownScale = HI_FALSE;

    return;
}

/*****************************************************************************
 Prototype       : SAMPLE_GetViCfg_DC_2Mpixel
 Description     : vi configs to input digital-camera 2.0M-pixels video.
 Input           : pstViDevAttr  **
                   pstViChnAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetViCfg_DC_2Mpixel(VI_PUB_ATTR_S* pstViDevAttr,
                                   VI_CHN_ATTR_S* pstViChnAttr)
{
    pstViDevAttr->enInputMode = VI_MODE_DIGITAL_CAMERA;

    pstViChnAttr->stCapRect.u32Width  = 1600;
    pstViChnAttr->stCapRect.u32Height = 1200;
    pstViChnAttr->stCapRect.s32X = 0;
    pstViChnAttr->stCapRect.s32Y = 0;
    pstViChnAttr->enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstViChnAttr->bHighPri = HI_FALSE;
    pstViChnAttr->bChromaResample = HI_FALSE;
    pstViChnAttr->enCapSel   = VI_CAPSEL_BOTH;
    pstViChnAttr->bDownScale = HI_FALSE;

    return;
}

/*********************************************************************************
 *
 * Table: vo configuration
 * -----------------------------------------------------------------------
 * device_id    device_name    interface_type   D/A      vo_interface_sync
 *
 *         2     VO_DEV_SD ----- VO_INTF_CVBS   (A) --|
 *                                                    |
 *         1     VO_DEV_AD --|-- VO_INTF_CVBS   (A) --|
 *                           |                        |
 *                           |-- VO_INTF_BT656  (D) --|
 *                           |                        |-- VO_OUTPUT_PAL
 *                           |                        |-- VO_OUTPUT_NTSC
 *                           |
 *                           |-- VO_INTF_VGA    (D) --|
 *                                                    |
 *         0     VO_DEV_HD --|-- VO_INTF_VGA    (D) --|
 *                           |-- VO_INTF_LCD    (A) --|
 *                           |                        |-- VO_OUTPUT_800x600_60
 *                           |                        |-- VO_OUTPUT_1024x768_60
 *                           |                        |-- VO_OUTPUT_1280x1024_60
 *                           |                        |-- VO_OUTPUT_1366x768_60
 *                           |                        |-- VO_OUTPUT_1440x900_60
 *                           |                        |-- VO_OUTPUT_USER
 *                           |
 *                           |-- VO_INTF_YPBPR  (D) --|
 *                           |-- VO_INTF_BT1120 (A) --|
 *                                                    |-- VO_OUTPUT_720P60
 *                                                    |-- VO_OUTPUT_1080I60
 *                                                    |-- VO_OUTPUT_1080P30
 *
 **********************************************************************************/

/*****************************************************************************
 Prototype       : SAMPLE_GetVoCfg_SD
 Description     : vo configs to output standard-definition cvbs video .
 Input           : pstVoDevAttr       **
                   pstVideoLayerAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetVoCfg_SD(VO_PUB_ATTR_S*         pstVoDevAttr,
                           VO_VIDEO_LAYER_ATTR_S* pstVideoLayerAttr)
{
    HI_U32 u32Width;
    HI_U32 u32Height;
    HI_U32 u32DisplayRate;

    pstVoDevAttr->u32BgColor = VO_BKGRD_BLUE;
    pstVoDevAttr->enIntfType = VO_INTF_CVBS;

    pstVoDevAttr->enIntfSync = gs_enSDTvMode;

    u32Width  = 720;
    u32Height = (VO_OUTPUT_PAL == gs_enSDTvMode) ? 576 : 480;
    u32DisplayRate = (VO_OUTPUT_PAL == gs_enSDTvMode) ? 25 : 30;
    pstVideoLayerAttr->stDispRect.s32X = 0;
    pstVideoLayerAttr->stDispRect.s32Y = 0;
    pstVideoLayerAttr->stDispRect.u32Width   = u32Width;
    pstVideoLayerAttr->stDispRect.u32Height  = u32Height;
    pstVideoLayerAttr->stImageSize.u32Width  = u32Width;
    pstVideoLayerAttr->stImageSize.u32Height = u32Height;
    pstVideoLayerAttr->u32DispFrmRt = u32DisplayRate;
    pstVideoLayerAttr->enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstVideoLayerAttr->s32PiPChn = VO_DEFAULT_CHN;

    return;
}

/*****************************************************************************
 Prototype       : SAMPLE_GetVoCfg_HD_720P
 Description     : vo configs to output high-definition 720p video.
 Input           : pstVoDevAttr       **
                   pstVideoLayerAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetVoCfg_HD_720P(VO_PUB_ATTR_S* pstVoDevAttr,
                                VO_VIDEO_LAYER_ATTR_S* pstVideoLayerAttr)
{
    /* set vo device attribute and start device. */
    pstVoDevAttr->u32BgColor = VO_BKGRD_BLUE;
    pstVoDevAttr->enIntfType = VO_INTF_YPBPR;
    pstVoDevAttr->enIntfSync = VO_OUTPUT_720P60;

    pstVideoLayerAttr->stDispRect.s32X = 0;
    pstVideoLayerAttr->stDispRect.s32Y = 0;
    pstVideoLayerAttr->stDispRect.u32Width   = 1280;
    pstVideoLayerAttr->stDispRect.u32Height  = 720;
    pstVideoLayerAttr->stImageSize.u32Width  = 1280;
    pstVideoLayerAttr->stImageSize.u32Height = 720;
    pstVideoLayerAttr->u32DispFrmRt = 60;
    pstVideoLayerAttr->enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstVideoLayerAttr->s32PiPChn = VO_DEFAULT_CHN;

    return;
}

/*****************************************************************************
 Prototype       : SAMPLE_GetVoCfg_VGA_800x600
 Description     : vo configs to output VGA video.
 Input           : pstVoDevAttr       **
                   pstVideoLayerAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetVoCfg_VGA_800x600(VO_PUB_ATTR_S*         pstVoDevAttr,
                                    VO_VIDEO_LAYER_ATTR_S* pstVideoLayerAttr)
{
    /* set vo device attribute and start device. */
    pstVoDevAttr->u32BgColor = VO_BKGRD_BLUE;
    pstVoDevAttr->enIntfType = VO_INTF_VGA;
    pstVoDevAttr->enIntfSync = VO_OUTPUT_800x600_60;

    pstVideoLayerAttr->stDispRect.s32X = 0;
    pstVideoLayerAttr->stDispRect.s32Y = 0;
    pstVideoLayerAttr->stDispRect.u32Width   = 800;
    pstVideoLayerAttr->stDispRect.u32Height  = 600;
    pstVideoLayerAttr->stImageSize.u32Width  = 720;
    pstVideoLayerAttr->stImageSize.u32Height = (VO_OUTPUT_PAL == gs_enSDTvMode) ? 576 : 480;
    pstVideoLayerAttr->u32DispFrmRt = (VO_OUTPUT_PAL == gs_enSDTvMode) ? 25 : 30;
    pstVideoLayerAttr->enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstVideoLayerAttr->s32PiPChn = VO_DEFAULT_CHN;

    return;
}

/*****************************************************************************
 Prototype       : SAMPLE_GetVoCfg_1280x1024_4Screen
 Description     : vo configs to output 1280*1024 on HD.
 Input           : pstVoDevAttr       **
                   pstVideoLayerAttr  **
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_VOID SAMPLE_GetVoCfg_1280x1024_4Screen(VO_PUB_ATTR_S*         pstVoDevAttr,
                                    VO_VIDEO_LAYER_ATTR_S* pstVideoLayerAttr)
{
    HI_U32 u32InPicHeight = 0;
    
    /* set vo device attribute and start device. */
    pstVoDevAttr->u32BgColor = VO_BKGRD_BLUE;
    pstVoDevAttr->enIntfType = VO_INTF_VGA;
    pstVoDevAttr->enIntfSync = VO_OUTPUT_1280x1024_60;

    pstVideoLayerAttr->stDispRect.s32X = 0;
    pstVideoLayerAttr->stDispRect.s32Y = 0;
    pstVideoLayerAttr->stDispRect.u32Width   = 1280;
    pstVideoLayerAttr->stDispRect.u32Height  = 1024;
    /*note: if input pictures of VOU are two field and showed on HD,
      image size are recommded to config as follows:
        image.width = dispRect.width
        image.height = (inputpic.height)* (screen num in vertical) */
    pstVideoLayerAttr->stImageSize.u32Width  = 1280;
    u32InPicHeight = (VO_OUTPUT_PAL == gs_enSDTvMode) ? 576 : 480;
    pstVideoLayerAttr->stImageSize.u32Height = u32InPicHeight*2;
    pstVideoLayerAttr->u32DispFrmRt = (VO_OUTPUT_PAL == gs_enSDTvMode) ? 25 : 30;
    pstVideoLayerAttr->enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstVideoLayerAttr->s32PiPChn = VO_DEFAULT_CHN;

    return;
}


/*****************************************************************************
 Prototype       : SAMPLE_VIO_1Screen_720P
 Description     : Vi capture 720P and display in 720P screen.
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :

*****************************************************************************/
HI_S32 SAMPLE_VIO_1Screen_720P()
{
    HI_S32 s32Ret;
    VI_DEV ViDev;
    VO_DEV VoDev;
    VB_CONF_S stVbConf = {0};
    HI_S32 s32ViChnTotal;
    HI_S32 s32VoChnTotal;
    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;
    VO_PUB_ATTR_S stVoDevAttr;
    VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr;

    stVbConf.astCommPool[0].u32BlkSize = 1280 * 720 * 2;/*720P*/
    stVbConf.astCommPool[0].u32BlkCnt = 6;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    /* start vi to input high-definition video. */
    ViDev = 2;
    s32ViChnTotal = 1;
    SAMPLE_GetViCfg_HD_720P(&stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByDev(s32ViChnTotal,ViDev,&stViDevAttr, &stViChnAttr);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    /* display high-definition video on vo HD divice. */
    VoDev = VO_DEV_HD;
    s32VoChnTotal = 1;
    SAMPLE_GetVoCfg_HD_720P(&stVoDevAttr, &stVideoLayerAttr);
    s32Ret = SAMPLE_StartVo(s32VoChnTotal, VoDev, &stVoDevAttr, &stVideoLayerAttr);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_BindOutput(ViDev, 0, VoDev, 0);
    if (0 != s32Ret)
    {
        return s32Ret;
    }
    printf("start VI to VO preview , input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();
    return 0;
}

HI_S32 SAMPLE_VIO_1Screen_VoVGA()
{
    VB_CONF_S stVbConf   = {0};
    HI_S32 s32ViChnTotal = 1;
    HI_S32 s32VoChnTotal = 1;
    VO_DEV VoDev;
    VO_CHN VoChn;
    VI_DEV ViDev;
    VI_CHN ViChn;
    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;
    VO_PUB_ATTR_S stVoDevAttr;
    VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr;
    HI_S32 s32Ret = HI_SUCCESS;

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt  = 20;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 288 * 2;/*CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 20;
    if (HI_SUCCESS != SAMPLE_InitMPP(&stVbConf))
    {
        return -1;
    }

    /* Config VI to input standard-definition video */
    ViDev = 0;
    SAMPLE_GetViCfg_SD(PIC_D1, &stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByChn(s32ViChnTotal, &stViDevAttr, &stViChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* display VGA video on vo HD divice. */
    VoDev = VO_DEV_HD;
    SAMPLE_GetVoCfg_VGA_800x600(&stVoDevAttr, &stVideoLayerAttr);
    s32Ret = SAMPLE_StartVo(s32VoChnTotal, VoDev, &stVoDevAttr, &stVideoLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* Display on HD screen. */
    s32Ret = HI_MPI_VI_BindOutput(ViDev, 0, VoDev, 0);
    if (HI_SUCCESS != s32Ret)
    {
        printf("bind vi2vo failed, vi(%d,%d),vo(%d,%d)!\n",
               ViDev, ViChn, VoDev, VoChn);
        return HI_FAILURE;
    }
    printf("start VI to VO preview , input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();
    return HI_SUCCESS;
}

HI_S32 SAMPLE_VIO_MutiScreen_Vo1280x1024()
{
    VB_CONF_S stVbConf   = {0};
    HI_S32 s32ViChnTotal = 4;
    HI_S32 s32VoChnTotal = 4;
    VO_DEV VoDev;
    VI_DEV ViDev;
    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;
    VO_PUB_ATTR_S stVoDevAttr;
    VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr;
    HI_S32 s32Ret = HI_SUCCESS;

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt  = 20;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 288 * 2;/*CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 20;
    if (HI_SUCCESS != SAMPLE_InitMPP(&stVbConf))
    {
        return -1;
    }

    /* Config VI to input standard-definition video */
    ViDev = 0;
    SAMPLE_GetViCfg_SD(PIC_D1, &stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByChn(s32ViChnTotal, &stViDevAttr, &stViChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* display VGA video on vo HD divice. */
    VoDev = VO_DEV_HD;
    SAMPLE_GetVoCfg_1280x1024_4Screen(&stVoDevAttr, &stVideoLayerAttr);
    s32Ret = SAMPLE_StartVo(s32VoChnTotal, VoDev, &stVoDevAttr, &stVideoLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* bind vi and vo in sequence */
    s32Ret = SAMPLE_ViBindVo(s32ViChnTotal, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }
    printf("start VI to VO preview , input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();
    return HI_SUCCESS;
}


/*****************************************************************************
 Prototype       : SAMPLE_VIO_Normal
 Description     : VI capture SD TV signal and output in SD TV screen.
                   VI 8 channel, VO 8 channel.
 Input           : None
 Output          : None
 Return Value    :
 Global Variable
    Read Only    :
    Read & Write :
*****************************************************************************/
HI_S32 SAMPLE_VIO_Normal()
{
    HI_CHAR ch;
    HI_S32 i = 0;
    VB_CONF_S stVbConf   = {0};
    VO_DEV VoDev;
    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;
    VO_PUB_ATTR_S stVoDevAttr;
    VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32VoPicDiv;
    HI_S32 s32ViChnTotal = VIU_MAX_CHN_NUM;
    HI_S32 s32VoChnTotal = 16;
    HI_S32 as32VoPicDiv[] = {1,4,9,16};

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt  = 10;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 576 * 2;/*2CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 64;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* Config VI to input SD */
    SAMPLE_GetViCfg_SD(PIC_2CIF, &stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByChn(s32ViChnTotal, &stViDevAttr, &stViChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* display standard-definition video on SDTV. */
    VoDev = VO_DEV_SD;
    SAMPLE_GetVoCfg_SD(&stVoDevAttr, &stVideoLayerAttr);
    s32Ret = SAMPLE_StartVo(s32VoChnTotal, VoDev, &stVoDevAttr, &stVideoLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_ViBindVo(s32ViChnTotal, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    printf("press 'q' to exit sample !\npress ENTER to switch vo\n");

    while ((ch = getchar()) != 'q')
    {
        /* 1 -> 4- > 9 -> 16 -> 1 -> ... */
        u32VoPicDiv = as32VoPicDiv[(i++)%(sizeof(as32VoPicDiv)/sizeof(HI_S32))];
        printf("switch to %d pic mode \n", u32VoPicDiv);
        if (SAMPLE_VoPicSwitch(VoDev, u32VoPicDiv))
        {
            break;
        }
    }

    return 0;
}

HI_S32 SAMPLE_VIO_VLossDet()
{
    VB_CONF_S stVbConf   = {0};
    VO_DEV VoDev;
    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;
    VO_PUB_ATTR_S stVoDevAttr;
    VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32ViChnTotal = VIU_MAX_CHN_NUM;
    HI_S32 s32VoChnTotal = 16;
    VIDEO_FRAME_INFO_S stUserFrame;

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2;/*D1*/
    stVbConf.astCommPool[0].u32BlkCnt  = 10;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 576 * 2;/*2CIF*/
    stVbConf.astCommPool[1].u32BlkCnt = 64;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* enable vi dev and chn by BT656 interface */
    SAMPLE_GetViCfg_SD(PIC_2CIF, &stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByChn(s32ViChnTotal, &stViDevAttr, &stViChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* display standard-definition video on SDTV. */
    VoDev = VO_DEV_SD;
    SAMPLE_GetVoCfg_SD(&stVoDevAttr, &stVideoLayerAttr);
    s32Ret = SAMPLE_StartVo(s32VoChnTotal, VoDev, &stVoDevAttr, &stVideoLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* bind vi and vo */
    s32Ret = SAMPLE_ViBindVo(s32ViChnTotal, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* set novideo pic */
    s32Ret = SAMPLE_SetViUserPic("pic_704_576_p420_novideo01.yuv", 704, 576, 704, &stUserFrame);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    /* start novideo detect */
    SAMPLE_StartVLossDet(s32ViChnTotal);

    printf("\nstart VI to VO preview , input twice Enter to stop sample ... ... \n");
    getchar();
    getchar();

    SAMPLE_StopVLossDet();
    
    return HI_SUCCESS;
}

/*****************************************************************************
 Prototype    : SampleViMixMode
 Description  : setting vi minor attribution, capture multi-size picture
 Input        : VI_DEV ViDev
                VI_CHN ViChn
                PIC_SIZE_E enMainSize
                PIC_SIZE_E enMinorSize
                HI_S32 s32MainFrmRate
 Output       : None
 Remark       : s32MainFrmRate is fps of VI main chn,
                fps of VI minor chn is (s32SrcFrmRate - s32MainFrmRate);
                if you not need minor chn, you can set s32MainFrmRate to 25 or 30;
*****************************************************************************/
HI_S32 SampleViMixMode(VI_DEV ViDev, VI_CHN ViChn,
                       PIC_SIZE_E enMainSize, PIC_SIZE_E enMinorSize,
                       HI_S32 s32MainFrmRate)
{
    VI_CHN_ATTR_S stChnAttr;

    stChnAttr.stCapRect.u32Width  = 704;
    stChnAttr.stCapRect.u32Height = ((0 == gs_enViNorm) ? 288 : 240);
    stChnAttr.stCapRect.s32X = 8;
    stChnAttr.stCapRect.s32Y = 0;
    stChnAttr.enViPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stChnAttr.bHighPri = HI_FALSE;
    stChnAttr.bChromaResample = HI_FALSE;
    stChnAttr.enCapSel = (PIC_D1 == enMainSize || PIC_2CIF == enMainSize) ? \
                         VI_CAPSEL_BOTH : VI_CAPSEL_BOTTOM;
    stChnAttr.bDownScale = (PIC_D1 == enMainSize || PIC_HD1 == enMainSize) ? \
                           HI_FALSE : HI_TRUE;

    /* set main chn attr*/
    if (HI_MPI_VI_SetChnAttr(ViDev, ViChn, &stChnAttr))
    {
        printf("set vi(%d,%d) chn main attr err\n", ViDev, ViChn);
        return HI_FAILURE;
    }

    stChnAttr.enCapSel = (PIC_D1 == enMinorSize || PIC_2CIF == enMinorSize) ? \
                         VI_CAPSEL_BOTH : VI_CAPSEL_BOTTOM;
    stChnAttr.bDownScale = (PIC_D1 == enMinorSize || PIC_HD1 == enMinorSize) ? \
                           HI_FALSE : HI_TRUE;
    /* set minor chn attr*/
    if (HI_MPI_VI_SetChnMinorAttr(ViDev, ViChn, &stChnAttr))
    {
        printf("set vi(%d,%d) chn minor attr err\n", ViDev, ViChn);
        return HI_FAILURE;
    }

    /* set framerate of main chn */
    if (HI_MPI_VI_SetFrameRate(ViDev, ViChn, s32MainFrmRate))
    {
        printf("set vi(%d,%d) framerate err\n", ViDev, ViChn);
        return HI_FAILURE;
    }

    /* if main is both filed, minor single filed,
       must display only bottom field, otherwise pic may dithering*/
    if (((PIC_CIF == enMinorSize) || (PIC_HD1 == enMinorSize))
        && ((PIC_D1 == enMainSize) || (PIC_2CIF == enMainSize)))
    {
        VO_DEV VoDev  = G_VODEV;
        VO_CHN BindVo = VIU_CHNID_DEV_FACTOR*ViDev + ViChn;
        HI_MPI_VO_SetChnField(VoDev, BindVo, VO_FIELD_BOTTOM);
    }

    return HI_SUCCESS;
}

/* ---------------------------------------------------------------------------*/

HI_S32 SAMPLE_VIO_MixMode()
{
    VO_DEV VoDev = G_VODEV;
    HI_S32 i, s32Ret, ViDev, ViChn;
    VB_CONF_S stVbConf = {0};
    VI_PUB_ATTR_S stViDevAttr;
    VI_CHN_ATTR_S stViChnAttr;
    VO_PUB_ATTR_S stVoDevAttr;
    VO_VIDEO_LAYER_ATTR_S stVideoLayerAttr;
    GET_STREAM_S stGetMultiChn;

    HI_S32 s32ViChnCnt = VIU_MAX_CHN_NUM;
    HI_S32 s32VoChnTotal = s32ViChnCnt;
    HI_S32 s32MainFrmRate = 7;

    stVbConf.astCommPool[0].u32BlkSize = 704 * 576 * 2; /*D1*/
    stVbConf.astCommPool[0].u32BlkCnt  = s32ViChnCnt * 12;
    stVbConf.astCommPool[1].u32BlkSize = 384 * 288 * 2; /*CIF*/
    stVbConf.astCommPool[1].u32BlkCnt  = s32ViChnCnt * 6;
    stVbConf.astCommPool[2].u32BlkSize = 192 * 192 * 2; /*QCIF*/
    stVbConf.astCommPool[2].u32BlkCnt = s32ViChnCnt * 6;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }

    /* set main chn to D1 */
    SAMPLE_GetViCfg_SD(PIC_D1, &stViDevAttr, &stViChnAttr);
    s32Ret = SAMPLE_StartViByChn(s32ViChnCnt, &stViDevAttr, &stViChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    SAMPLE_GetVoCfg_SD(&stVoDevAttr, &stVideoLayerAttr);
    s32Ret = SAMPLE_StartVo(s32VoChnTotal, VoDev, &stVoDevAttr, &stVideoLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }

    for (i = 0; i < s32ViChnCnt; i++)
    {
#ifdef hi3515
        ViDev  = i / VIU_MAX_CHN_NUM_PER_DEV * 2;
#else
        ViDev  = i / VIU_MAX_CHN_NUM_PER_DEV;
#endif
        ViChn  = i % VIU_MAX_CHN_NUM_PER_DEV;
        s32Ret = SampleViMixMode(ViDev, ViChn, PIC_D1, PIC_CIF, s32MainFrmRate);
        if (s32Ret != HI_SUCCESS)
        {
            return s32Ret;
        }
    }

    /* bind vi and vo in sequence */
    s32Ret = SAMPLE_ViBindVo(s32ViChnCnt, VoDev);
    if (0 != s32Ret)
    {
        return s32Ret;
    }

    /* init venc, one vi chn bind two group,there is one chn in one group */
    for (i = 0; i < s32ViChnCnt; i++)
    {
        VENC_GRP VeGrp;
        VIDEO_PREPROC_CONF_S stVppConf;
#ifdef hi3515
        ViDev  = i / VIU_MAX_CHN_NUM_PER_DEV * 2;
#else
        ViDev  = i / VIU_MAX_CHN_NUM_PER_DEV;
#endif
        ViChn  = i % VIU_MAX_CHN_NUM_PER_DEV;
        
        /* init main venc chn */
        VeGrp = i*2;
        if (SAMPLE_StartOneVenc(VeGrp,ViDev,ViChn,PT_H264,PIC_D1,s32MainFrmRate))
        {
            return HI_FAILURE;
        }
        
        /* init minor venc chn */
        VeGrp = i*2 + 1; 
        if (SAMPLE_StartOneVenc(VeGrp,ViDev,ViChn,PT_H264,PIC_QCIF,25))
        {
            return HI_FAILURE;
        }
        
        /* modify scale mode of minor group to USEBOTTOM2 */
        if (HI_MPI_VPP_GetConf(VeGrp, &stVppConf))
        {
            return HI_FAILURE;
        }
        stVppConf.enScaleMode = VPP_SCALE_MODE_USEBOTTOM2;
        if (HI_MPI_VPP_SetConf(VeGrp, &stVppConf))
        {
            return HI_FAILURE;
        }
    }

    stGetMultiChn.enPayload     = PT_H264;
    stGetMultiChn.VeChnStart    = 0;
    stGetMultiChn.s32ChnTotal   = s32ViChnCnt * 2;/* one vi bind two venc group */
    SAMPLE_StartVencGetStream(&stGetMultiChn);
    printf("start MixMode demo ok, press twice Enter to exit ... ... \n");

    getchar();
    getchar();

    SAMPLE_StopVencGetStream();

    return 0;
}

#define SAMPLE_VIO_HELP(void) \
    {\
        printf("\nUsage : %s <index> [norm]\n", argv[0]); \
        printf("\nIndex:\n"); \
        printf("\t 0\t VI 16_CIF  -> VO 1/4/9/16-screen CVBS display \n"); \
        printf("\t 1\t VI 1_D1    -> VO 1-screen VGA display\n"); \
        printf("\t 2\t VI 4_D1    -> VO 4-screen of 1280*1024 display\n"); \
        printf("\t 3\t VI 16_CIF  -> VO mut-screen CVBS, NoVideo Detect\n"); \
        printf("\t 7\t VI 1_720P  -> VO 1-screen 720P display \n"); \
        printf("\t10\t VI 16_D1 (for preview in real time, but encode not in real time) \n"); \
        printf("\nNorm:\n\t p\t PAL format \n\t n\t NTSC format\n"); \
    }

HI_S32 main(int argc, char *argv[])
{
    HI_S32 index, s32Ret;

    if (argc < 2)
    {
        SAMPLE_VIO_HELP();
        return -1;
    }

    /* the first arg is sample index */
    index = atoi(argv[1]);

    /* the second arg is video norm */
    if (argc > 2)
    {
        if (!strcmp(argv[2], "p"))
        {
            gs_enViNorm   = VIDEO_ENCODING_MODE_PAL;
        	gs_enSDTvMode = VO_OUTPUT_PAL;
        }
        else if (!strcmp(argv[2], "n"))
        {
            gs_enViNorm   = VIDEO_ENCODING_MODE_NTSC;
        	gs_enSDTvMode = VO_OUTPUT_NTSC;
        }
    }
    
    switch (index)
    {
    case 0:
    {
        SAMPLE_TW2865_CfgV(gs_enViNorm, BT656_WORKMODE);
        s32Ret = SAMPLE_VIO_Normal();
        break;
    }
    case 1:
    {
        SAMPLE_TW2865_CfgV(gs_enViNorm, BT656_WORKMODE);
        s32Ret = SAMPLE_VIO_1Screen_VoVGA();
        break;
    }
    case 2:
    {
        SAMPLE_TW2865_CfgV(gs_enViNorm, BT656_WORKMODE);
        s32Ret = SAMPLE_VIO_MutiScreen_Vo1280x1024();
        break;
    }
    case 3:
    {
        SAMPLE_TW2865_CfgV(gs_enViNorm, BT656_WORKMODE);
        s32Ret = SAMPLE_VIO_VLossDet();
        break;
    }

    case 7:
    {
        SAMPLE_ADV7441_CfgV(VIDEO_FORMAT_720P_60HZ);
        s32Ret = SAMPLE_VIO_1Screen_720P();
        break;
    }
    case 10:
    {
        SAMPLE_TW2865_CfgV(gs_enViNorm, BT656_WORKMODE);
        s32Ret = SAMPLE_VIO_MixMode();
        break;
    }
    default:
    {
        s32Ret = HI_FAILURE;
        SAMPLE_VIO_HELP();
        break;
    }
    }

    if (s32Ret)
    {
        SAMPLE_ExitMPP();
        return s32Ret;
    }

    SAMPLE_ExitMPP();
    return 0;
}
