include $(PWD)/../Makefile.param
			
obj-m := gpioi2c1.o
gpioi2c1-y += gpio_i2c1.o

EXTRA_CFLAGS += -D$(HI_FPGA) -D$(CHIP_ARCH)
 
default:	
	$(CC) -g -Wall -o i2c1_read i2c1_read.c strfunc.c	
	$(CC) -g -Wall -o i2c1_write i2c1_write.c strfunc.c
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean:
	rm *.o; rm i2c1_read; rm i2c1_write 
	@make -C $(LINUXROOT) M=$(PWD) clean 

