#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ringbuffer.h"

void *handle(void *arg)
{
	RING_BUFFER_s *rb =(RING_BUFFER_s *)arg;
	char buf[10]={0};

	while(1){
		memset(buf,0,sizeof(buf));
		ringbuffer_get(rb, buf, 1,0);
		printf("ringbuffer read =[%s]\n",buf);

		printf("ringbuffer used=%ld,unused=%ld\n",ringbuffer_used(rb),ringbuffer_unused(rb));
		sleep(1);
	}
}

int main()
{
    printf("Hello world\n");
	RING_BUFFER_s *rb = ringbuffer_create(100);
	pthread_t p;
	pthread_create(&p, NULL, handle, (void*)rb);

	printf("ringbuffer size =%ld emptey=%d\n",rb->cap, ringbuffer_empty(rb));
	printf("ringbuffer used=%ld,unused=%ld\n",ringbuffer_used(rb),ringbuffer_unused(rb));
	ringbuffer_put(rb, "1234567890", 10,0);
	printf("ringbuffer used=%ld,unused=%ld\n",ringbuffer_used(rb),ringbuffer_unused(rb));
	ringbuffer_put(rb, "1234567890", 10,0);
	printf("ringbuffer used=%ld,unused=%ld\n",ringbuffer_used(rb),ringbuffer_unused(rb));
	ringbuffer_put(rb, "1234567890", 10,0);
	ringbuffer_put(rb, "1234567890", 10,0);
	ringbuffer_put(rb, "1234567890", 10,0);
	
	ringbuffer_put(rb, "1234567890", 10,0);
	ringbuffer_put(rb, "1234567890", 10,0);
	ringbuffer_put(rb, "1234567890", 10,0);
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,5000));
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,5000));
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,5000));
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,0));
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,0));
		printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,0));
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,0));
		printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,0));
	printf("put ret=%d\n",ringbuffer_put(rb, "1234567890", 10,0));

		
	printf("ringbuffer emptey=%d\n",ringbuffer_empty(rb));
	printf("ringbuffer used=%ld,unused=%ld\n",ringbuffer_used(rb),ringbuffer_unused(rb));

	while(1){
		sleep(1);
	}
/*

	char buf[100]={0};

	ringbuffer_get(rb, buf, 100,0);
	printf("ringbuffer read 100=[%s]\n",buf);
	printf("ringbuffer emptey=%d\n",ringbuffer_empty(rb));
	printf("ringbuffer used=%d,unused=%d\n",ringbuffer_used(rb),ringbuffer_unused(rb));
*/
	
	ringbuffer_destroy(rb);
	
    return 0;
}

