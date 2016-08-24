#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

include $(PWD)/../Makefile.param
			
obj-m := adv_7441.o
adv_7441-y += adv7441.o
EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c
default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 

