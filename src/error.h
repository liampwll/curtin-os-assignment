/**
 * @file   error.h
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Error reporting functions.
 */

#ifndef ERROR_H
#define ERROR_H

#include <limits.h>

// AE stands for application errno

enum ae_codes {
    // The reason we use negative numbers is that POSIX errno values are
    // always positive. This allows us to return a POSIX error value or a
    // custom error value and differentiate between them.

    /** Argument did not represent a number. */
    AE_STR_NOT_A_NUMBER = INT_MIN,

    /** The program was called with the wrong number of arguments. */
    AE_WRONG_NUM_ARGS,

    /** The file could not be parsed. */
    AE_BAD_FILE
};

/**
 * @brief Converts an application error value or an errno value to a
 *        string. Not thread safe.
 *
 * @param err The error number to convert.
 *
 * @return String relevant to the error.
 */
char *errno_or_ae_to_str(int err);

#endif /* ERROR_H */
