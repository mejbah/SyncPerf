#OBJS = nptl-init.o 
CC = gcc
CFLAGS= -I. -D_GNU_SOURCE -fPIC
LD = $(CC)
LDFLAGS = -lpthread -ldl  -shared 

TARGET = libmutex.so 

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

all: $(TARGET) test

$(TARGET) : $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS)
%.o : %.C
	$(CC) $(CFLAGS) -c $<
	#gcc -o libmutex.so ${CXXFLAGS} -shared -fPIC -lpthread -ldl -D_GNU_SOURCE -m32 lowlevellock.S mutex.c
#	gcc -o libmutex.so ${CXXFLAGS} -shared -fPIC -lpthread -ldl tpp.c lowlevellock.c my_mutex_init.c my_mutex_lock.c pthread_mutex_cond_lock.c my_cond_wait.c
#	g++ -o test test.cpp -rdynamic ./libmutex.so -ldl
test : test.o
	g++ -o test test.cpp -rdynamic ./$(TARGET) -ldl
	#g++ -o test -lpthread test.cpp 
clean:
	rm -f libmutex.so test *.o
