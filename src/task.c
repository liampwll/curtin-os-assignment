/**
 * @file   task.c
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Implementation of task().
 */

#define _POSIX_C_SOURCE 200809L

#include "task.h"
#include "job.h"
#include "log.h"
#include "error.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

/**
 * @brief Fill @p buffer with jobs from @p job_file.
 *
 * The file should contain "<job id> <job time in seconds> <job id> <job time
 * in seconds> ..." separated by whitespace.
 *
 * @param[in,out] job_file The file to read jobs from.
 * @param length The maximum number of jobs to read.
 * @param[out] buffer The buffer to fill with at most @p length jobs.
 * @param[out] used The number of jobs placed in the buffer. Zero if end of
 *                  file is reached.
 *
 * @return Zero if the function succeeds, else an error code that can be
 *         passed to errno_or_ae_to_str().
 */
static int fill_job_buffer(FILE *job_file, size_t length,
                           struct job_struct *buffer, size_t *used);

void *task(void *ptr)
{
    int retval = 0;

    // Input arguments
    struct task_params *params = ptr;
    tsqueue *queue = params->queue;
    FILE *job_file = params->job_file;
    struct job_struct *job_buffer = params->job_buffer;
    size_t job_buffer_length = params->job_buffer_length;
    FILE *log_file = params->log_file;

    // Total number of jobs processed
    unsigned long n_jobs = 0;

    if (tsqueue_capacity(queue) < job_buffer_length)
    {
        job_buffer_length = tsqueue_capacity(queue);
    }


    int queue_retval = 0;
    while (retval == 0 && !feof(job_file) && queue_retval == 0)
    {
        size_t jobs_in_buffer = 0;
        retval = fill_job_buffer(job_file, job_buffer_length, job_buffer,
                                 &jobs_in_buffer);
        if (retval == 0)
        {
            queue_retval = tsqueue_wait_for_space(queue, jobs_in_buffer);
        }

        if (retval == 0 && queue_retval == 0)
        {
            for (size_t i = 0; i < jobs_in_buffer; ++i)
            {
                clock_gettime(CLOCK_MONOTONIC, &job_buffer[i].arrival_mono);
                clock_gettime(CLOCK_REALTIME, &job_buffer[i].arrival_real);
            }
            queue_retval = tsqueue_put(queue, jobs_in_buffer, job_buffer);
            n_jobs += jobs_in_buffer;
        }

        if (retval == 0 && queue_retval == 0)
        {
            for (size_t i = 0; i < jobs_in_buffer; ++i)
            {
                retval = log_arrival(log_file, &job_buffer[i]);
                if (retval != 0)
                {
                    break;
                }
            }
        }
    }

    tsqueue_set_done(queue, true);

    if (retval == 0)
    {
        struct timespec time = {0};
        clock_gettime(CLOCK_REALTIME, &time);
        retval = log_task_done(log_file, time, n_jobs);
    }

    params->retval = retval;
    return NULL;
}

static int fill_job_buffer(FILE *job_file, size_t length,
                           struct job_struct *buffer, size_t *used)
{
    int retval = 0;

    *used = 0;
    while (!feof(job_file) && retval == 0 && *used < length)
    {
        // I have used fscanf rather than a more robust function here to keep
        // this function simple.
        struct job_struct *job = &buffer[*used];
        unsigned int tmp_time;
        errno = 0;
        if (fscanf(job_file, " %u %u ", &job->id, &tmp_time) != 2)
        {
            retval = (errno != 0) ? errno : AE_BAD_FILE;
        }
        else
        {
            job->cpu_burst = (time_t)tmp_time;
            ++*used;
        }
    }

    return retval;
}
