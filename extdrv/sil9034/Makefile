
include $(PWD)/../Makefile.param

obj-m := sil9034.o
sil9034-y += sil_9034.o

EXTRA_CFLAGS += -D$(HI_FPGA)
EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c -I$(PWD)/../gpio-i2c1

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 



