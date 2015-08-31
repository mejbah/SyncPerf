#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <lowlevellock.h>
#include <pthread.h>
//#include <kernel-features.h>
#include <atomic.h>
#include "pthreadP.h"
#include <sysdep.h>
#include "mutex_manager.h"

struct _condvar_cleanup_buffer
{
      int oldtype;
      pthread_cond_t *cond;
      pthread_mutex_t *mutex;
      unsigned int bc_seq;
};

void
__attribute__ ((visibility ("hidden")))
__condvar_cleanup (void *arg)
{
  struct _condvar_cleanup_buffer *cbuffer =
    (struct _condvar_cleanup_buffer *) arg;
  unsigned int destroying;
  int pshared = (cbuffer->cond->__data.__mutex == (void *) ~0l)
		? LLL_SHARED : LLL_PRIVATE;

  /* We are going to modify shared data.  */
  lll_lock (cbuffer->cond->__data.__lock, pshared);

  if (cbuffer->bc_seq == cbuffer->cond->__data.__broadcast_seq)
    {
      /* This thread is not waiting anymore.  Adjust the sequence counters
	 appropriately.  We do not increment WAKEUP_SEQ if this would
	 bump it over the value of TOTAL_SEQ.  This can happen if a thread
	 was woken and then canceled.  */
      if (cbuffer->cond->__data.__wakeup_seq
	  < cbuffer->cond->__data.__total_seq)
	{
	  ++cbuffer->cond->__data.__wakeup_seq;
	  ++cbuffer->cond->__data.__futex;
	}
      ++cbuffer->cond->__data.__woken_seq;
    }

  cbuffer->cond->__data.__nwaiters -= 1 << COND_NWAITERS_SHIFT;

  /* If pthread_cond_destroy was called on this variable already,
     notify the pthread_cond_destroy caller all waiters have left
     and it can be successfully destroyed.  */
  destroying = 0;
  if (cbuffer->cond->__data.__total_seq == -1ULL
      && cbuffer->cond->__data.__nwaiters < (1 << COND_NWAITERS_SHIFT))
    {
      lll_futex_wake (&cbuffer->cond->__data.__nwaiters, 1, pshared);
      destroying = 1;
    }

  /* We are done.  */
  lll_unlock (cbuffer->cond->__data.__lock, pshared);

  /* Wake everybody to make sure no condvar signal gets lost.  */
  if (! destroying)
    lll_futex_wake (&cbuffer->cond->__data.__futex, INT_MAX, pshared);

  /* Get the mutex before returning unless asynchronous cancellation
     is in effect.  We don't try to get the mutex if we already own it.  */

  if (!(USE_REQUEUE_PI (cbuffer->mutex))
      || ((cbuffer->mutex->__data.__lock & FUTEX_TID_MASK)
	  != THREAD_GETMEM (THREAD_SELF, tid)))
  {
    __pthread_mutex_cond_lock (cbuffer->mutex);
  }
  else
    __pthread_mutex_cond_lock_adjust (cbuffer->mutex);
}



int
pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	struct _pthread_cleanup_buffer buffer;
	struct _condvar_cleanup_buffer cbuffer;
	int err;
	int pshared = (cond->__data.__mutex == (void *) ~0l)
		? LLL_SHARED : LLL_PRIVATE;
#if (defined lll_futex_wait_requeue_pi \
	&& defined __ASSUME_REQUEUE_PI)
	int pi_flag = 0;
#endif
#ifndef ORIGINAL
     my_mutex_t *tmp = (my_mutex_t *)get_mutex(mutex);
		 tmp->count = tmp->count + 1; // no of times mutex accessed
     mutex = &tmp->mutex;
#endif

	//LIBC_PROBE (cond_wait, 2, cond, mutex);

	/* Make sure we are alone.  */
	lll_lock (cond->__data.__lock, pshared);

	/* Now we can release the mutex.  */
	err = __pthread_mutex_unlock_usercnt (mutex, 0);
    if (err)
    {
      lll_unlock (cond->__data.__lock, pshared);
      return err;
    }

	/* We have one new user of the condvar.  */
	++cond->__data.__total_seq;
	++cond->__data.__futex;
	cond->__data.__nwaiters += 1 << COND_NWAITERS_SHIFT;

	/* Remember the mutex we are using here.  If there is already a
	different address store this is a bad user bug.  Do not store
	anything for pshared condvars.  */
	if (cond->__data.__mutex != (void *) ~0l)
		cond->__data.__mutex = mutex;

	/* Prepare structure passed to cancellation handler.  */
	cbuffer.cond = cond;
	cbuffer.mutex = mutex;

	/* Before we block we enable cancellation.  Therefore we have to
	install a cancellation handler.  */
	__pthread_cleanup_push (&buffer, __condvar_cleanup, &cbuffer);
     
	/* The current values of the wakeup counter.  The "woken" counter
	must exceed this value.  */
	unsigned long long int val;
	unsigned long long int seq;
	val = seq = cond->__data.__wakeup_seq;
	/* Remember the broadcast counter.  */
	cbuffer.bc_seq = cond->__data.__broadcast_seq;
#ifndef ORIGINAL
  //mejbah added for wait time
	futex_start_timestamp(tmp);
	//mejbah added end
#endif
	do
	{
		unsigned int futex_val = cond->__data.__futex;
		/* Prepare to wait.  Release the condvar futex.  */
		lll_unlock (cond->__data.__lock, pshared);

		/* Enable asynchronous cancellation.  Required by the standard.  */
		cbuffer.oldtype = __pthread_enable_asynccancel ();

#if (defined lll_futex_wait_requeue_pi \
	&& defined __ASSUME_REQUEUE_PI)
		/* If pi_flag remained 1 then it means that we had the lock and the mutex
		but a spurious waker raced ahead of us.  Give back the mutex before
		going into wait again.  */
		if (pi_flag)
		{
			__pthread_mutex_cond_lock_adjust (mutex);
			__pthread_mutex_unlock_usercnt (mutex, 0);
		}
		pi_flag = USE_REQUEUE_PI (mutex);

		if (pi_flag)
		{
			err = lll_futex_wait_requeue_pi (&cond->__data.__futex,
				futex_val, &mutex->__data.__lock,
				pshared);

			pi_flag = (err == 0);
		}
		else 
#endif
        {
#ifdef MY_DEBUG
            printf("In my pthread cond wait\n");
#endif

			/* Wait until woken by signal or broadcast.  */
			lll_futex_wait (&cond->__data.__futex, futex_val, pshared);
        }

		/* Disable asynchronous cancellation.  */
		__pthread_disable_asynccancel (cbuffer.oldtype);

		/* We are going to look at shared data again, so get the lock.  */
		lll_lock (cond->__data.__lock, pshared);

		/* If a broadcast happened, we are done.  */
		if (cbuffer.bc_seq != cond->__data.__broadcast_seq)
			goto bc_out;

		/* Check whether we are eligible for wakeup.  */
		val = cond->__data.__wakeup_seq;
	}
	while (val == seq || cond->__data.__woken_seq == val);

	/* Another thread woken up.  */
	++cond->__data.__woken_seq;

bc_out:

	cond->__data.__nwaiters -= 1 << COND_NWAITERS_SHIFT;

	/* If pthread_cond_destroy was called on this varaible already,
	notify the pthread_cond_destroy caller all waiters have left
	and it can be successfully destroyed.  */
	if (cond->__data.__total_seq == -1ULL
		&& cond->__data.__nwaiters < (1 << COND_NWAITERS_SHIFT))
		lll_futex_wake (&cond->__data.__nwaiters, 1, pshared);

	/* We are done with the condvar.  */
	lll_unlock (cond->__data.__lock, pshared);

	/* The cancellation handling is back to normal, remove the handler.  */
	 __pthread_cleanup_pop (&buffer, 0);

#ifndef ORIGINAL
	//mejbah added for wait time
	add_futex_wait(tmp);
	//mejbah added end
#endif

	/* Get the mutex before returning.  Not needed for PI.  */
#if (defined lll_futex_wait_requeue_pi \
	&& defined __ASSUME_REQUEUE_PI)
	  if (pi_flag)
	    {
	      __pthread_mutex_cond_lock_adjust (mutex);
	      return 0;
	    }
	  else
	#endif
	    return __pthread_mutex_cond_lock (mutex);
	//return pthread_mutex_lock(mutex);//mejbah added
}

