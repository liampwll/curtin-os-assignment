/**
 * @file   task.h
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  The task() function described by the assignment spec.
 */

#ifndef TASK_H
#define TASK_H

#include "tsqueue.h"
#include <stdio.h>

/** Parameters to pass to task(). */
struct task_params
{
    /** The ready-queue, the task() thread only acts as a producer. */
    tsqueue *queue;

    /** The file to  get jobs from. */
    FILE *job_file;

    /** The buffer to store jobs in before placing them in the queue. */
    struct job_struct *job_buffer;

    /** The length of job_buffer. */
    size_t job_buffer_length;

    /** The file to write log messages to. */
    FILE *log_file;

    /** The return value of the task() call. task() will set this before
     * exiting. Zero if successful, otherwise can be passed to
     * errno_or_ae_to_str(). */
    int retval;
};

/**
 * @brief Places jobs in the provided queue until the end of the file is
 *        reached or an error occurs.
 *
 * @param data See task_params for details.
 *
 * @return NULL, see task_params for return value.
 */
void *task(void *data);

#endif /* TASK_H */
