#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"
#include "hifb.h"
#include "vo_open.h"


#define TDE_PRINT printf

#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static const HI_CHAR *pszImageNames[] =
{
    "res/apple.bits",
    "res/applets.bits",
    "res/calendar.bits",
    "res/foot.bits",
    "res/gmush.bits",
    "res/gimp.bits",
    "res/gsame.bits",
    "res/keys.bits"
};

#define N_IMAGES (HI_S32)((sizeof (pszImageNames) / sizeof (pszImageNames[0])))

#define BACKGROUND_NAME  "res/background.bits"

#define PIXFMT  TDE2_COLOR_FMT_ARGB1555
#define BPP     2
#define SCREEN_WIDTH    720
#define SCREEN_HEIGHT   576
#define CYCLE_LEN       60

static HI_S32   g_s32FrameNum;
static TDE2_SURFACE_S g_stScreen[2];
static TDE2_SURFACE_S g_stBackGround;
static TDE2_SURFACE_S g_stImgSur[N_IMAGES];

static HI_S32 TDE_CreateSurfaceByFile(const HI_CHAR *pszFileName, TDE2_SURFACE_S *pstSurface, HI_U8 *pu8Virt)
{
    FILE *fp;
    HI_U32 colorfmt, w, h, stride;

    if((NULL == pszFileName) || (NULL == pstSurface))
    {
        printf("%s, LINE %d, NULL ptr!\n", __FUNCTION__, __LINE__);
        return -1;
    }

    fp = fopen(pszFileName, "rb");
    if(NULL == fp)
    {
        printf("error when open pszFileName %s, line:%d\n", pszFileName, __LINE__);
        return -1;
    }

    fread(&colorfmt, 1, 4, fp);
    fread(&w, 1, 4, fp);
    fread(&h, 1, 4, fp);
    fread(&stride, 1, 4, fp);

    pstSurface->enColorFmt = colorfmt;
    pstSurface->u32Width = w;
    pstSurface->u32Height = h;
    pstSurface->u32Stride = stride;
    pstSurface->u8Alpha0 = 0;
    pstSurface->u8Alpha1 = 0xff;
    pstSurface->bAlphaMax255 = HI_TRUE;

    fread(pu8Virt, 1, stride*h, fp);

    fclose(fp);

    return 0;
}


static HI_VOID circumrotate (HI_U32 u32CurOnShow)
{
    TDE_HANDLE s32Handle;
    TDE2_OPT_S stOpt = {0};
    HI_FLOAT eXMid, eYMid;
    HI_FLOAT eRadius;
    HI_U32 i;
    HI_FLOAT f;
    HI_U32 u32NextOnShow;
    TDE2_RECT_S stSrcRect;
    TDE2_RECT_S stDstRect;
	HI_S32 s32Ret;

    u32NextOnShow = !u32CurOnShow;

    stOpt.enOutAlphaFrom = TDE2_OUTALPHA_FROM_FOREGROUND;
    stOpt.enColorKeyMode = TDE2_COLORKEY_MODE_FOREGROUND;
    stOpt.unColorKeyValue.struCkARGB.stAlpha.bCompIgnore = HI_TRUE;
    f = (float) (g_s32FrameNum % CYCLE_LEN) / CYCLE_LEN;

    stSrcRect.s32Xpos = 0;
    stSrcRect.s32Ypos = 0;
    stSrcRect.u32Width = g_stBackGround.u32Width;
    stSrcRect.u32Height = g_stBackGround.u32Height;

    eXMid = g_stBackGround.u32Width/2.16f;
    eYMid = g_stBackGround.u32Height/2.304f;

    eRadius = MIN (eXMid, eYMid) / 2.0f;

    /* 1. start job */
    s32Handle = HI_TDE2_BeginJob();
    if(HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        TDE_PRINT("start job failed!\n");
        return ;
    }

    /* 2. bitblt background to screen */
    s32Ret = HI_TDE2_QuickCopy(s32Handle, &g_stBackGround, &stSrcRect, &g_stScreen[u32NextOnShow], &stSrcRect);
	if(s32Ret < 0)
	{
        TDE_PRINT("Line:%d,HI_TDE2_QuickCopy failed,ret=0x%x!\n", __LINE__, s32Ret);
		HI_TDE2_CancelJob(s32Handle);
        return ;
	}

    for(i = 0; i < N_IMAGES; i++)
    {
        HI_FLOAT ang;
        HI_FLOAT r;

        stSrcRect.s32Xpos = 0;
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Width = g_stImgSur[i].u32Width;
        stSrcRect.u32Height = g_stImgSur[i].u32Height;

        /* 3. calculate new pisition */
        ang = 2.0f * (HI_FLOAT) M_PI * (HI_FLOAT) i / N_IMAGES - f * 2.0f * (HI_FLOAT) M_PI;
        r = eRadius + (eRadius / 3.0f) * sinf (f * 2.0 * M_PI);

        stDstRect.s32Xpos = eXMid + r * cosf (ang) - g_stImgSur[i].u32Width / 2.0f;;
        stDstRect.s32Ypos = eYMid + r * sinf (ang) - g_stImgSur[i].u32Height / 2.0f;
        stDstRect.u32Width = g_stImgSur[i].u32Width;
        stDstRect.u32Height = g_stImgSur[i].u32Height;

        /* 4. bitblt image to screen */
        s32Ret = HI_TDE2_Bitblit(s32Handle, &g_stScreen[u32NextOnShow], &stDstRect, &g_stImgSur[i], &stSrcRect, &g_stScreen[u32NextOnShow], &stDstRect, &stOpt);
		
		if(s32Ret < 0)
		{
			TDE_PRINT("Line:%d,HI_TDE2_Bitblit failed,ret=0x%x!\n", __LINE__, s32Ret);
			HI_TDE2_CancelJob(s32Handle);
			return ;
		}

    }

    /* 5. submit job */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 10);
	if(s32Ret < 0)
	{
		TDE_PRINT("Line:%d,HI_TDE2_EndJob failed,ret=0x%x!\n", __LINE__, s32Ret);
		HI_TDE2_CancelJob(s32Handle);
		return ;
	}

    g_s32FrameNum++;
    return;
}

HI_S32 TDE_DrawGraphicSample()
{
    HI_U32 u32Size;
    HI_S32 s32Fd;
    HI_U32 u32Times;
    HI_U8* pu8Screen;

    HI_U32 u32PhyAddr;
    HI_S32 s32Ret = -1;
    HI_U32 i = 0;


    struct fb_fix_screeninfo stFixInfo;
    struct fb_var_screeninfo stVarInfo;
    struct fb_bitfield stR32 = {10, 5, 0};
    struct fb_bitfield stG32 = {5, 5, 0};
    struct fb_bitfield stB32 = {0, 5, 0};
    struct fb_bitfield stA32 = {15, 1, 0};

    //HI_UNF_DISPLAY_SetEnable(HI_TRUE);

    /* 1. open tde device */
    HI_TDE2_Open();

    /* 2. framebuffer operation */
    s32Fd = open("/dev/fb0", O_RDWR);
    if (s32Fd == -1)
    {
        printf("open frame buffer device error\n");
        goto FB_OPEN_ERROR;
    }

    stVarInfo.xres_virtual	 	= SCREEN_WIDTH;
    stVarInfo.yres_virtual		= SCREEN_HEIGHT*2;
    stVarInfo.xres      		= SCREEN_WIDTH;
    stVarInfo.yres      		= SCREEN_HEIGHT;
    stVarInfo.activate  		= FB_ACTIVATE_NOW;
    stVarInfo.bits_per_pixel	= 16;
    stVarInfo.xoffset = 0;
    stVarInfo.yoffset = 0;
    stVarInfo.red   = stR32;
    stVarInfo.green = stG32;
    stVarInfo.blue  = stB32;
    stVarInfo.transp = stA32;

    if (ioctl(s32Fd, FBIOPUT_VSCREENINFO, &stVarInfo) < 0)
    {
        printf("process frame buffer device error\n");
        goto FB_PROCESS_ERROR0;
    }

    if (ioctl(s32Fd, FBIOGET_FSCREENINFO, &stFixInfo) < 0)
    {
        printf("process frame buffer device error\n");
        goto FB_PROCESS_ERROR0;
    }

    u32Size 	= stFixInfo.smem_len;
    u32PhyAddr  = stFixInfo.smem_start;
    pu8Screen   = mmap(NULL, u32Size, PROT_READ|PROT_WRITE, MAP_SHARED, s32Fd, 0);

    memset(pu8Screen, 0xff, SCREEN_WIDTH*SCREEN_HEIGHT*4);

    /* 3. create surface */
    g_stScreen[0].enColorFmt = PIXFMT;
    g_stScreen[0].u32PhyAddr = u32PhyAddr;
    g_stScreen[0].u32Width = SCREEN_WIDTH;
    g_stScreen[0].u32Height = SCREEN_HEIGHT;
    g_stScreen[0].u32Stride = stFixInfo.line_length;
    g_stScreen[0].bAlphaMax255 = HI_TRUE;
	
    g_stScreen[1] = g_stScreen[0];
    g_stScreen[1].u32PhyAddr = g_stScreen[0].u32PhyAddr + g_stScreen[0].u32Stride * g_stScreen[0].u32Height;

    g_stBackGround.u32PhyAddr = g_stScreen[1].u32PhyAddr + g_stScreen[1].u32Stride * g_stScreen[1].u32Height;
    TDE_CreateSurfaceByFile(BACKGROUND_NAME, &g_stBackGround, pu8Screen + ((HI_U32)g_stBackGround.u32PhyAddr - u32PhyAddr));
    g_stImgSur[0].u32PhyAddr = g_stBackGround.u32PhyAddr + g_stBackGround.u32Stride * g_stBackGround.u32Height;
    for(i = 0; i < N_IMAGES - 1; i++)
    {
      TDE_CreateSurfaceByFile(pszImageNames[i], &g_stImgSur[i], pu8Screen + ((HI_U32)g_stImgSur[i].u32PhyAddr - u32PhyAddr));
      g_stImgSur[i+1].u32PhyAddr = g_stImgSur[i].u32PhyAddr + g_stImgSur[i].u32Stride * g_stImgSur[i].u32Height;
    }
    TDE_CreateSurfaceByFile(pszImageNames[i], &g_stImgSur[i], pu8Screen + ((HI_U32)g_stImgSur[i].u32PhyAddr - u32PhyAddr));


    g_s32FrameNum = 0;

    /* 3. use tde and framebuffer to realize rotational effect */
    for (u32Times = 0; u32Times < 1000; u32Times++)
    {
            circumrotate(u32Times%2);
            stVarInfo.yoffset = (u32Times%2)?0:576;

            /*set frame buffer start position*/
            if (ioctl(s32Fd, FBIOPAN_DISPLAY, &stVarInfo) < 0)
            {
                TDE_PRINT("process frame buffer device error\n");
                goto FB_PROCESS_ERROR1;
            }
    }

    s32Ret = 0;
FB_PROCESS_ERROR1:
    munmap(pu8Screen, u32Size);
FB_PROCESS_ERROR0:
    close(s32Fd);
FB_OPEN_ERROR:
    HI_TDE2_Close();

    //HI_UNF_DISPLAY_SetEnable(HI_FALSE);

    return s32Ret;
}


/*****************************************************************************
description: 	this sample shows how to use TDE interface to draw graphic.
                
note	   :    for showing graphic layer, VO device should be enabled first.
				This sample draws graphic on layer G0 which belongs to HD VO device.
				(here we insmod HiFB.ko like 'insmod hifb.ko video="hifb:vram0_size=XXX" '
				 so opening hifb sub-device '/dev/fb0' means to opening G0,
				 and G0 was fixed binded to HD VO device in Hi3520)
*****************************************************************************/

int main()
{
	HI_S32 s32Ret = 0;
	
    /*1 enable Vo device HD first*/
	if(HI_SUCCESS != MppSysInit())
		return -1;
	
	if(HI_SUCCESS != EnableVoDev(SD))
	{
		MppSysExit();
		return -1;
	}
	
	
	/*2 run tde sample which draw grahpic on HiFB memory*/
	s32Ret = TDE_DrawGraphicSample();
	if(s32Ret != HI_SUCCESS)
	{
		goto err;
	}

	err:
	DisableVoDev(SD);
	MppSysExit();

	return 0;

}		


