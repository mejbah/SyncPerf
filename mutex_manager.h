#ifndef __MY_MUTEX__
#define __MY_MUTEX__

#include<pthread.h>
#include<assert.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "finetime.h"

#define MAX_CALL_STACK_DEPTH 5
#define MAX_NUM_STACKS 32

#ifdef GET_STATISTICS
extern volatile unsigned long totalLocks;
#endif
typedef unsigned long WAIT_TIME_TYPE;
//typedef double  WAIT_TIME_TYPE;
typedef unsigned int UINT32;


typedef struct {
	
	pthread_mutex_t mutex;

	// Keep the address of nominal mutex;
	pthread_mutex_t * nominalmutex;

	size_t entry_index; // mutex entry per thread index 

	int stack_count; // how many different call sites
	long stacks[MAX_NUM_STACKS][MAX_CALL_STACK_DEPTH+1];

	unsigned int ebp_offset[MAX_NUM_STACKS];
	long ret_address[MAX_NUM_STACKS];
	
}mutex_t;


//per thread data
typedef struct { 	

	UINT32 access_count;
	UINT32 fail_count;
	UINT32 cond_waits;	
	UINT32 trylock_fail_count;
	WAIT_TIME_TYPE cond_futex_wait; // time spend in cond wait
	WAIT_TIME_TYPE futex_wait; // time spend for lock grabbing
}thread_mutex_t;




mutex_t* create_mutex( pthread_mutex_t *mutex );

int is_my_mutex( void *mutex );
void* get_mutex( void *mutex );

UINT32 get_thd_mutex_entry( UINT32 offset, int thd_idx );

void inc_access_count(size_t mut_index, int thd_idx);
void inc_fail_count(size_t mut_index, int thd_idx);
void inc_cond_wait_count(size_t mut_index, int thd_idx);
void add_cond_wait_time(size_t mut_index, int thd_idx, struct timeinfo *st);
void add_futex_wait(size_t mut_index, int thd_idx, struct timeinfo *st);

void start_timestamp( struct timeinfo *st ); 
//void add_futex_wait( mutex_meta_t *mutex, int idx, struct timeinfo *st );
//void add_cond_wait( mutex_meta_t *mutex, int idx, struct timeinfo *st);
void inc_trylock_fail_count(size_t mut_index, int thd_idx);

// return 1 if new mutex 
int setSyncEntry( void* syncvar, void* realvar);

int add_new_context( mutex_t *mutex, long ret_address, unsigned int ebp_offset ) ;


void report();


/* Define the stack_frame layout */
struct stack_frame {
  struct stack_frame * prev;/* pointing to previous stack_frame */
  long   caller_address;/* the address of caller */
};

/* pointing to the stack_bottom from libc */
extern void * __libc_stack_end;


int back_trace(long stacks[ ], int size);
int do_backtrace(long stacks[ ], int size);

#ifdef __cplusplus
}
#endif


#endif
