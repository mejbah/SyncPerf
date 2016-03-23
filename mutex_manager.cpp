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
#include <unistd.h>
#include <map>


#define MAXBUFSIZE 4096

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
    	if(mutex->esp_offset[i] == esp_offset) {
				return 0;
			}
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
	struct timeinfo end;
  double elapse = stop(st, &end); 
  WAIT_TIME_TYPE waits = elapsed2ms(elapse);

	get_thread_mutex_data(mut_index,thd_idx)->cond_futex_wait += waits;	

}

void add_futex_wait(size_t mut_index, int thd_idx, struct timeinfo *st)
{
	struct timeinfo end;
  double elapse = stop(st, &end); 
  WAIT_TIME_TYPE waits = elapsed2ms(elapse);

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




/**
	* Report functions
	*/

  std::string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
  }

  std::string get_call_stack_string( long *call_stack ){

    char _curFilename[MAXBUFSIZE];
    char buf[MAXBUFSIZE];
    int count = readlink("/proc/self/exe", _curFilename, MAXBUFSIZE);
    if (count <= 0 || count >= MAXBUFSIZE)
    {
      fprintf(stderr, "Failed to get current executable file name\n" );
      exit(1);
    }
    _curFilename[count] = '\0';



    std::string stack_str="";
    //std::stringstream ss;

    int j=0;
    while(call_stack[j] != 0 ) {
      //printf("%#lx\n", m->stacks[i][j]);  
      //std::cout << std::hex << m->stacks[i][j] << std::endl;

      //ss << std::hex << call_stack[j];

      sprintf(buf, "addr2line -e %s  -a 0x%lx  | tail -1", _curFilename, call_stack[j] );

      std::string source_line =  exec(buf);
      //ss << source_line.erase(source_line.size()-1); // remove the newline at the end 

      //stack_str += ss.str();    
      //ss.str("");
      if(source_line[0] != '?') { // not found
        //get the file name only
        std::size_t found = source_line.find_last_of("/\\");
        source_line = source_line.substr(found+1);
        stack_str += source_line.erase(source_line.size()-1); // remove the newline at the end
        stack_str += " ";
      }
      j++;
    }
    return stack_str;
  }

#ifdef CONTEXT_SORT
	typedef struct {
		size_t addr; // last call
		unsigned int access_count;
		unsigned int fail_count;	
	}context_data_t;
#endif


	void report() {

#ifdef CONTEXT_SORT
		std::map<size_t, context_data_t* > context_map; //map all mutex id to context
#endif
		
		int total_threads = xthread::getInstance().getMaxThreadIndex();

		WAIT_TIME_TYPE *thread_waits = malloc(sizeof(WAIT_TIME_TYPE)*total_threads);
		for(int idx=0; idx<total_threads; idx++) thread_waits[idx] = 0;	

		int total_sync_vars = sync_vars.getEntriesNumb();

		std::cout << "Report...\n";
		std::fstream fs;
		fs.open("mutex-conflicts.csv", std::fstream::out);
		
		//mutex_id, call stacks, futex_wait, cond_wait, trylock_wait, trylock fail count
		fs << "mutex_id, access_count, fail_count, lock_ratio, trylock fails, contexts"<< std::endl;
		int id = 0; // mutex_id just for reporting
	
		for(int i=0; i<total_sync_vars; i++) {
			mutex_t *m = sync_vars.getEntry(i);
			assert(m->entry_index == i);
			
			unsigned int total_access_count = 0;
			unsigned int total_fail_count = 0;
			unsigned int total_cond_wait = 0;
			unsigned int total_trylock_fails = 0;
			WAIT_TIME_TYPE total_wait_time = 0;
			WAIT_TIME_TYPE total_lock_wait = 0;			

			// sum all thread local data
			for(int idx=0; idx<total_threads; idx++ ){
			  thread_mutex_t *per_thd_data =get_thread_mutex_data( m->entry_index, idx);
				total_access_count += per_thd_data->access_count;
				total_fail_count += per_thd_data->fail_count;
				total_cond_wait += per_thd_data->cond_waits;
				total_trylock_fails += per_thd_data->trylock_fail_count;
				total_wait_time += per_thd_data->cond_futex_wait;
				total_lock_wait += per_thd_data->futex_wait;
			
				thread_waits[idx] += (per_thd_data->cond_futex_wait + per_thd_data->futex_wait);
			}

			double conflict_rate;
			if( total_access_count > 0 ) { //TODO: access_count = 0 is poosible as fix setSyncEntry ignores new mutex ,index already increased in recordentries
				id++;
				//print call stacks
				std::string call_contexts = "";
				for(int con=0; con<m->stack_count; con++){
					call_contexts += " #";
					std::stringstream ss;
					ss << (con + 1) << " ";
					call_contexts += ss.str();
#ifdef REPORT_LINE_INFO
					call_contexts += get_call_stack_string(m->stacks[con]);		
#else
					int depth = 0;
					while(m->stacks[con][depth]){	
							call_contexts += "0x";
							call_contexts += m->stacks[con][depth];
							call_contexts += ",";
							depth++; 
					}									
#endif 

#ifdef CONTEXT_SORT
					std::map<size_t, context_data_t*>::iterator it;			
					it = context_map.find(m->esp_offset[con]);
					if( it != context_map.end()){ // already threre
						assert(it->second->access_count == m->stacks[con][0]);
						it->second->access_count += total_access_count;
						it->second->fail_count += total_fail_count;
						break;
					}
					else {// new data insert
						context_data_t *new_data = malloc(sizeof(context_data_t));
						new_data->addr = m->stacks[con][0]; // store the stack top
						new_data->access_count = total_access_count;
						new_data->fail_count =  total_fail_count;
						context_map.insert(std::pair<size_t, context_data_t*>(m->esp_offset[con], new_data));
					}
#endif
				}

		
				double conflict_rate = (100*total_fail_count)/double(total_access_count);
//				fs << std::dec << id << ": " <<  total_access_count << "," << total_fail_count << "," << conflict_rate<< "," << total_trylock_fails <<"," << call_contexts << std::endl;
				fs << std::dec << id << ": " <<  total_access_count << "," << total_fail_count << "," << conflict_rate<< "," << total_trylock_fails <<","<< m->stack_count  << std::endl;
//				fs << std::dec << id << ": " <<  total_access_count << "," << total_fail_count << "," << conflict_rate<< "," << total_cond_wait << "," << total_trylock_fails <<"," << total_lock_wait <<","<<  total_wait_time << "," << call_contexts << std::endl;
			}
		}	

		fs.close();

		std::fstream thd_fs;

		thd_fs.open("thread_waits.csv", std::fstream::out);
		thd_fs << "tid, type, runtime" << std::endl; 

		for(int idx=0; idx< total_threads; idx++){
			thread_t *thd = xthread::getInstance().getThreadInfoByIndex(idx);
			thd_fs << idx << ", " << std::hex <<(void*)( thd->startRoutine)<< ", " <<std::dec<< (thd->actualRuntime - thread_waits[idx]) << std::endl;
		}

		thd_fs.close();	
		std::cout << total_threads << " threads, " << id <<  " mutexes\n";

#ifdef CONTEXT_SORT
		//print context map
		std::map<size_t, context_data_t*>::iterator p_it;
		std::fstream con_fs;
		
		con_fs.open("context.csv", std::fstream::out);

		con_fs << "context, count, fails" << std::endl;

		for(p_it=context_map.begin(); p_it!=context_map.end(); p_it++){
			std::string stack_str="";
			char _curFilename[MAXBUFSIZE];
			char buf[MAXBUFSIZE];

			int count = readlink("/proc/self/exe", _curFilename, MAXBUFSIZE);
			if (count <= 0 || count >= MAXBUFSIZE)
			{
				fprintf(stderr, "Failed to get current executable file name\n" );
				exit(1);
			}
			_curFilename[count] = '\0';

				
			sprintf(buf, "addr2line -e %s  -a 0x%zx  | tail -1", _curFilename, p_it->second->addr );

      std::string source_line =  exec(buf);
			//std::cout << source_line << std::endl;
      //ss << source_line.erase(source_line.size()-1); // remove the newline at the end 

      //stack_str += ss.str();    
      //ss.str("");
      if(source_line[0] != '?') { // not found
        //get the file name only
        std::size_t found = source_line.find_last_of("/\\");
        source_line = source_line.substr(found+1);
        stack_str += source_line.erase(source_line.size()-1); // remove the newline at the end
        stack_str += " ";
      }

			con_fs << stack_str << ", " << p_it->second->access_count << ", " << p_it->second->fail_count << std::endl; 	
		}

		con_fs.close();	

#endif

	}




#endif //ORIGINAL



