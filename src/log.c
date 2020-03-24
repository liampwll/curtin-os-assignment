/**
 * @file   log.c
 * @author Liam Powell
 * @date   2019-04-28
 *
 * @brief  Implementation of log.h.
 */


#define _POSIX_C_SOURCE 200809L

#include "log.h"
#include "job.h"
#include "config.h"
#include "cpu.h"
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>

/**
 * @brief Log the job @p j and the event @p event at @p time to @p log_file.
 *
 * Uses the format:
 *
 *     Statistics for CPU-<cpu_id>:
 *     Job #<j.id>
 *     Arrival time: <j.arrival>
 *     <time> time: <j.end>
 *
 * @param[in,out] log_file The file to write to.
 * @param cpu_id The id of the cpu.
 * @param[in] job The job to be logged.
 * @param time The time for the event.
 * @param[in] event The event to log.
 *
 * @return Zero if the function succeeds, else an error code that can be
 *         passed to errno_or_app_errnum_to_str().
 */
static int log_cpu_event(FILE *log_file, unsigned cpu_id,
                         const struct job_struct *job, struct timespec time,
                         const char *event)
{
    int retval = 0;

    struct tm arrival_tm;
    struct tm event_tm;
    if (localtime_r(&job->arrival_real.tv_sec, &arrival_tm) == NULL
        || localtime_r(&time.tv_sec, &event_tm) == NULL)
    {
        retval = errno;
    }
    else
    {
        int res = fprintf(log_file,
                          "Statistics for CPU %u:\n"
                          "Job #%u\n"
                          "Arrival time: %02d:%02d:%02d\n"
                          "%s time: %02d:%02d:%02d\n\n",
                          cpu_id, job->id, arrival_tm.tm_hour,
                          arrival_tm.tm_min, arrival_tm.tm_sec, event,
                          event_tm.tm_hour, event_tm.tm_min, event_tm.tm_sec);
        if (res < 0)
        {
            retval = errno;
        }
    }

    return retval;
}

int log_service(FILE *log_file, unsigned cpu_id, const struct job_struct *job)
{
    return log_cpu_event(log_file, cpu_id, job, job->service_real, "Service");
}

int log_completion(FILE *log_file, unsigned cpu_id, const struct job_struct *job)
{
    // This is used to make the gantt charts in my report.
#ifdef CONFIG_STDOUT_PGFGANTT
    printf("%jd.%09lu "
           "\\ganttset{bar/.append style={fill=white}} "
           "\\ganttbar{%u}{%jd}{%jd} "
           "\\ganttbar[inline]{}{%jd}{%jd} "
           "\\ganttset{bar/.append style={fill=lightgray}} "
           "\\ganttbar[inline]{CPU-%u}{%jd}{%jd}\\\\\n",
           (intmax_t)job->arrival_mono.tv_sec,
           job->arrival_mono.tv_nsec, job->id,
           (intmax_t)job->arrival_mono.tv_sec,
           (intmax_t)job->arrival_mono.tv_sec - 1,
           (intmax_t)job->arrival_mono.tv_sec,
           (intmax_t)job->service_mono.tv_sec - 1, cpu_id,
           (intmax_t)job->service_mono.tv_sec,
           (intmax_t)job->completion_mono.tv_sec - 1);
#endif
    return log_cpu_event(log_file, cpu_id, job, job->completion_real,
                         "Completion");
}

int log_cpu_done(FILE *log_file, unsigned cpu_id, unsigned long n_jobs)
{
    int retval = 0;

    int res = fprintf(log_file,
                      "CPU-%u terminates after servicing %lu tasks\n\n",
                      cpu_id, n_jobs);
    if (res < 0)
    {
        retval = res;
    }

    return retval;
}

int log_task_done(FILE *log_file, struct timespec time, unsigned long n_jobs)
{
    int retval = 0;

    struct tm tm;
    if (localtime_r(&time.tv_sec, &tm) == NULL)
    {
        retval = errno;
    }
    else
    {
        int res = fprintf(log_file,
                          "Number of tasks put into Ready-Queue: %lu\n"
                          "Terminates at time: %02d:%02d:%02d\n\n",
                          n_jobs, tm.tm_hour, tm.tm_min,
                          tm.tm_sec);
        if (res < 0)
        {
            retval = res;
        }
    }

    return retval;
}

int log_arrival(FILE *log_file, const struct job_struct *job)
{
    int retval = 0;

    struct tm tm;
    if (localtime_r(&job->arrival_real.tv_sec, &tm) == NULL)
    {
        retval = errno;
    }
    else
    {
        int res = fprintf(log_file,
                          "%u: %jd\n"
                          "Arrival time: %02d:%02d:%02d\n\n",
                          job->id, (intmax_t)job->cpu_burst, tm.tm_hour,
                          tm.tm_min, tm.tm_sec);
        if (res < 0)
        {
            retval = errno;
        }
    }

    return retval;
}

int log_main_done(FILE *log_file, struct cpu_shared_stats *stats)
{
    uintmax_t avg_wait = 0;
    uintmax_t avg_turn = 0;
    if (stats->num_tasks != 0)
    {
        avg_wait = (uintmax_t)stats->total_waiting_time / stats->num_tasks;
        avg_turn = (uintmax_t)stats->total_turnaround_time / stats->num_tasks;
    }
    int retval =
        fprintf(log_file,
                "Number of tasks: %lu\n"
                "Average waiting time: %ju seconds\n"
                "Average turn around time: %ju seconds\n\n",
                stats->num_tasks, avg_wait, avg_turn);
    return (retval < 0) ? errno : 0;
}
