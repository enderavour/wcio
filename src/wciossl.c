#include "include/wciossl.h"
#include "include/wcio.h"
#include <openssl/err.h>

wcio_ssl_result wcio_init_ssl_ctx()
{
    SSL_METHOD *method;
    SSL_CTX *ctx;

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    method = (SSL_METHOD*)TLS_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        return (wcio_ssl_result){
            .ctx = NULL,
            .err_msg = ERR_error_string(ERR_get_error(), NULL)
        };
    }

    return (wcio_ssl_result){
        .ctx = ctx,
        .err_msg = NULL
    };
}
