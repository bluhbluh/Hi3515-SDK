include ../Makefile.param

LIBRARY_PATH:=../../lib

LDFLAGS:= -L$(LIBRARY_PATH) -ltde -lm -lc -static
CFLAGS:=$(INC_PATH)
CFLAGS += -D$(CHIP_ARCH)

# common source
COMM_SRC := $(MPP_PATH)/sample/common/vo_open.c
COMM_OBJ := $(COMM_SRC:%.c=%.o)

SRC = $(wildcard *.c)
DST = $(SRC:%.c=%)

all:$(DST)

$(DST): % : %.c $(COMM_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(COMM_SRC) $(LIBRARY_PATH)/libmpi.a

clean:
	rm -f *.o $(DST)

