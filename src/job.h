/**
 * @file   job.h
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Data structure for representing jobs.
 */

#ifndef JOB_H
#define JOB_H

#include <time.h>

/** Information about a job. All values are set by task(). */
struct job_struct {
    /** ID of the job. */
    unsigned id;

    /** Time required for the job in seconds. */
    time_t cpu_burst;

    /** Arrival time of the job from CLOCK_MONOTONIC. Used for statistics,
     * CLOCK_REALTIME is not appropriate for this as it can change
     * dramatically for various reasons (such as switching to daylight savings
     * time). */
    struct timespec arrival_mono;

    /** Arrival time of the job from CLOCK_REALTIME. Used in logs. */
    struct timespec arrival_real;

    /** Service time of the job from CLOCK_MONOTONIC. Used for statistics. */
    struct timespec service_mono;

    /** Service time of the job from CLOCK_REALTIME. Used for logs. */
    struct timespec service_real;

    /** Completion time of the job from CLOCK_MONOTONIC. Used for statistics. */
    struct timespec completion_mono;

    /** Completion time of the job from CLOCK_REALTIME. Used for logs. */
    struct timespec completion_real;
};

#endif /* JOB_H */
