include $(PWD)/../Makefile.param
			
obj-m := gpioi2c.o
gpioi2c-y += gpio_i2c.o

EXTRA_CFLAGS += -D$(HI_FPGA)-D$(CHIP_ARCH)
EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c
 
default:	
	$(CC) -g -Wall -o i2c_read i2c_read.c strfunc.c	
	$(CC) -g -Wall -o i2c_write i2c_write.c	strfunc.c
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean:
	rm *.o; rm i2c_read; rm i2c_write 
	@make -C $(LINUXROOT) M=$(PWD) clean 

