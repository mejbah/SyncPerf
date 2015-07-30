#include <errno.h>
#include "pthreadP.h"
#include "my_mutex.h"

//#include <stap-probe.h>


int
pthread_mutex_destroy (mutex)
         pthread_mutex_t *mutex;
{
      //LIBC_PROBE (mutex_destroy, 1, mutex);
      printf("In my mutex destroy\n");
      my_mutex_t *tmp = get_mutex(mutex);
      mutex = &tmp->mutex;
      if ((mutex->__data.__kind & PTHREAD_MUTEX_ROBUST_NORMAL_NP) == 0
                      && mutex->__data.__nusers != 0)
                return EBUSY;
      /* Set to an invalid value.  */
      mutex->__data.__kind = -1;

      return 0;
}

