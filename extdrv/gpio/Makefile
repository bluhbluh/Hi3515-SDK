include $(PWD)/../Makefile.param
			
obj-m := gpio.o
gpio-y += hi_gpio.o

EXTRA_CFLAGS += -D$(HI_FPGA)-D$(CHIP_ARCH)
EXTRA_CFLAGS+=-I$(PWD)/

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 
