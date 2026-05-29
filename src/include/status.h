#ifndef WCIO_STATUS_H
#define WCIO_STATUS_H

#include <stdint.h>

typedef struct
{
    int32_t code;
    char *error_msg;
} wcio_status;

#define WCIO_STATUS_OK (wcio_status){ .code = 0, .error_msg = NULL }

#define WCIO_IS_OK(status) (status.code == 0)

#endif
