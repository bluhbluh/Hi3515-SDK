/******************************************************************************

  Copyright (C), 2009-2060, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : api_sample_hifb_switchGlayer.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/9/21
  Last Modified :
  Description   : this sample show hows to use each graphic layer
      			  and how to switch cursor between HD and AD output
      			  device
  Function List :
  
******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <pthread.h>
#include "hifb.h"
#include "mkp_vd.h"
#include "vo_open.h"

typedef enum hiVOU_GLAYER_E
{
	G0 = 0,
	G1 = 1,
	G2 = 2,
	G3 = 3,
	HC = 4,
	VOU_GLAYER_BUTT
}VOU_GLAYER_E;

/*picture info for Glayer to show*/
typedef struct hiPICFILE_INFO
{
	HI_S32 s32PicNum;				/*picture number*/
	HI_CHAR aszFileName[2][128];	/*picture name*/
	HI_U32 u32PicWidth;				/*picture width*/
	HI_U32 u32PicHeight;			/*picture height*/
}PICFILE_INFO;

typedef struct hiPROC_SHOW_PARA_S
{
	VOU_GLAYER_E Glayer;
	HI_CHAR aszHifbDevName[128];
	PICFILE_INFO stPicInfo;
}PROC_SHOW_PARA_S;

pthread_t g_s32ShowThdId[5] = {0};
HI_S32 g_s32GlayerFd[5] = {-1};
HI_BOOL g_bShowFlag[5] = {0};

static PROC_SHOW_PARA_S s_astShowPara[5] =
{
		{
			.Glayer = G0,
			.aszHifbDevName = "/dev/fb0",
			.stPicInfo = {2,"./res/G0-1.bits","./res/G0-2.bits",1024,768},
		},
		{
			.Glayer = G1,
			.aszHifbDevName = "/dev/fb1",
			.stPicInfo = {2,"./res/G1-1.bits","./res/G1-2.bits",720,576},
		},
		{
			.Glayer = G2,
			.aszHifbDevName = "/dev/fb2",
			.stPicInfo = {2,"./res/G2-1.bits","./res/G2-2.bits",720,576},
		},
		{
			.Glayer = G3,
			.aszHifbDevName = "/dev/fb3",
			.stPicInfo = {2,"./res/G3-1.bits","./res/G3-2.bits",720,576},
		},
		{
			.Glayer = HC,
			.aszHifbDevName = "/dev/fb4",
			.stPicInfo = {1,"./res/cursor.bits","",32,32},
		}
};

static struct fb_bitfield g_r16 = {10, 5, 0};
static struct fb_bitfield g_g16 = {5, 5, 0};
static struct fb_bitfield g_b16 = {0, 5, 0};
static struct fb_bitfield g_a16 = {15, 1, 0};

static HI_S32 ReadImgToFb(const HI_CHAR *pszFileName,HI_U8 *pu8Virt, HI_U32 u32FixScreenStride)
{
    FILE *fp;
    HI_U32 colorfmt, w, h, stride;
	HI_S32 j = 0;
	HI_CHAR *pDstVir = NULL;

    if((NULL == pszFileName) || (NULL == pu8Virt))
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

	pDstVir = pu8Virt;
	for(j=0; j<h; j++)
	{
		fread(pDstVir, 1, stride, fp);
		pDstVir += u32FixScreenStride;
	}
	
    fclose(fp);

    return 0;
}

/******************************************************************************
	Function : 		ProcShowCursor
	Description :	use graphic layer 4th (G4) to show cursor: 
	                1)just use one buff: draw cursor in G4 buff,and 
	                  notify vo to show this buff
	                2)move cursor by changing G4 show position (through 
	                  HiFB cmd FBIOPUT_SCREEN_ORIGIN_HIFB)
				    
*******************************************************************************/
HI_VOID * ProcShowCursor(HI_VOID *para)
{
	int ret;
    int fd;
    int i;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
	HI_U32 u32FixScreenStride = 0;
    unsigned char *pShowScreen;
	HIFB_POINT_S stPoint = {40,40};
	HIFB_ALPHA_S stAlpha = {0};
	HIFB_COLORKEY_S stColorKey = {0};
	HI_S32 Layer = 0;
	PROC_SHOW_PARA_S *pShowPara = NULL;
	PICFILE_INFO *pstPicInfo = NULL;
	HI_U32 u32PicWidht = 0;
	HI_U32 u32PicHeight= 0;
	RECT_S stCursorMoveRect = {40,40,500,400};	/*cursor moved in this square*/
	
	if(NULL == para)
	{
		printf("para is NULL\n");
		return NULL;
	}
	pShowPara = (PROC_SHOW_PARA_S *)para;
	Layer = pShowPara->Glayer;
	pstPicInfo = &(pShowPara->stPicInfo);
	
    /* 1. open framebuffer device overlay 0 */
    fd = open(pShowPara->aszHifbDevName, O_RDWR, 0);
    if(fd < 0)
    {
        printf("open %s failed!\n",pShowPara->aszHifbDevName);
        return NULL;
    }

    /* 2. set the screen original position */
    if (ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        printf("set screen original show position failed!\n");
        close(fd);
        return NULL;
    }
	printf("set screen original show position (%d %d)ok!\n",stPoint.u32PosX,stPoint.u32PosY);

	/* 3.set alpha: not transparent */
	stAlpha.bAlphaEnable = HI_FALSE;
	if (ioctl(fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
	{
	    printf("Set alpha failed!\n");
        close(fd);
        return NULL;
	}

	/*set ColorKey: blue is key color*/
	stColorKey.bKeyEnable = HI_TRUE;
	stColorKey.u32Key = 0x1F;		/*key color is blue */
	stColorKey.bMaskEnable = HI_TRUE;
	stColorKey.u8BlueMask = 0xFF;
	stColorKey.u8GreenMask = 0x00;
	stColorKey.u8RedMask = 0x00;
	if (ioctl(fd, FBIOPUT_COLORKEY_HIFB,  &stColorKey) < 0)
	{
	    printf("Set alpha failed!\n");
        close(fd);
        return NULL;
	}
	
    /* 4. get the variable screen info */
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
   	    printf("Get variable screen info failed!\n");
        close(fd);
        return NULL;
    }

    /* 5. modify the variable screen info
          the pixel format: ARGB1555
    */
    u32PicWidht  = pstPicInfo->u32PicWidth;
	u32PicHeight = pstPicInfo->u32PicHeight;

    var.xres = u32PicWidht;
    var.yres = u32PicHeight;
    var.xres_virtual = var.xres;
	var.yres_virtual = var.yres;
	
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
        return NULL;
    }

    /* 7. get the fix screen info */
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
   	    printf("Get fix screen info failed!\n");
        close(fd);
        return NULL;
    }
	u32FixScreenStride = fix.line_length;	/*fix screen stride*/

	printf("Glayer:%d picw:%d pich:%d s:%d\n ",Layer,u32PicWidht,u32PicHeight,u32FixScreenStride);
	
    /* 8. map the physical video memory for user use */
    pShowScreen = mmap(NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
   	    printf("mmap framebuffer failed!\n");
        close(fd);
        return NULL;
    }
	
    memset(pShowScreen, 0, u32FixScreenStride*u32PicHeight);

	ret = ReadImgToFb(pstPicInfo->aszFileName[0],pShowScreen,u32FixScreenStride);
	if(0 != ret)
	{
		printf("read pic:%s to fb mem failed\n",pstPicInfo->aszFileName[i]);
		munmap(pShowScreen, fix.smem_len);
        close(fd);
		return NULL;
	}
			
	/*move cursor */
	while(HI_TRUE == g_bShowFlag[Layer])
	{
		stPoint.u32PosX =  (stPoint.u32PosX >= stCursorMoveRect.u32Width)?
							stCursorMoveRect.s32X: (stPoint.u32PosX+10);
		stPoint.u32PosY =  (stPoint.u32PosY >= stCursorMoveRect.u32Height)?
							stCursorMoveRect.s32Y: (stPoint.u32PosY+10);
		
		if (ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
	    {
	        printf("set screen original show position failed!\n");
			munmap(pShowScreen, fix.smem_len);
	        close(fd);
	        return NULL;
	    }	
    	
		usleep(500*1000);
	}

    /* 10.unmap the physical memory */
    munmap(pShowScreen, fix.smem_len);

    /* 11. close the framebuffer device */
    close(fd);

    return NULL;
}		

HI_VOID * ProcShowPicOnGLayer(HI_VOID *para)
{
	int ret;
    int fd;
    int i;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
	HI_U32 u32FixScreenStride = 0;
    unsigned char *pShowScreen;
    unsigned char *pHideScreen;
	HIFB_POINT_S stPoint = {0,0};
	HIFB_ALPHA_S stAlpha = {0};
	HI_CHAR *pDst = NULL;
	HI_S32 Layer = 0;
	PROC_SHOW_PARA_S *pShowPara = NULL;
	PICFILE_INFO *pstPicInfo = NULL;
	HI_U32 u32PicWidht = 0;
	HI_U32 u32PicHeight= 0;
	
	if(NULL == para)
	{
		printf("para is NULL\n");
		return NULL;
	}
	pShowPara = (PROC_SHOW_PARA_S *)para;
	Layer = pShowPara->Glayer;
	pstPicInfo = &(pShowPara->stPicInfo);
	
    /* 1. open framebuffer device overlay 0 */
    fd = open(pShowPara->aszHifbDevName, O_RDWR, 0);
    if(fd < 0)
    {
        printf("open %s failed!\n",pShowPara->aszHifbDevName);
        return NULL;
    }

    /* 2. set the screen original position */
    if (ioctl(fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        printf("set screen original show position failed!\n");
        close(fd);
        return NULL;
    }
	printf("set screen original show position (%d %d)ok!\n",stPoint.u32PosX,stPoint.u32PosY);

	/* 3.set alpha: not transparent */
	stAlpha.bAlphaEnable = HI_FALSE;
	if (ioctl(fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0)
	{
	    printf("Set alpha failed!\n");
        close(fd);
        return NULL;
	}
	
    /* 4. get the variable screen info */
    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
   	    printf("Get variable screen info failed!\n");
        close(fd);
        return NULL;
    }

    /* 5. modify the variable screen info
          the pixel format: ARGB1555
          note: here use pingpang buff mechanism
    */
    u32PicWidht  = pstPicInfo->u32PicWidth;
	u32PicHeight = pstPicInfo->u32PicHeight;
	
    var.xres = u32PicWidht;
    var.yres = u32PicHeight;
    var.xres_virtual = var.xres;
	var.yres_virtual = var.yres * 2;
	
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
        return NULL;
    }

    /* 7. get the fix screen info */
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
   	    printf("Get fix screen info failed!\n");
        close(fd);
        return NULL;
    }
	u32FixScreenStride = fix.line_length;	/*fix screen stride*/

    /* 8. map the physical video memory for user use */
    pShowScreen = mmap(NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
   	    printf("mmap framebuffer failed!\n");
        close(fd);
        return NULL;
    }

    pHideScreen = pShowScreen + u32FixScreenStride*u32PicHeight;
    memset(pShowScreen, 0, u32FixScreenStride*u32PicHeight);

	while(HI_TRUE == g_bShowFlag[Layer])
	{
    	/* 9. load the bitmaps from file to hide screen and set pan display the hide screen */
    	for(i = 0; i < pstPicInfo->s32PicNum; i++)
	    {
			/*read image data from file into fix screen memory
			note: we should read image data into memory line by line,
			      because fix screen memory's stride equals to fix.line_length,
			      which may not equals to image stride of file*/
			pDst = pHideScreen;
			ret = ReadImgToFb(pstPicInfo->aszFileName[i],pDst,u32FixScreenStride);
			if(0 != ret)
			{
				printf("read pic:%s to fb mem failed\n",pstPicInfo->aszFileName[i]);
				munmap(pShowScreen, fix.smem_len);
	            close(fd);
				return NULL;
			}
			
	        if(i%2)
	        {
	            var.yoffset = 0;
	            pHideScreen = pShowScreen + u32FixScreenStride*u32PicHeight;
	        }
	        else
	        {
	            var.yoffset = u32PicHeight;
	            pHideScreen = pShowScreen;
	        }

	        if (ioctl(fd, FBIOPAN_DISPLAY, &var) < 0)
	        {
	            printf("FBIOPAN_DISPLAY failed!\n");
	            munmap(pShowScreen, fix.smem_len);
	            close(fd);
	            return NULL;
	        }
			usleep(1000*1000);

	    }
	}

    /* 10.unmap the physical memory */
    munmap(pShowScreen, fix.smem_len);

    /* 11. close the framebuffer device */
    close(fd);

    return NULL;
}		

HI_VOID StartShowGlayer(HI_S32 Layer)
{
	if(g_s32ShowThdId[Layer] != 0)
	{
		printf("not allow to create thread of layer:%d, "\
			"there is still a thread %lu running\n",Layer,g_s32ShowThdId[Layer]);
		return;
	}
	
	g_bShowFlag[Layer] = HI_TRUE;
	if(HC != Layer)
	{
		pthread_create(&g_s32ShowThdId[Layer],NULL,ProcShowPicOnGLayer,&s_astShowPara[Layer]);
	}
	else
	{
		pthread_create(&g_s32ShowThdId[Layer],NULL,ProcShowCursor,&s_astShowPara[Layer]);
	}
	
	printf("success to create thread %lu to show layer:%d\n",g_s32ShowThdId[Layer],Layer);
	
}

HI_VOID StopShowGlayer(HI_S32 Layer)
{
	g_bShowFlag[Layer] = HI_FALSE;
	pthread_join(g_s32ShowThdId[Layer],NULL);

	printf("thread %lu of layer %d showing withdraw\n",g_s32ShowThdId[Layer],Layer);
	
	g_s32ShowThdId[Layer] = 0;
}

/*****************************************************************************
*description: 	this sample show hows to use each graphic layer
*      			and how to switch cursor between HD and AD output device
*****************************************************************************/
int main(int argc, char *argv[])
{
	int fd;
	HI_S32 s32Ret = 0;
	VD_BIND_S stBind;
	HI_BOOL bLastShowOnHd = HI_FALSE;
	char ch = '0';
	#ifdef hi3520
	char ping[1024] =   "\n/**************************************/\n"
						"q      --> quit\n"
						"other  --> switch HC between HD and AD\n"
						"please enter order:\n"
						"/**************************************/\n";
	#else
	char ping[1024] =   "\n/**************************************/\n"
						"q      --> quit\n"
						"other  --> switch HC between HD and SD\n"
						"please enter order:\n"
						"/**************************************/\n";
	#endif
	
	MppSysExit();
	MppSysInit();
		
    /*---------1 bind graphic layer G4(here used to show cursor) to HD, defaultly----------*/
	/*1.1 open vd fd*/
	fd = open("/dev/vd", O_RDWR, 0);\
    if(fd < 0)
    {
        printf("open vd failed!\n");
        return -1;
    }
	/*1.2 bind HC to HD*/
	stBind.s32GraphicId = HC;
	stBind.DevId = HD;
	if (ioctl(fd, VD_SET_GRAPHIC_BIND, &stBind) != HI_SUCCESS)
    {
        printf("[fd:%d]set bind glayer %d to dev %d failed!\n",fd,HC,HD);
        close(fd);
        return -1;
    }
	bLastShowOnHd = HI_TRUE;	/*now HC will be showed on HD*/

	/*---------2 enable video ouput device ----------
	hi3520 : HD AD SD
	hi3515 : HD SD
	*/
	s32Ret |=  EnableVoDev(HD);
	s32Ret |=  EnableVoDev(SD);
	#ifdef hi3520
	s32Ret |=  EnableVoDev(AD);
	#endif
	if(HI_SUCCESS != s32Ret)
	{
		close(fd);
    	return -1;
	}
	

	/*---------3 start Glayer --------------------------------
	  hi3520  G0: fixed showed on HD
	          G2: fixed showed on AD
	          G3: fixed showed on SD
	          HC: can be binded to HD or AD
	          
	  hi3515  G0: fixed showed on HD
	          G2: fixed showed on SD
	          HC: can be binded to HD or SD        
	*/
	StartShowGlayer(G0);
	StartShowGlayer(G2);
	StartShowGlayer(HC);
	
    #ifdef hi3520
	StartShowGlayer(G3);
	#endif
	
	/*--------4 to switch HC between HD and AD--------------*/
	printf("%s",ping);
	while((ch = getchar())!= 'q')
	{
		/*4.1 stop HC layer first*/
		StopShowGlayer(HC);

		/*4.2 bind HC on another vo device*/
		stBind.s32GraphicId = HC;
		#ifdef hi3520
		stBind.DevId = (HI_TRUE == bLastShowOnHd)?AD:HD; 
		#else
		stBind.DevId = (HI_TRUE == bLastShowOnHd)?SD:HD; 
		#endif
		if (ioctl(fd, VD_SET_GRAPHIC_BIND, &stBind) != HI_SUCCESS)
	    {
	        printf("[fd:%d]set bind glayer %d to dev %d failed!\n",fd,HC,HD);
	    }
		
		/*4.1 start to show HC layer again*/
		StartShowGlayer(HC);
		
		bLastShowOnHd = !bLastShowOnHd; /*record last device on which HC showed*/
		
		printf("%s",ping);
	}

	/*--------5 close all Glayer--------------*/
	StopShowGlayer(G0);
	StopShowGlayer(G2);
	StopShowGlayer(HC);

	#ifdef hi3520
	StopShowGlayer(G3);
	#endif
	
	/*---------6 disable video ouput device---------
	hi3520 : HD AD SD
	hi3515 : HD SD
	*/

	s32Ret |=  DisableVoDev(HD);
	s32Ret |=  DisableVoDev(SD);
	#ifdef hi3520
	s32Ret |=  DisableVoDev(AD);
	#endif
	if(HI_SUCCESS != s32Ret)
	{
		close(fd);
    	return -1;
	}
	
	
	/*---------7 close vd fd----------*/
	close(fd);

	MppSysExit();
	
    return 0;
}

