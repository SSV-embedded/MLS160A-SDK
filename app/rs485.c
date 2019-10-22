#include "rs485.h"

void _rx_cb(void *arg, uint8_t c)
{
    rs485_t *ctx = arg;
    tsrb_add_one(&ctx->rb, c);
}

int rs485_init(rs485_t *ctx, const rs485_params_t *params)
{
    int rc;

    assert(ctx);
    assert(params);

    ctx->uart = params->uart;
    rc = uart_init(ctx->uart, params->baudrate, _rx_cb, ctx);
    if (rc) {
        return rc;
    }

    ctx->dir_pin = params->dir_pin;
    rc = gpio_init(ctx->dir_pin, GPIO_OUT);
    if (rc) {
        return rc;
    }
    gpio_clear(ctx->dir_pin);

    tsrb_init(&ctx->rb, ctx->buf, sizeof(ctx->buf));

    return 0;
}

int rs485_get(rs485_t *ctx, uint8_t *dst, size_t n)
{
    while (tsrb_avail(&ctx->rb) < n) {}
    return tsrb_get(&ctx->rb, dst, n);
}
