//
// Created by lining on 2/15/22.
//

#include "ringBuffer/RingBuffer.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

RingBuffer *RingBuffer_New(size_t capacity) {
    RingBuffer *p = (RingBuffer *) calloc(1, sizeof(struct rb) + capacity * sizeof(uint8_t));
    if (p == NULL)
        return NULL;

    p->capacity = capacity;
    p->buff = (uint8_t *) p + sizeof(struct rb);

    p->read_pos = 0;
    p->write_pos = 0;

    p->available_for_read = 0;
    //int available_for_write = rb_capacity - available_for_read;

    pthread_cond_init(&p->cond, NULL);
    pthread_mutex_init(&p->rwlock, NULL);

//	printf("p = %p,buff = %p\n", p, p->buff);
    return p;
};

void RingBuffer_Delete(RingBuffer *rb) {
    if (rb) {
        pthread_mutex_destroy(&rb->rwlock);
        pthread_cond_destroy(&rb->cond);
        free((uint8_t *) rb);
        rb = NULL;
    }
}

size_t RingBuffer_Read(RingBuffer *rb, void *data, size_t count) {
    assert(rb != NULL);
    assert(data != NULL);

//	printf("READ[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    pthread_mutex_lock(&rb->rwlock);
    while (rb->available_for_read < count) {
        pthread_cond_wait(&rb->cond, &rb->rwlock);
    }
//	printf("READ[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);
    if (rb->available_for_read >= count) {

        if (rb->read_pos + count > rb->capacity) {

            int len = rb->capacity - rb->read_pos;
            memcpy(data, rb->buff + rb->read_pos, len);
            memcpy(data + len, rb->buff, count - len);
            rb->read_pos = count - len;
        } else {
            memcpy(data, rb->buff + rb->read_pos, count);
            rb->read_pos += count;
        }

        rb->read_pos %= rb->capacity;
        rb->available_for_read -= count;
    } else {
        printf("READ read error !\n");
    }
    pthread_cond_signal(&rb->cond);
//	printf("READ[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    pthread_mutex_unlock(&rb->rwlock);
    return count;
}

size_t RingBuffer_Write(RingBuffer *rb, const void *data, size_t count) {
    assert(rb != NULL);
    assert(data != NULL);
//	printf("WRITE[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    pthread_mutex_lock(&rb->rwlock);
    while (rb->capacity - rb->available_for_read < count) {
        pthread_cond_wait(&rb->cond, &rb->rwlock);
    }
//	printf("WRITE[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);
    if (rb->capacity - rb->available_for_read >= count) {

        if (rb->write_pos + count > rb->capacity) {

            int len = rb->capacity - rb->write_pos;
            printf("<%d>\n", __LINE__);
            memcpy(rb->buff + rb->write_pos, data, len);
            memcpy(rb->buff, data + len, count - len);
            rb->write_pos = count - len;
        } else {

            memcpy(&rb->buff[rb->write_pos], data, count);
            rb->write_pos += count;
        }

        rb->write_pos %= rb->capacity;
        rb->available_for_read += count;
    } else {
        printf("WRITE   error !\n");
    }
    pthread_cond_signal(&rb->cond);
//	printf("WRITE[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    pthread_mutex_unlock(&rb->rwlock);
    return count;
}
