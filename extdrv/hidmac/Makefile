
include $(PWD)/../Makefile.param

obj-m := hidmac.o
hidmac-y += hi_dmac.o kcom.o

EXTRA_CFLAGS += -D$(HI_FPGA) -D$(CHIP_ARCH)

default:
	@make -C $(LINUXROOT) M=$(PWD) modules 
clean: 
	@make -C $(LINUXROOT) M=$(PWD) clean 



