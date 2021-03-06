
#define TW2864_ID 0xff

/* notes: only defined for Hi3520 FPGA */
#define TW2864A_I2C_ADDR 0x50
#define TW2864B_I2C_ADDR 0x54
#define TW2864C_I2C_ADDR 0x52
#define TW2864D_I2C_ADDR 0x56

#define SERIAL_CONTROL 0x62
#define SERIAL_PLAYBACK_CONTROL 0x6c
#define SERIAL_AUDIO_OUTPUT_GAIN_REG 0xdf
#define SERIAL_AUDIO_INPUT_GAIN_REG1  0xd0
#define SERIAL_AUDIO_INPUT_GAIN_REG2  0xd1

/*process of bit*/
#define SET_BIT(x,y)        ((x) |= (y))
#define CLEAR_BIT(x,y)      ((x) &= ~(y))
#define CHKBIT_SET(x,y)     (((x)&(y)) == (y))
#define CHKBIT_CLR(x,y)     (((x)&(y)) == 0)

unsigned char tw2864_i2c_addr[] = {
    TW2864A_I2C_ADDR, TW2864B_I2C_ADDR, TW2864C_I2C_ADDR, TW2864D_I2C_ADDR};

//==================================================================================
//==================================================================================
//						tw2864 initialize table description
//==================================================================================

unsigned char tbl_tw2864_0x80[] = {        
	0x3F,0x02,0x10,0xCC,0x00,0x30,0x44,0x50,0x42};
unsigned char tbl_tw2864_0x8a[] = {        
	0xD8,0xBC,0xB8,0x44,0x2A,0x00};
    
unsigned char tbl_tw2864_0x80_0x8f[] = {        
    0x3F,0x02,0x10,0xCC,0x00,0x30,0x44,0x50,0x42, 0x02, 0xD8,0xBC,0xB8,0x44,0x2A,0x00
};    


/* 108M 4D1 mode,change addr:0x9f value to 0x30 from 0x77*/
unsigned char tbl_tw2864_0x90[] = {        
  //0x00,0x68,0x4C,0x30,0x14,0xA5,0xE6,0x05,0x00,0x28,0x44,0x00,0x20,0x90,0x52,0x77 //blue
	0x00,0x68,0x4C,0x30,0x14,0xA5,0xE0,0x05,0x00,0x28,0x44,0x00,0x20,0x90,0x52,0x30 //Black.
		};

unsigned char tbl_tw2864_0xa4[] = {        
	0x1A,0x1A,0x1A,0x1A,0x00,0x00,0x00,0xF0,0xF0,0xF0,0xF0,0x00
		};

unsigned char tbl_tw2864_0xb0[] = {0x00};

unsigned char tbl_tw2864_0xc4[] = {        
	0x00,0x00,0x00,0x00,0xFF,0xFF, 0xaa, 0x00,0x4e,0xe4,0x40,0x00
		};

/* audio */
unsigned char tbl_tw2864_0xd0[] = {   
  //0x22,0x22,0x02,0x20,0x64,0xa8,0xec,0x31,0x75,0xb9,0xfd,0xC1,0x00,0x00,0x00,0x80	//0xdc 1f-->00
  //0xAA,0xAA,0x01,0x03,0x21,0xa9,0xcd,0x21,0x87,0xb9,0xdb,0xc1,0x0f,0x00,0x00,0x80	//0xdc 1f-->00

  //0xd0,0xd1,0xd2, 0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda, 0xdb,0xdc,0xdd,0xde,0xdf
    0x33,0x33,0x00, 0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe, 0xe1,0x00,0x00,0x00,0x80	
		};

/* audio */
/* audio */
unsigned char tbl_tw2864_0xe0_0xe3[] = {        
  //0xe0,0xe1,0xe2,0xe3
	0x14,0xCF,0x44,0x44
		};

unsigned char tbl_tw2864_0xe0[] = {        
  //0xe0,0xe1,0xe2,0xe3, 0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef
	0x14,0xCF,0x44,0x44, 0x00,0x11,0x00,0x00,0x11,0x00,0x00,0x11,0x00,0x00,0x11,0x00	
		};

unsigned char tbl_tw2864_0xf0[] = {
  //0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff  
    0x83,0xB5,0x09,0x78,0x85,0x00,0x01,0x20,0x64,0x11,0x4a,0x0F,0xFF,0x00,0x00,0x00
};

//==================================================================================

//--------------------------		NTSC		------------------------------//
//=================================================================================
unsigned char tbl_ntsc_tw2864_common[] = {        
//												CH1			CH2			CH3			CH4
	0x00,0x08,0x59,0x11,		//...		0x00~0x03	0x10~0x13	0x20~0x23	0x30~0x33
	0x80,0x80,0x00,0x02,		//...		0x04~0x07	0x14~0x17	0x24~0x27	0x34~0x37
	0x12,0xf0,0x0c,0xd0,		//...		0x08~0x0b	0x18~0x1b	0x28~0x2b	0x38~0x3b
	0x00,0x00,0x07,0x7f				//...		0x0c~0x0e	0x1c~0x1e	0x2c~0x2e	0x3c~0x3e
	};

unsigned char tbl_ntsc_tw2864_sfr1[] = {
	0x00,0x00,0x40,0xc0,		//...		0x40~0x43//sync code(0x42)set to 0x1
	0x45,0xa0,0xd0,0x2f,		//...		0x44~0x47
	0x64,0x80,0x80,0x82,		//...		0x48~0x4b
	0x82,0x00,0x00,0x00		//...		0x4c~0x4f
	};

unsigned char tbl_ntsc_tw2864_sfr2[] = {
	0x00,0x0f,0x05,0x00,		//...		0x50~0x53
	0x00,0x80,0x06,0x00,		//...		0x54~0x57
	0x00,0x00					//...		0x58~0x59
	};

//=================================================================================


//--------------------------		PAL		------------------------------//
//=================================================================================
unsigned char tbl_pal_tw2864_common[] = {        
//												CH1			CH2			CH3			CH4
	0x00,0x08,0x59,0x11,		//...		0x00~0x03	0x10~0x13	0x20~0x23	0x30~0x33
	0x80,0x80,0x00,0x12,		//...		0x04~0x07	0x14~0x17	0x24~0x27	0x34~0x37
	0x12,0x20,0x01,0xd0,		//...		0x08~0x0b	0x18~0x1b	0x28~0x2b	0x38~0x3b
	0x00,0x00,0x07,0x7f		//...		0x0c~0x0e 	0x1c~0x1e	       0x2c~0x2e	       0x3c~0x3e
	};


unsigned char tbl_pal_tw2864_sfr1[] = {
	0x00,0x00,0x40,0xc0,		//...		0x40~0x43
	0x45,0xa0,0xd0,0x2f,		//...		0x44~0x47
	0x64,0x80,0x80,0x82,		//...		0x48~0x4b
	0x82,0x00,0x00,0x00		//...		0x4c~0x4f
	};

unsigned char tbl_pal_tw2864_sfr2[] = {
	0x00,0x0f,0x05,0x00,		//...		0x50~0x53
	0x00,0x80,0x06,0x00,		//...		0x54~0x57
	0x00,0x00					//...		0x58~0x59
	};
//=================================================================================
//=================================================================================
unsigned char tbl_tw2864_audio[] = {
				   0x00,0xff,		//...		0x5a~0x5b
	0x2f,0x00,0x00,0x00,		//...		0x5c~0x5f
	0x88,0x88,0xc2,0x00,		//...		0x60~0x63
	0x10,0x32,0x54,0x76,		//...		0x64~0x67
	0x98,0xba,0xdc,0xfe,		//...		0x68~0x6b
	0x02,0x1f,0x88,0x88,		//...		0x6c~0x6f
	0x88,0x11,0x40,0x88,		//...		0x70~0x73
	0x88,0x00					//...		0x74~0x75
	};
//=================================================================================


