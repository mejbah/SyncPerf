#ifndef __MY_MUTEX__
#define __MY_MUTEX__

#include<pthread.h>
#include<assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "finetime.h"
//#include "xdefines.h"

#define M_MAX_THREADS 50 // TODO: fix this equal to xdefines::MAX_THREADS
#define MAX_CALL_STACK_DEPTH 10
#define MAX_NUM_STACKS 20 

typedef unsigned long WAIT_TIME_TYPE;
typedef unsigned int UINT32;

typedef struct {
  struct timeinfo futex_start[M_MAX_THREADS]; //futex start time
	WAIT_TIME_TYPE futex_wait[M_MAX_THREADS]; // time spend in futex wait
	WAIT_TIME_TYPE cond_futex_wait[M_MAX_THREADS]; // time spend in cond wait
	struct timeinfo trylock_first[M_MAX_THREADS]; // has to be zero initialized
	WAIT_TIME_TYPE trylock_wait_time[M_MAX_THREADS];
	int trylock_flag[M_MAX_THREADS];
	int trylock_fail_count[M_MAX_THREADS]; 
}mutex_meta_t;

typedef struct {
	UINT32 count; // how many times accessed
	pthread_mutex_t mutex;
	UINT32 stack_count; // how many different call sites
	long stacks[MAX_NUM_STACKS][MAX_CALL_STACK_DEPTH+1];
	mutex_meta_t data[MAX_NUM_STACKS]; // all the meta data collections
}my_mutex_t;


my_mutex_t* create_mutex( pthread_mutex_t *mutex );

int is_my_mutex(void *mutex);
void* get_mutex( void *mutex );


void add_call_stack( my_mutex_t *mutex, long call_stack[] );

//return 1 if match, otherwise 0
int comp_stack( long s1[], long s2[] );

//TODO: use a hashtable
/*
 * reutrn matching call stack data
 */

mutex_meta_t* get_mutex_meta( my_mutex_t *mutex, long call_stack[] );


#if 1
void futex_start_timestamp( mutex_meta_t *mutex );

void add_futex_wait( mutex_meta_t *mutex );

void add_cond_wait( mutex_meta_t *mutex );

void add_trylock_fail_time( mutex_meta_t *mutex );

void trylock_first_timestamp( mutex_meta_t *mutex );

void inc_trylock_fail_count( mutex_meta_t *mutex );
#else

void futex_start_timestamp( my_mutex_t *mutex );

void add_futex_wait( my_mutex_t *mutex );

void add_cond_wait( my_mutex_t *mutex );

void add_trylock_fail_time( my_mutex_t *mutex );

void trylock_first_timestamp( my_mutex_t *mutex );

void inc_trylock_fail_count( my_mutex_t *mutex );

#endif
void append( my_mutex_t *mut ); 

// return 1 if new mutex 
int setSyncEntry( void* syncvar, void* realvar);

void report();

/* Define the stack_frame layout */
struct stack_frame {
  struct stack_frame * prev;/* pointing to previous stack_frame */
  long   caller_address;/* the address of caller */
};

/* pointing to the stack_bottom from libc */
extern void * __libc_stack_end;


int back_trace(long stacks[ ], int size);
#if 0
static int back_trace(long stacks[ ], int size)
{
  void * stack_top;/* pointing to current API stack top */
  struct stack_frame * current_frame;
  int    i, found = 0;

  /* get current stack-frame */
  current_frame = (struct stack_frame*)(__builtin_frame_address(0));
  
  stack_top = &found;/* pointing to curent API's stack-top */
  
  /* Omit current stack-frame due to calling current API 'back_trace' itself */
  for (i = 0; i < 1; i++) {
    if (((void*)current_frame < stack_top) || ((void*)current_frame > __libc_stack_end)) break;
    current_frame = current_frame->prev;
  }
  
  /* As we pointing to chains-beginning of real-callers, let's collect all stuff... */
  for (i = 0; i < size; i++) {
    /* Stop in case we hit the back-stack information */
    if (((void*)current_frame < stack_top) || ((void*)current_frame > __libc_stack_end)) break;
    /* omit some weird caller's stack-frame info * if hits. Avoid dead-loop */
    if ((current_frame->caller_address == 0) || (current_frame == current_frame->prev)) break;
    /* make sure the stack_frame is aligned? */
    if (((unsigned long)current_frame) & 0x01) break;

    /* Ok, we can collect the guys right now... */
    stacks[found++] = current_frame->caller_address;
    /* move to previous stack-frame */
    current_frame = current_frame->prev;
  }

  /* omit the stack-frame before main, like API __libc_start_main */
  if (found > 1) found--;

  stacks[found] = 0;/* fill up the ending */

  return found;
}
#endif


#ifdef __cplusplus
}
#endif


#endif
