/**
 * @file   tsqueue.c
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Implementation of tsqueue.
 */

#include "tsqueue.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/** The internal structure of tsqueue. */
struct tsqueue
{
    /** The lock for the data in this struct. Must be held before reading or
     * writing any values in this struct. */
    pthread_mutex_t lock;

    /** The capacity of the queue. */
    size_t capacity;

    /** The number of used elements in the queue. */
    size_t used;

    /** The size of an element in the queue. */
    size_t elem_size;

    /** The actual data provided by the user. Currently this is just an array
     * where new elements are inserted at the end and everything is shifted
     * when elements are removed. */
    void *data;

    /** The number of unused elements that a producer is waiting for, or zero
     * if no producer is waiting. */
    size_t producer_n_elems;

    /** Used to signal the waiting producer to be called by consumers if they
     * find there is enough free elements after consuming some. */
    pthread_cond_t producer_wakeup;

    /** The number of waiting consumers. */
    size_t n_consumers_waiting;

    /** Used to signal waiting consumers. */
    pthread_cond_t consumer_wakeup;

    /** Used to signal to tsqueue_destroy() that all consumers and producers
     * have exited. */
    pthread_cond_t all_dead;

    /** Indicates that consumers should not wait for more items to be
     * inserted. */
    bool producers_done;

    /** Indicates that the queue is about to be destroyed and all functions
     * should return an error indicator. */
    bool die;
};

/**
 * @brief Wait for space in the queue, the queue lock must be held when
 *        calling this function.
 *
 * @param queue The queue.
 * @param n_elems Number of unused elements to wait for.
 *
 * @return Zero is successful, TSQUEUE_CLOSED if the queue is closed,
 *         else a POSIX error number.
 */
static int wait_for_space_internal(struct tsqueue *queue, size_t n_elems);

/**
 * @brief Signals queue.all_dead if there are no producers or consumers
 *        waiting, the queue lock must be held when calling this function.
 *
 * @param queue The queue.
 */
static void signal_if_all_dead(struct tsqueue *queue);

int tsqueue_create(struct tsqueue **queue, void *data, size_t capacity,
                   size_t elem_size, size_t used)
{
    int retval = 0;

    *queue = malloc(sizeof(**queue));

    if (queue == NULL) {
        retval = errno;
    }

    if (retval == 0) {
        **queue = (struct tsqueue){
            .capacity = capacity,
            .elem_size = elem_size,
            .data = data,
            .used = used
        };
    }

    int steps_done = 0;
    if (retval == 0)
    {
        retval = pthread_mutex_init(&(*queue)->lock, NULL);
    }

    if (retval == 0)
    {
        steps_done = 1;
        retval = pthread_cond_init(&(*queue)->producer_wakeup, NULL);
    }

    if (retval == 0)
    {
        steps_done = 2;
        retval = pthread_cond_init(&(*queue)->consumer_wakeup, NULL);
    }

    if (retval == 0)
    {
        steps_done = 3;
        retval = pthread_cond_init(&(*queue)->all_dead, NULL);
    }

    if (retval == 0)
    {
        steps_done = 4;
    }

    if  (retval != 0)
    {
        switch (steps_done)
        {
        case 4:
            pthread_cond_destroy(&(*queue)->all_dead);
            /* FALL THROUGH */
        case 3:
            pthread_cond_destroy(&(*queue)->consumer_wakeup);
            /* FALL THROUGH */
        case 2:
            pthread_cond_destroy(&(*queue)->producer_wakeup);
            /* FALL THROUGH */
        case 1:
            pthread_mutex_destroy(&(*queue)->lock);
            /* FALL THROUGH */
        default:
            break;
        }

        free(*queue);
    }

    return retval;
}

void tsqueue_close(struct tsqueue *queue)
{
    pthread_mutex_lock(&queue->lock);

    queue->die = true;

    if (queue->producer_n_elems != 0)
    {
        pthread_cond_signal(&queue->producer_wakeup);
    }

    for (size_t i = 0; i < queue->n_consumers_waiting; ++i)
    {
        pthread_cond_signal(&queue->consumer_wakeup);
    }

    while (queue->producer_n_elems != 0 && queue->n_consumers_waiting != 0)
    {
        pthread_cond_wait(&queue->all_dead, &queue->lock);
    }

    pthread_mutex_unlock(&queue->lock);
}

void tsqueue_destroy(struct tsqueue *queue, size_t *used)
{
    tsqueue_close(queue);

    if (used != NULL)
    {
        *used = queue->used;
    }

    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->producer_wakeup);
    pthread_cond_destroy(&queue->consumer_wakeup);
    pthread_cond_destroy(&queue->all_dead);
    free(queue);
}

size_t tsqueue_capacity(struct tsqueue *queue)
{
    pthread_mutex_lock(&queue->lock);
    size_t capacity = queue->capacity;
    pthread_mutex_unlock(&queue->lock);
    return capacity;
}

int tsqueue_wait_for_space(struct tsqueue *queue, size_t n_elems)
{
    pthread_mutex_lock(&queue->lock);
    int retval = wait_for_space_internal(queue, n_elems);
    pthread_mutex_unlock(&queue->lock);
    return retval;
}

int tsqueue_put(struct tsqueue *queue, size_t n_elems, void *in)
{
    int retval = 0;

    pthread_mutex_lock(&queue->lock);

    retval = wait_for_space_internal(queue, n_elems);

    if (retval == 0)
    {
        memcpy((char *)queue->data + (queue->used * queue->elem_size),
               in, n_elems * queue->elem_size);
        if (n_elems != 0 && queue->n_consumers_waiting != 0)
        {
            pthread_cond_signal(&queue->consumer_wakeup);
        }

        queue->used += n_elems;
    }

    pthread_mutex_unlock(&queue->lock);

    return retval;
}

int tsqueue_pop(struct tsqueue *queue, size_t *n_elems, void *out)
{
    int retval = 0;

    pthread_mutex_lock(&queue->lock);

    if (*n_elems > queue->capacity)
    {
        retval = TSQUEUE_TOO_MANY;
    }

    if (retval == 0)
    {
        ++queue->n_consumers_waiting;
        while (!queue->producers_done && !queue->die && queue->used < *n_elems)
        {
            pthread_cond_wait(&queue->consumer_wakeup, &queue->lock);
        }
        --queue->n_consumers_waiting;
    }

    if (queue->die)
    {
        retval = TSQUEUE_CLOSED;
        *n_elems = 0;
        signal_if_all_dead(queue);
    }

    if (retval == 0)
    {
        if (queue->used < *n_elems)
        {
            *n_elems = queue->used;
        }
        memcpy(out, queue->data, *n_elems * queue->elem_size);
        queue->used -= *n_elems;
        memmove(queue->data,
                (char *)queue->data + (*n_elems * queue->elem_size),
                queue->used * queue->elem_size);

        if (queue->producer_n_elems != 0
            && queue->producer_n_elems <= (queue->capacity - queue->used))
        {
            pthread_cond_signal(&queue->producer_wakeup);
        }

        if (queue->used != 0 && queue->n_consumers_waiting != 0)
        {
            pthread_cond_signal(&queue->consumer_wakeup);
        }
    }

    pthread_mutex_unlock(&queue->lock);

    return retval;
}

void tsqueue_set_done(struct tsqueue *queue, bool done)
{
    pthread_mutex_lock(&queue->lock);

    queue->producers_done = done;
    for (size_t i = 0; i < queue->n_consumers_waiting; ++i)
    {
        pthread_cond_signal(&queue->consumer_wakeup);
    }

    pthread_mutex_unlock(&queue->lock);
}

static int wait_for_space_internal(struct tsqueue *queue, size_t n_elems)
{
    int retval = 0;

    if (n_elems > queue->capacity)
    {
        retval = TSQUEUE_TOO_MANY;
    }

    if (queue->producer_n_elems != 0)
    {
        retval = TSQUEUE_SINGLE_PRODUCER;
    }

    if (retval == 0)
    {
        queue->producer_n_elems = n_elems;
        while (!queue->die && (queue->capacity - queue->used) < n_elems)
        {
            pthread_cond_wait(&queue->producer_wakeup, &queue->lock);
        }
        queue->producer_n_elems = 0;
    }

    if (queue->die)
    {
        retval = TSQUEUE_CLOSED;
        signal_if_all_dead(queue);
    }

    return retval;
}

static void signal_if_all_dead(struct tsqueue *queue)
{
    if (queue->producer_n_elems == 0 && queue->n_consumers_waiting == 0)
    {
        pthread_cond_signal(&queue->all_dead);
    }
}
