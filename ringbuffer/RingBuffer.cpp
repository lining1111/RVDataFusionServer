//
// Created by lining on 1/11/23.
//

#include <cassert>
#include <cstring>
#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t capacity) {
    this->capacity = capacity;
    this->buff = new uint8_t[capacity];

    this->read_pos = 0;
    this->write_pos = 0;

    this->available_for_read = 0;
    this->available_for_write = this->capacity - this->available_for_read;

}

RingBuffer::~RingBuffer() {
    pthread_mutex_destroy(&this->rwlock);
    pthread_cond_destroy(&this->cond);
    delete[] buff;
}

size_t RingBuffer::Read(void *data, size_t count) {
    assert(data != NULL);

//	printf("READ[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    pthread_mutex_lock(&rwlock);
    while (available_for_read < count) {
        pthread_cond_wait(&cond, &rwlock);
    }
//	printf("READ[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);
    if (available_for_read >= count) {

        if (read_pos + count > capacity) {

            int len = capacity - read_pos;
            memcpy(data, buff + read_pos, len);
            memcpy((uint8_t *) data + len, buff, count - len);
            read_pos = count - len;
        } else {
            memcpy(data, buff + read_pos, count);
            read_pos += count;
        }

        read_pos %= capacity;
        available_for_read -= count;
        available_for_write = capacity - available_for_read;
    } else {
        printf("READ read error !\n");
    }
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&rwlock);
//	printf("READ[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    return count;
}

size_t RingBuffer::Write(const void *data, size_t count) {
    assert(data != NULL);
//	printf("WRITE[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    pthread_mutex_lock(&rwlock);
    while (available_for_write < count) {
        pthread_cond_wait(&cond, &rwlock);
    }
//	printf("WRITE[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);
    if (available_for_write >= count) {

        if (write_pos + count > capacity) {

            int len = capacity - write_pos;
            memcpy(&buff[write_pos], data, len);
            memcpy(buff, (uint8_t *) data + len, count - len);
            write_pos = count - len;
        } else {
            memcpy(&buff[write_pos], data, count);
            write_pos += count;
        }

        write_pos %= capacity;
        available_for_read += count;
        available_for_write = capacity - available_for_read;
    } else {
        printf("WRITE   error !\n");
    }
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&rwlock);
//	printf("WRITE[%d]count=%ld,cap=%ld,read_pos=%d,write_pos=%d,ava_read=%d\n",
//			__LINE__, count, rb->capacity, rb->read_pos, rb->write_pos,
//			rb->available_for_read);

    return count;
}

int RingBuffer::GetReadLen() {
    return available_for_read;
}

int RingBuffer::GetWriteLen() {
    return available_for_write;
}
