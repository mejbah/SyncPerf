#ifndef __MY_MUTEX__
#define __MY_MUTEX__

#include<pthread.h>
#include<assert.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "finetime.h"
//#include "xdefines.h"

#define M_MAX_THREADS 4096 // TODO: fix this equal to xdefines::MAX_THREADS

typedef unsigned int UINT32;
typedef struct {
	UINT32 count;
	pthread_mutex_t mutex;
	struct timeinfo futex_start[M_MAX_THREADS]; //futex start time
	unsigned long futex_wait[M_MAX_THREADS];
}my_mutex_t;

my_mutex_t* create_mutex( pthread_mutex_t *mutex );

int is_my_mutex(void *mutex);
void* get_mutex( void *mutex );

void futex_start_timestamp( my_mutex_t *mutex );

void add_futex_wait( my_mutex_t *mutex );

void append( my_mutex_t *mut ); 

// return 1 if new mutex 
int setSyncEntry( void* syncvar, void* realvar);

void report();
#ifdef __cplusplus
}
#endif


#endif
