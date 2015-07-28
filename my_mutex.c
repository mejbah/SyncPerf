#include "my_mutex.h"


my_mutex_t* create_mutex( pthread_mutex_t *mutex )
{
    printf("create my mutex\n");
    my_mutex_t *new_mutex =  malloc(sizeof(my_mutex_t));
    new_mutex->count = 0;
    new_mutex->mutex = *mutex;
    return new_mutex;
}

int is_my_mutex(pthread_mutex_t *mutex)
{
    my_mutex_t **tmp;
    tmp = mutex;

    if( *tmp != NULL)
        return 1;
    else 
        return 0;
}
my_mutex_t* get_mutex( pthread_mutex_t *mutex )
{
    my_mutex_t **tmp;
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


