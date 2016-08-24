/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : sample_vdec.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/04
  Description   :
  History       :

******************************************************************************/

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include "hi_comm_vb.h"
#include "hi_comm_sys.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vdec.h"
#include "mpi_venc.h"
#include "hi_comm_vdec.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_venc.h"
#include "mpi_vo.h"
#include "tw2864/tw2864.h"
#include "adv7441/adv7441.h"
#include "mt9d131/mt9d131.h"
#include "sample_common.h"

#define Printf(fmt...) \
    do {\
        printf("\n [%s]: %d ", __FUNCTION__, __LINE__); \
        printf(fmt); \
    } while (0)

#define G_VODEV VO_DEV_SD

extern VIDEO_NORM_E gs_enViNorm;
extern VO_INTF_SYNC_E gs_enSDTvMode;

static HI_VOID sampleSendEos(VDEC_CHN VdChn)
{
    VDEC_STREAM_S stStreamEos;
    HI_U32 au32EOS[2] = {0x01000000, 0x0100000b};

    stStreamEos.pu8Addr = (HI_U8 *)au32EOS;
    stStreamEos.u32Len = sizeof(au32EOS);
    stStreamEos.u64PTS = 0;

    (void)HI_MPI_VDEC_SendStream(VdChn, &stStreamEos, HI_IO_BLOCK);

    return;
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
    if (HI_NULL == file)
    {
        printf("open file %s err\n", filename);
        return NULL;
    }

    while (1)
    {
        s32ReadLen = fread(gs_aBuf, 1, MAX_READ_LEN, file);
        if (s32ReadLen <= 0)
        {
            /* send EOS(End Of Stream) to ensure decoding the last frame. */
            sampleSendEos(0);
            break;
        }
        stStream.pu8Addr = gs_aBuf;
        stStream.u32Len = s32ReadLen;
        stStream.u64PTS = 0;

        s32Ret = HI_MPI_VDEC_SendStream(0, &stStream, HI_IO_BLOCK);
        if (HI_SUCCESS != s32Ret)
        {
            Printf("send to vdec 0x %x \n", s32Ret);
            break;
        }
    }

    fclose(file);
    return NULL;
}

/*
 * description:
 *    There are two mode to displsy decoded picture in VO:
 *    (1) bind mode.
 *    (2) user-send mode.
 *
 *    To use bind mode, call HI_MPI_VDEC_BindOutput(). It is recommended.
 *    To use user-send mode, do as thread_recvPic() did. It is not recommended.
 *
 * remark:
 *    How to send pictures to VO is totally left to user in user-send mode.
 *    So it is more complicated, but also more flexible. User can do some
 *    special control in user-send mode.
 */
void* thread_recvPic(void* p)
{
    VO_DEV VoDev = G_VODEV;
    HI_S32 s32Ret;

    while (1)
    {
        VDEC_DATA_S stVdecData;
        s32Ret = HI_MPI_VDEC_GetData(0, &stVdecData, HI_IO_BLOCK);
        if (HI_SUCCESS == s32Ret)
        {
            if (stVdecData.stFrameInfo.bValid)
            {
                s32Ret = HI_MPI_VO_SendFrame(VoDev, 0, &stVdecData.stFrameInfo.stVideoFrameInfo);
                if (HI_SUCCESS != s32Ret)
                {
                    printf("HI_MPI_VO_SendFrame(%d, %d) fail, err code: 0x%08x\n",
                        0, 0, s32Ret);
                }
                usleep(40000);
            }

            s32Ret = HI_MPI_VDEC_ReleaseData(0, &stVdecData);
            if  ( HI_SUCCESS != s32Ret )
            {
                printf("HI_MPI_VDEC_ReleaseData(%d) fail, err code: 0x%08x\n",
                    0, s32Ret);
            }
        }
    }

    return NULL;
}

/*
 * Create vdec chn
 */
static HI_S32 SampleCreateVdecchn(HI_S32 s32ChnID, PAYLOAD_TYPE_E enType, void* pstAttr)
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
            Printf("err type \n");
            return HI_FAILURE;
    }

    s32ret = HI_MPI_VDEC_CreateChn(s32ChnID, &stAttr, NULL);
    if (HI_SUCCESS != s32ret)
    {
        Printf("HI_MPI_VDEC_CreateChn failed errno 0x%x \n", s32ret);
        return s32ret;
    }

    s32ret = HI_MPI_VDEC_StartRecvStream(s32ChnID);
    if (HI_SUCCESS != s32ret)
    {
        Printf("HI_MPI_VDEC_StartRecvStream failed errno 0x%x \n", s32ret);
        return s32ret;
    }

    return HI_SUCCESS;
}

/* force to stop decoder and destroy channel. 
 * stream left in decoder will not be decoded. */
void SampleForceDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        Printf("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32Ret);
    }

    s32Ret = HI_MPI_VDEC_DestroyChn(s32ChnID);
    if (HI_SUCCESS != s32Ret)
    {
        Printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32Ret);
    }
}

/* wait for decoder finished and destroy channel.
 * Stream left in decoder will be decoded. */
void SampleWaitDestroyVdecChn(HI_S32 s32ChnID)
{
    HI_S32 s32ret;
    VDEC_CHN_STAT_S stStat;

    memset(&stStat, 0, sizeof(VDEC_CHN_STAT_S));

    s32ret = HI_MPI_VDEC_StopRecvStream(s32ChnID);
    if (s32ret != HI_SUCCESS)
    {
        Printf("HI_MPI_VDEC_StopRecvStream failed errno 0x%x \n", s32ret);
        return;
    }

    while (1)
    {
        usleep(40000);
        s32ret = HI_MPI_VDEC_Query(s32ChnID, &stStat);
        if (s32ret != HI_SUCCESS)
        {
            Printf("HI_MPI_VDEC_Query failed errno 0x%x \n", s32ret);
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
        Printf("HI_MPI_VDEC_DestroyChn failed errno 0x%x \n", s32ret);
        return;
    }
}

void printhelp(void)
{
    printf("1-- h264d to vo,vo bind h264d\n" );
    printf("2-- h264d to vo,user get pic from vdec\n" );
    printf("3-- 1d1 h264 decoder to vo, stream and vo frame rate is 6\n" );
    printf("4-- 1cif h264 decoder to vo, stream frame rate is 25,vo frame rate is 50\n" );
}

HI_S32 SAMPLE_VDEC_1D1H264_BindVo(HI_VOID)
{
    pthread_t pidSendStream;
    VDEC_ATTR_H264_S stH264Attr;
    VO_DEV VoDev = G_VODEV;
    VO_CHN VoChn;
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    HI_CHAR filename[32] = "sample_d1.h264";
    VB_CONF_S stVbConf = {0};

    stVbConf.astCommPool[0].u32BlkSize = 720 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 20;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    VdChn = 0;
    VoChn = 0;

    /* VO 1-screen, display d1. */
    s32Ret = SAMPLE_StartVo_SD(1, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    stH264Attr.u32Priority  = 0;
    stH264Attr.u32PicHeight = 576;
    stH264Attr.u32PicWidth = 720;
    stH264Attr.u32RefFrameNum = 2;
    stH264Attr.enMode = H264D_MODE_STREAM;

    /*create vdec chn and vo chn*/
    if (HI_SUCCESS != SampleCreateVdecchn(VdChn, PT_H264, &stH264Attr))
    {
        SAMPLE_ExitMPP();
        return HI_FAILURE;
    }

    /*bind vdec to vo*/
    if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(VdChn, VoDev, VoChn))
    {
        SAMPLE_ExitMPP();
        return HI_FAILURE;
    }

    pthread_create(&pidSendStream, NULL, thread_sendh264stream, filename);
    pthread_join(pidSendStream, 0);

    /* 销毁解码通道 */
    SampleWaitDestroyVdecChn(VdChn);

    SAMPLE_StopVo(  1, VoDev );

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VDEC_1D1H264_UserSendVo(HI_VOID)
{
    pthread_t pidSendStream;
    pthread_t pidRecvPic;
    VB_CONF_S stVbConf = {0};
    VDEC_ATTR_H264_S stH264Attr;
    VO_DEV VoDev = G_VODEV;
    VO_CHN VoChn;
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    HI_CHAR filename[32] = "sample_d1.h264";

    stVbConf.astCommPool[0].u32BlkSize = 720 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 20;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    VdChn = 0;
    VoChn = 0;

    /* 1 channel VO display. */
    s32Ret = SAMPLE_StartVo_SD(1, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    stH264Attr.u32Priority  = 0;
    stH264Attr.u32PicHeight = 576;
    stH264Attr.u32PicWidth = 720;
    stH264Attr.u32RefFrameNum = 3;
    stH264Attr.enMode = H264D_MODE_STREAM;

    /*create vdec chn and vo chn*/
    if (HI_SUCCESS != SampleCreateVdecchn(VdChn, PT_H264, &stH264Attr))
    {
        SAMPLE_ExitMPP();
        return HI_FAILURE;
    }

    pthread_create(&pidSendStream, NULL, thread_sendh264stream, filename);

    /* thread_recvPic use vo device 0, vo channel 0. */
    pthread_create(&pidRecvPic, NULL, thread_recvPic, NULL);

    pthread_join(pidSendStream, 0);

    /* 销毁解码通道 */
    SampleWaitDestroyVdecChn(VdChn);

    SAMPLE_StopVo( 1, VoDev );

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VDEC_1D1H264_6fps(HI_VOID)
{
    pthread_t pidSendStream;
    VDEC_ATTR_H264_S stH264Attr;
    VO_DEV VoDev = G_VODEV;
    VO_CHN VoChn;
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    VB_CONF_S stVbConf = {0};
    HI_CHAR filename[32] = "sample_d1_6fps.h264";

    stVbConf.astCommPool[0].u32BlkSize = 720 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 20;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    VdChn = 0;
    VoChn = 0;

    /* 1 channel VO display. */
    s32Ret = SAMPLE_StartVo_SD(1, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    stH264Attr.u32Priority  = 0;
    stH264Attr.u32PicHeight = 576;
    stH264Attr.u32PicWidth = 720;
    stH264Attr.u32RefFrameNum = 3;
    stH264Attr.enMode = H264D_MODE_STREAM;

    if (HI_SUCCESS != HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 6))
    {
        printf("HI_MPI_VO_SetChnFrameRate err in line:%d\n", __LINE__);
        return HI_FAILURE;
    }

    /*create vdec chn and vo chn*/
    if (HI_SUCCESS != SampleCreateVdecchn(VdChn, PT_H264, &stH264Attr))
    {
        return HI_FAILURE;
    }

    /*bind vdec to vo*/
    if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(VdChn, VoDev, VoChn))
    {
        return HI_FAILURE;
    }

    pthread_create(&pidSendStream, NULL, thread_sendh264stream, filename);
    pthread_join(pidSendStream, 0);

    SampleWaitDestroyVdecChn(VdChn);

    SAMPLE_StopVo( 1, VoDev );

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VDEC_1CifH264_50fps()
{
    pthread_t pidSendStream;
    VB_CONF_S stVbConf = {0};
    VDEC_ATTR_H264_S stH264Attr;
    VO_DEV VoDev = G_VODEV;
    VO_CHN VoChn;
    VDEC_CHN VdChn;
    HI_S32 s32Ret;
    HI_CHAR filename[100] = "sample_cif_25fps.h264";

    stVbConf.astCommPool[0].u32BlkSize = 720 * 576 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 20;
    stVbConf.astCommPool[0].u32BlkSize = 352 * 288 * 2;
    stVbConf.astCommPool[0].u32BlkCnt = 20;
    s32Ret = SAMPLE_InitMPP(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    VdChn = 0;
    VoChn = 0;

    /* 1 channel VO display. */
    s32Ret = SAMPLE_StartVo_SD(1, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    stH264Attr.u32Priority  = 0;
    stH264Attr.u32PicHeight = 288;
    stH264Attr.u32PicWidth = 352;
    stH264Attr.u32RefFrameNum = 3;
    stH264Attr.enMode = H264D_MODE_STREAM;

    if (HI_SUCCESS != HI_MPI_VO_SetChnFrameRate(VoDev, VoChn, 50))
    {
        printf("HI_MPI_VO_SetChnFrameRate err\n");
        return HI_FAILURE;
    }

    /*create vdec chn and vo chn*/
    if (HI_SUCCESS != SampleCreateVdecchn(VdChn, PT_H264, &stH264Attr))
    {
        printf("SampleCreateVdecchn err\n");
        return HI_FAILURE;
    }

    /*bind vdec to vo*/
    if (HI_SUCCESS != HI_MPI_VDEC_BindOutput(VdChn, VoDev, VoChn))
    {
        printf("HI_MPI_VDEC_BindOutput err\n");
        return HI_FAILURE;
    }

    pthread_create(&pidSendStream, NULL, thread_sendh264stream, filename);
    pthread_join(pidSendStream, 0);

    SampleWaitDestroyVdecChn(VdChn);
    SAMPLE_StopVo( 1, VoDev );

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

int main(int argc, char* argv[] )
{
    HI_S32 S32Index;

    if (argc != 2)
    {
        printf("usage :%s 1|2|3|4|5\n", argv[0]);
        printhelp();
        return HI_FAILURE;
    }

    S32Index = atoi(argv[1]);

    if ((S32Index < 1) || (S32Index > 5))
    {
        printhelp();
        return HI_FAILURE;
    }

    switch (S32Index)
    {
    case 1:         /*one h264 decoder to vo, d1*/
        SAMPLE_VDEC_1D1H264_BindVo();
        break;
    case 2:         /*get user data from h264 stream*/
        SAMPLE_VDEC_1D1H264_UserSendVo();
        break;
    case 3:         /*one d1 h264 decoder to vo, stream and vo frame rate is 6*/
        SAMPLE_VDEC_1D1H264_6fps();
        break;
    case 4:         /*one cif h264 decoder to vo, stream frame rate is full,vo frame rate is 12*/
        SAMPLE_VDEC_1CifH264_50fps();
        break;
    case 5:         /*one cif h264 decoder to vo, stream frame rate is full,vo frame rate is 50*/

    default:
        Printf("err arg \n");
        break;
    }

    return HI_SUCCESS;
}

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* End of #ifdef __cplusplus */
