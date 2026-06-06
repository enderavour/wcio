#include "include/wcio.h"
#include "include/frame.h"
#include "include/status.h"
#include "include/base64.h"
#include "include/definitions.h"
#include "include/wciossl.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <time.h>


#ifdef __APPLE__

#include <libkern/OSByteOrder.h>
#define be64toh(x) OSSwapBigToHostInt64(x)


#endif

struct _wcio_ctx
{
    wcio_connect_info conn_info;
    SSL_CTX *ssl_ctx;
    BIO *ssl_bio;
    SSL *ssl;
    SSL_METHOD *method;
    struct addrinfo *addr;
    int32_t socket_fd;
    uint8_t is_encrypted; // if SSL is enabled
};

static wcio_write_result wcio_write(wcio_ctx *ctx, uint8_t *dat, size_t data_sizes);
wcio_ws_frame *wcio_recv_frame(wcio_ctx *ctx);

wcio_status wcio_connect(wcio_ctx **out_ctx, wcio_connect_info *conn_info)
{
    srand(time(NULL));

    wcio_ctx *ctx = (wcio_ctx*)malloc(sizeof(wcio_ctx));
    ctx->is_encrypted = 0;

    if (conn_info->port == 80)
    {
        int32_t sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
        {
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        struct addrinfo hints = {0}, *res;

        // Conversion of integer valued port into the string
        char s_port[10] = {0};
        sprintf(s_port, "%d", conn_info->port);

        int32_t code = getaddrinfo(conn_info->addr, s_port, &hints, &res);
        if (code != 0)
        {
            free(ctx);
            close(sfd);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        if (connect(sfd, res->ai_addr, res->ai_addrlen) == -1)
        {
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        char upgrade_header[256] = {0};

        uint8_t *ws_key = wcio_gen_secure_key();
        sprintf(upgrade_header, "GET / HTTP/1.1\r\n"
                                "Host: %s\r\n"
                                "Upgrade: websocket\r\n"
                                "Connection: Upgrade\r\n"
                                "Sec-WebSocket-Key: %s\r\n"
                                "Origin: null\r\n"
                                "Sec-WebSocket-Protocol: soap, wamp\r\n"
                                "Sec-WebSocket-Version: 13\r\n\r\n", conn_info->addr, ws_key);
        free(ws_key);

        if (send(sfd, upgrade_header, strlen(upgrade_header), 0) == -1)
        {
            printf("Error sending the upgrade header to endpoint, Reason: %s\n", strerror(errno));
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        uint8_t *recvbuf = (uint8_t*)malloc(WCIO_HANDSHAKE_RECV_BUFFER_SIZE);
        recvbuf[WCIO_HANDSHAKE_RECV_BUFFER_SIZE - 1] = 0;

        int32_t recv_size = recv(sfd, recvbuf, WCIO_HANDSHAKE_RECV_BUFFER_SIZE - 1, 0);

        if (recv_size == -1)
        {
            printf("Error receiving the data from the endpoint, Reason: %s\n", strerror(errno));
            free(recvbuf);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        char *resp_first_field = strchr((char*)recvbuf, '\r');
        if (!resp_first_field)
        {
            free(recvbuf);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        int32_t len = (uint8_t*)resp_first_field - recvbuf;

        // Extracting server code
        char arr[40] = {0};
        strncpy(arr, (char*)recvbuf, len);
        char *status_code_str = strtok(arr, " ");
        status_code_str = strtok(NULL, " ");
        int32_t status_code = atoi(status_code_str);

        if (status_code == 101) {}
        else
        {
            free(recvbuf);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }
        free(recvbuf);

        ctx->addr = res;
        ctx->conn_info = *conn_info;
        ctx->socket_fd = sfd;

        *out_ctx = ctx;
        return WCIO_STATUS_OK;
    }
    else if (conn_info->port == 443)
    {
        ctx->is_encrypted = 1;
        wcio_ssl_result ssl_res = wcio_init_ssl_ctx();
        if (ssl_res.ctx != NULL)
            ctx->ssl_ctx = ssl_res.ctx;

        ctx->ssl = SSL_new(ctx->ssl_ctx);

        int32_t sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
        {
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        struct addrinfo hints = {0}, *res;

        // Conversion of integer valued port into the string
        char s_port[10] = {0};
        sprintf(s_port, "%d", conn_info->port);

        int32_t code = getaddrinfo(conn_info->addr, s_port, &hints, &res);
        if (code != 0)
        {
            free(ctx);
            close(sfd);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        if (connect(sfd, res->ai_addr, res->ai_addrlen) == -1)
        {
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        SSL_set_fd(ctx->ssl, sfd);

        if (SSL_connect(ctx->ssl) == -1)
        {
            SSL_shutdown(ctx->ssl);
            SSL_free(ctx->ssl);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = ERR_get_error(),
                .error_msg = ERR_error_string(ERR_get_error(), NULL)
            };
        }

        char upgrade_header[256] = {0};

        uint8_t *ws_key = wcio_gen_secure_key();
        sprintf(upgrade_header, "GET / HTTP/1.1\r\n"
                                "Host: %s\r\n"
                                "Upgrade: websocket\r\n"
                                "Connection: Upgrade\r\n"
                                "Sec-WebSocket-Key: %s\r\n"
                                "Origin: null\r\n"
                                "Sec-WebSocket-Protocol: soap, wamp\r\n"
                                "Sec-WebSocket-Version: 13\r\n\r\n", conn_info->addr, ws_key);
        free(ws_key);

        if (SSL_write(ctx->ssl, upgrade_header, strlen(upgrade_header)) == -1)
        {
            printf("Error sending the upgrade header to endpoint, Reason: %s\n", strerror(errno));
            SSL_shutdown(ctx->ssl);
            SSL_free(ctx->ssl);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = ERR_get_error(),
                .error_msg = ERR_error_string(ERR_get_error(), NULL)
            };
        }

        uint8_t *recvbuf = (uint8_t*)malloc(WCIO_HANDSHAKE_RECV_BUFFER_SIZE);
        recvbuf[WCIO_HANDSHAKE_RECV_BUFFER_SIZE - 1] = 0;

        int32_t recv_size = SSL_read(ctx->ssl, recvbuf, WCIO_HANDSHAKE_RECV_BUFFER_SIZE - 1);

        if (recv_size == -1)
        {
            printf("Error receiving the data from the endpoint, Reason: %s\n", strerror(errno));
            SSL_shutdown(ctx->ssl);
            SSL_free(ctx->ssl);
            free(recvbuf);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = ERR_get_error(),
                .error_msg = ERR_error_string(ERR_get_error(), NULL)
            };
        }

        char *resp_first_field = strchr((char*)recvbuf, '\r');
        if (!resp_first_field)
        {
            SSL_shutdown(ctx->ssl);
            SSL_free(ctx->ssl);
            free(recvbuf);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }

        int32_t len = (uint8_t*)resp_first_field - recvbuf;

        // Extracting server code
        char arr[40] = {0};
        strncpy(arr, (char*)recvbuf, len);
        char *status_code_str = strtok(arr, " ");
        status_code_str = strtok(NULL, " ");
        int32_t status_code = atoi(status_code_str);

        if (status_code == 101) {}
        else
        {
            SSL_shutdown(ctx->ssl);
            SSL_free(ctx->ssl);
            free(recvbuf);
            free(ctx);
            close(sfd);
            freeaddrinfo(res);
            return (wcio_status){
                .code = errno,
                .error_msg = strerror(errno)
            };
        }
        free(recvbuf);

        ctx->addr = res;
        ctx->conn_info = *conn_info;
        ctx->socket_fd = sfd;

        *out_ctx = ctx;
        return WCIO_STATUS_OK;
    }
}

wcio_status wcio_close(wcio_ctx *ctx)
{
    size_t resulting_size;
    uint16_t close_code = htons(1000);
    wcio_ws_frame *frame = wcio_build_ws_frame(1, CLOSE_CONNECTION, (uint8_t*)&close_code, 2, &resulting_size);
    uint8_t *bytes = wcio_ws_frame_to_bytes(frame, resulting_size);
    wcio_write(ctx, bytes, resulting_size);

    wcio_ws_frame *close_frame = wcio_recv_frame(ctx);
    if (close_frame)
    {
        uint8_t opcode = close_frame->hdr[0] & 0x0F;
        if (opcode == 0x8)
        {
            free(close_frame);
            freeaddrinfo(ctx->addr);
            close(ctx->socket_fd);
            return WCIO_STATUS_OK;
        }
    }
    freeaddrinfo(ctx->addr);
    close(ctx->socket_fd);
    return (wcio_status){
        .code = -1,
        .error_msg = "Close frame was not received from the endpoint, but the connection was still closed"
    };
}

static wcio_write_result wcio_write(wcio_ctx *ctx, uint8_t *data, size_t data_size)
{
    int32_t bytes_written = 0;

    if (ctx->is_encrypted)
    {
        bytes_written = SSL_write(ctx->ssl, data, (int32_t)data_size);

        if (bytes_written <= 0)
        {
            int32_t ssl_err = SSL_get_error(ctx->ssl, bytes_written);

            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
            {
                return (wcio_write_result){
                    .bytes_written = 0,
                    .status = WCIO_STATUS_OK
                };
            }

            uint64_t err = ERR_get_error();

            return (wcio_write_result){
                .bytes_written = -1,
                .status = {
                    .code = err,
                    .error_msg = ERR_error_string(err, NULL)
                }
            };
        }
    }
    else
    {
        bytes_written = (int32_t)send(ctx->socket_fd, data, data_size, 0);

        if (bytes_written < 0)
        {
            return (wcio_write_result){
                .bytes_written = -1,
                .status = {
                    .code = errno,
                    .error_msg = strerror(errno)
                }
            };
        }
    }

    return (wcio_write_result){
        .bytes_written = bytes_written,
        .status = WCIO_STATUS_OK
    };
}

wcio_status wcio_send_text(wcio_ctx *ctx, const char *text)
{
    size_t payload_size = strlen(text);
    size_t resulting_frame_size = 0;

    if (payload_size > WCIO_FRAGMENT_SIZE)
    {
        size_t chunk_size = WCIO_FRAGMENT_SIZE;

        size_t chunk_ptr = 0;

        size_t first_size = chunk_size;
        uint8_t *chunk = (uint8_t*)malloc(first_size);

        memcpy(chunk, text, first_size);

        wcio_ws_frame *frame =
            wcio_build_ws_frame(0, TEXT_FRAME, chunk, first_size, &resulting_frame_size);

        uint8_t *bytes = wcio_ws_frame_to_bytes(frame, resulting_frame_size);
        wcio_write(ctx, bytes, resulting_frame_size);

        free(bytes);
        free(chunk);

        chunk_ptr += first_size;

        while (payload_size - chunk_ptr > chunk_size)
        {
            uint8_t *mid = (uint8_t*)malloc(chunk_size);

            memcpy(mid, text + chunk_ptr, chunk_size);

            wcio_ws_frame *frame =
                wcio_build_ws_frame(0, 0, mid, chunk_size, &resulting_frame_size);

            uint8_t *bytes = wcio_ws_frame_to_bytes(frame, resulting_frame_size);
            wcio_write(ctx, bytes, resulting_frame_size);

            free(bytes);
            free(mid);

            chunk_ptr += chunk_size;
        }

        size_t last_size = payload_size - chunk_ptr;

        uint8_t *last_chunk = (uint8_t*)malloc(last_size);

        memcpy(last_chunk, text + chunk_ptr, last_size);

        wcio_ws_frame *last_frame =
            wcio_build_ws_frame(1, 0, last_chunk, last_size, &resulting_frame_size);

        uint8_t *last_bytes =
            wcio_ws_frame_to_bytes(last_frame, resulting_frame_size);

        wcio_write_result res = wcio_write(ctx, last_bytes, resulting_frame_size);

        free(last_bytes);
        free(last_chunk);

        return res.status;
    }
    else
    {
        wcio_ws_frame *frame =
            wcio_build_ws_frame(1, TEXT_FRAME, (uint8_t*)text, payload_size, &resulting_frame_size);

        uint8_t *bytes =
            wcio_ws_frame_to_bytes(frame, resulting_frame_size);

        wcio_write_result res = wcio_write(ctx, bytes, resulting_frame_size);

        free(bytes);
        return res.status;
    }

    return WCIO_STATUS_OK;
}

wcio_status wcio_send_binary(wcio_ctx *ctx, uint8_t *data, size_t data_len)
{
    size_t payload_size = data_len;
    size_t resulting_frame_size = 0;

    if (payload_size > WCIO_FRAGMENT_SIZE)
    {
        size_t chunk_size = WCIO_FRAGMENT_SIZE;

        size_t chunk_ptr = 0;

        size_t first_size = chunk_size;
        uint8_t *chunk = (uint8_t*)malloc(first_size);

        memcpy(chunk, data, first_size);

        wcio_ws_frame *frame =
            wcio_build_ws_frame(0, BINARY_FRAME, chunk, first_size, &resulting_frame_size);

        uint8_t *bytes = wcio_ws_frame_to_bytes(frame, resulting_frame_size);
        wcio_write(ctx, bytes, resulting_frame_size);

        free(bytes);
        free(chunk);

        chunk_ptr += first_size;

        while (payload_size - chunk_ptr > chunk_size)
        {
            uint8_t *mid = (uint8_t*)malloc(chunk_size);

            memcpy(mid, data + chunk_ptr, chunk_size);

            wcio_ws_frame *frame =
                wcio_build_ws_frame(0, 0, mid, chunk_size, &resulting_frame_size);

            uint8_t *bytes = wcio_ws_frame_to_bytes(frame, resulting_frame_size);
            wcio_write(ctx, bytes, resulting_frame_size);

            free(bytes);
            free(mid);

            chunk_ptr += chunk_size;
        }

        size_t last_size = payload_size - chunk_ptr;

        uint8_t *last_chunk = (uint8_t*)malloc(last_size);

        memcpy(last_chunk, data + chunk_ptr, last_size);

        wcio_ws_frame *last_frame =
            wcio_build_ws_frame(1, 0, last_chunk, last_size, &resulting_frame_size);

        uint8_t *last_bytes =
            wcio_ws_frame_to_bytes(last_frame, resulting_frame_size);

        wcio_write_result res = wcio_write(ctx, last_bytes, resulting_frame_size);

        free(last_bytes);
        free(last_chunk);

        return res.status;
    }
    else
    {
        wcio_ws_frame *frame =
            wcio_build_ws_frame(1, BINARY_FRAME, data, payload_size, &resulting_frame_size);

        uint8_t *bytes =
            wcio_ws_frame_to_bytes(frame, resulting_frame_size);

        wcio_write_result res = wcio_write(ctx, bytes, resulting_frame_size);

        free(bytes);
        return res.status;
    }

    return WCIO_STATUS_OK;
}

wcio_read_result wcio_read(wcio_ctx *ctx, size_t read_size, uint8_t *buffer)
{
    int bytes_read = 0;

    if (ctx->is_encrypted)
    {
        bytes_read = SSL_read(ctx->ssl, buffer, (int32_t)read_size);

        if (bytes_read <= 0)
        {
            int32_t ssl_err = SSL_get_error(ctx->ssl, bytes_read);

            if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
            {
                return (wcio_read_result){
                    .bytes_read = 0,
                    .status = WCIO_STATUS_OK
                };
            }

            uint64_t err = ERR_get_error();

            return (wcio_read_result){
                .bytes_read = -1,
                .status = {
                    .code = err,
                    .error_msg = ERR_error_string(err, NULL)
                }
            };
        }
    }
    else
    {
        bytes_read = (int32_t)read(ctx->socket_fd, buffer, read_size);

        if (bytes_read < 0)
        {
            return (wcio_read_result){
                .bytes_read = -1,
                .status = {
                    .code = errno,
                    .error_msg = strerror(errno)
                }
            };
        }
    }

    return (wcio_read_result){
        .bytes_read = bytes_read,
        .status = WCIO_STATUS_OK
    };
}


wcio_ws_frame *wcio_recv_frame(wcio_ctx *ctx)
{
    uint8_t hdr[2];
    wcio_read(ctx, 2, hdr);

    uint64_t len = hdr[1] & 0x7F;
    uint8_t masked = (hdr[1] & 0x80) != 0;

    if (len == 126)
    {
        uint16_t tmp;
        wcio_read(ctx, 2, (uint8_t*)&tmp);
        len = ntohs(tmp);
    }
    else if (len == 127)
    {
        uint64_t tmp;
        wcio_read(ctx, 8, (uint8_t*)&tmp);
        len = be64toh(tmp);
    }

    uint8_t mask_key[4] = {0};
    if (masked)
        wcio_read(ctx, 4, mask_key);

    uint8_t *payload = malloc(len);
    wcio_read(ctx, len, payload);

    if (masked)
    {
        for (uint64_t i = 0; i < len; i++)
            payload[i] ^= mask_key[i % 4];
    }

    wcio_ws_frame *frame =
        (wcio_ws_frame*)malloc(sizeof(*frame) + len);

    frame->hdr[0] = hdr[0];
    frame->hdr[1] = hdr[1];

    memcpy(frame->data, payload, len);

    free(payload);

    return frame;
}

char *wcio_recv(wcio_ctx *conn, size_t *out_len)
{
    char *buffer = NULL;
    size_t total_size = 0;

    while (1)
    {
        wcio_ws_frame *frame = wcio_recv_frame(conn);

        if (!frame)
            return NULL;

        uint8_t opcode = frame->hdr[0] & 0x0F;
        uint8_t fin = (frame->hdr[0] & 0x80) != 0;

        uint64_t payload_len;
        uint8_t payload_len_initial = frame->hdr[1] & 0x7F;
        uint16_t payload_len16 = 0;

        if (payload_len == 126)
        {
            memcpy(&payload_len16, frame->data, 2);
            payload_len = payload_len16;
        }
        if (payload_len == 127)
            memcpy(&payload_len, frame->data, 8);

        payload_len = payload_len_initial;

        // Close frame
        if (opcode == 0x8)
        {
            free(frame);
            return NULL;
        }

        // Ping frame
        if (opcode == 0x9)
        {
            size_t pong_size;

            wcio_ws_frame *pong = wcio_build_ws_frame(
                1,
                PONG,
                frame->data,
                payload_len,
                &pong_size
            );

            uint8_t *bytes = wcio_ws_frame_to_bytes(pong, pong_size);
            wcio_write(conn, bytes, pong_size);

            free(bytes);
            continue;
        }

        buffer = realloc(buffer, total_size + payload_len + 1);
        memcpy(buffer + total_size, frame->data, payload_len);
        total_size += payload_len;

        free(frame);

        if (fin)
            break;
    }

    buffer[total_size] = 0;

    if (out_len)
        *out_len = total_size;

    return buffer;
}


wcio_status wcio_send_ping(wcio_ctx *ctx, const char *content)
{
    size_t rs_frame_sz;
    wcio_ws_frame *frame = wcio_build_ws_frame(1, PING, (uint8_t*)content, strlen(content), &rs_frame_sz);
    uint8_t *raw_frame = wcio_ws_frame_to_bytes(frame, rs_frame_sz);
    wcio_write(ctx, raw_frame, rs_frame_sz);
    free(raw_frame);
    wcio_ws_frame *pong = wcio_recv_frame(ctx);
    uint8_t opcode = pong->hdr[0] & 0x0F;

    if (opcode == 0xA)
    {
        free(frame);
        return WCIO_STATUS_OK;
    }
    else
    {
        free(frame);
        return (wcio_status){
            .code = 1,
            .error_msg = "Did not receive PONG frame from server"
        };
    }
}

wcio_status wcio_send_pong(wcio_ctx *ctx, const char *content)
{
    size_t rs_frame_sz;
    wcio_ws_frame *frame = wcio_build_ws_frame(1, PONG, (uint8_t*)content, strlen(content), &rs_frame_sz);
    uint8_t *raw_frame = wcio_ws_frame_to_bytes(frame, rs_frame_sz);
    wcio_write_result wr_rs = wcio_write(ctx, raw_frame, rs_frame_sz);
    free(raw_frame);
    return wr_rs.status;
}
