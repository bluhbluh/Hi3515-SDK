
#ifndef _GPIO_I2C1_H
#define _GPIO_I2C1_H


#define GPIO_I2C1_READ   0x01
#define GPIO_I2C1_WRITE  0x02

unsigned char gpio_i2c1_read(unsigned char devaddress, unsigned char address);
void gpio_i2c1_write(unsigned char devaddress, unsigned char address, unsigned char value);


#endif

