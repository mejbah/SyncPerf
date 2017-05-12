#ifndef _PTHREADP_H
#define _PTHREADP_H 1

#include <pthread.h>
#include <setjmp.h>
#include <stdbool.h>
#include <sys/syscall.h>
//#include "descr.h"
#include <tls.h>
#include <lowlevellock.h>
//#include <stackinfo.h>
#include <internaltypes.h>
//#include <pthread-functions.h>
#include <atomic.h>
//#include <kernel-features.h>
#include <errno.h>
//#include <nptl-signals.h>


//mejbah added
#define FUTEX_LOCK_PI       6
#define FUTEX_UNLOCK_PI     7
#define FUTEX_TRYLOCK_PI    8

/* Adaptive mutex definitions.  */
#ifndef MAX_ADAPTIVE_COUNT
# define MAX_ADAPTIVE_COUNT 100
#endif

/* Bits used in robust mutex implementation.  */
#define FUTEX_WAITERS       0x80000000
#define FUTEX_OWNER_DIED    0x40000000
#define FUTEX_TID_MASK      0x3fffffff

/* Magic cookie representing robust mutex with dead owner.  */
#define PTHREAD_MUTEX_INCONSISTENT  INT_MAX
/* Magic cookie representing not recoverable robust mutex.  */
#define PTHREAD_MUTEX_NOTRECOVERABLE    (INT_MAX - 1)

//extern int __pthread_mutex_cond_lock (pthread_mutex_t *__mutex);
//extern int __pthread_mutex_unlock_usercnt (pthread_mutex_t *__mutex,
//                               int __decr);
enum
{
  PTHREAD_MUTEX_KIND_MASK_NP = 3,

  PTHREAD_MUTEX_ELISION_NP    = 256,
  PTHREAD_MUTEX_NO_ELISION_NP = 512,

  PTHREAD_MUTEX_ROBUST_NORMAL_NP = 16,
  PTHREAD_MUTEX_ROBUST_RECURSIVE_NP
  = PTHREAD_MUTEX_ROBUST_NORMAL_NP | PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ROBUST_ERRORCHECK_NP
  = PTHREAD_MUTEX_ROBUST_NORMAL_NP | PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_ROBUST_ADAPTIVE_NP
  = PTHREAD_MUTEX_ROBUST_NORMAL_NP | PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_PRIO_INHERIT_NP = 32,
  PTHREAD_MUTEX_PI_NORMAL_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_NORMAL,
  PTHREAD_MUTEX_PI_RECURSIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_PI_ERRORCHECK_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_PI_ADAPTIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_PI_ROBUST_NORMAL_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_NORMAL_NP,
  PTHREAD_MUTEX_PI_ROBUST_RECURSIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_RECURSIVE_NP,
  PTHREAD_MUTEX_PI_ROBUST_ERRORCHECK_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_ERRORCHECK_NP,
  PTHREAD_MUTEX_PI_ROBUST_ADAPTIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_ADAPTIVE_NP,
  PTHREAD_MUTEX_PRIO_PROTECT_NP = 64,
  PTHREAD_MUTEX_PP_NORMAL_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_NORMAL,
  PTHREAD_MUTEX_PP_RECURSIVE_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_PP_ERRORCHECK_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_PP_ADAPTIVE_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_ELISION_FLAGS_NP
  = PTHREAD_MUTEX_ELISION_NP | PTHREAD_MUTEX_NO_ELISION_NP,

  PTHREAD_MUTEX_TIMED_ELISION_NP =
	  PTHREAD_MUTEX_TIMED_NP | PTHREAD_MUTEX_ELISION_NP,
  PTHREAD_MUTEX_TIMED_NO_ELISION_NP =
	  PTHREAD_MUTEX_TIMED_NP | PTHREAD_MUTEX_NO_ELISION_NP,
};

/* The kernel when waking robust mutexes on exit never uses
 *    FUTEX_PRIVATE_FLAG FUTEX_WAKE.  */
#define PTHREAD_ROBUST_MUTEX_PSHARED(m) LLL_SHARED

#if LLL_PRIVATE == 0 && LLL_SHARED == 128
# define PTHREAD_MUTEX_PSHARED(m) \
      ((m)->__data.__kind & 128)
#else
# define PTHREAD_MUTEX_PSHARED(m) \
      (((m)->__data.__kind & 128) ? LLL_SHARED : LLL_PRIVATE)
#endif


#define PTHREAD_MUTEX_PSHARED_BIT 128
#define PTHREAD_MUTEX_TYPE(m) \
      ((m)->__data.__kind & 127)
/* Don't include NO_ELISION, as that type is always the same
 *    as the underlying lock type.  */
#define PTHREAD_MUTEX_TYPE_ELISION(m) \
      ((m)->__data.__kind & (127|PTHREAD_MUTEX_ELISION_NP))
/* Ceiling in __data.__lock.  __data.__lock is signed, so don't
 *    use the MSB bit in there, but in the mask also include that bit,
 *       so that the compiler can optimize & PTHREAD_MUTEX_PRIO_CEILING_MASK
 *          masking if the value is then shifted down by
 *             PTHREAD_MUTEX_PRIO_CEILING_SHIFT.  */
#define PTHREAD_MUTEX_PRIO_CEILING_SHIFT    19
#define PTHREAD_MUTEX_PRIO_CEILING_MASK     0xfff80000
/* Thread Priority Protection.  */
extern int __sched_fifo_min_prio;// attribute_hidden;
extern int __sched_fifo_max_prio;// attribute_hidden;
extern void __init_sched_fifo_prio (void) ;//attribute_hidden;
extern int __pthread_tpp_change_priority (int prev_prio, int new_prio);//attribute_hidden;
extern int __pthread_current_priority (void);//attribute_hidden;

/* Flags in mutex attr.  */
#define PTHREAD_MUTEXATTR_PROTOCOL_SHIFT    28
#define PTHREAD_MUTEXATTR_PROTOCOL_MASK     0x30000000
#define PTHREAD_MUTEXATTR_PRIO_CEILING_SHIFT    12
#define PTHREAD_MUTEXATTR_PRIO_CEILING_MASK 0x00fff000
#define PTHREAD_MUTEXATTR_FLAG_ROBUST       0x40000000
#define PTHREAD_MUTEXATTR_FLAG_PSHARED      0x80000000
#define PTHREAD_MUTEXATTR_FLAG_BITS \
      (PTHREAD_MUTEXATTR_FLAG_ROBUST | PTHREAD_MUTEXATTR_FLAG_PSHARED \
          | PTHREAD_MUTEXATTR_PROTOCOL_MASK | PTHREAD_MUTEXATTR_PRIO_CEILING_MASK)


#ifndef __ASSUME_SET_ROBUST_LIST
/* Negative if we do not have the system call and we can use it.  */
//extern int __set_robust_list_avail ;//attribute_hidden;
#endif

/* Test if the mutex is suitable for the FUTEX_WAIT_REQUEUE_PI operation.  */
#if (defined lll_futex_wait_requeue_pi \
             && defined __ASSUME_REQUEUE_PI)
# define USE_REQUEUE_PI(mut) \
       ((mut) && (mut) != (void *) ~0l \
            && (((mut)->__data.__kind \
                         & (PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_NORMAL_NP)) \
                    == PTHREAD_MUTEX_PRIO_INHERIT_NP))
#else
# define USE_REQUEUE_PI(mut) 0
#endif

/* Called when a thread reacts on a cancellation request.  */
static inline void
__attribute ((noreturn, always_inline))
__do_cancel (void)
{
      struct pthread *self = THREAD_SELF;

        /* Make sure we get no more cancellations.  */
        THREAD_ATOMIC_BIT_SET (self, cancelhandling, EXITING_BIT);

          __pthread_unwind ((__pthread_unwind_buf_t *)
                              THREAD_GETMEM (self, cleanup_jmp_buf));
}





#endif  /* pthreadP.h */



