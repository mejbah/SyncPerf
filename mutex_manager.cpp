#include<stdlib.h>
#include<string.h>
#include<list>
#include<map>
#include<vector>
#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include "mutex_manager.h"
#include "recordentries.hh"
//#include "libfuncs.h"
#include "xdefines.h"
#include "xthread.h"
#include "report.h"
#include <unistd.h>
#include <map>

/*
 * @file   mutex_manager.cpp
 * @brief  Utils for managing mutex meta data
 * @author Mejbah<mohammad.alam@utsa.edu>
 */



extern RecordEntries<mutex_t>sync_vars;
extern void *thread_sync_data;

#ifndef ORIGINAL
pthread_mutex_t mutex_map_lock=PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t g_mutex_list_lock=PTHREAD_MUTEX_INITIALIZER; // global lock 


mutex_t* create_mutex( pthread_mutex_t *mutex )
{
  //printf("create my mutex\n");
  size_t entry_index = sync_vars.get_next_index(); 
	mutex_t *new_mutex = sync_vars.getEntry(entry_index);
  new_mutex->stack_count = 0;
	new_mutex->entry_index = entry_index;
	
  return new_mutex;
}

int is_my_mutex(void *mutex)
{
    void **tmp;
    tmp = mutex;

    if( *tmp != NULL)
        return 1;
    else 
        return 0;
}

void* get_mutex( void *mutex )
{
    void **tmp;
    tmp = mutex;

    assert(*tmp != NULL);
    return *tmp;    
    
}

#if 0
pthread_mutex_t* get_orig_mutex( my_mutex *m ) 
{
    return &m->mutex;
}
#endif

int setSyncEntry( void* syncvar, void* realvar) {
  int ret = 0;
  unsigned long* target = (unsigned long*)syncvar;
  unsigned long expected = *(unsigned long*)target;
 
  if( !is_my_mutex(target) ) //double check
  {
      if(__sync_bool_compare_and_swap(target, expected, (unsigned long)realvar)) 
      {
          //printf("new mutex \n");
          ret = 1;
      }
			
  }

  return ret;
}

//UINT32 get_thd_mutex_entry(UINT32 offset, int thd_idx){
//
//	sync_vars.getEntry(current->entryStart + offset)->entry_offset = offset;
//	return (current->entryStart + offset);
//}

//my_mutex_t* get_thd_mutex( UINT32 offset, int thd_idx ) {
//	my_mutex *m =  sysvars.getEntry(current->entryStart + offset);
//	m->entry_offset = offset;
//}
#if 0
void add_call_stack( my_mutex_t *mutex, long call_stack[], int idx ){
	int i=0;
	assert(idx  <  MAX_NUM_STACKS);
	while(call_stack[i] != 0 ) {
		mutex->stacks[idx][i] = call_stack[i];
		i++;
	}
	mutex->stacks[idx][i] = 0;
}

//return 1 if match, otherwise 0
int comp_stack( long s1[], long s2[] ){
	int found = 1;
	int idx = 0;
	while( s1[idx] != 0 && s2[idx] !=0 ){
		if(s1[idx] != s2[idx] ) {
			found = 0;
			break;
		}
		idx++;
	}

	if( found != 0 ) {
		if(s1[idx] != 0 || s2[idx] != 0 ){
			found = 0;
		}
	}

	return found;
}
#endif

int add_new_context( mutex_t *mutex, long ret_address, unsigned int esp_offset ) {
	for( int i=0; i< mutex->stack_count; i++ ){
    if(mutex->eip[i] == ret_address) {
    	//if(mutex->esp_offset[i] == esp_offset) {
				return 0;
			//}
		}
  }

  assert(mutex->stack_count < MAX_NUM_STACKS);
	
  // increment stack count atomically
  do_backtrace(mutex->stacks[mutex->stack_count], MAX_CALL_STACK_DEPTH);
	mutex->eip[mutex->stack_count] = ret_address;
  mutex->esp_offset[mutex->stack_count] = esp_offset;
	mutex->stack_count++;
  return 0;
}


thread_mutex_t* get_thread_mutex_data( size_t mut_index, int thd_index ){
	size_t index	= (thd_index * xdefines::MAX_SYNC_ENTRIES) + mut_index; 
	thread_mutex_t *start = (thread_mutex_t*)thread_sync_data;
	return &start[index]; 
}

void inc_access_count(size_t mut_index, int thd_idx)
{
	//thread_mutex_t *thd_data = get_thread_mutex_data(mut_index,thd_idx);
	//thd_data->access_count++;
	//__atomic_fetch_add(&thd_data->access_count, 1, __ATOMIC_RELAXED);
	get_thread_mutex_data(mut_index,thd_idx)->access_count++;	
}

void inc_fail_count(size_t mut_index, int thd_idx)
{
	get_thread_mutex_data(mut_index,thd_idx)->fail_count++;	
}

void inc_cond_wait_count(size_t mut_index, int thd_idx)
{
	get_thread_mutex_data(mut_index,thd_idx)->cond_waits++;
}

void start_timestamp( struct timeinfo *st ) 
{	
		start(st);
}

void add_cond_wait_time(size_t mut_index, int thd_idx, struct timeinfo *st)
{
	//struct timeinfo end;
	
	WAIT_TIME_TYPE waits = get_elapsed2ms( st, NULL);
  //double elapse = stop(st, &end); 
  //WAIT_TIME_TYPE waits = elapsed2ms(elapse);

	get_thread_mutex_data(mut_index,thd_idx)->cond_futex_wait += waits;	

}

void add_futex_wait(size_t mut_index, int thd_idx, struct timeinfo *st)
{
	//struct timeinfo end;
	WAIT_TIME_TYPE waits = get_elapsed2ms( st, NULL);
  //double elapse = stop(st, &end); 
  //WAIT_TIME_TYPE waits = elapsed2ms(elapse);

	get_thread_mutex_data(mut_index,thd_idx)->futex_wait += waits;
}

#if 0

void add_futex_wait( mutex_meta_t *mutex, int idx, struct timeinfo *st )
{
	struct timeinfo end;
	//struct timeinfo *st = &mutex->futex_start[idx];
	//mutex->futex_wait[idx] = stop(&(mutex->futex_start[idx]), &end);
	double elapse = stop(st, &end); 
	//mutex->futex_wait[idx] += elapse;
  mutex->futex_wait[idx] += elapsed2ms(elapse);
}

void add_cond_wait( mutex_meta_t *mutex, int idx, struct timeinfo *st )
{
#if 0
	struct timeinfo end;
	//struct timeinfo *st = &mutex->futex_start[idx];
	//mutex->futex_wait[idx] = stop(&(mutex->futex_start[idx]), &end);
	double elapse = stop(st, &end); 
	//mutex->cond_futex_wait[idx] += elapse;
	mutex->cond_futex_wait[idx] += elapsed2ms(elapse);
#endif
}
#endif

#ifdef WITH_TRYLOCK
void inc_trylock_fail_count(size_t mut_index, int thd_idx){
	get_thread_mutex_data(mut_index,thd_idx)->fail_count++;
	get_thread_mutex_data(mut_index,thd_idx)->trylock_fail_count++;
}

#endif // WITH_TRYLOCK



int do_backtrace(long stacks[ ], int size)
{
  void * stack_top;/* pointing to current API stack top */
  struct stack_frame * current_frame;
  int    i, found = 0;

  /* get current stack-frame */
  current_frame = (struct stack_frame*)(__builtin_frame_address(0));
  
  stack_top = &found;/* pointing to curent API's stack-top */
  
  /* Omit current stack-frame due to calling current API 'back_trace' itself */
  for (i = 0; i < 2; i++) {
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


int back_trace(long stacks[ ], int size)
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



#endif //ORIGINAL



