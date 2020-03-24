/**
 * @file   log.h
 * @author Liam Powell
 * @date   2019-04-28
 *
 * @brief  Logging functions used throughout the program.
 */


#ifndef LOG_H
#define LOG_H

#include "job.h"
#include "cpu.h"
#include <stdio.h>
#include <time.h>

/**
 * @brief Log the service of @p j at @p time to the file @p log_file.
 *
 * Uses the format:
 *
 *     Statistics for CPU-<cpu_id>:
 *     Job #<j.id>
 *     Arrival time: <j.arrival>
 *     Service time: <j.end>
 *
 * @param[in,out] log_file The file to write to.
 * @param cpu_id The id of the cpu.
 * @param[in] job The job to be logged.
 *
 * @return Zero if the function succeeds, else a POSIX error number.
 */
int log_service(FILE *log_file, unsigned cpu_id, const struct job_struct *job);

/**
 * @brief Log the completion of @p j at @p time to the file @p log_file.
 *
 * Uses the format:
 *
 *     Statistics for CPU-<cpu_id>:
 *     Job #<j.id>
 *     Arrival time: <j.arrival>
 *     Completion time: <j.end>
 *
 * @param[in,out] log_file The file to write to.
 * @param cpu_id The id of the cpu.
 * @param[in] job The job to be logged.
 *
 * @return Zero if the function succeeds, else a POSIX error number.
 */
int log_completion(FILE *log_file, unsigned cpu_id, const struct job_struct *job);

/**
 * @brief Log the total number of jobs executed by a cpu thread.
 *
 * Uses the format:
 *
 *     CPU-<cpu_id> terminates after servicing <n_jobs> tasks
 *
 * @param[in,out] log_file The file to write to.
 * @param cpu_id The id of the cpu.
 * @param n_jobs The number of jobs.
 *
 * @return Zero if the function succeeds, else a POSIX error number.
 */
int log_cpu_done(FILE *log_file, unsigned cpu_id, unsigned long n_jobs);

/**
 * @brief Log the arrival of a job.
 *
 * Uses the format:
 *
 *     <j.id>: <j.cpu_burst>
 *     Arrival time: <j.arrival>
 *
 * @param[in,out] log_file The file to write to.
 * @param[in] job The job to log.
 *
 * @return Zero if the function succeeds, else a POSIX error number.
 */
int log_arrival(FILE *log_file, const struct job_struct *job);

/**
 * @brief Log the total number of jobs put in to the queue by task().
 *
 * @param[in,out] log_file The file to write to.
 * @param time The time to add to the log.
 * @param n_jobs The number of jobs to add to the log.
 *
 * Uses the format:
 *
 *     Number of tasks put into Ready-Queue: <n_jobs>
 *     Terminates at time: <time>
 *
 * @return Zero if the function succeeds, else an error code that can be
 *         passed to errno_or_ae_to_str().
 */
int log_task_done(FILE *log_file, struct timespec time, unsigned long n_jobs);

/**
 * @brief Log statistics after all tasks are finished.
 *
 * Uses the format:
 * @verbatim
 * Number of tasks: #
 * Average waiting time: # seconds
 * Average turn around time: # seconds
 * @endverbatim
 *
 * @param log_file The file to write to.
 * @param stats The stats to get data from.
 *
 * @return Zero if the function succeeds, else an error code that can be
 *         passed to errno_or_ae_to_str().
 */
int log_main_done(FILE *log_file, struct cpu_shared_stats *stats);

#endif /* LOG_H */
