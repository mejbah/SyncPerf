#ifndef __MY_MUTEX__
#define __MY_MUTEX__

#include<pthread.h>
#include<assert.h>
#include "finetime.h"

typedef unsigned int UINT32;
#define MAX_THREADS 100
typedef struct {
	UINT32 count;
	pthread_mutex_t mutex;
	struct timeinfo futex_start[MAX_THREADS]; //futex start time
	unsigned long futex_wait[MAX_THREADS];
}my_mutex_t;

my_mutex_t* create_mutex( pthread_mutex_t *mutex );


void* get_mutex( void *mutex );

void futex_start_timestamp( my_mutex_t *mutex, int id );

void add_futex_wait( my_mutex_t *mutex, int id );

#endif
