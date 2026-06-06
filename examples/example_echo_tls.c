#include "include/status.h"
#include "include/wcio.h"
#include <stdio.h>
#include <stdlib.h>

int32_t main()
{
    wcio_connect_info conn_info = {
        .addr = "websocket-echo.com",
        .port = 443 // Just specify 443 port
    };
    wcio_ctx *ctx;
    wcio_status stat = wcio_connect(&ctx, &conn_info);
    if (!WCIO_IS_OK(stat))
    {
        printf("Error establishing connection: %s", stat.error_msg);
        return stat.code;
    }
    wcio_send_text(ctx, "Hello from encrypted connection!");
    size_t msg_len = 0;
    char *res = wcio_recv(ctx, &msg_len);
    printf("%s\n", res);
    free(res);
    if (WCIO_IS_OK(wcio_close(ctx)))
        printf("Received closing frame, closing connection\n");
    return 0;
}
