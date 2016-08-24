#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

include $(PWD)/../Makefile.param
			
obj-m := tw2864.o
tw_2864-y += tw2864.o

EXTRA_CFLAGS += -D$(HI_FPGA) -D$(CHIP_ARCH)
EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c -I$(PWD)/../gpio-i2c1

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 

