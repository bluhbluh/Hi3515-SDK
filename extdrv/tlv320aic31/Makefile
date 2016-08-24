#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

include $(PWD)/../Makefile.param
			
obj-m := tlv_320aic31.o
tlv_320aic31-y += tlv320aic31.o

EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 

