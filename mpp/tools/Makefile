#
#CROSS=arm-uclibc-linux-

include ./Makefile.param

INC_FLAGS += -I. -I./libmtran/lib/inc  -lpthread\
             -I$(EXTDRV_DIR)/tw2865  \
			 -I$(EXTDRV_DIR)/tw2864  \
             -I$(TEST_ROOT)/test_frame \
             -I$(MPP_PATH)/code/mkp/vedu/h264e \
             
CFLAGS := -Wall -g -DHI_DEBUG $(INC_PATH) $(INC_FLAGS)

SRC  := $(wildcard *.c)
OBJ  := $(SRC:%.c=%.o)

TARGET := $(OBJ:%.o=%)

.PHONY : clean all

all: $(TARGET)

$(TARGET):%:%.o 
#	@echo $(TARGET)
	$(CC) $(CFLAGS) -lpthread  -o $@ $^ $(LIBS)
#	$(CC) $(CFLAGS) -L../testframe/ -lpthread -ltestframe -o $@ $^ $(LIB)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
