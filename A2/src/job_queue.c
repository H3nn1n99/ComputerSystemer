#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "job_queue.h"


int job_queue_init(struct job_queue *job_queue, int capacity) {
    job_queue->capacity = capacity;
    job_queue->size = 0;
    job_queue->front = 0;
    job_queue->rear = 0;
    job_queue->destroyed = 0; 
    job_queue->runningOperations = 0;

    job_queue->array = (void**)malloc(capacity * sizeof(void*));
    if (job_queue->array == NULL) {
        return -1;  
    }

    pthread_mutex_init(&job_queue->mutex, NULL);
    pthread_cond_init(&job_queue->not_full, NULL);
    pthread_cond_init(&job_queue->not_empty, NULL);

    return 0; 
}

int job_queue_destroy(struct job_queue *job_queue) {
    pthread_mutex_lock(&job_queue->mutex);

    if (job_queue->runningOperations > 0) {
        return -1;
    }

    job_queue->destroyed = 1;

    while (job_queue->size > 0) {
        void *job_data;
        job_queue_pop(job_queue, &job_data);
        free(job_data);
    }

    free(job_queue->array);
    pthread_cond_destroy(&job_queue->not_full);
    pthread_cond_destroy(&job_queue->not_empty);
    pthread_mutex_unlock(&job_queue->mutex);
    pthread_mutex_destroy(&job_queue->mutex);

    return 0;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
    pthread_mutex_lock(&job_queue->mutex);

    if (job_queue->destroyed) {
        pthread_mutex_unlock(&job_queue->mutex);
        return -1;
    }

    job_queue->runningOperations++; 

    while (job_queue->size == job_queue->capacity) {
        pthread_cond_wait(&job_queue->not_full, &job_queue->mutex);
    }

    job_queue->array[job_queue->rear] = data;
    job_queue->rear = (job_queue->rear + 1) % job_queue->capacity;
    job_queue->size++;

    pthread_cond_signal(&job_queue->not_empty);

    job_queue->runningOperations--;
    pthread_mutex_unlock(&job_queue->mutex);

    return 0;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
    pthread_mutex_lock(&job_queue->mutex);

    if (job_queue->destroyed) {
        pthread_mutex_unlock(&job_queue->mutex);
        return -1;
    }

    job_queue->runningOperations++; 

    while (job_queue->size == 0) {
        pthread_cond_wait(&job_queue->not_empty, &job_queue->mutex);
    }

    *data = job_queue->array[job_queue->front];
    job_queue->front = (job_queue->front + 1) % job_queue->capacity;
    job_queue->size--;

    pthread_cond_signal(&job_queue->not_full);

    job_queue->runningOperations--; 
    pthread_mutex_unlock(&job_queue->mutex);

    return 0;
}
