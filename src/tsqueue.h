/**
 * @file   tsqueue.h
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Thread safe single-producer, multi-consumer FIFO queue.
 *
 * This is a thread safe single-producer, multi-consumer FIFO queue with the
 * following properties:
 * - Uses memory provided by the user for the queue itself, allowing the user
 *   to ensure any alignment requirements. This memory may already contain
 *   elements.
 * - Arbitrary queue element sizes.
 * - Supports pushing and popping multiple elements atomically.
 * - Leaves elements in the user provided memory after destroying the queue in
 *   the order they would have been popped.
 */

#ifndef TSQUEUE_H
#define TSQUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

/** Thread safe single-producer, multi-consumer FIFO queue. */
typedef struct tsqueue tsqueue;

/**
 * @brief Create a new tsqueue.
 *
 * @param[out] queue The tsqueue, will be NULL if creation fails.
 * @param[in] data Array used to store queue elements, must not be used until
 *                 tsqueue_destroy() is called.
 * @param capacity The number of elements that @p data can hold.
 * @param elem_size The size of an element in @p data.
 * @param used The number of items already in @p data, these elements will be
 *             accessible via tsqueue_pop(). The first element to be popped
 *             will be the first element of @p data.
 *
 * @return Zero if the function succeeds, else a POSIX error number.
 */
int tsqueue_create(tsqueue **queue, void *data, size_t capacity,
                   size_t elem_size, size_t used);

/**
 * @brief Forces all tsqueue_put() and tsqueue_pop() functions using this
 *        queue to exit and return TSQUEUE_CLOSED.
 *
 * Will block until all calls have exited. This function is intended to be
 * used before tsqueue_destroy as a way to signal to other threads that they
 * should stop reading from the queue. All future calls to tsqueue_put() and
 * tsqueue_pop() will also return TSQUEUE_CLOSED.
 *
 * @param[in] queue The tsqueue to close.
 */
void tsqueue_close(tsqueue *queue);

/**
 * @brief Calls tsqueue_close() before freeing resources allocated by tsqueue
 *        functions.
 *
 * Using a tsqueue while it is being destroyed or after it is destroyed will
 * cause undefined behaviour. tsqueue_close() and tsqueue_set_done() can be
 * useful for indicating to other threads that they should stop using the
 * queue.
 *
 * @param[in] queue The tsqueue to destroy.
 * @param[out] used The number of queue items left in the array provided to
 *                  tsqueue_create(). The first array element would have been
 *                  popped next. Can be NULL.
 */
void tsqueue_destroy(tsqueue *queue, size_t *used);

/**
 * @brief Returns the capacity of the queue.
 *
 * @param[in] queue The tsqueue.
 *
 * @return The capacity of @p queue.
 */
size_t tsqueue_capacity(tsqueue *queue);

/**
 * @brief Blocks until there are @p n_elems free spaces in the queue.
 *
 * @param[in] queue The tsqueue.
 * @param n_elems The number of elements to wait for.
 *
 * @return Zero if the function is successful.
 *
 *         TSQUEUE_CLOSED if the queue is closed.
 *
 *         TSQUEUE_TOO_MANY if @p n_elems is greater than the queue's
 *         capacity.
 *
 *         TSQUEUE_SINGLE_PRODUCER if a tsqueue_put() or
 *         tsqueue_wait_for_space() call is already running.
 */
int tsqueue_wait_for_space(tsqueue *queue, size_t n_elems);

/**
 * @brief Waits until there are @p n_elem free slots in the queue and then
 *        adds all elements to the end of the queue.
 *
 * The first element in @p in will be popped first, after all elements already
 * in the queue.
 *
 * @param[in] queue The tsqueue.
 * @param n_elems The number of elements in @p in.
 * @param[in] in The items to insert in to the queue.
 *
 * @return Zero if the function is successful.
 *
 *         TSQUEUE_CLOSED if the queue is closed.
 *
 *         TSQUEUE_TOO_MANY if @p n_elems is greater than the queue's
 *         capacity.
 *
 *         TSQUEUE_SINGLE_PRODUCER if a a tsqueue_put() or
 *         tsqueue_wait_for_space() call is already running.
 */
int tsqueue_put(tsqueue *queue, size_t n_elems, void *in);

/**
 * @brief Retrieve elements from a tsqueue.
 *
 * Will block until there is n_elems elements unless the queue is closed or
 * `tsqueue_set_done(queue, true)` has been called.
 *
 * @param[in] queue The tsqueue.
 * @param[in,out] n_elems The maximum number of elements to place in @p
 *                        out. Will be set to the actual number of elements
 *                        retrieved. Will be set to zero if the queue is
 *                        closed or `tsqueue_set_done(queue, true)` has been
 *                        called.
 * @param[out] out Buffer to place the elements in. The first element was
 *                 first in the queue.
 *
 * @return Zero if the function is successful, including when zero elements
 *         are retrieved.
 *
 *         TSQUEUE_CLOSED if the queue is closed.
 */
int tsqueue_pop(tsqueue *queue, size_t *n_elems, void *out);

/**
 * @brief Indicate that no more items will be placed in the queue. Can be
 *        reversed.
 *
 * @param[in] queue The tsqueue.
 * @param done True to indicate that no more items will be placed in the
 *             queue, false to reset to the normal state.
 */
void tsqueue_set_done(tsqueue *queue, bool done);

enum
{
    // The reason we use negative numbers is that POSIX errno
    // values are always positive. This allows us to return a POSIX error
    // value or a custom error value and differentiate between them.

    /** The queue was closed. */
    TSQUEUE_CLOSED = INT_MIN,

    /** tsqueue_put() was called with more items than the queue can hold. */
    TSQUEUE_TOO_MANY,

    /** A tsqueue_put() or tsqueue_wait_for_space() call was made while one
     * was already running. */
    TSQUEUE_SINGLE_PRODUCER
};

#endif /* TSQUEUE_H */
