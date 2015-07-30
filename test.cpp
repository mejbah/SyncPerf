#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

#if 0
int count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *run( void *arg ){
    pthread_mutex_lock(&mutex);
    count++;
    pthread_mutex_unlock(&mutex);

}



int main(int argc, char ** argv) {
	    pthread_t t1, t2, t3;

	//fprintf(stderr, "Calling pthread_mutex_init\n");
   
    //pthread_mutex_init(&mutex, NULL);
    
    pthread_create(&t1,NULL,run,NULL);
    pthread_create(&t2,NULL,run,NULL);
    pthread_create(&t3,NULL,run,NULL);
    
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    pthread_join(t3,NULL);
    
    printf("\n\nIn test program : %d\n", count);

    pthread_mutex_destroy(&mutex);

}


#endif


pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t con=PTHREAD_COND_INITIALIZER;
pthread_t t1,t2;

int count = 1;
int max_count = 20;
int start = 10;
int end = 15;

void *fcount1(void *arg)
{
    while(1) {
        pthread_mutex_lock(&mut);

        pthread_cond_wait(&con, &mut);
        count++;
        printf("fcount1 val %d\n", count);
        
        pthread_mutex_unlock(&mut);

        if(count > max_count) break;
    }
    
}


void *fcount2(void *arg)
{
    while(1){
        pthread_mutex_lock(&mut);

        if(count < start || count > end)
            pthread_cond_signal(&con);
        else {
            count ++;
            printf("fcount2 val %d\n", count);
        }

        pthread_mutex_unlock(&mut);

        if(count > max_count) break;
    }
}


int main() {

    pthread_create(&t1, NULL, fcount1, NULL);
    pthread_create(&t2, NULL, fcount2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

}

