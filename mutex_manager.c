#include<stdlib.h>
#include<string.h>


#include "mutex_manager.h"


my_mutex_t* create_mutex( pthread_mutex_t *mutex )
{
    //printf("create my mutex\n");
    my_mutex_t *new_mutex =(my_mutex_t*)malloc(sizeof(my_mutex_t));
    new_mutex->count = 0;
    new_mutex->mutex = *mutex;
		memset(new_mutex->futex_wait, 0, sizeof(unsigned long)*MAX_THREADS);
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
// return 1 if new mutex 
inline int setSyncEntry( void* syncvar, void* realvar) {
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

#if 0
pthread_mutex_t* get_orig_mutex( my_mutex *m ) 
{
    return &m->mutex;
}

#endif


void futex_start_timestamp( my_mutex_t *mutex, int idx ) 
{
	idx= 0; //TODO: fix this, add thread index
	struct timeinfo *st = &mutex->futex_start[idx];
	start(st);
}

void add_futex_wait( my_mutex_t *mutex, int idx )
{
	idx = 0; // TODO: fix this, add thread index
	struct timeinfo end;
	struct timeinfo *st = &mutex->futex_start[idx];
	mutex->futex_wait[idx] = stop(st, &end);
}


