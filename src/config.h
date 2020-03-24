/**
 * @file   config.h
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Constants used to configure the rest of the program.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

/** The number of cpu function threads to spawn. */
static const unsigned int CPU_COUNT = 3;

/** The minimum size of the job queue. */
static const size_t QUEUE_SIZE_MIN = 1;

/** The maximum size of the job queue. */
static const size_t QUEUE_SIZE_MAX = 10;

/** The number of jobs for the task function to buffer before inserting in to
 * the queue. */
static const size_t TASK_JOB_BUFFER_LENGTH = 2;

/** The path of the simulation log to write to. */
static const char *const LOG_FILE_PATH = "simulation_log";

#endif /* CONFIG_H */
