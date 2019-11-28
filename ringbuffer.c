#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "ringbuffer.h"



/* create a new ringbuffer
 * @capacity max buffer size of the ringbuffer
 * @return the address of the new ringbuffer, NULL for error.
 */
RING_BUFFER_s *ringbuffer_create(int capacity)
{
    RING_BUFFER_s *rbuf;
    int ret;

    rbuf = malloc(sizeof(RING_BUFFER_s));
    if (rbuf == NULL) {
        printf("malloc ringbuf error !\n");
        return NULL;
    }

    rbuf->cap = capacity;
    rbuf->buf = malloc(rbuf->cap);
    if (rbuf->buf == NULL) {
        printf("malloc error!\n");
        goto err0;
    }

    rbuf->size = 0;
    rbuf->roffset = 0;
    rbuf->woffset = 0;

    ret = pthread_mutex_init(&rbuf->mutex_io, NULL);
    if (ret) {
        printf("pthread_mutex_init error: %s\n", strerror(ret));
        goto err1;
    }

    ret = pthread_cond_init(&rbuf->cont_read, NULL);
    if (ret) {
        printf("pthread_cond_init cont_read error: %s\n", strerror(ret));
        goto err2;
    }

    ret = pthread_cond_init(&rbuf->cont_write, NULL);
    if (ret) {
        printf("pthread_cond_init cont_write error: %s\n", strerror(ret));
        goto err3;
    }

    return rbuf;

err3:
    pthread_cond_destroy(&rbuf->cont_read);

err2:
    pthread_mutex_destroy(&rbuf->mutex_io);

err1:
    free(rbuf->buf);

err0:
    free(rbuf);

    return NULL;
}

void ringbuffer_destroy(RING_BUFFER_s *rbuf)
{
    if (rbuf) {
        pthread_cond_destroy(&rbuf->cont_write);
        pthread_cond_destroy(&rbuf->cont_read);
        pthread_mutex_destroy(&rbuf->mutex_io);
        free(rbuf->buf);
        free(rbuf);
    }
}

/* get data from ringbuffer @rbuf
 * @rbuf ringbuffer where to get data
 * @out_buf output buffer where to store data
 * @size size of @out_buf
 * @timeout timeout in ms
 * @return return number of bytes read; 0 for timeout; -1 for error
 */
int ringbuffer_get(RING_BUFFER_s *rbuf, void *out_buf, int size, unsigned long timeout)
{
    int ret;
    int nr;

    ret = pthread_mutex_lock(&rbuf->mutex_io);
    if (ret) {
        return -1;
    }

    struct timespec ts;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000 * 1000;

    while (rbuf->size == 0)
    {
        if (timeout) {
            if (pthread_cond_timedwait(&rbuf->cont_read, &rbuf->mutex_io, &ts)) {
                pthread_mutex_unlock(&rbuf->mutex_io);
                return 0;
            }
        } else {
            if (pthread_cond_wait(&rbuf->cont_read, &rbuf->mutex_io)) {
                pthread_mutex_unlock(&rbuf->mutex_io);
                return -1;
            }
        }
    }
    
    if (rbuf->woffset > rbuf->roffset) {
        int avail_count = rbuf->woffset - rbuf->roffset;

        // number to read
        nr = size > avail_count ? avail_count : size;

        // copy data 
        memcpy(out_buf, rbuf->buf + rbuf->roffset, nr);

        // update read offset
        rbuf->roffset += nr;
        rbuf->size -= nr;
    } else {
        // number to read
        int part1 = rbuf->cap - rbuf->roffset;
        int num_to_read = size > part1 ? part1 : size;

        memcpy(out_buf, rbuf->buf + rbuf->roffset, num_to_read);
        nr = num_to_read;

        // update read offset
        rbuf->size -= nr;
        rbuf->roffset += nr;
        if (rbuf->roffset == rbuf->cap) {
            rbuf->roffset = 0;
        }

        int remain = size - nr;
        if (remain > 0) {
            num_to_read = remain > rbuf->woffset ? rbuf->woffset : remain; 
            memcpy(out_buf + nr, rbuf->buf, num_to_read); //  part 2

            // update read offset 
            rbuf->roffset = num_to_read;
            rbuf->size -= num_to_read;
            remain -= num_to_read;
        }
        nr = size - remain;
    }

    pthread_cond_signal(&rbuf->cont_write);
    pthread_mutex_unlock(&rbuf->mutex_io);

    return nr;
}

/* write data to ringbuffer @rbuf;
 * @rbuf ringbuffer where to write data to;
 * @in_buf input buffer;
 * @size size of input buffer @in_buf
 * @timeout timeout in ms;
 * @return the number of bytes written to ringbuffer; 0 for timeout; -1 for error;
 */
int ringbuffer_put(RING_BUFFER_s *rbuf, const void *in_buf, int size, unsigned int timeout)
{
    int ret;
    int nw;

    ret = pthread_mutex_lock(&rbuf->mutex_io);
    if (ret) {
        return -1;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000;

    while ( rbuf->cap - rbuf->size < size) // check have no enough space
    {
        if (timeout) {
            if (pthread_cond_timedwait(&rbuf->cont_write, &rbuf->mutex_io, &ts)) {
                pthread_mutex_unlock(&rbuf->mutex_io);
                return 0;
            }
        } else {
            if (pthread_cond_wait(&rbuf->cont_write, &rbuf->mutex_io)) {
                pthread_mutex_unlock(&rbuf->mutex_io);
                return -1;
            }
        }
    }
 
    if (rbuf->woffset < rbuf->roffset) {
        int free_space = rbuf->roffset - rbuf->woffset;
        nw = size > free_space ? free_space : size;
        memcpy(rbuf->buf + rbuf->woffset, in_buf, nw);
        rbuf->woffset += nw;
        rbuf->size += nw;
    } else {
        int part1 = rbuf->cap - rbuf->woffset;
        int num_to_write = size > part1 ? part1 : size;

        // copy part 1
        memcpy(rbuf->buf + rbuf->woffset, in_buf, num_to_write);

        // update write offset
        nw = num_to_write;
        rbuf->size += nw;
        rbuf->woffset += nw;
        if (rbuf->woffset == rbuf->cap) {
            rbuf->woffset = 0;
        }

        int remain = size - nw;
        if (remain > 0) {
            // copy part2
            num_to_write = remain > rbuf->roffset ? rbuf->roffset : remain;
            memcpy(rbuf->buf, in_buf + nw, num_to_write);

            // update write offset
            rbuf->size += num_to_write;
            rbuf->woffset = num_to_write;
            nw += num_to_write;
        }
    }

    pthread_cond_signal(&rbuf->cont_read);
    pthread_mutex_unlock(&rbuf->mutex_io);   

    return nw;
}


bool ringbuffer_full(RING_BUFFER_s *rbuf)
{
	bool ret = false;
	if(rbuf){
		pthread_mutex_lock(&rbuf->mutex_io);
		ret = (rbuf->size == rbuf->cap);
		pthread_mutex_unlock(&rbuf->mutex_io);
		return ret;
	}
	else
		return false;
}

bool ringbuffer_empty(RING_BUFFER_s *rbuf)
{
	bool ret = false;

	if(rbuf){
		pthread_mutex_lock(&rbuf->mutex_io);
		ret = (rbuf->size == 0);
		pthread_mutex_unlock(&rbuf->mutex_io);
		return ret;
	}
	else
		return false;
}

long ringbuffer_used(RING_BUFFER_s *rbuf)
{
	long ret=0;
	if(rbuf){
		pthread_mutex_lock(&rbuf->mutex_io);
		ret = rbuf->size;
		pthread_mutex_unlock(&rbuf->mutex_io);
		return ret;
	}
	else
		return -1;
}

long ringbuffer_unused(RING_BUFFER_s *rbuf)
{
	long ret = 0;
	if(rbuf){
		pthread_mutex_lock(&rbuf->mutex_io);
		ret= rbuf->cap - rbuf->size;
		pthread_mutex_unlock(&rbuf->mutex_io);
		return ret;
	}else
		return -1;
}


