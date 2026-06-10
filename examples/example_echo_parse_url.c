#include "include/status.h"
#include "include/wcio.h"
#include <stdio.h>
#include <stdlib.h>

int32_t main()
{
    wcio_connect_info *pinfo = wcio_parse_url("ws://ws.ifelse.io");
    wcio_ctx *ctx = wcio_ctx_alloc();
    wcio_status stat = wcio_connect(&ctx, pinfo);
    if (!WCIO_IS_OK(stat))
    {
        printf("Error establishing connection: %s", stat.error_msg);
        return stat.code;
    }
    wcio_send_text(ctx, "Hello from unencrypted connection!");
    size_t msg_len = 0;
    char *res = wcio_recv(ctx, &msg_len);
    printf("%s\n", res);
    free(res);
    wcio_connect_info_free(pinfo);
    if (WCIO_IS_OK(wcio_close(ctx)))
        printf("Received closing frame, closing connection\n");
    wcio_ctx_free(ctx);
    return 0;
}
