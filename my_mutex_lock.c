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

#include<my_mutex.h>

#ifndef LLL_MUTEX_LOCK
# define LLL_MUTEX_LOCK(mutex) \
      lll_lock ((mutex)->__data.__lock, PTHREAD_MUTEX_PSHARED (mutex))
# define LLL_MUTEX_TRYLOCK(mutex) \
      lll_trylock ((mutex)->__data.__lock)
# define LLL_ROBUST_MUTEX_LOCK(mutex, id) \
      lll_robust_lock ((mutex)->__data.__lock, id, \
                         PTHREAD_ROBUST_MUTEX_PSHARED (mutex))
//# define LLL_MUTEX_LOCK_ELISION(mutex) \
//      lll_lock_elision ((mutex)->__data.__lock, (mutex)->__data.__elision, \
//                         PTHREAD_MUTEX_PSHARED (mutex))
//# define LLL_MUTEX_TRYLOCK_ELISION(mutex) \
//      lll_trylock_elision((mutex)->__data.__lock, (mutex)->__data.__elision, \
//                         PTHREAD_MUTEX_PSHARED (mutex))
#endif


#define MIN(a,b) (((a)<(b))?(a):(b))

int
pthread_mutex_lock (pthread_mutex_t *mutex)
{
    //printf("In my pthread mutex lock\n");

#ifndef NO_INCR // when called for cond_lock
    if( !is_my_mutex(mutex) ) 
    {
        my_mutex_t *new_mutex = create_mutex(mutex);
        setSyncEntry(mutex, new_mutex);
    }

    my_mutex_t *tmp = get_mutex(mutex);
    tmp->count = tmp->count + 1;
    //printf("---lock count: %u---\n", tmp->count);
    mutex = &tmp->mutex;
#endif	
    assert (sizeof (mutex->__size) >= sizeof (mutex->__data));

    
	int oldval;
	pid_t id = THREAD_GETMEM (THREAD_SELF, tid);

	int retval = 0;
	switch (__builtin_expect (mutex->__data.__kind, PTHREAD_MUTEX_TIMED_NP))
	{
		/* Recursive mutex.  */
	case PTHREAD_MUTEX_RECURSIVE_NP:
		printf("PTHREAD_MUTEX_RECURSIVE_NP\n");
		/* Check whether we already hold the mutex.  */
		if (mutex->__data.__owner == id)
		{
			/* Just bump the counter.  */
			if (__builtin_expect (mutex->__data.__count + 1 == 0, 0))
				/* Overflow of the counter.  */
				return EAGAIN;

			++mutex->__data.__count;

			return 0;
		}

		/* We have to get the mutex.  */
		//LLL_MUTEX_LOCK (mutex->__data.__lock);
		LLL_MUTEX_LOCK (mutex);


		assert (mutex->__data.__owner == 0);
		mutex->__data.__count = 1;
		break;

		/* Error checking mutex.  */
	case PTHREAD_MUTEX_ERRORCHECK_NP:
		/* Check whether we already hold the mutex.  */
		if (__builtin_expect (mutex->__data.__owner == id, 0))
			return EDEADLK;

		/* FALLTHROUGH */

	case PTHREAD_MUTEX_TIMED_NP:
simple:
		//printf("PTHREAD_MUTEX_TIMED_NP\n");
		/* Normal mutex.  */
		// LLL_MUTEX_LOCK (mutex->__data.__lock);
		LLL_MUTEX_LOCK (mutex);
		assert (mutex->__data.__owner == 0);
		break;

	case PTHREAD_MUTEX_ADAPTIVE_NP:
		//if (! __is_smp)
		//goto simple;
		printf("PTHREAD_MUTEX_ADAPTIVE_NP\n");
		if (LLL_MUTEX_TRYLOCK (mutex) != 0)
		{
			int cnt = 0;
			int max_cnt = MIN (MAX_ADAPTIVE_COUNT,
				mutex->__data.__spins * 2 + 10);
			do
			{
				if (cnt++ >= max_cnt)
				{
					// LLL_MUTEX_LOCK (mutex->__data.__lock);
					LLL_MUTEX_LOCK (mutex);
					break;
				}

#ifdef BUSY_WAIT_NOP
				BUSY_WAIT_NOP;
#endif
			}
			while (LLL_MUTEX_TRYLOCK (mutex) != 0);

			mutex->__data.__spins += (cnt - mutex->__data.__spins) / 8;
		}
		assert (mutex->__data.__owner == 0);
		break;

	case PTHREAD_MUTEX_ROBUST_RECURSIVE_NP:
	case PTHREAD_MUTEX_ROBUST_ERRORCHECK_NP:
	case PTHREAD_MUTEX_ROBUST_NORMAL_NP:
	case PTHREAD_MUTEX_ROBUST_ADAPTIVE_NP:
    printf("PTHREAD_MUTEX_ROBUST_*_NP\n");
		THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending,
			&mutex->__data.__list.__next);

		oldval = mutex->__data.__lock;
		do
		{
again:
			if ((oldval & FUTEX_OWNER_DIED) != 0)
			{
				/* The previous owner died.  Try locking the mutex.  */
				int newval = id;
#ifdef NO_INCR
				newval |= FUTEX_WAITERS;
#endif

				newval
					= atomic_compare_and_exchange_val_acq (&mutex->__data.__lock,
					newval, oldval);

				if (newval != oldval)
				{
					oldval = newval;
					goto again;
				}

				/* We got the mutex.  */
				mutex->__data.__count = 1;
				/* But it is inconsistent unless marked otherwise.  */
				mutex->__data.__owner = PTHREAD_MUTEX_INCONSISTENT;

				ENQUEUE_MUTEX (mutex);
				THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);

				/* Note that we deliberately exit here.  If we fall
				through to the end of the function __nusers would be
				incremented which is not correct because the old
				owner has to be discounted.  If we are not supposed
				to increment __nusers we actually have to decrement
				it here.  */
#ifdef NO_INCR
				--mutex->__data.__nusers;
#endif

				return EOWNERDEAD;
			}
			/* Check whether we already hold the mutex.  */
			if (__builtin_expect ((oldval & FUTEX_TID_MASK) == id, 0))
			{
				if (mutex->__data.__kind
					== PTHREAD_MUTEX_ROBUST_ERRORCHECK_NP)
				{
					THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending,
						NULL);
					return EDEADLK;
				}

				if (mutex->__data.__kind
					== PTHREAD_MUTEX_ROBUST_RECURSIVE_NP)
				{
					THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending,
						NULL);

					/* Just bump the counter.  */
					if (__builtin_expect (mutex->__data.__count + 1 == 0, 0))
						/* Overflow of the counter.  */
						return EAGAIN;

					++mutex->__data.__count;

					return 0;
				}
			}

			oldval = LLL_ROBUST_MUTEX_LOCK (mutex, id);

			if (__builtin_expect (mutex->__data.__owner
				== PTHREAD_MUTEX_NOTRECOVERABLE, 0))
			{
				/* This mutex is now not recoverable.  */
				mutex->__data.__count = 0;
				lll_unlock (mutex->__data.__lock, PTHREAD_MUTEX_PSHARED (mutex));
				THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);
				return ENOTRECOVERABLE;
			}
		}
		while ((oldval & FUTEX_OWNER_DIED) != 0);

		mutex->__data.__count = 1;
		ENQUEUE_MUTEX (mutex);
		THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);
		break;

	case PTHREAD_MUTEX_PI_RECURSIVE_NP:
	case PTHREAD_MUTEX_PI_ERRORCHECK_NP:
	case PTHREAD_MUTEX_PI_NORMAL_NP:
	case PTHREAD_MUTEX_PI_ADAPTIVE_NP:
	case PTHREAD_MUTEX_PI_ROBUST_RECURSIVE_NP:
	case PTHREAD_MUTEX_PI_ROBUST_ERRORCHECK_NP:
	case PTHREAD_MUTEX_PI_ROBUST_NORMAL_NP:
	case PTHREAD_MUTEX_PI_ROBUST_ADAPTIVE_NP:
		{
			printf("PTHREAD_MUTEX_PI_ROBUST_*_NP\n");
			int kind = mutex->__data.__kind & PTHREAD_MUTEX_KIND_MASK_NP;
			int robust = mutex->__data.__kind & PTHREAD_MUTEX_ROBUST_NORMAL_NP;

			if (robust)
				/* Note: robust PI futexes are signaled by setting bit 0.  */
				THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending,
				(void *) (((uintptr_t) &mutex->__data.__list.__next)
				| 1));

			oldval = mutex->__data.__lock;

			/* Check whether we already hold the mutex.  */
			if (__builtin_expect ((oldval & FUTEX_TID_MASK) == id, 0))
			{
				if (kind == PTHREAD_MUTEX_ERRORCHECK_NP)
				{
					THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);
					return EDEADLK;
				}

				if (kind == PTHREAD_MUTEX_RECURSIVE_NP)
				{
					THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);

					/* Just bump the counter.  */
					if (__builtin_expect (mutex->__data.__count + 1 == 0, 0))
						/* Overflow of the counter.  */
						return EAGAIN;

					++mutex->__data.__count;

					return 0;
				}
			}

			int newval = id;
#ifdef NO_INCR
			newval |= FUTEX_WAITERS;
#endif
			oldval = atomic_compare_and_exchange_val_acq (&mutex->__data.__lock,
				newval, 0);

			if (oldval != 0)
			{
				/* The mutex is locked.  The kernel will now take care of
				everything.  */
				INTERNAL_SYSCALL_DECL (__err);
				//int __err;
				int e = INTERNAL_SYSCALL (futex, __err, 4, &mutex->__data.__lock,
					FUTEX_LOCK_PI, 1, 0);

				if (INTERNAL_SYSCALL_ERROR_P (e, __err)
					&& (INTERNAL_SYSCALL_ERRNO (e, __err) == ESRCH
					|| INTERNAL_SYSCALL_ERRNO (e, __err) == EDEADLK))
				{
					assert (INTERNAL_SYSCALL_ERRNO (e, __err) != EDEADLK
						|| (kind != PTHREAD_MUTEX_ERRORCHECK_NP
						&& kind != PTHREAD_MUTEX_RECURSIVE_NP));
					/* ESRCH can happen only for non-robust PI mutexes where
					the owner of the lock died.  */
					assert (INTERNAL_SYSCALL_ERRNO (e, __err) != ESRCH || !robust);

					/* Delay the thread indefinitely.  */
					while (1)
						//pause_not_cancel ();
						; 
				}

				oldval = mutex->__data.__lock;

				assert (robust || (oldval & FUTEX_OWNER_DIED) == 0);
			}

			if (__builtin_expect (oldval & FUTEX_OWNER_DIED, 0))
			{
				atomic_and (&mutex->__data.__lock, ~FUTEX_OWNER_DIED);

				/* We got the mutex.  */
				mutex->__data.__count = 1;
				/* But it is inconsistent unless marked otherwise.  */
				mutex->__data.__owner = PTHREAD_MUTEX_INCONSISTENT;

				ENQUEUE_MUTEX_PI (mutex);
				THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);

				/* Note that we deliberately exit here.  If we fall
				through to the end of the function __nusers would be
				incremented which is not correct because the old owner
				has to be discounted.  If we are not supposed to
				increment __nusers we actually have to decrement it here.  */
#ifdef NO_INCR
				--mutex->__data.__nusers;
#endif

				return EOWNERDEAD;
			}

			if (robust
				&& __builtin_expect (mutex->__data.__owner
				== PTHREAD_MUTEX_NOTRECOVERABLE, 0))
			{
				/* This mutex is now not recoverable.  */
				mutex->__data.__count = 0;

				INTERNAL_SYSCALL_DECL (__err);
				INTERNAL_SYSCALL (futex, __err, 4, &mutex->__data.__lock,
					FUTEX_UNLOCK_PI, 0, 0);

				THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);
				return ENOTRECOVERABLE;
			}

			mutex->__data.__count = 1;
			if (robust)
			{
				ENQUEUE_MUTEX_PI (mutex);
				THREAD_SETMEM (THREAD_SELF, robust_head.list_op_pending, NULL);
			}
		}
		break;

	case PTHREAD_MUTEX_PP_RECURSIVE_NP:
	case PTHREAD_MUTEX_PP_ERRORCHECK_NP:
	case PTHREAD_MUTEX_PP_NORMAL_NP:
	case PTHREAD_MUTEX_PP_ADAPTIVE_NP:
		{
			int kind = mutex->__data.__kind & PTHREAD_MUTEX_KIND_MASK_NP;
            printf("PTHREAD_MUTEX_PP_*_NP\n");
			oldval = mutex->__data.__lock;

			/* Check whether we already hold the mutex.  */
			if (mutex->__data.__owner == id)
			{
				if (kind == PTHREAD_MUTEX_ERRORCHECK_NP)
					return EDEADLK;

				if (kind == PTHREAD_MUTEX_RECURSIVE_NP)
				{
					/* Just bump the counter.  */
					if (__builtin_expect (mutex->__data.__count + 1 == 0, 0))
						/* Overflow of the counter.  */
						return EAGAIN;

					++mutex->__data.__count;

					return 0;
				}
			}

			int oldprio = -1, ceilval;
			do
			{
				int ceiling = (oldval & PTHREAD_MUTEX_PRIO_CEILING_MASK)
					>> PTHREAD_MUTEX_PRIO_CEILING_SHIFT;

				if (__pthread_current_priority () > ceiling)
				{
					if (oldprio != -1)
						__pthread_tpp_change_priority (oldprio, -1);
					return EINVAL;
				}

				retval = __pthread_tpp_change_priority (oldprio, ceiling);
				if (retval)
					return retval;

				ceilval = ceiling << PTHREAD_MUTEX_PRIO_CEILING_SHIFT;
				oldprio = ceiling;

				oldval
					= atomic_compare_and_exchange_val_acq (&mutex->__data.__lock,
#ifdef NO_INCR
					ceilval | 2,
#else
					ceilval | 1,
#endif
					ceilval);

				if (oldval == ceilval)
					break;

				do
				{
					oldval
						= atomic_compare_and_exchange_val_acq (&mutex->__data.__lock,
						ceilval | 2,
						ceilval | 1);

					if ((oldval & PTHREAD_MUTEX_PRIO_CEILING_MASK) != ceilval)
						break;
					if (oldval != ceilval)
						lll_futex_wait (&mutex->__data.__lock, ceilval | 2,PTHREAD_MUTEX_PSHARED (mutex));


				}
				while (atomic_compare_and_exchange_val_acq (&mutex->__data.__lock,
					ceilval | 2, ceilval)
					!= ceilval);
			}
			while ((oldval & PTHREAD_MUTEX_PRIO_CEILING_MASK) != ceilval);

			assert (mutex->__data.__owner == 0);
			mutex->__data.__count = 1;
		}
		break;

	default:
		/* Correct code cannot set any other type.  */
		return EINVAL;
	}

	/* Record the ownership.  */
	mutex->__data.__owner = id;
#ifndef NO_INCR
	++mutex->__data.__nusers;
#endif
    return retval;
}

#ifdef NO_INCR
void
__pthread_mutex_cond_lock_adjust (mutex)
pthread_mutex_t *mutex;
{
	assert ((mutex->__data.__kind & PTHREAD_MUTEX_PRIO_INHERIT_NP) != 0);
	assert ((mutex->__data.__kind & PTHREAD_MUTEX_ROBUST_NORMAL_NP) == 0);
	assert ((mutex->__data.__kind & PTHREAD_MUTEX_PSHARED_BIT) == 0);

	/* Record the ownership.  */
	pid_t id = THREAD_GETMEM (THREAD_SELF, tid);
	mutex->__data.__owner = id;

	if (mutex->__data.__kind == PTHREAD_MUTEX_PI_RECURSIVE_NP)
		++mutex->__data.__count;
}
#endif
