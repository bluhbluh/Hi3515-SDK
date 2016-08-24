/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_audio.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/7/29
  Description   : 
  History       :
  1.Date        : 2009/7/29
    Author      : Hi3520MPP
    Modification: Created file    
******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "sample_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define TW2865_FILE     "/dev/tw2865dev"
#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define AUDIO_AAC_TYPE AAC_TYPE_AACLC   /* AAC_TYPE_AACLC, AAC_TYPE_EAAC, AAC_TYPE_EAACPLUS*/
#define G726_BPS MEDIA_G726_16K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */
#define AMR_FORMAT AMR_FORMAT_MMS       /* AMR_FORMAT_MMS, AMR_FORMAT_IF1, AMR_FORMAT_IF2*/
#define AMR_MODE AMR_MODE_MR74         /* AMR_MODE_MR122, AMR_MODE_MR102 ... */
#define AMR_DTX 0


typedef struct tagSAMPLE_AIAENC_SINGLE_S
{
    HI_S32 s32AencChn; 
    FILE *pfd;
    HI_S32 u32GetStrmCnt;
} SAMPLE_AIAENC_SINGLE_S;

typedef struct tagSAMPLE_AIAENC_S
{
    HI_BOOL bStart;
    pthread_t stAencPid;
    HI_U32 u32AeChnCnt;
    HI_S32 s32BindAeChn;    /* send aenc stream to adec directly fot testing */
    SAMPLE_AIAENC_SINGLE_S astAencChn[AENC_MAX_CHN_NUM];
} SAMPLE_AIAENC_S;

typedef struct tagSAMPLE_AOADEC_S
{
    HI_BOOL bStart;
    HI_S32 AdChn; 
    FILE *pfd;
    pthread_t stAdPid;
} SAMPLE_AOADEC_S;


static SAMPLE_AIAENC_S s_stSampleAiAenc;
static SAMPLE_AOADEC_S s_stSampleAoAdec;
static PAYLOAD_TYPE_E s_enPayloadType = PT_ADPCMA;

static HI_BOOL s_bAioReSample = HI_FALSE;
static AUDIO_RESAMPLE_ATTR_S s_stAiReSmpAttr;
static AUDIO_RESAMPLE_ATTR_S s_stAoReSmpAttr;

HI_S32 SAMPLE_TW2865_CfgAudio(AUDIO_SAMPLE_RATE_E enSample)
{
    int fd;
    tw2865_audio_samplerate samplerate;
        
    fd = open(TW2865_FILE, O_RDWR);
    if (fd < 0)
    {
        printf("open %s fail\n", TW2865_FILE);
        return -1;
    }
    
    if (AUDIO_SAMPLE_RATE_8000 == enSample)
    {
        samplerate = TW2865_SAMPLE_RATE_8000;
    }
    else if (AUDIO_SAMPLE_RATE_16000 == enSample)
    {
        samplerate = TW2865_SAMPLE_RATE_16000;
    }
    else if (AUDIO_SAMPLE_RATE_32000 == enSample)
    {
        samplerate = TW2865_SAMPLE_RATE_32000;
    }
    else if (AUDIO_SAMPLE_RATE_44100 == enSample)
    {
        samplerate = TW2865_SAMPLE_RATE_44100;
    }
    else if (AUDIO_SAMPLE_RATE_48000 == enSample)
    {
        samplerate = TW2865_SAMPLE_RATE_48000;
    }
    else 
    {
        printf("not support enSample:%d\n",enSample);
        return -1;
    }
        
    if (ioctl(fd, TW2865_SET_SAMPLE_RATE, &samplerate))
    {
        printf("ioctl TW2865_SET_SAMPLE_RATE err !!! \n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

static char* SamplePt2Str(PAYLOAD_TYPE_E enType)
{
    if (PT_G711A == enType)  return "g711a";
    else if (PT_G711U == enType)  return "g711u";
    else if (PT_ADPCMA == enType)  return "adpcm";
    else if (PT_G726 == enType)  return "g726";
    else if (PT_AMR == enType)  return "amr";
    else if (PT_AAC == enType)  return "aac";
    else if (PT_LPCM == enType)  return "pcm";
    else return "data";
}

static FILE * SampleOpenAencFile(AENC_CHN AeChn, PAYLOAD_TYPE_E enType)
{
    FILE *pfd;
    HI_CHAR aszFileName[128];
    
    /* create file for save stream*/
    sprintf(aszFileName, "audio_chn%d.%s", AeChn, SamplePt2Str(enType));
    pfd = fopen(aszFileName, "w+");
    if (NULL == pfd)
    {
        printf("open file %s err\n", aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for aenc ok\n", aszFileName);
    return pfd;
}

static FILE * SampleOpenAdecFile(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    FILE *pfd;
    HI_CHAR aszFileName[128];
    
    /* create file for save stream*/        
    sprintf(aszFileName, "audio_chn%d.%s", AdChn, SamplePt2Str(enType));
    pfd = fopen(aszFileName, "rb");
    if (NULL == pfd)
    {
        printf("open file %s err\n", aszFileName);
        return NULL;
    }
    printf("open stream file:\"%s\" for adec ok\n", aszFileName);
    return pfd;
}

HI_S32 SAMPLE_StartAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt,AIO_ATTR_S *pstAioAttr)
{
    HI_S32 j, s32Ret;
    
    s32Ret = HI_MPI_AI_SetPubAttr(AiDevId, pstAioAttr);
    if (s32Ret)
    {
        printf("HI_MPI_AI_SetPubAttr aidev:%d err, %x\n", AiDevId, s32Ret);
        return HI_FAILURE;
    }
    if (HI_MPI_AI_Enable(AiDevId))
    {
        printf("enable ai dev:%d fail\n", AiDevId);
        return HI_FAILURE;
    }                
    for (j=0; j<s32AiChnCnt; j++)
    {
        if (HI_MPI_AI_EnableChn(AiDevId, j))
        {
            printf("enable ai(%d,%d) fail\n", AiDevId, j);
            return HI_FAILURE;
        }
    }

    if (HI_TRUE == s_bAioReSample)
    {
        if (HI_MPI_AI_EnableReSmp(AiDevId, 0, &s_stAiReSmpAttr))
        {
            printf("HI_MPI_AI_EnableReSmp ai(%d,%d) fail\n", AiDevId, 0);
            return HI_FAILURE;
        }
    }
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_StopAi(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt)
{
    HI_S32 i;    
    for (i=0; i<s32AiChnCnt; i++)
    {
        HI_MPI_AI_DisableChn(AiDevId, i);
    }  
    HI_MPI_AI_Disable(AiDevId);
    return HI_SUCCESS;
}

HI_S32 SAMPLE_StartAo(AUDIO_DEV AoDevId, AO_CHN AoChn, AIO_ATTR_S *pstAioAttr)
{
    HI_S32 s32ret;

    s32ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
    if(HI_SUCCESS != s32ret)
    {
        printf("set ao %d attr err:0x%x\n", AoDevId,s32ret);
        return HI_FAILURE;
    }
    s32ret = HI_MPI_AO_Enable(AoDevId);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ao dev %d err:0x%x\n", AoDevId, s32ret);
        return HI_FAILURE;
    }
    s32ret = HI_MPI_AO_EnableChn(AoDevId, AoChn);
    if(HI_SUCCESS != s32ret)
    {
        printf("enable ao chn %d err:0x%x\n", AoChn, s32ret);
        return HI_FAILURE;
    }
    
    if (HI_TRUE == s_bAioReSample)
    {
        if (HI_MPI_AO_EnableReSmp(AoDevId, AoChn, &s_stAoReSmpAttr))
        {
            printf("HI_MPI_AO_EnableReSmp ao(%d,%d) fail\n", AoDevId, AoChn);
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_StopAo(AUDIO_DEV AoDevId, AO_CHN AoChn)
{
    HI_MPI_AO_DisableChn(AoDevId, AoChn);
    HI_MPI_AO_Disable(AoDevId);
    return HI_SUCCESS;
}

HI_S32 SAMPLE_StartAenc(HI_S32 s32AencChnCnt)
{
    AENC_CHN AeChn;
    HI_S32 s32Ret, j;
    AENC_CHN_ATTR_S stAencAttr;
    
    /* set AENC chn attr */
    
    stAencAttr.enType = s_enPayloadType;
    stAencAttr.u32BufSize = 30;
    
    if (PT_ADPCMA == stAencAttr.enType)
    {
        AENC_ATTR_ADPCM_S stAdpcmAenc;
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = AUDIO_ADPCM_TYPE;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        AENC_ATTR_G711_S stAencG711;
        stAencAttr.pValue       = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        AENC_ATTR_G726_S stAencG726;
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = G726_BPS;
    }
    else if (PT_AMR == stAencAttr.enType)
    {
        AENC_ATTR_AMR_S stAencAmr;
        stAencAttr.pValue       = &stAencAmr;
        stAencAmr.enFormat      = AMR_FORMAT ;
        stAencAmr.enMode        = AMR_MODE ;
        stAencAmr.s32Dtx        = AMR_DTX ;
    }
    else if (PT_AAC == stAencAttr.enType)
    {
        AENC_ATTR_AAC_S stAencAac;
        stAencAttr.pValue       = &stAencAac;
        stAencAac.enAACType     = AUDIO_AAC_TYPE;        
        stAencAac.enBitRate     = AAC_BPS_128K;
        stAencAac.enBitWidth    = AUDIO_BIT_WIDTH_16;
        stAencAac.enSmpRate     = AUDIO_SAMPLE_RATE_16000;
        stAencAac.enSoundMode   = AUDIO_SOUND_MODE_MOMO;
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        AENC_ATTR_LPCM_S stAencLpcm;
        stAencAttr.pValue = &stAencLpcm;
    }
    else
    {
        printf("invalid aenc payload type:%d\n", stAencAttr.enType);
        return HI_FAILURE;
    }    

    for (j=0; j<s32AencChnCnt; j++)
    {            
        AeChn = j;
        
        /* create aenc chn*/
        s32Ret = HI_MPI_AENC_CreateChn(AeChn, &stAencAttr);
        if (s32Ret != HI_SUCCESS)
        {
            printf("create aenc chn %d err:0x%x\n", AeChn, s32Ret);
            return s32Ret;
        }        
    }
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_StopAenc(HI_S32 s32AencChnCnt)
{
    HI_S32 i;
    for (i=0; i<s32AencChnCnt; i++)
    {
        HI_MPI_AENC_DestroyChn(i);
    }
    return HI_SUCCESS;
}

HI_S32 SAMPLE_StartAdec(ADEC_CHN AdChn)
{
    HI_S32 s32ret;
    ADEC_CHN_ATTR_S stAdecAttr;
        
    /*------------------------------------------------------------------------*/
    stAdecAttr.enType = s_enPayloadType;
    stAdecAttr.u32BufSize = 20;
    stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */
        
    if (PT_ADPCMA == stAdecAttr.enType)
    {
        ADEC_ATTR_ADPCM_S stAdpcm;
        stAdecAttr.pValue = &stAdpcm;
        stAdpcm.enADPCMType = AUDIO_ADPCM_TYPE ;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        ADEC_ATTR_G711_S stAdecG711;
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        ADEC_ATTR_G726_S stAdecG726;
        stAdecAttr.pValue = &stAdecG726;
        stAdecG726.enG726bps = G726_BPS ;      
    }
    else if (PT_AMR == stAdecAttr.enType)
    {
        ADEC_ATTR_AMR_S stAdecAmr;
        stAdecAttr.pValue = &stAdecAmr;
        stAdecAmr.enFormat = AMR_FORMAT;
    }
    else if (PT_AAC == stAdecAttr.enType)
    {
        ADEC_ATTR_AAC_S stAdecAac;
        stAdecAttr.pValue = &stAdecAac;
        stAdecAttr.enMode = ADEC_MODE_STREAM;/* aac now only support stream mode */
    }
    else if (PT_LPCM == stAdecAttr.enType)
    {
        ADEC_ATTR_LPCM_S stAdecLpcm;
        stAdecAttr.pValue = &stAdecLpcm;
        stAdecAttr.enMode = ADEC_MODE_PACK;/* lpcm must use pack mode */
    }
    else
    {
        printf("invalid aenc payload type:%d\n", stAdecAttr.enType);
        return HI_FAILURE;
    }     
    
    /* create adec chn*/
    s32ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
    if (s32ret)
    {
        printf("create adec chn %d err:0x%x\n", AdChn,s32ret);
        return s32ret;
    }
    return 0;
}

HI_S32 SAMPLE_StopAdec(ADEC_CHN AdChn)
{
    HI_MPI_ADEC_DestroyChn(AdChn);
    return HI_SUCCESS;
}

HI_S32 SAMPLE_StartAoAdec()
{
    HI_S32 s32Ret;
    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;
    ADEC_CHN AdChn = 0;
    AIO_ATTR_S stAioAttr;
    
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSamplerate = (s_enPayloadType!=PT_AAC) ? AUDIO_SAMPLE_RATE_8000 : AUDIO_SAMPLE_RATE_16000;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = (s_enPayloadType!=PT_AAC) ? 320 : 1024;
    stAioAttr.u32ClkSel = 0;
    s32Ret = SAMPLE_StartAdec(AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_StartAo(AoDev, AoChn, &stAioAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_AO_BindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }
    
    printf("start adec and ao ok, audio stream will send to ao directly \n");
    return HI_SUCCESS;
}

void *Sample_AencProc(void *parg)
{
    HI_S32 s32ret, i, max, u32ChnCnt;
    HI_S32 AencFd[AENC_MAX_CHN_NUM];
    AENC_CHN AeChn;
    SAMPLE_AIAENC_S *pstAencCtl = (SAMPLE_AIAENC_S *)parg;
    AUDIO_STREAM_S stStream; 
    fd_set read_fds;
    struct timeval TimeoutVal;

    u32ChnCnt = pstAencCtl->u32AeChnCnt;
    printf("u32ChnCnt is %d \n", u32ChnCnt);
    
    FD_ZERO(&read_fds);
    for (i=0; i<u32ChnCnt; i++)
    {
        AencFd[i] = HI_MPI_AENC_GetFd(i);
		FD_SET(AencFd[i],&read_fds);
		if(max <= AencFd[i])    max = AencFd[i];
    }
    
    while (pstAencCtl->bStart)
    {     
        TimeoutVal.tv_sec = 1;
		TimeoutVal.tv_usec = 0;
        
        FD_ZERO(&read_fds);
        for (i=0; i<u32ChnCnt; i++)
        {
            FD_SET(AencFd[i], &read_fds);
        }
        
        s32ret = select(max+1, &read_fds, NULL, NULL, &TimeoutVal);
		if (s32ret < 0) 
		{
            break;
		}
		else if (0 == s32ret) 
		{
			printf("get aenc stream select time out\n");
            break;
		}
        
        for (i=0; i<u32ChnCnt; i++)
        {
            AeChn = i;
            if (FD_ISSET(AencFd[AeChn], &read_fds))
            {
                /* get stream from aenc chn */
                s32ret = HI_MPI_AENC_GetStream(AeChn, &stStream, HI_IO_NOBLOCK);
                if (HI_SUCCESS != s32ret )
                {
                    printf("HI_MPI_AENC_GetStream chn:%d, fail\n",AeChn);
                    pstAencCtl->bStart = HI_FALSE;
                    return NULL;
                }
                pstAencCtl->astAencChn[AeChn].u32GetStrmCnt ++;

                /* send stream to decoder and play for testing */
                if (AeChn == pstAencCtl->s32BindAeChn)
                {
                    HI_MPI_ADEC_SendStream(0, &stStream, HI_IO_BLOCK);
                }
                
                /* save audio stream to file */
                fwrite(stStream.pStream,1,stStream.u32Len, pstAencCtl->astAencChn[AeChn].pfd);

                /* finally you must release the stream */
                HI_MPI_AENC_ReleaseStream(AeChn, &stStream);
            }            
        }
    }
    
    for (i=0; i<u32ChnCnt; i++)
    {
        fclose(pstAencCtl->astAencChn[i].pfd);
    }
    pstAencCtl->bStart = HI_FALSE;
    return NULL;
}


void * SampleAdecProc(void *parg)
{
    HI_S32 s32ret;
    AUDIO_STREAM_S stAudioStream;    
    HI_U32 u32Len = 640;
    HI_U32 u32ReadLen;
    HI_S32 s32AdecChn;
    HI_U8 *pu8AudioStream = NULL;
    SAMPLE_AOADEC_S *pstAdecCtl = (SAMPLE_AOADEC_S *)parg;    
    FILE *pfd = pstAdecCtl->pfd;
    s32AdecChn = pstAdecCtl->AdChn;
    
    pu8AudioStream = (HI_U8*)malloc(sizeof(HI_U8)*MAX_AUDIO_STREAM_LEN);
    if (NULL == pu8AudioStream)
    {
        return NULL;
    }

    while (HI_TRUE == pstAdecCtl->bStart)
    {
        /* read from file */
        stAudioStream.pStream = pu8AudioStream;
        u32ReadLen = fread(stAudioStream.pStream, 1, u32Len, pfd);
        if (u32ReadLen <= 0)
        {            
            fseek(pfd, 0, SEEK_SET);/*read file again*/
            continue;
        }

        /* here only demo adec streaming sending mode, but pack sending mode is commended */
        stAudioStream.u32Len = u32ReadLen;
        s32ret = HI_MPI_ADEC_SendStream(s32AdecChn, &stAudioStream, HI_IO_BLOCK);
        if (s32ret)
        {
            printf("HI_MPI_ADEC_SendStream chn:%d err:0x%x\n",s32AdecChn,s32ret);
            break;
        }
        //printf("over send adec \n");
    }
    
    free(pu8AudioStream);
    pu8AudioStream = NULL;
    fclose(pfd);
    pstAdecCtl->bStart = HI_FALSE;
    return NULL;
}

HI_S32 SAMPLE_AiAenc()
{
    HI_S32 i, s32Ret;
    AUDIO_DEV AiDev = 0;
    AI_CHN AiChn;
    AIO_ATTR_S stAioAttr;
    HI_S32 s32AiChnCnt;
    HI_S32 s32AencChnCnt;
    AENC_CHN AeChn;
    SAMPLE_AIAENC_S *pstAiAenc;

    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.u32ChnCnt = (0==AiDev) ? 2 : 16;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSamplerate = (s_enPayloadType!=PT_AAC) ? AUDIO_SAMPLE_RATE_8000 : AUDIO_SAMPLE_RATE_16000;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ClkSel = 0;
    
    if (PT_AAC == s_enPayloadType)/* AACLC encoder must be 1024 point */
    {
        stAioAttr.u32PtNumPerFrm = 1024;
    }
    else if (PT_AMR == s_enPayloadType)/* AMR encoder must be 160 point */
    {
        stAioAttr.u32PtNumPerFrm = 160;
    }

    /* config audio codec */
    SAMPLE_TW2865_CfgAudio(stAioAttr.enSamplerate);
    
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32AencChnCnt = 1;//s32AiChnCnt;

    /* enable AI channle */
    s32Ret = SAMPLE_StartAi(AiDev, s32AiChnCnt, &stAioAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }
    
    /* create AENC channle */
    s32Ret = SAMPLE_StartAenc(s32AencChnCnt);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }    

    pstAiAenc = &s_stSampleAiAenc;
    pstAiAenc->s32BindAeChn = -1;

    /* if you want test aenc to adec, seting a aenc chn */   
    pstAiAenc->s32BindAeChn = -1;
    if (pstAiAenc->s32BindAeChn >= 0)
    {
        SAMPLE_StartAoAdec();
    }    

    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;
        
        /* bind AENC to AI channel  */
        s32Ret = HI_MPI_AENC_BindAi(AeChn, AiDev, AiChn);
        if (s32Ret != HI_SUCCESS)
        {
            printf("Ai(%d,%d) bind to Aenc(%d) err \n", AiDev, AiChn, AeChn);
            return s32Ret;
        }
        printf("Ai(%d,%d) bind to AencChn:%d <%s> ok ! \n",
            AiDev, AiChn, AeChn, SamplePt2Str(s_enPayloadType));
        
        pstAiAenc->astAencChn[AeChn].s32AencChn = AeChn;
        pstAiAenc->astAencChn[AeChn].pfd = SampleOpenAencFile(AeChn, s_enPayloadType);
        if (!pstAiAenc->astAencChn[AeChn].pfd)
        {
            return HI_FAILURE;
        }                
    }

    /* create pthread to get aenc stream */
    pstAiAenc->u32AeChnCnt = s32AencChnCnt;
    pstAiAenc->bStart = HI_TRUE;    
    pthread_create(&pstAiAenc->stAencPid, 0, Sample_AencProc, pstAiAenc);

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /* clear------------------------------------------*/
    if (HI_TRUE == pstAiAenc->bStart)
    {
        pstAiAenc->bStart = HI_FALSE;
        pthread_join(pstAiAenc->stAencPid, 0);
    }
    SAMPLE_StopAenc(s32AencChnCnt);
    SAMPLE_StopAdec(0);
    SAMPLE_StopAi(AiDev, s32AiChnCnt);
    
    return HI_SUCCESS;
}


HI_S32 SAMPLE_AdecAo()
{
    HI_S32 s32Ret;
    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;
    ADEC_CHN AdChn = 0;
    AIO_ATTR_S stAioAttr;
    SAMPLE_AOADEC_S *pstAoAdec;
    
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.u32ChnCnt = 2;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSamplerate = (s_enPayloadType!=PT_AAC) ? AUDIO_SAMPLE_RATE_8000 : AUDIO_SAMPLE_RATE_16000;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ClkSel = 0;

    if (PT_AAC == s_enPayloadType)/* AACLC encoder must be 1024 point */
    {
        stAioAttr.u32PtNumPerFrm = 1024;
    }
    else if (PT_AMR == s_enPayloadType)/* AMR encoder must be 160 point */
    {
        stAioAttr.u32PtNumPerFrm = 160;
    }

    /* config ao resample attr if needed */
    if (HI_TRUE == s_bAioReSample)
    {
        AUDIO_RESAMPLE_ATTR_S stReSampleAttr;
        
        stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_32000;
        stAioAttr.u32PtNumPerFrm = 320 * 4; 

        /* ao 8k -> 32k */
        stReSampleAttr.u32InPointNum = 320;
        stReSampleAttr.enInSampleRate = AUDIO_SAMPLE_RATE_8000;
        stReSampleAttr.enReSampleType = AUDIO_RESAMPLE_1X4;
        memcpy(&s_stAoReSmpAttr, &stReSampleAttr, sizeof(AUDIO_RESAMPLE_ATTR_S));
    }

    /* config audio codec */
    SAMPLE_TW2865_CfgAudio(stAioAttr.enSamplerate);
    
    s32Ret = SAMPLE_StartAdec(AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_StartAo(AoDev, AoChn, &stAioAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_AO_BindAdec(AoDev, AoChn, AdChn);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }    

    pstAoAdec = &s_stSampleAoAdec;
    
    pstAoAdec->pfd = SampleOpenAdecFile(AdChn, s_enPayloadType);
    if (!pstAoAdec->pfd)
    {
        return HI_FAILURE;
    }

    pstAoAdec->bStart = HI_TRUE;
    pthread_create(&pstAoAdec->stAdPid, 0, SampleAdecProc, pstAoAdec);
    
    printf("bind adec:%d<%s> to ao(%d,%d) ok \n", AdChn, SamplePt2Str(s_enPayloadType), AoDev, AoChn);

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /* clear------------------------------------------*/
    if (HI_TRUE == pstAoAdec->bStart)
    {
        pstAoAdec->bStart = HI_FALSE;
        pthread_join(pstAoAdec->stAdPid, 0);
    }
    SAMPLE_StopAo(AoDev, AoChn);
    SAMPLE_StopAdec(AdChn);
    
    return HI_SUCCESS;
}

HI_S32 SAMPLE_AiAo()
{
    HI_S32 s32Ret, s32AiChnCnt;
    AUDIO_DEV AiDev = 0;
    AI_CHN AiChn = 0;
    AUDIO_DEV AoDev = 0;
    AO_CHN AoChn = 0;
    AIO_ATTR_S stAioAttr;

    /* config aio dev attr */    
    stAioAttr.enWorkmode = AIO_MODE_I2S_SLAVE;
    stAioAttr.u32ChnCnt = (0==AiDev) ? 2 : 16;
    stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
    stAioAttr.u32EXFlag = 1;
    stAioAttr.u32FrmNum = 30;
    stAioAttr.u32PtNumPerFrm = 320;
    stAioAttr.u32ClkSel = 0;

    /* config aio resample attr if needed */
    if (HI_TRUE == s_bAioReSample)
    {
        AUDIO_RESAMPLE_ATTR_S stReSampleAttr;
        
        stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_32000;
        stAioAttr.u32PtNumPerFrm = 320 * 4; 

        /* ai 32k -> 8k */
        stReSampleAttr.u32InPointNum = 320*4;
        stReSampleAttr.enInSampleRate = AUDIO_SAMPLE_RATE_32000;
        stReSampleAttr.enReSampleType = AUDIO_RESAMPLE_4X1; 
        memcpy(&s_stAiReSmpAttr, &stReSampleAttr, sizeof(AUDIO_RESAMPLE_ATTR_S));

        /* ao 8k -> 32k */
        stReSampleAttr.u32InPointNum = 320;
        stReSampleAttr.enInSampleRate = AUDIO_SAMPLE_RATE_8000;
        stReSampleAttr.enReSampleType = AUDIO_RESAMPLE_1X4;
        memcpy(&s_stAoReSmpAttr, &stReSampleAttr, sizeof(AUDIO_RESAMPLE_ATTR_S));
    }
    
    /* config audio codec */
    SAMPLE_TW2865_CfgAudio(stAioAttr.enSamplerate);
    
    s32AiChnCnt = stAioAttr.u32ChnCnt; 

    /* enable AI channle */
    s32Ret = SAMPLE_StartAi(AiDev, s32AiChnCnt, &stAioAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    /* enable AO channle */
    stAioAttr.u32ChnCnt = 2;
    s32Ret = SAMPLE_StartAo(AoDev, AoChn, &stAioAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    /* bind AI to AO channle */
    s32Ret = HI_MPI_AO_BindAi(AoDev, AoChn, AiDev, AiChn);
    if (s32Ret != HI_SUCCESS)
    {
        return HI_FAILURE;
    }
    
    printf("ai(%d,%d) bind to ao(%d,%d) ok\n", AiDev, AiChn, AoDev, AoChn);

    printf("\nplease press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    HI_MPI_AO_UnBindAi(AoDev, AoChn, AiDev, AiChn);
    SAMPLE_StopAi(AiDev, s32AiChnCnt);
    SAMPLE_StopAo(AoDev, AoChn);
    return HI_SUCCESS;
}


#define SAMPLE_AUDIO_HELP(void)\
{\
    printf("\n/************************************/\n");\
    printf("press sample command as follows!\n");\
    printf("1:  send audio frame to AENC channel form AI, save them\n");\
    printf("2:  read audio stream from file,decode and send AO\n");\
    printf("3:  start AI to AO loop\n");\
    printf("q:  quit whole audio sample\n\n");\
    printf("sample command:");\
}


HI_S32 main(int argc, char *argv[])
{
    char ch;
    HI_S32 s32Ret= HI_SUCCESS;
    VB_CONF_S stVbConf;

    if (argc >= 2)
    {
        /* arg 1 is audio payload type */
        s_enPayloadType = atoi(argv[1]);
        printf("\nargv[1]:%d is payload type ID, suport such type:\n", s_enPayloadType);
        printf("%d:g711a, %d:g711u, %d:adpcm, %d:g726, %d:amr, %d:aac, %d:lpcm\n",
            PT_G711A, PT_G711U, PT_ADPCMA, PT_G726, PT_AMR, PT_AAC, PT_LPCM);
    }
    
    memset(&stVbConf, 0, sizeof(VB_CONF_S));
    if (SAMPLE_InitMPP(&stVbConf))
    {
        return HI_FAILURE;
    }

    SAMPLE_AUDIO_HELP();
    
    while ((ch = getchar()) != 'q')
    {
        switch (ch)
        {
            case '1':
            {
                s32Ret = SAMPLE_AiAenc();/* send audio frame to AENC channel form AI, save them*/
                break;
            }
            case '2':
            {
                s32Ret = SAMPLE_AdecAo();/* read audio stream from file,decode and send AO*/
                break;
            }
            case '3':
            {
                s32Ret = SAMPLE_AiAo();/* AI to AO*/
                break;
            }
            default:
            {
                SAMPLE_AUDIO_HELP();
                break;
            }
        }
        if (s32Ret != HI_SUCCESS)
        {
            break;
        }
    }

    SAMPLE_ExitMPP();

    return HI_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif 
#endif /* End of #ifdef __cplusplus */

