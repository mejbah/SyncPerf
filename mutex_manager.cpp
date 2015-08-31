#include<stdlib.h>
#include<string.h>
#include<list>
#include<iostream>
#include<fstream>
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
		memset(new_mutex->futex_wait, 0, sizeof(WAIT_TIME_TYPE)*M_MAX_THREADS);
		memset(new_mutex->trylock_wait_time, 0, sizeof(WAIT_TIME_TYPE)*M_MAX_THREADS);
	  memset(new_mutex->trylock_flag, 0, sizeof(int)*M_MAX_THREADS);
		memset(new_mutex->trylock_fail_count, 0, sizeof(int)*M_MAX_THREADS);
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
	double elapse = stop(st, &end); 
	mutex->futex_wait[idx] += elapsed2ms(elapse);
}


void trylock_first_timestamp( my_mutex_t *mutex ) 
{
	int idx= getThreadIndex(); //TODO: fix this, add thread index
	struct timeinfo *st = &mutex->trylock_first[idx];
	if(!mutex->trylock_flag[idx]){
		start(st);
		mutex->trylock_flag[idx] = 1;
	}
}

void add_trylock_fail_time( my_mutex_t *mutex )
{
	int idx = getThreadIndex();
	assert(mutex->trylock_flag[idx] == 1);
	struct timeinfo end;
	struct timeinfo *st = &mutex->trylock_first[idx];
	double elapse = stop(st, &end); 
	mutex->trylock_wait_time[idx] += elapsed2ms(elapse);
	mutex->trylock_flag[idx] = 0;
}

void inc_trylock_fail_count( my_mutex_t *mutex ) {
	int idx = getThreadIndex();	
	mutex->trylock_fail_count[idx]++;
}

void append( my_mutex_t *mut ) {
  WRAP(pthread_mutex_lock)(&mutex_list_lock);  
  mutex_list.push_back(mut);
	WRAP(pthread_mutex_unlock)(&mutex_list_lock);

}

void report() {
	std::list<my_mutex_t*>::iterator it;
	WAIT_TIME_TYPE contention = 0; 
	int fail_count = 0;
	WAIT_TIME_TYPE trylock_delay = 0;
	std::fstream fs;
	fs.open("report.log", std::fstream::out);
  fs << "# of mutex " << mutex_list.size() << std::endl;
  for(it = mutex_list.begin(); it != mutex_list.end(); ++it ){
  	my_mutex_t *m = *it;
		fs << &m->mutex << std::endl; //mutex address??

		WAIT_TIME_TYPE m_contention = 0; 
		int m_fail_count = 0;
		WAIT_TIME_TYPE m_trylock_delay = 0;
		fs << "count " << m->count << std::endl;
		fs << "\t\t THREAD FUTEX_WAIT TRY_FAIL TRY_WAIT" << std::endl;
		for(int i=0; i< M_MAX_THREADS; i++){
			fs<< "\t\tT" << i << " : " << m->futex_wait[i]  << " " <<  m->trylock_fail_count[i] 
							<< " " << m->trylock_wait_time[i] << std::endl;
    	m_contention += m->futex_wait[i]; 
			m_fail_count += m->trylock_fail_count[i];
			m_trylock_delay += m->trylock_wait_time[i];
		}
		
		fs << "\n\t\tMutex Total " << m_contention << " " << m_fail_count << " " << m_trylock_delay
					 << std::endl;

		contention += m_contention;
		fail_count += m_fail_count;
		trylock_delay += m_trylock_delay;
		//contention +=  elapsed2ms(m->futex_wait[0]);
		//std::cout << "Contention " << elapsed2ms(m->futex_wait[0]) << "ms" << std::endl;
		//printf("%lu\n", mutex->futex_start[0]);
  } 
  fs << "Total futex wait " << contention << " ms" << std::endl;
	fs << "Total trylock fail " << fail_count << std::endl;
	fs << "Total trylock fail time \n\n" << trylock_delay << std::endl;

  fs.close();
}
