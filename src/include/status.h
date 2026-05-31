#ifndef WCIO_STATUS_H
#define WCIO_STATUS_H

#include <stdint.h>

// Return status of the wcio functions.
typedef struct
{
    int32_t code;
    char *error_msg;
} wcio_status;

// Helper macro to return success value from the functions.
#define WCIO_STATUS_OK (wcio_status){ .code = 0, .error_msg = NULL }
// Hepler macro to check if returned value is success.
#define WCIO_IS_OK(status) (status.code == 0)

#endif
