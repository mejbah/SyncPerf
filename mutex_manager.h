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

typedef unsigned long WAIT_TIME_TYPE;

typedef unsigned int UINT32;
typedef struct {
	UINT32 count;
	pthread_mutex_t mutex;
	struct timeinfo futex_start[M_MAX_THREADS]; //futex start time
	WAIT_TIME_TYPE futex_wait[M_MAX_THREADS]; // time spend in futex wait
	struct timeinfo trylock_first[M_MAX_THREADS]; // has to be zero initialized
	WAIT_TIME_TYPE trylock_wait_time[M_MAX_THREADS];
	int trylock_flag[M_MAX_THREADS];
	int trylock_fail_count[M_MAX_THREADS]; 
}my_mutex_t;

my_mutex_t* create_mutex( pthread_mutex_t *mutex );

int is_my_mutex(void *mutex);
void* get_mutex( void *mutex );

void futex_start_timestamp( my_mutex_t *mutex );

void add_futex_wait( my_mutex_t *mutex );

void add_trylock_fail_time( my_mutex_t *mutex );

void trylock_first_timestamp( my_mutex_t *mutex );

void inc_trylock_fail_count( my_mutex_t *mutex );

void append( my_mutex_t *mut ); 

// return 1 if new mutex 
int setSyncEntry( void* syncvar, void* realvar);

void report();
#ifdef __cplusplus
}
#endif


#endif
