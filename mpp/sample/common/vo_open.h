/******************************************************************************
 
  Copyright (C), 2009-2060, Hisilicon Tech. Co., Ltd.
 
 ******************************************************************************
  File Name     : vo_open.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/05/19
  Description   : 
  History       :
******************************************************************************/

#ifndef __VO_OPEN_H__
#define __VO_OPEN_H__

typedef enum hiVOU_DEV_E
{
	HD = 0,
	AD = 1,
	SD = 2,
	VOU_DEV_BUTT
}VOU_DEV_E;

/*init mpp system*/
HI_S32 MppSysInit(HI_VOID);

/*exit mpp system*/
HI_VOID MppSysExit(HI_VOID);

/*enable vo device*/
HI_S32 EnableVoDev(HI_S32 DevId);

/*disable vo device*/
HI_S32 DisableVoDev(HI_S32 DevId);




#endif /* end of #ifndef __VO_OPEN_H__ */
