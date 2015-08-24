
// -*- C++ -*-

/*
Allocate and manage thread index.
Second, try to maintain a thread local variable to save some thread local information.
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * @file   xthread.h
 * @brief  Managing the thread creation, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XTHREAD_H_
#define _XTHREAD_H_

#include <new>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> //mejbah
#include <assert.h> //mejbah
#include "xdefines.h"
#include "finetime.h"
#ifdef USING_IBS
#include "IBS/ibs.h"
#endif

extern "C" {
	struct threadLevelInfo {
		int beginIndex;
		int endIndex;
		struct timeinfo startTime;
		unsigned long elapse;
	};
}; 

class xthread {
private:
    xthread() 
    { }

public:
  static xthread& getInstance() {
    static char buf[sizeof(xthread)];
    static xthread * theOneTrueObject = new (buf) xthread();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize()
  {
    _aliveThreads = 0;
		_threadIndex = 0;
    _heapid = 0;
		_predPerfImprovement = true;
		_origThreadId = gettid();

		// Acquire the number of CPUs.
		_numCPUs= sysconf(_SC_NPROCESSORS_ONLN);
		if(_numCPUs < 0) {
			fprintf(stderr, "Can't get the CPU's number\n");
			abort();
		}

		// How we can get the information in the beginning.
    WRAP(pthread_mutex_init)(&_lock, NULL); //mejbah added WRAP

    // Shared the threads information. 
    memset(&_threads, 0, sizeof(_threads));

		// Set all entries to be available initially 
    //for(int i = 0; i < xdefines::NUM_HEAPS; i++) {
		//	_HeapAvailable[i] = true;
    //}

		// Set this thread level information to 0.
		memset(&_threadLevelInfo, 0, sizeof(struct threadLevelInfo)*xdefines::MAX_THREAD_LEVELS);

    // Allocate the threadindex for current thread.
    initInitialThread();
  }

	// The end of system. 
	void finalize(void) {
		// Stop the last level
		stopThreadLevelInfo();
	}

  // Initialize the first threadd
  void initInitialThread(void) {
    int tindex;

		// We know that we will going to execute     
    current = getThreadInfoByIndex(0);

     // Allocate a global thread index for current thread.
    tindex = allocThreadIndex();

		assert(tindex == 0);

    // Get corresponding thread_t structure.
    current->self  = pthread_self();
    current->tid  = gettid();
  }

  thread_t * getThreadInfoByIndex(int index) {
		assert(index < xdefines::MAX_THREADS);

    return &_threads[index];
  }

	unsigned long getMaxThreadIndex(void) {
		return _threadIndex;
  }
	
	// Updating the end index in every allocThreadIndex
	// There is no need to hold the lock since only the main thread can call allocThreadIndex
	void updateThreadLevelInfo(int threadindex) {
		struct threadLevelInfo * info = &_threadLevelInfo[_threadLevel];

		if(info->endIndex < threadindex) {
			info->endIndex = threadindex;
		}
	}

	// Start a thread level
	void startThreadLevelInfo(int threadIndex) {
		struct threadLevelInfo * info = &_threadLevelInfo[_threadLevel];

		start(&info->startTime);
		info->beginIndex = threadIndex;
		info->endIndex = threadIndex;
		
//		fprintf(stderr, "starting a new thread level\n");
	}
	
	// Start a thread level
	void stopThreadLevelInfo(void) {
		struct threadLevelInfo * info = &_threadLevelInfo[_threadLevel];

		info->elapse = elapsed2ms(stop(&info->startTime, NULL));

//		fprintf(stderr, "PHASE end %ld\n", info->elapse);
	}

	unsigned long getTotalThreadLevels(void) {
		return _threadLevel;
	}

	struct threadLevelInfo * getThreadLevelByIndex(int index) {
		return &_threadLevelInfo[index];
	}

  // Allocate a thread index under the protection of global lock
  int allocThreadIndex(void) {
		global_lock();

		int index = _threadIndex++;
		int alivethreads = _aliveThreads++;

		// Check whether we have created too many threads or there are too many alive threads now.
    if(index >= xdefines::MAX_THREADS || alivethreads >= xdefines::MAX_ALIVE_THREADS) {
      fprintf(stderr, "Set xdefines::MAX_THREADS to larger. _alivethreads %ld totalthreads %ld maximum alive threads %d", _aliveThreads, _threadIndex, xdefines::MAX_ALIVE_THREADS);
      abort(); 
    } 

		// Initialize 
    thread_t * thread = getThreadInfoByIndex(index);
		thread->ptid = gettid(); //parent tid information
		thread->index = index;
		thread->latency = 0;
		thread->accesses = 0;
		thread->levelIndex = _threadLevel; 
		start(&thread->startTime);

		// Now find one available heapid for this thread.
		//thread->heapid = allocHeapId();

		// Check whether we are still in the the normal case
		if(thread->ptid != _origThreadId) {
		//	fprintf(stderr, "thread->ptid %d origthreadid %d\n", thread->ptid, _origThreadId);
			_predPerfImprovement = false;
		}
 
		// If alivethreads is 1, we are creating new threads now.
	//	fprintf(stderr, "allocThreadIndex line %d\n", __LINE__);
		if(alivethreads == 0) {
			// We need to save the starting time
			startThreadLevelInfo(index);
		}
		else if(alivethreads == 1) {
			// Now we are trying to create more threads
			// Serial phase is ended now.
			_isMultithreading = true;
	
			current->childBeginIndex = index;
			current->childEndIndex = index;
	
			// Now we need to get the elapse of the serial phase	
			stopThreadLevelInfo();
			
			// Now we are entering into a new level
			_threadLevel++;		
			startThreadLevelInfo(index);
		}
		else if (alivethreads > 1) {
			// We don't know how many threads are we going to create.
			// thus, we simply update the endindex now.
			updateThreadLevelInfo(index);
			if(index > current->childEndIndex) {
				current->childEndIndex = index;
			}
		}

		//fprintf(stderr, "threadindex %d\n", _threadIndex);
		if(alivethreads == 0) {
			// Set the pindex to 0 for the initial thread
			thread->pindex = 0;
			thread->parentRuntime = 0;
		}
		else {
			thread->parentRuntime=getParentRuntime(thread->pindex);
			thread->pindex = getThreadIndex();
		}

		global_unlock();
    
    return index; 
  }

	// How we can get the parent's runtime on the last epoch?
	unsigned long getParentRuntime(int index) {
		thread_t *thread = getThreadInfoByIndex(index);

		return thread->actualRuntime;
	}

  /// Create the wrapper 
  /// @ Intercepting the thread_creation operation.
  int thread_create(pthread_t * tid, const pthread_attr_t * attr, threadFunction * fn, void * arg) {
    void * ptr = NULL;
    int tindex;
    int result;

    // Allocate a global thread index for current thread.
    tindex = allocThreadIndex();
    thread_t * children = getThreadInfoByIndex(tindex);
    
    children->startRoutine = fn;
    children->startArg = arg;

    result =  WRAP(pthread_create)(tid, attr, startThread, (void *)children);
		

    return result;
  }      

	thread_t * getChildThreadStruct( pthread_t thread) {
		thread_t * thisThread = NULL;

		int index = current->childBeginIndex;

		while(true) {
			thisThread = &_threads[index];

			index++;

			// We find the child
			if(thisThread->self == thread) {
				//current->childBeginIndex = index;
				break;
			}
			else {
				if(index <= current->childEndIndex) {
					continue;
				}
				else {
					printf("Can't find the thread_t structure with specifid thread\n");
					abort();
				}
			}	
		}

		return thisThread;
	}

	int thread_join(pthread_t thread, void **retval)  {
		int ret;

		
		ret = WRAP(pthread_join(thread, retval));

		if(ret == 0) {
			thread_t * thisThread;

			// Finding out the thread with this pthread_t 
			thisThread = getChildThreadStruct(thread);

			markThreadExit(thisThread);
		}

		return ret;
	}

  // @Global entry of all entry function.
  static void * startThread(void * arg) {
    void * result;

    current = (thread_t *)arg;

#ifdef USING_IBS
    startIBS(current->index);
#endif
    current->self = pthread_self();
		current->tid = gettid();

//		fprintf(stderr, "CHILD:tid %d index %d\n", current->tid, current->index);
    // from the TLS storage.
    result = current->startRoutine(current->startArg);

		// Get the stop time.
		current->actualRuntime = elapsed2ms(stop(&current->startTime, NULL));
//		fprintf(stderr, "tid %d index %d latency %lx actualRuntime %ld\n", current->tid, current->index, current->latency, current->actualRuntime);

    return result;
  }
	
	// In the end, we should compute the total latency.
	unsigned long getTotalLatency(int startIndex, int stopIndex) {
    int index = startIndex;

		unsigned long latency = 0;

		//fprintf(stderr, "threadindex %d\n", _threadIndex);
    thread_t * thread;
    while(true) {

			// Get the latency of all active threads. 
      thread = getThreadInfoByIndex(index);
			latency += thread->latency;				        

			index++;
			
			// We will skipp the original thread.
			if(index%xdefines::MAX_THREADS == 0) {
				index = 1; 
			}
			
			// If we have checked all threads, let's return now.
			if(index == stopIndex) {
				break;
			}
		}

		// Return the full latency
		return latency;
	}

	// In the end, we should compute the total latency.
	unsigned long getTotalAccesses(int startIndex, int stopIndex) {
    int index = startIndex;

		unsigned long accesses = 0;

		//fprintf(stderr, "threadindex %d\n", _threadIndex);
    thread_t * thread;
    while(true) {

			// Get the latency of all active threads. 
      thread = getThreadInfoByIndex(index);
			accesses += thread->accesses;				        

			index++;
			
			// We will skipp the original thread.
			if(index%xdefines::MAX_THREADS == 0) {
				index = 1; 
			}
			
			// If we have checked all threads, let's return now.
			if(index == stopIndex) {
				break;
			}
		}
		
		return accesses;

	}


private:
  /// @brief Lock the lock.
  void global_lock(void) {
    WRAP(pthread_mutex_lock)(&_lock); //mejbah added WRAP
  }

  /// @brief Unlock the lock.
  void global_unlock(void) {
    WRAP(pthread_mutex_unlock)(&_lock); //mejbah added WRAP
  }

#if 0 //mejbah
	/// @ Allocation should be always successful.
	int allocHeapId(void) {
		int heapid;

		while(true) {
			if(_HeapAvailable[_heapid] == true) {
				heapid = _heapid;
				_HeapAvailable[_heapid] = false;
				_heapid = (_heapid+1)%xdefines::NUM_HEAPS;
				break;	
			}
			_heapid = (_heapid+1)%xdefines::NUM_HEAPS;
		}
	
		return heapid;
	}

	// Set the heap to be available if the thread is exiting.
	void releaseHeap(int heapid) {
		_HeapAvailable[heapid] = true;
	}
#endif
	// Now we will mark the exit of a thread 
  void markThreadExit(thread_t * thread) {
  // fprintf(stderr, "remove thread %p with thread index %d\n", thread, thread->index);
    global_lock();

    --_aliveThreads;

		if(_aliveThreads == 1) {
			_isMultithreading = false;
			
			// Now we have to update latency information for the current level
    	if(_predPerfImprovement) {
				stopThreadLevelInfo();

				// Now we will udpate the level.
				_threadLevel++;

				// Now we will start a new serial phase.			
				startThreadLevelInfo(_threadIndex);
    	}
		}

		// Release the heap id for this thread.
		//releaseHeap(thread->heapid);

    global_unlock();
  }


  pthread_mutex_t _lock;
  volatile unsigned long _threadIndex;
  volatile unsigned long _aliveThreads;
  int _tid;
	int _numCPUs;
	pid_t _origThreadId;
	int _heapid;
		
	// We are adding a total latency here.
	bool  _predPerfImprovement;

	//bool     _HeapAvailable[xdefines::NUM_HEAPS];
  // Total threads we can support is MAX_THREADS
  thread_t  _threads[xdefines::MAX_THREADS];

	// We will update these information
	int _threadLevel;
	struct threadLevelInfo _threadLevelInfo[xdefines::MAX_THREAD_LEVELS];
};
#endif

