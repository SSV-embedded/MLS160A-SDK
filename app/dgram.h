#ifndef DGRAM_H
#define DGRAM_H

#include "crc8.h"
#include "rs485.h"

#define DGRAM_MAXLEN (256)

typedef struct {
    size_t len;
    size_t offset;
    uint8_t buf[DGRAM_MAXLEN];
} dgram_t;

void dgram_factory_init(dgram_t *dgram, char type);
void dgram_factory_append(dgram_t *dgram, const void *data, size_t len);
void dgram_factory_send(dgram_t *dgram, rs485_t *rs485);

void dgram_reader_init(dgram_t *dgram, char type);
void dgram_reader_receive(dgram_t *dgram, rs485_t *rs485, size_t len);
int dgram_reader_finish(dgram_t *dgram);

uint8_t dgram_get_u8(dgram_t *dgram, size_t offset);
uint16_t dgram_get_u16(dgram_t *dgram, size_t offset);
uint32_t dgram_get_u32(dgram_t *dgram, size_t offset);

#endif
