#ifndef __HI_GPIO_H__ 
#define __HI_GPIO_H__

#define GPIO_DIR_IN 0
#define GPIO_DIR_OUT 1

#define GPIO_SET_DIR 0x1
#define GPIO_GET_DIR 0x2
#define GPIO_READ_BIT 0x3
#define GPIO_WRITE_BIT 0x4

typedef struct {
	unsigned int  groupnumber;
	unsigned int  bitnumber;
	unsigned int  value;
}gpio_groupbit_info;

#endif
