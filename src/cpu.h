/**
 * @file   cpu.h
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  The cpu() function described by the assignment spec.
 */

#ifndef CPU_H
#define CPU_H

#include "tsqueue.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>

/** Parameters to pass to cpu(). */
struct cpu_params
{
    /** Statistics shared between all cpu() threads. */
    struct cpu_shared_stats *stats;

    /** The ready-queue, cpu() threads only act as consumers. */
    tsqueue *queue;

    /** The id of the cpu to be used for logging. */
    unsigned id;

    /** The file to write log messages to. */
    FILE *log_file;

    /** The return value of the cpu() call. cpu() will set this before
     * exiting. Zero is successful, otherwise can be passed to
     * errno_or_ae_to_str(). */
    int retval;
};

/** Statistics shared between all cpu() calls. */
struct cpu_shared_stats
{
    /** Lock for the data in this struct. All cpu() threads must hold this
     * lock when reading or writing any of the values in this struct. */
    pthread_mutex_t lock;

    /** Total amount of time spent waiting by jobs in the ready queue. */
    time_t total_waiting_time;

    /** Total amount of time spent by jobs waiting in the queue or
     * running. */
    time_t total_turnaround_time;

    /** Number of jobs which have been inserted in to the queue. */
    unsigned long num_tasks;
};

/**
 * @brief Runs jobs from the provided queue until the queue is empty or an
 *        error occurs.
 *
 * @param data See cpu_params for details.
 *
 * @return NULL, see cpu_params for return value.
 */
void *cpu(void *data);

#endif /* CPU_H */
