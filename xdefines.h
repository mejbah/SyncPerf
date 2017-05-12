// -*- C++ -*-

/*
 
  Copyright (c) 2007-2012 Emery Berger, University of Massachusetts Amherst.

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

#ifndef _XDEFINES_H_
#define _XDEFINES_H_

#include <sys/mman.h>
#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ucontext.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syscall.h>

#include "finetime.h"
#include "libfuncs.h"


/*
 * @file   xdefines.h   
 * @brief  Global definitions for SyncPerf
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 
#ifdef __cplusplus
extern "C"
{
#endif
  typedef void * threadFunction (void *);
	#define gettid() syscall(SYS_gettid)
 
  enum { CACHE_LINE_SIZE = 64 };
	#define LOG_SIZE 4096


  typedef struct thread {

    int       index;

		// What is the actual thread id. tid can be greater than 1024.
		int       tid;
		
		// Who is my parent;
		int       pindex;
		pid_t     ptid;

		// Who is my children in this phase;
		int childBeginIndex;
		int childEndIndex;
 
    pthread_t self; // Results of pthread_self
		char outputBuf[LOG_SIZE];	

    // The following is the parameter about starting function. 
    threadFunction * startRoutine;
    void * startArg; 

		// How much latency for all accesses on this thread?
		struct timeinfo startTime;
		//unsigned long actualRuntime;
		double actualRuntime;
		//unsigned long parentRuntime;
		double parentRuntime;
		unsigned long levelIndex; // In which phase


    // We used this to record the stack range
    void * stackBottom;
    void * stackTop;	

  } thread_t;

  // Whether current thread is inside backtrace phase
  // If yes, then we do not need to get backtrace for current malloc.
  extern __thread thread_t * current;
  //extern __thread bool isBacktrace; 
  //extern bool initialized;
	extern bool _isMultithreading;
	

	// inline char getThreadBuffer()
	inline char * getThreadBuffer() {
		return current->outputBuf;
	}

  // Get thread index
  inline int getTid(void) {
    return current->tid;
  }

  // Get thread index
  inline int getThreadIndex(void) {
    return current->index;
  }
	// Get thread stackTop
	inline unsigned int getThreadStackTop(void) {
    return (unsigned int)current->stackTop;
  }


  enum { USER_HEAP_BASE     = 0x40000000 }; // 1G
  enum { USER_HEAP_SIZE = 1048576UL * 8192  * 8}; // 8G
  enum { MAX_USER_SPACE     = USER_HEAP_BASE + USER_HEAP_SIZE };
  enum { INTERNAL_HEAP_BASE = 0x100000000000 };
  enum { MEMALIGN_MAGIC_WORD = 0xCFBECFBECFBECFBE };
  enum { CALL_SITE_DEPTH = 2 };

  inline size_t alignup(size_t size, size_t alignto) {
    return ((size + (alignto - 1)) & ~(alignto -1));
  }
  
  inline size_t aligndown(size_t addr, size_t alignto) {
    return (addr & ~(alignto -1));
  }
  
  // It is easy to get the 
  inline size_t getCacheStart(void * addr) {
    return aligndown((size_t)addr, CACHE_LINE_SIZE);
  }

  inline unsigned long getMin(unsigned long a, unsigned long b) {
    return (a < b ? a : b);
  }
  
  inline unsigned long getMax(unsigned long a, unsigned long b) {
    return (a > b ? a : b);
  }

#ifdef __cplusplus
};
#endif


#ifdef __cplusplus
class xdefines {
public:
  enum { STACK_SIZE = 1024 * 1024 };
  enum { PHEAP_CHUNK = 1048576 };

  enum { INTERNALHEAP_SIZE = 1048576UL * 1024 * 8};
  enum { PAGE_SIZE = 4096UL };
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PAGE_SIZE-1) };

  enum { MAX_THREADS = 2048 };//4096 };

	enum { MAX_SYNC_ENTRIES = 1000000 };
		
	// We only support 64 heaps in total.
  enum { NUM_HEAPS = 128 };
  enum { MAX_ALIVE_THREADS = NUM_HEAPS };
  enum { MAX_THREAD_LEVELS = 256 };
  
  // 2^6 = 64
  // Since the "int" is most common word, we track reads/writes based on "int"
  enum { WORD_SIZE = sizeof(int) };
  enum { WORDS_PER_CACHE_LINE = CACHE_LINE_SIZE/WORD_SIZE };

  enum { ADDRESS_ALIGNMENT = sizeof(void *) };


  // sizeof(unsigned long) = 8;
  enum { ADDRESS_ALIGNED_BITS = 0xFFFFFFFFFFFFFFF8 };

 

};
#endif
#endif
