
include $(PWD)/../Makefile.param

obj-m := tw2865.o
tw_2865-y += tw2865.o

EXTRA_CFLAGS += -D$(HI_FPGA) -D$(CHIP_ARCH)
EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c -I$(PWD)/../gpio-i2c1

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 



