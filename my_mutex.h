#ifndef __MUTEX_META__
#define __MUTEX_META__

#include<pthread.h>
#include<assert.h>
typedef unsigned int UINT32;

typedef struct {
    UINT32 count;
    pthread_mutex_t mutex;
}my_mutex_t;


my_mutex_t* create_mutex( pthread_mutex_t *mutex );

my_mutex_t* get_mutex( pthread_mutex_t *mutex );


#endif
