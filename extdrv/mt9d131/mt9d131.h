#ifndef _MT9D131_LINUX
#define _MT9D131_LINUX

/*SET CMD*/
#define DC_SET_IMAGESIZE       0x01    /* imagesize£¬value_range£º 
                                        DC_VAL_VGA
                                        DC_VAL_UXGA 
                                       */
                                       
#define DC_SET_POWERFREQ       0x13    /* power frequency£¬value_range£º
                                        DC_VAL_50HZ
                                        DC_VAL_60HZ
                                       */
                                       
/*===============================================================*/
/*Value Define*/

#define    DC_VAL_VGA          0x01    /* imagesize£¬VGA value*/
#define    DC_VAL_UXGA         0x04   /* imagesize£¬UXGA value*/

#define    DC_VAL_50HZ         0x01    /*power frequency,50HZ*/
#define    DC_VAL_60HZ         0x02    /*power frequency,60HZ*/
#endif
