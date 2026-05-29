#ifndef CORE_H
#define CORE_H

#include "frame.h"
#include <stdint.h>
#include <stddef.h>

#include "status.h"

// Opaque data structure, session handle
typedef struct _wcio_ctx wcio_ctx;

typedef struct
{
    const char *addr;
    int32_t port;
} wcio_connect_info;

// Establishes socket connection with endpoint, upgrades it to websocket
wcio_status wcio_connect(wcio_ctx **out_ctx, wcio_connect_info *conn_info);
// Closes the connection with endpoint, frees the wcio_ctx handle
int32_t wcio_close(wcio_ctx *ctx);


void wcio_send_text(wcio_ctx *ctx, const char *text);

// Content is optional, can be NULL
wcio_status wcio_send_ping(wcio_ctx *ctx, const char *content);
wcio_status wcio_send_pong(wcio_ctx *ctx, const char *content);

typedef struct
{
    int32_t bytes_read;
    wcio_status status;
} wcio_read_result;

typedef struct
{
    int32_t bytes_written;
    wcio_status status;
} wcio_write_result;

wcio_read_result wcio_read(wcio_ctx *ctx, size_t read_size, uint8_t *buffer);
char *wcio_recv(wcio_ctx *conn, size_t *out_len);

#endif
