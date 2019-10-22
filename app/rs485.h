#ifndef RS485_H
#define RS485_H

#include "periph/uart.h"
#include "periph/gpio.h"
#include "tsrb.h"

#define RS485_BUFSIZE (2048)

typedef struct {
    uart_t uart;
    gpio_t dir_pin;
    uint8_t buf[RS485_BUFSIZE];
    tsrb_t rb;
} rs485_t;

typedef struct {
    uart_t uart;
    uint32_t baudrate;
    gpio_t dir_pin;
} rs485_params_t;

int rs485_init(rs485_t *ctx, const rs485_params_t *params);

inline void rs485_send(rs485_t *ctx, const uint8_t *data, size_t len)
{
    gpio_set(ctx->dir_pin);
    uart_write(ctx->uart, data, len);
    gpio_clear(ctx->dir_pin);
}

inline int rs485_try_get_one(rs485_t *ctx)
{
    return tsrb_get_one(&ctx->rb);
}

int rs485_get(rs485_t *ctx, uint8_t *dst, size_t n);

#endif
