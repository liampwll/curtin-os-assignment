/**
 * @file   error.c
 * @author Liam Powell
 * @date   2019-04-25
 *
 * @brief  Implementation of error reporting functions.
 */

#include "error.h"
#include <string.h>

char *errno_or_ae_to_str(int err)
{
    char *retval = "Unknown error.";

    if (err >= 0)
    {
        retval = strerror(err);
    }
    else
    {
        switch ((enum ae_codes)err)
        {
        case AE_STR_NOT_A_NUMBER:
            retval = "Argument was not a number.";
            break;
        case AE_WRONG_NUM_ARGS:
            retval = "Wrong number of arguments.";
            break;
        case AE_BAD_FILE:
            retval = "File could not be parsed.";
        }
    }

    return retval;
}
