#include<stdlib.h>
#include<string.h>
#include<list>
#include<iostream>

#include "mutex_manager.h"
//#include "libfuncs.h"
#include "xdefines.h"

std::list<my_mutex_t*>mutex_list;
pthread_mutex_t mutex_list_lock=PTHREAD_MUTEX_INITIALIZER; // global lock 

my_mutex_t* create_mutex( pthread_mutex_t *mutex )
{
    //printf("create my mutex\n");
    my_mutex_t *new_mutex =(my_mutex_t*)malloc(sizeof(my_mutex_t));
    new_mutex->count = 0;
    new_mutex->mutex = *mutex;
		memset(new_mutex->futex_wait, 0, sizeof(unsigned long)*M_MAX_THREADS);
		append(new_mutex);
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
        else {
            free(realvar);
        } 
    }
    return ret;
}



void futex_start_timestamp( my_mutex_t *mutex ) 
{
	int idx= getThreadIndex(); //TODO: fix this, add thread index
	struct timeinfo *st = &mutex->futex_start[idx];
	//start(&(mutex->futex_start[idx]));
	start(st);
}

void add_futex_wait( my_mutex_t *mutex )
{
	int idx = getThreadIndex();//0; // TODO: fix this, add thread index
	struct timeinfo end;
	struct timeinfo *st = &mutex->futex_start[idx];
	//mutex->futex_wait[idx] = stop(&(mutex->futex_start[idx]), &end);
	mutex->futex_wait[idx] = stop(st, &end);
}


void append( my_mutex_t *mut ) {
  WRAP(pthread_mutex_lock)(&mutex_list_lock);  
  mutex_list.push_back(mut);
	WRAP(pthread_mutex_unlock)(&mutex_list_lock);

}

void report() {
	std::list<my_mutex_t*>::iterator it;
	unsigned long contention = 0; 
  std::cout << "# of mutex " << mutex_list.size() << std::endl;
  for(it = mutex_list.begin(); it != mutex_list.end(); ++it ){
  	my_mutex_t *m = *it;
		for(int i=0; i< M_MAX_THREADS; i++)
    	contention += m->futex_wait[i];
		//contention +=  elapsed2ms(m->futex_wait[0]);
		//std::cout << "Contention " << elapsed2ms(m->futex_wait[0]) << "ms" << std::endl;
		//printf("%lu\n", mutex->futex_start[0]);
  } 
  std::cout << "Total futex wait " << contention << " ticks" << std::endl;
}
