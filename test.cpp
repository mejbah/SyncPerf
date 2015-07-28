#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

#if 1
int count = 0;
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;

void *run( void *arg ){
    pthread_mutex_lock(&mutex);
    count++;
    pthread_mutex_unlock(&mutex);

}



int main(int argc, char ** argv) {
	    pthread_t t1, t2, t3;

	//fprintf(stderr, "Calling pthread_mutex_init\n");
   
//    pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&t1,NULL,run,NULL);
    sleep(1);
    pthread_create(&t2,NULL,run,NULL);
    pthread_create(&t3,NULL,run,NULL);
    
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    
    printf("\n\nIn test program : %d\n", count);

    pthread_mutex_destroy(&mutex);

}



#endif
#if 0
//#include "pthread.h"
//#include "stdio.h"
#include<unistd.h>
 
volatile int sv=10;
volatile int x,y,temp=10;
pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t con=PTHREAD_COND_INITIALIZER;
pthread_t child1,child2,child3;
int condition=1,val=1;
int count=1;
 
void *ChildThread1(void *arg)
{

    sleep(1);
    pthread_mutex_lock (&mut);
        printf("wait called\n");
        while(count<10)                      //if while loop with signal complete first don't wait
        {
            printf("In wait\n");
            pthread_cond_wait(&con, &mut);  //wait for the signal with con as condition variable
        }
        x=sv;
        x++;
        sv=x;
        printf("The child1 sv is %d\n",sv);
    pthread_mutex_unlock (&mut);


}
 
void *ChildThread2(void *arg)
{

    sleep(1); 
    while(count<10)
    {
        //sleep(1);
        pthread_mutex_lock (&mut);
            pthread_cond_signal(&con);  //wake up waiting thread with condition variable
                                        //con if it is called before this function
            if(val==1)
            {
                y=sv;
                y--;
                sv=y;
                printf("The child2 sv is %d\n",sv);
                val++;
            }
            count++;
        pthread_mutex_unlock (&mut);
         
    }
    printf("mutex released\n");


}

int main(void)
{
    int co=1;
    pthread_create(&child1,NULL,ChildThread1,NULL);
    pthread_create(&child2,NULL,ChildThread2,NULL);
    //pthread_create(&child3,NULL,ChildThread1,NULL);
    pthread_join(child1,NULL);
    pthread_join(child2,NULL);
    //pthread_join(child3,NULL);
 
    pthread_cond_destroy(&con);
    pthread_mutex_destroy(&mut);
    pthread_exit(NULL);
    return 0;
}

#endif
