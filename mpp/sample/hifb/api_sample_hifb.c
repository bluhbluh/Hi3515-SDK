#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include "hifb.h"
#include "vo_open.h"

#define IMAGE_WIDTH     640
#define IMAGE_HEIGHT    352
#define IMAGE_SIZE      (640*352*2)
#define IMAGE_NUM       14
#define IMAGE_PATH		"./res/%d.bits"

#define VIR_SCREEN_WIDTH	640					/*virtual screen width*/
#define VIR_SCREEN_HEIGHT	IMAGE_HEIGHT*2		/*virtual screen height*/

static struct fb_bitfield g_r16 = {10, 5, 0};
static struct fb_bitfield g_g16 = {5, 5, 0};
static struct fb_bitfield g_b16 = {0, 5, 0};
static struct fb_bitfield g_a16 = {15, 1, 0};

int RunHiFB(int layer)
{
    int fd;
    int i;
	int j;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
	HI_U32 u32FixScreenStride = 0;
    unsigned char *pShowScreen;
    unsigned char *pHideScreen;
	HIFB_ALPHA_S stAlpha;
    HIFB_POINT_S stPoint = {40, 112};
    FILE *fp;
    char file[12] = "/dev/fb0";
    char image_name[128];
	HI_CHAR *pDst = NULL;

    switch (layer)
    {
        case 0 :
            strcpy(file, "/dev/fb0");
            break;
        case 1 :
            strcpy(file, "/dev/fb1");
            break;
        case 2 :
            strcpy(file, "/dev/fb2");
            break;
        case 3 :
            strcpy(file, "/dev/fb3");
            break;
        case 4 :
            strcpy(file, "/dev/fb4");
            break;
        default:
            strcpy(file, "/dev/fb0");
            break;
    }

    printf(":>>>> Graphics on %d layer %s\n", layer, file);

    /* 1. open framebuffer device overlay 0 */
    fd = open(file, O_RDWR, 0);
    if(fd < 0)
    {
        printf("open %s failed!\n",file);
        return -1;
    }

    /* 2. set the screen original position */
    if (ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        printf("set screen original show position failed!\n");
        close(fd);
        return -1;
    }

	/* 3.set alpha */
	stAlpha.bAlphaEnable = HI_FALSE;
	stAlpha.bAlphaChannel = HI_FALSE;
	stAlpha.u8Alpha0 = 0xff;
	stAlpha.u8Alpha1 = 0x8f;
    stAlpha.u8GlobalAlpha = 0x80;
	if (ioctl(fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
	{
	    printf("Set alpha failed!\n");
        close(fd);
        return -1;
	}

    /* 4. get the variable screen info */
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
   	    printf("Get variable screen info failed!\n");
        close(fd);
        return -1;
    }

    /* 5. modify the variable screen info
          the screen size: IMAGE_WIDTH*IMAGE_HEIGHT
          the virtual screen size: VIR_SCREEN_WIDTH*VIR_SCREEN_HEIGHT
          (which equals to VIR_SCREEN_WIDTH*(IMAGE_HEIGHT*2))
          the pixel format: ARGB1555
    */
    var.xres_virtual = VIR_SCREEN_WIDTH;
	var.yres_virtual = VIR_SCREEN_HEIGHT;
	var.xres = IMAGE_WIDTH;
    var.yres = IMAGE_HEIGHT;

    var.transp= g_a16;
    var.red = g_r16;
    var.green = g_g16;
    var.blue = g_b16;
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_FORCE;
    /* 6. set the variable screeninfo */
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &var) < 0)
    {
   	    printf("Put variable screen info failed!\n");
        close(fd);
        return -1;
    }

    /* 7. get the fix screen info */
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
   	    printf("Get fix screen info failed!\n");
        close(fd);
        return -1;
    }
	u32FixScreenStride = fix.line_length;	/*fix screen stride*/

    /* 8. map the physical video memory for user use */
    pShowScreen = mmap(NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
   	    printf("mmap framebuffer failed!\n");
        close(fd);
        return -1;
    }

    pHideScreen = pShowScreen + u32FixScreenStride*IMAGE_HEIGHT;
    memset(pShowScreen, 0, u32FixScreenStride*IMAGE_HEIGHT);
    /* 9. load the bitmaps from file to hide screen and set pan display the hide screen */
    for(i = 0; i < IMAGE_NUM; i++)
    {
        sprintf(image_name, IMAGE_PATH, i);
        fp = fopen(image_name, "rb");
        if(NULL == fp)
        {
            printf("Load %s failed!\n", image_name);
            munmap(pShowScreen, fix.smem_len);
            close(fd);
            return -1;
        }

		/*read image data from file into fix screen memory
		note: we should read image data into memory line by line,
		      because fix screen memory's stride equals to fix.line_length,
		      which may not equals to image stride of file*/
		pDst = pHideScreen;
		for(j=0; j<IMAGE_HEIGHT; j++)
		{
			fread(pDst, 1, IMAGE_WIDTH*2, fp);
			pDst += u32FixScreenStride;
		}

        fclose(fp);

        if(i%2)
        {
            var.yoffset = 0;
            pHideScreen = pShowScreen + u32FixScreenStride*IMAGE_HEIGHT;
        }
        else
        {
            var.yoffset = IMAGE_HEIGHT;
            pHideScreen = pShowScreen;
        }

        if (ioctl(fd, FBIOPAN_DISPLAY, &var) < 0)
        {
            printf("FBIOPAN_DISPLAY failed!\n");
            munmap(pShowScreen, fix.smem_len);
            close(fd);
            return -1;
        }
		usleep(200*1000);
    }

    printf("Enter to quit!\n");
    getchar();

    /* 10.unmap the physical memory */
    munmap(pShowScreen, fix.smem_len);

    /* 11. close the framebuffer device */
    close(fd);

    return 0;
}

/*****************************************************************************
description: 	this sample shows how to use HiFB interface
                to show GUI on video output device(VO)
note	   :    for showing graphic layer, VO device should be enabled first.
				This sample draws graphic on layer G0 which belongs to HD VO device.
				(here we insmod HiFB.ko like 'insmod hifb.ko video="hifb:vram0_size=XXX" '
				 so opening hifb sub-device '/dev/fb0' means to opening G0,
				 and G0 was fixed binded to HD VO device in Hi3520)
*****************************************************************************/

int main(int argc, char *argv[])
{
	HI_S32 s32Ret = 0;
    VOU_DEV_E VoDev = HD;
    int layer = 0;

    /*1 enable Vo device HD first*/
	if(HI_SUCCESS != MppSysInit())
		return -1;

    if (argc > 1)
    {
        layer = atoi(argv[1]);
        if (layer < 0 || layer > 4)
        {
            layer = 0;
        }

        VoDev = SD;
    }

	if(HI_SUCCESS != EnableVoDev(HD))
	{
		MppSysExit();
		return -1;
	}

	if(HI_SUCCESS != EnableVoDev(SD))
	{
		MppSysExit();
		return -1;
	}

	/*2 run hifb*/
	s32Ret = RunHiFB(layer);
	if(s32Ret != HI_SUCCESS)
	{
		goto err;
	}

	err:
	DisableVoDev(HD);
	DisableVoDev(SD);
	MppSysExit();

	return 0;

}

