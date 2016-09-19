//coder : archer
#include "threadpool.h"

extern mythread_pool *pool;

void pool_init(int max_thread_num)
{
	pool=(mythread_pool *) malloc(sizeof(mythread_pool));
	
	pthread_mutex_init(&(pool->queue_lock),NULL);
	pthread_cond_init(&(pool->queue_ready),NULL);

	pool->queue_head=NULL;
	pool->cur_queue_size=0;
	pool->is_shutdown=0;
	
	pool->max_thread_num=max_thread_num;
	pool->threadid = (pthread_t *) malloc(max_thread_num*sizeof(pthread_t));
	
	int i=0;
	for (i=0;i<max_thread_num;i++)
		{
			pthread_create(&(pool->threadid[i]),NULL,thread_routine,NULL);
		}
}
//init the thread pool;
int pool_add_worker(void *(*process)(void *arg),void *arg)
{
	mythread_worker * newworker=(mythread_worker *) malloc(sizeof(mythread_worker));
	newworker->process=process;
	newworker->arg=arg;
	newworker->next=NULL;
	
	pthread_mutex_lock(&(pool->queue_lock));
	
	mythread_worker *p=pool->queue_head;
	if (p!=NULL)
	{
		while (p->next!=NULL) p=p->next;
		p->next=newworker;
	}
	else
	{
		pool->queue_head=newworker;
	}
	
	assert(pool->queue_head != NULL);

	pool->cur_queue_size++;
	
	pthread_cond_signal(&(pool->queue_ready));
	//send signal to threads;
	pthread_mutex_unlock(&(pool->queue_lock));
	
	return 0;
}
//add work;
void *thread_routine(void *arg)
{
	arg_t arg_tmp;
	int fd;

	if ((fd = socket(AF_INET,SOCK_DGRAM,0))==-1)
	{
		perror("routime socket");
		exit(1);
	}

	while(1)
	{
		pthread_mutex_lock(&(pool->queue_lock));
		
		while(pool->cur_queue_size == 0 && !pool->is_shutdown)
		{
			pthread_cond_wait(&(pool->queue_ready),&(pool->queue_lock));
		}

		if (pool->is_shutdown)
		{
			pthread_mutex_unlock(&(pool->queue_lock));
			close(fd);
			pthread_exit(NULL);
		}


		assert(pool->cur_queue_size != 0);
		assert(pool->queue_head != NULL);
		
		pool->cur_queue_size--;
		mythread_worker * myworker=pool->queue_head;
		pool->queue_head=myworker->next;
	
		pthread_mutex_unlock(&(pool->queue_lock));		
		
		strcpy(arg_tmp.buf,myworker->arg);
		arg_tmp.fd=fd;
	
		(*(myworker->process))((void *)&arg_tmp);
		//handle work;
		free(myworker->arg);
		free(myworker);
		myworker=NULL;
	}
	close(fd);
	pthread_exit(NULL); //for safety
}

int pool_destroy()
{
	if (pool->is_shutdown)
		return -1;
	pool->is_shutdown=1;

	pthread_cond_broadcast(&(pool->queue_ready));
	
	int i;
	for (i=0;i<pool->max_thread_num;i++)
		pthread_join(pool->threadid[i],NULL);
	free(pool->threadid);
	//make sure every thread is done;
	mythread_worker * p=NULL;
	while (pool->queue_head!=NULL)
		{
			p=pool->queue_head;
			pool->queue_head=p->next;
			free(p);
		}

	pthread_mutex_destroy(&(pool->queue_lock));
	pthread_cond_destroy(&(pool->queue_ready));

	free(pool);
	pool=NULL;

	return 0;		
}
