/**
 * @file   cpu.c
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Implementation of cpu().
 */

#define _POSIX_C_SOURCE 200809L

#include "cpu.h"
#include "job.h"
#include "log.h"
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>

/**
 * @brief Logs service time, waits for @p job.cpu_burst seconds, then logs
 *        completion time. Also increments all values in @p stats.
 *
 * Calls log_service() before waiting and log_completion() after.
 *
 * @param[in] job The job to handle.
 * @param[in,out] log_file The file to log to.
 * @param cpu_id The id to be used in log messages.
 * @param[in,out] stats Stats to modify.
 *
 * @return Zero if the function succeeds, else an error code that can be
 *         passed to errno_or_ae_to_str().
 */
static int handle_job(struct job_struct *job, FILE *log_file, unsigned cpu_id,
                      struct cpu_shared_stats *stats);

void *cpu(void *ptr)
{
    int retval = 0;

    // Input arguments
    struct cpu_params *params = ptr;
    struct cpu_shared_stats *stats = params->stats;
    tsqueue *queue = params->queue;
    unsigned cpu_id = params->id;
    FILE *log_file = params->log_file;

    // Total number of jobs inserted
    unsigned long n_jobs = 0;

    size_t jobs_from_queue = 1;
    // The only time this will be non-zero is if the queue is closed due to an
    // error occurring elsewhere.
    int queue_retval;
    do
    {
        struct job_struct job;
        queue_retval = tsqueue_pop(queue, &jobs_from_queue, &job);
        if (jobs_from_queue == 1 && queue_retval == 0)
        {
            ++n_jobs;
            retval = handle_job(&job, log_file, cpu_id, stats);
        }
    } while (retval == 0 && jobs_from_queue == 1 && queue_retval == 0);

    if (retval == 0 && queue_retval == 0)
    {
        retval = log_cpu_done(log_file, cpu_id, n_jobs);
    }

    params->retval = retval;
    return NULL;
}



static void run_job(const struct job_struct *job)
{
    struct timespec ts = {.tv_sec = job->cpu_burst};
    // I have chosen to use clock_nanosleep instead of sleep here because it
    // allows us to use a monotonic clock explicitly.
    while (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &ts) != 0)
    {
    }
}

static int handle_job(struct job_struct *job, FILE *log_file, unsigned cpu_id,
                      struct cpu_shared_stats *stats)
{
    int retval = 0;

    clock_gettime(CLOCK_MONOTONIC, &job->service_mono);
    clock_gettime(CLOCK_REALTIME, &job->service_real);

    pthread_mutex_lock(&stats->lock);
    ++stats->num_tasks;
    stats->total_waiting_time +=
        job->service_mono.tv_sec - job->arrival_mono.tv_sec;
    pthread_mutex_unlock(&stats->lock);

    retval = log_service(log_file, cpu_id, job);
    if (retval == 0)
    {
        run_job(job);
        clock_gettime(CLOCK_MONOTONIC, &job->completion_mono);
        clock_gettime(CLOCK_REALTIME, &job->completion_real);

        pthread_mutex_lock(&stats->lock);
        stats->total_turnaround_time +=
            job->completion_mono.tv_sec - job->arrival_mono.tv_sec;
        pthread_mutex_unlock(&stats->lock);

        retval = log_completion(log_file, cpu_id, job);
    }

    return retval;
}
