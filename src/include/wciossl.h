#ifndef WCIO_SSL_IMPL_H
#define WCIO_SSL_IMPL_H

#include "wcio.h"

typedef struct
{
    SSL_CTX *ctx;
    const char *err_msg;
} wcio_ssl_result;

wcio_ssl_result wcio_init_ssl_ctx();
void wcio_ssl_load_certs();

#endif
