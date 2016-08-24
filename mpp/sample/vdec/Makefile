
include ../Makefile.param
CFLAGS += -Wall -g $(INC_PATH)

# common source
COMM_SRC := $(MPP_PATH)/sample/common/sample_common.c
COMM_OBJ := $(COMM_SRC:%.c=%.o)

# target source
SRC  := $(wildcard *.c) 
OBJ  := $(SRC:%.c=%.o)

TARGET := $(OBJ:%.o=%)
.PHONY : clean all

all: $(TARGET)

$(TARGET):%:%.o $(COMM_OBJ)
	$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ $(LIBS) 

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(COMM_OBJ)

