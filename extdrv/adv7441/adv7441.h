

#ifndef __ADV7441_H__
#define __ADV7441_H__

#define SET_ADV7441_REG			1
#define GET_ADV7441_REG			2


#define ADV7441_READ_REG            0x0
#define ADV7441_WRITE_REG           0x1

#define ADV7441_SET_VIDEO_FORMAT    0x2

typedef	enum _tagADV7441_VIDEO_FORMAT_E
{
	VIDEO_FORMAT_720P_60HZ,
    VIDEO_FORMAT_1080I_60HZ,    
} ADV7441_VIDEO_FORMAT_E;


typedef struct hiadv7400_w_reg
{
    unsigned char addr;
    unsigned char value;
}adv7400_w_reg;

#define	DETECT_NONE		0xff
#define 	DETECT_BEGIN		0xfe

//define for TW2815A
#define DEV_7400 0x40  // ALSB =0 
//#define DEV_7400 0x42  // ALSB =1

#define serial_control 0x62
#define serial_playback_control 0x6c

unsigned char ErrorId;

//#define	DEV_7400	0x40
/********************BLUE SCREEN****************************/
#define	DEF_VAL_EN				0x01
#define	CP_DEF_COL_FORCE		0x01
#define	CP_DEF_COL_MAIN_VAL	0x04
/*********************DETECT SYSTEM*************************/
#define	STATUS3_INST_HLOCK				0x01
#define	STATUS1_IN_LOCK					0x01
#define	STATUS1_DETECT_SYSTEM			0x70
#define	STATUS1_NTSC_M					0x00
#define	STATUS1_NTSC_443					0x10
#define	STATUS1_PAL_M						0x20
#define	STATUS1_PAL_60					0x30
#define	STATUS1_PAL_BGHID					0x40
#define	STATUS1_SECAM						0x50
#define	STATUS1_PAL_N						0x60
#define	STATUS1_SECAM_525				0x70


/**********************RGB DETECT***************************/
#define	SDTI_DVALID						0x80


//write
#define DEFAULT_VALUE_Y  0x0c
#define CP_DEF_VAL_EN 0xbf
//read
#define STATUS1					0x10
#define STATUS3					0x13
/****************************DETECT REG*****************************/
#define	RB_STANDARD_IDENTIFICATION_1	0xb1
#define	RB_STANDARD_IDENTIFICATION_2	0xb2
#define	RB_STANDARD_IDENTIFICATION_3	0xb3
#define	RB_STANDARD_IDENTIFICATION_4	0xb4
/***************************MODE REG*******************************/
#define	PRIM_MODE				0x05
#define	SDM_SEL				0x69
#define	ADC_POWER_CONTROL	0x3a

#define BL_480I 		0x3598
#define BL_576I 	0x3600
#define BL_480P 	0x1AB8
#define BL_576P 	0x1B00
#define BL_720P 	0x12C0
#define BL_1080I50 	0x1FC9
#define BL_1080I60 	0x1A82
#define BL_DITHER	0x040

#define SCF_480I_1		270
#define SCF_480I_2		318
#define SCF_576I_1		319
#define SCF_576I_2		391
#define SCF_480P_1		525
#define SCF_480P_2		589
#define SCF_576P_1		625
#define SCF_576P_2		689
#define SCF_720P_1		730
#define SCF_720P_2		780
#define SCF_1080I50_1	1080
#define SCF_1080I50_2	1180
#define SCF_1080I60_1	 1080
#define SCF_1080I60_2	1180

typedef struct _tagRegTable
{
	unsigned char ucAddr;
	unsigned char ucValue;
}REGTABLE;

typedef struct _tagModeDetectTb
{
	unsigned char ucMode;
	unsigned short bl;
	unsigned short scf1;
	unsigned short scf2;
}MODEDETECTTB;

typedef	enum _tagCvbsSys
{
	DETECT_NTSC_M,
	DETECT_NTSC_443,
	DETECT_PAL_M,
	DETECT_PAL_60,
	DETECT_PAL_BGHID,
	DETECT_SECAM,
	DETECT_PAL_N,
	DETECT_SECAM_525
}CVBSSYS;

typedef	enum _tagYcbcrSys
{
	DETECT_720_480_I60,
	DETECT_720_576_I50,
	DETECT_720_480_P60,
	DETECT_720_576_P50,
	DETECT_1280_720P,
	DETECT_1920_1080_I50,
	DETECT_1920_1080_I60
}YCBCRSYS;

typedef	enum _tagRGBSys
{
	DETECT_DOS185,
	DETECT_DOS285,
	DETECT_60VGA,
	DETECT_72VGA,
	DETECT_75VGA,
	DETECT_85VGA,
	DETECT_60SVGA,
	DETECT_72SVGA,
	DETECT_75SVGA,
	DETECT_85SVGA,
	DETECT_60XGA,
	DETECT_70XGA,
	DETECT_75XGA,
	DETECT_85XGA
}RGBSYS;

typedef struct _tagInputModeDetect
{
	unsigned char ucSourceModifyDelay;
	unsigned char ucModeDetectDelay;
}INPUTMODEDETECT;


typedef enum _tagSource
{
	AV_IN,
	SVIDEO_IN,
	YCBCR_IN,
	VGA_IN,
	DIGITAL_IN
}SOURCE;

typedef struct _tagControl
{
	
	unsigned	char ucSource;
	unsigned char ucDigitalMode;	
	unsigned char ucAnglogtMode;
	
	unsigned char ucBright;
	unsigned char ucContrast;
}CONTROL;
 
void Adv7400_blue_screen(unsigned char enable);
void Adc7400_Set_VgaMode(unsigned char video_mode);
void Adv7400_cvbs_set(unsigned char videomode);
void Adv7400_svideo_set(unsigned char videomode);
void Adv7400_Set_YcbcrMode(unsigned char videomode);
void Adv7400_channel(unsigned char video_in);
unsigned char Adv7400_detect(unsigned char video_in);
void Wr_adv7400_ycbcr480i_24bit(void);
#endif

