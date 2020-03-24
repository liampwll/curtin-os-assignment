/**
 * @file   main.c
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  User interaction and initialisation for other functions.
 */


#include "config.h"
#include "cpu.h"
#include "task.h"
#include "error.h"
#include "tsqueue.h"
#include "job.h"
#include "log.h"
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

/**
 * @brief Returns errno if @p ptr is NULL, else returns 0. Used to make main()
 *        a little bit cleaner.
 *
 * @param ptr The pointer to check.
 *
 * @return errno if @p ptr is NULL, else zero.
 */
static int errno_if_null(void *ptr);

int main(int argc, char **argv)
{
    // This function is very long but most of it is just braces and
    // whitespace.

    int retval = 0;

    FILE *log_file = NULL;
    FILE *input_file = NULL;
    pthread_t *cpu_threads = NULL;
    pthread_t task_thread;
    struct cpu_params *cpu_params = NULL;
    struct cpu_shared_stats stats = {0};
    struct task_params task_params = {0};
    struct job_struct *queue_data = NULL;
    tsqueue *queue = NULL;
    bool shared_is_initialised = false;
    size_t queue_length = 0;

    if (argc != 3)
    {
        retval = AE_WRONG_NUM_ARGS;
    }

    /*****************************************************/
    /* BEGINNING OF PARSING AND RESOURCE ALLOCATION CODE */
    /*****************************************************/

    if (retval == 0)
    {
        char *end;
        errno = 0;
        uintmax_t tmp = strtoumax(argv[2], &end, 10);
        if (errno)
        {
            retval = errno;
        }
        else if (tmp < QUEUE_SIZE_MIN || tmp > QUEUE_SIZE_MAX)
        {
            retval = EINVAL;
        }
        else
        {
            queue_length = (size_t)tmp;
        }
    }

    if (retval == 0)
    {
        retval = pthread_mutex_init(&stats.lock, NULL);
        if (retval == 0)
        {
            shared_is_initialised = true;
        }
    }

    if (retval == 0)
    {
        retval = errno_if_null(log_file = fopen(LOG_FILE_PATH, "a"));
    }

    if (retval == 0)
    {
        retval = errno_if_null(input_file = fopen(argv[1], "r"));
    }

    if (retval == 0)
    {
        retval =
            errno_if_null(cpu_threads = malloc(sizeof(*cpu_threads) * CPU_COUNT));
    }

    if (retval == 0)
    {
        retval =
            errno_if_null(cpu_params = malloc(sizeof(*cpu_params) * CPU_COUNT));
    }

    if (retval == 0)
    {
        retval =
            errno_if_null(queue_data = malloc(sizeof(*queue_data) * queue_length));
    }

    if (retval == 0)
    {
        retval = tsqueue_create(&queue, queue_data, queue_length,
                                sizeof(*queue_data), 0);
    }

    if (retval == 0)
    {
        for (unsigned int i = 0; i < CPU_COUNT; ++i)
        {
            cpu_params[i] = (struct cpu_params){
                .stats = &stats,
                .queue = queue,
                .id = i + 1,
                .log_file = log_file
            };
        }

        task_params = (struct task_params){
            .queue = queue,
            .job_file = input_file,
            .job_buffer_length = TASK_JOB_BUFFER_LENGTH,
            .log_file = log_file
        };
        retval = errno_if_null(task_params.job_buffer =
                                   malloc(sizeof(*task_params.job_buffer)
                                          * task_params.job_buffer_length));
    }

    /******************************************/
    /* END OF PARSING AND RESOURCE ALLOCATION */
    /******************************************/

    if (retval == 0)
    {
        retval = pthread_create(&task_thread, NULL, &task, &task_params);
    }

    if (retval == 0)
    {
        size_t i = 0;
        while (retval == 0 && i < CPU_COUNT)
        {
            retval =
                pthread_create(&cpu_threads[i], NULL, &cpu, &cpu_params[i]);
            ++i;
        }

        if (retval != 0)
        {
            tsqueue_close(queue);
            --i;
        }

        pthread_join(task_thread, NULL);
        for (size_t j = 0; j < i; ++j)
        {
            pthread_join(cpu_threads[j], NULL);
            if (retval == 0)
            {
                retval = cpu_params[j].retval;
            }
        }

        if (retval == 0)
        {
            retval = task_params.retval;
        }

    }

    if (retval == 0)
    {
        retval = log_main_done(log_file, &stats);
    }

    /******************************/
    /* BEGINNING OF TEARDOWN CODE */
    /******************************/

    if (retval != 0)
    {
        fprintf(stderr, "%s\nUsage: %s [job file] [queue size]\n",
                errno_or_ae_to_str(retval), argv[0]);
    }

    if (queue != NULL)
    {
        tsqueue_destroy(queue, NULL);
    }

    if (input_file != NULL)
    {
        fclose(input_file);
    }

    if (log_file != NULL)
    {
        fclose(log_file);
    }

    if (shared_is_initialised)
    {
        pthread_mutex_destroy(&stats.lock);
    }

    free(task_params.job_buffer);
    free(cpu_params);
    free(cpu_threads);
    free(queue_data);

    /************************/
    /* END OF TEARDOWN CODE */
    /************************/

    return retval;
}

static int errno_if_null(void *ptr)
{
    return (ptr == NULL) ? errno : 0;
}
