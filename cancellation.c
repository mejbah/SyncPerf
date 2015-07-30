/* Copyright (C) 2002-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <setjmp.h>
#include <stdlib.h>
#include "pthreadP.h"
//#include <futex-internal.h>
#define FUTEX_PRIVATE LLL_PRIVATE

#if FUTEX_PRIVATE != 0
# error FUTEX_PRIVATE must be equal to 0
#endif

static __always_inline int
futex_wait (unsigned int *futex_word, unsigned int expected, int private)
{
      int err = lll_futex_timed_wait (futex_word, expected, NULL, private);
        switch (err)
                {
                        case 0:
                                case -EAGAIN:
                                case -EINTR:
                                  return -err;

                                      case -ETIMEDOUT: /* Cannot have happened as we provided no timeout.  */
                                      case -EFAULT: /* Must have been caused by a glibc or application bug.  */
                                      case -EINVAL: /* Either due to wrong alignment or due to the timeout not
                                                                    being normalized.  Must have been caused by a glibc or
                                                                                 application bug.  */
                                      case -ENOSYS: /* Must have been caused by a glibc bug.  */
                                      /* No other errors are documented at this time.  */
                                      default:
                                        //futex_fatal_error ();
                                        __libc_fatal ("The futex facility returned an unexpected error code.");
                                            }
}

/* Ignore the value of an expression when a cast to void does not
 *    suffice (in particular, for a call to a function declared with
 *       attribute warn_unused_result).  */
#define ignore_value(x) \
      ({ __typeof__ (x) __ignored_value = (x); (void) __ignored_value; })


/* Like futex_wait but does not provide any indication why we stopped waiting.
 *    Thus, when this function returns, you have to always check FUTEX_WORD to
 *       determine whether you need to continue waiting, and you cannot detect
 *          whether the waiting was interrupted by a signal.  Example use:
 *               while (atomic_load_relaxed (&futex_word) == 23)
 *                      futex_wait_simple (&futex_word, 23, FUTEX_PRIVATE);
 *                         This is common enough to make providing this wrapper worthwhile.  */
static __always_inline void
futex_wait_simple (unsigned int *futex_word, unsigned int expected,
                   int private)
{
      ignore_value (futex_wait (futex_word, expected, private));
}


/* The next two functions are similar to pthread_setcanceltype() but
   more specialized for the use in the cancelable functions like write().
   They do not need to check parameters etc.  */
int
__pthread_enable_asynccancel (void)
{
  struct pthread *self = THREAD_SELF;
  int oldval = THREAD_GETMEM (self, cancelhandling);

  while (1)
    {
      int newval = oldval | CANCELTYPE_BITMASK;

      if (newval == oldval)
	break;

      int curval = THREAD_ATOMIC_CMPXCHG_VAL (self, cancelhandling, newval,
					      oldval);
      if (__glibc_likely (curval == oldval))
	{
	  if (CANCEL_ENABLED_AND_CANCELED_AND_ASYNCHRONOUS (newval))
	    {
	      THREAD_SETMEM (self, result, PTHREAD_CANCELED);
	      __do_cancel ();
	    }

	  break;
	}

      /* Prepare the next round.  */
      oldval = curval;
    }

  return oldval;
}


void
__pthread_disable_asynccancel (int oldtype)
{
  /* If asynchronous cancellation was enabled before we do not have
     anything to do.  */
  if (oldtype & CANCELTYPE_BITMASK)
    return;

  struct pthread *self = THREAD_SELF;
  int newval;

  int oldval = THREAD_GETMEM (self, cancelhandling);

  while (1)
    {
      newval = oldval & ~CANCELTYPE_BITMASK;

      int curval = THREAD_ATOMIC_CMPXCHG_VAL (self, cancelhandling, newval,
					      oldval);
      if (__glibc_likely (curval == oldval))
	break;

      /* Prepare the next round.  */
      oldval = curval;
    }

  /* We cannot return when we are being canceled.  Upon return the
     thread might be things which would have to be undone.  The
     following loop should loop until the cancellation signal is
     delivered.  */
  while (__builtin_expect ((newval & (CANCELING_BITMASK | CANCELED_BITMASK))
			   == CANCELING_BITMASK, 0))
    {
      futex_wait_simple ((unsigned int *) &self->cancelhandling, newval,
			 FUTEX_PRIVATE);
      newval = THREAD_GETMEM (self, cancelhandling);
    }
}
