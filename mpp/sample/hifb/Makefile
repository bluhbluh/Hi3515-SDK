include ../Makefile.param
CFLAGS := -Wall -g $(INC_PATH)

CFLAGS += -D$(CHIP_ARCH)
LDFLAGS:= -lm -lc 

COMM_SRC := $(MPP_PATH)/sample/common/vo_open.c
COMM_OBJ := $(COMM_SRC:%.c=%.o)

SRC = $(wildcard *.c)
DST = $(SRC:%.c=%)

all:$(DST)

$(DST):%:%.c $(COMM_SRC)
	@echo $(COMM_SRC)
	$(CC) $(CFLAGS) -lpthread -o $@ $< $(COMM_SRC) $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o $(DST)
