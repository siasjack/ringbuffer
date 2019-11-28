#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <pthread.h>


typedef struct 
{
    long size;   // current size of data in ringbuffer
    long cap;    // capacity of ringbuffer
    void *buf;
    long roffset;
    long woffset;

    pthread_mutex_t mutex_io;
    pthread_cond_t cont_read;
    pthread_cond_t cont_write;
}RING_BUFFER_s;

typedef enum{
	false=0,
	true=1,
}bool;



RING_BUFFER_s *ringbuffer_create(int capacity);
void ringbuffer_destroy(RING_BUFFER_s *rbuf);
int ringbuffer_get(RING_BUFFER_s *rbuf, void *out_buf, int size, unsigned long timeout);
int ringbuffer_put(RING_BUFFER_s *rbuf, const void *in_buf, int size, unsigned int timeout);
bool ringbuffer_full(RING_BUFFER_s *rbuf);
bool ringbuffer_empty(RING_BUFFER_s *rbuf);
long ringbuffer_used(RING_BUFFER_s *rbuf);
long ringbuffer_unused(RING_BUFFER_s *rbuf);





#endif
