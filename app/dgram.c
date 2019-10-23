#include <string.h>
#include "dgram.h"

typedef union {
    uint8_t u8[4];
    uint16_t u16[2];
    uint32_t u32[1];
} num_t;

void dgram_factory_init(dgram_t *dgram, char type)
{
    assert(dgram);
    dgram->buf[0] = type;
    dgram->len = 1;
    dgram->offset = 1;
}

void dgram_factory_append(dgram_t *dgram, const void *data, size_t len)
{
    assert(dgram);
    assert(data);
    assert(dgram->len + len < DGRAM_MAXLEN);

    memcpy(dgram->buf + dgram->offset, data, len);
    dgram->offset += len;
    dgram->len += len;
}

void dgram_factory_send(dgram_t *dgram, rs485_t *rs485)
{
    assert(dgram);
    assert(rs485);
    assert(dgram->len < DGRAM_MAXLEN);

    dgram->buf[dgram->len] = crc8(dgram->buf, dgram->len);
    dgram->len += 1;
    rs485_send(rs485, dgram->buf, dgram->len);
}

void dgram_reader_init(dgram_t *dgram, char type)
{
    assert(dgram);

    dgram->buf[0] = type;
    dgram->len = 1;
    dgram->offset = 1;
}

void dgram_reader_receive(dgram_t *dgram, rs485_t *rs485, size_t len)
{
    assert(dgram);
    assert(rs485);
    assert(dgram->len + len <= DGRAM_MAXLEN);

    rs485_get(rs485, dgram->buf + dgram->offset, len);
    dgram->offset += len;
    dgram->len += len;
}

int dgram_reader_finish(dgram_t *dgram)
{
    assert(dgram);
    assert(dgram->len >= 2);

    uint8_t crc = crc8(dgram->buf, dgram->len - 1);
    return (crc == dgram->buf[dgram->len - 1]) ? 0 : -1;
}

uint8_t dgram_get_u8(dgram_t *dgram, size_t offset)
{
    num_t *data = (num_t*) &dgram->buf[offset];
    return data->u8[0];
}

uint16_t dgram_get_u16(dgram_t *dgram, size_t offset)
{
    num_t *data = (num_t*) &dgram->buf[offset];
    return data->u16[0];
}

uint32_t dgram_get_u32(dgram_t *dgram, size_t offset)
{
    num_t *data = (num_t*) &dgram->buf[offset];
    return data->u32[0];
}
