#ifndef CORE_H
#define CORE_H

#include "frame.h"
#include <stdint.h>
#include <stddef.h>

#include "status.h"

// Opaque data structure, session handle
typedef struct _wcio_ctx wcio_ctx;

// Information about the endpoint to connect.
typedef struct
{
    const char *addr;
    int32_t port;
} wcio_connect_info;

// Establishes socket connection with endpoint, upgrades it to websocket
wcio_status wcio_connect(wcio_ctx **out_ctx, wcio_connect_info *conn_info);
// Closes the connection with endpoint, frees the wcio_ctx handle
wcio_status wcio_close(wcio_ctx *ctx);
// Sends text data into the endpoint
wcio_status wcio_send_text(wcio_ctx *ctx, const char *text);
// Sends binary data into the endpoint
wcio_status wcio_send_binary(wcio_ctx *ctx, uint8_t *data, size_t data_len);

// Sends PING frame. Content is optional, can be null
wcio_status wcio_send_ping(wcio_ctx *ctx, const char *content);
// Sends PONG frame. Content is optional, can be null
wcio_status wcio_send_pong(wcio_ctx *ctx, const char *content);

// Return status of the wcio_read function.
typedef struct
{
    int32_t bytes_read;
    wcio_status status;
} wcio_read_result;

// Return status of the wcio_write function.
typedef struct
{
    int32_t bytes_written;
    wcio_status status;
} wcio_write_result;

// Reads bytes from the socket into the buffer.
wcio_read_result wcio_read(wcio_ctx *ctx, size_t read_size, uint8_t *buffer);
// Receives frame(s), composes them together and returns the payload of the frame(s)
char *wcio_recv(wcio_ctx *conn, size_t *out_len);

#endif
