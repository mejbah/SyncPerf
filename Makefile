#OBJS = nptl-init.o 
CC = gcc
#CFLAGS= -I. -D_GNU_SOURCE -fPIC -DORIGINAL -DMY_DEBUG
CFLAGS= -g -O0 -fno-omit-frame-pointer -I. -D_GNU_SOURCE -fPIC 
LD = $(CC)
LDFLAGS = -lpthread -ldl  -shared 

TARGET = liblockperf.so 

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
#OBJECTS_AS := $(patsubst %.s,%.o,$(wildcard *.S))
OBJECTS_AS = lowlevellock.o
all: $(TARGET) 

$(TARGET) : $(OBJS) $(OBJECTS_AS)
#$(TARGET) : $(OBJS) 
	$(LD) -o $@ $^ $(LDFLAGS)
%.o : %.c
	$(CC) $(CFLAGS) -c $<
%.o : %.S
	$(CC) $(CFLAGS) -c $<
clean:
	rm -f liblockperf.so *.o
