#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "mpi_sys.h"
#include "hi_comm_vb.h"
#include "mpi_vb.h"
#include "hi_comm_vo.h"
#include "mpi_vo.h"

#define MAX_FRM_CNT 256

static HI_S32 s_s32MemDev = 0;

HI_S32 memopen( void )
{
    if (s_s32MemDev <= 0)
    {
        s_s32MemDev = open ("/dev/mem", O_CREAT|O_RDWR|O_SYNC);
        if (s_s32MemDev <= 0)
        {
            return -1;
        }
    }
    return 0;
}

HI_VOID memclose()
{
	close(s_s32MemDev);
}

void * memmap( HI_U32 u32PhyAddr, HI_U32 u32Size )
{
    HI_U32 u32Diff;
    HI_U32 u32PagePhy;
    HI_U32 u32PageSize;
    HI_U8 * pPageAddr;

    u32PagePhy = u32PhyAddr & 0xfffff000;
    u32Diff = u32PhyAddr - u32PagePhy;

    /* size in page_size */
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;
    pPageAddr = mmap ((void *)0, u32PageSize, PROT_READ|PROT_WRITE,
                                    MAP_SHARED, s_s32MemDev, u32PagePhy);
    if (MAP_FAILED == pPageAddr )
    {
            return NULL;
    }
    return (void *) (pPageAddr + u32Diff);
}

HI_S32 memunmap(HI_VOID* pVirAddr, HI_U32 u32Size )
{
    HI_U32 u32PageAddr;
    HI_U32 u32PageSize;
    HI_U32 u32Diff;

    u32PageAddr = (((HI_U32)pVirAddr) & 0xfffff000);
    /* size in page_size */
    u32Diff     = (HI_U32)pVirAddr - u32PageAddr;
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;

    return munmap((HI_VOID*)u32PageAddr, u32PageSize);
}



/* sp420 转存为 p420 ; sp422 转存为 p422  */
void sample_yuv_dump(VIDEO_FRAME_S * pVBuf, FILE *pfd)
{
    unsigned int w, h;
    char * pVBufVirt_Y;
    char * pVBufVirt_C;
    char * pMemContent;
    unsigned char TmpBuff[1024];
    HI_U32 phy_addr,size;
	HI_CHAR *pUserPageAddr[2];
    PIXEL_FORMAT_E  enPixelFormat = pVBuf->enPixelFormat;
    HI_U32 u32UvHeight;/* 存为planar 格式时的UV分量的高度 */

    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat)
    {
        size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*3/2;
        u32UvHeight = pVBuf->u32Height/2;
    }
    else
    {
        size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*2;
        u32UvHeight = pVBuf->u32Height;
    }

    phy_addr = pVBuf->u32PhyAddr[0];

    //printf("phy_addr:%x, size:%d\n", phy_addr, size);
    pUserPageAddr[0] = (HI_U8 *) memmap(phy_addr, size);
    if (NULL == pUserPageAddr[0])
    {
        return;
    }
    //printf("stride: %d,%d\n",pVBuf->u32Stride[0],pVBuf->u32Stride[1] );

	pVBufVirt_Y = pUserPageAddr[0];
	pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0])*(pVBuf->u32Height);

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Y......");
    fflush(stderr);
    for(h=0; h<pVBuf->u32Height; h++)
    {
        pMemContent = pVBufVirt_Y + h*pVBuf->u32Stride[0];
        fwrite(pMemContent, pVBuf->u32Width, 1, pfd);
    }
    fflush(pfd);


    /* save U ----------------------------------------------------------------*/
    fprintf(stderr, "U......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        pMemContent += 1;

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

    /* save V ----------------------------------------------------------------*/
    fprintf(stderr, "V......");
    fflush(stderr);
    for(h=0; h<u32UvHeight; h++)
    {
        pMemContent = pVBufVirt_C + h*pVBuf->u32Stride[1];

        for(w=0; w<pVBuf->u32Width/2; w++)
        {
            TmpBuff[w] = *pMemContent;
            pMemContent += 2;
        }
        fwrite(TmpBuff, pVBuf->u32Width/2, 1, pfd);
    }
    fflush(pfd);

    fprintf(stderr, "done %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    memunmap(pUserPageAddr[0], size);
}

HI_S32 SAMPLE_MISC_VoDump(VO_DEV VoDev, HI_U32 u32Cnt)
{
    int i, j;
    int ret;
    VIDEO_FRAME_INFO_S stFrame;
    VIDEO_FRAME_INFO_S astFrame[MAX_FRM_CNT];
    HI_CHAR szYuvName[128];
    HI_CHAR szPixFrm[10];
    FILE *pfd;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    printf("usage: ./vo_dump [vodev] [frmcnt]. sample: ./vo_dump 0 5\n\n");

    ret = HI_MPI_VO_GetScreenFrame(VoDev, &stFrame);
    if (ret != HI_SUCCESS)
    {
        printf("HI_MPI_VO(%d)_GetScreenFrame errno %#x\n", VoDev, ret);
        return -1;
    }

    /* make file name */
    strcpy(szPixFrm,
    (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == stFrame.stVFrame.enPixelFormat)?"p420":"p422");
    sprintf(szYuvName, "./vo(%d)_%d_%d_%s_%d.yuv",VoDev,
        stFrame.stVFrame.u32Width, stFrame.stVFrame.u32Height,szPixFrm,u32Cnt);
	printf("Dump YUV frame of vo(%d) to file: \"%s\"\n",VoDev, szYuvName);

    /* open file */
    pfd = fopen(szYuvName, "wb");

    if (NULL == pfd)
    {
        return -1;
    }

    /* get VI frame  */
    for (i=0; i<u32Cnt; i++)
    {
        if (HI_MPI_VO_GetScreenFrame(VoDev, &astFrame[i])<0)
        {
            printf("get vo(%d) frame err\n", VoDev);
            printf("only get %d frame\n", i);
            break;
        }
#if 0
        if (astFrame[i].stVFrame.u32Width != stFrame.stVFrame.u32Width
            || astFrame[i].stVFrame.u32Height != stFrame.stVFrame.u32Height)
        {
            printf("vi size has changed when frame %d, break\n",i);
            break;
        }
#endif
    }

    memopen();

    for(j=0; j<i; j++)
    {
        /* save VO frame to file */
		sample_yuv_dump(&astFrame[j].stVFrame, pfd);

        /* release frame after using */
        HI_MPI_VO_ReleaseScreenFrame(VoDev, &astFrame[j]);
    }

    memclose();

    fclose(pfd);
    HI_MPI_VO_ReleaseScreenFrame(VoDev, &stFrame);

	return 0;
}

HI_S32 main(int argc, char *argv[])
{
    VO_DEV VoDev = 0;
    HI_U32 u32FrmCnt = 1;

    if (argc > 1)/* VO设备号*/
    {
        VoDev = atoi(argv[1]);
    }

    if (argc > 2)
    {
        u32FrmCnt = atoi(argv[2]);/* 需要采集的帧数目*/
    }

    SAMPLE_MISC_VoDump(VoDev, u32FrmCnt);

	return HI_SUCCESS;
}


