#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

include $(PWD)/../Makefile.param
			
obj-m := mt_9d131.o
mt_9d131-y += mt9d131.o

EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 

