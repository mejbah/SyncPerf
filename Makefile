#OBJS = nptl-init.o 
CC = gcc
#CFLAGS= -I. -D_GNU_SOURCE -fPIC -DORIGINAL -DMY_DEBUG
CFLAGS= -g -O0 -fno-omit-frame-pointer -I. -D_GNU_SOURCE -fPIC 
LD = $(CC)
LDFLAGS = -lpthread -ldl  -shared 

TARGET = libmutex.so 

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
#OBJECTS_AS := $(patsubst %.s,%.o,$(wildcard *.S))

all: $(TARGET) test

#$(TARGET) : $(OBJS) $(OBJECTS_AS)
$(TARGET) : $(OBJS) 
	$(LD) -o $@ $^ $(LDFLAGS)
%.o : %.c
	$(CC) $(CFLAGS) -c $<
test : test.o
	g++ -o test $(CFLAGS) test.cpp -rdynamic ./$(TARGET) -ldl
	#g++ -o test -lpthread test.cpp 
clean:
	rm -f libmutex.so test *.o
