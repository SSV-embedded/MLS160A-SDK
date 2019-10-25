#include <stdio.h>
#include <string.h>
#include "periph/gpio.h"
#include "xtimer.h"
#include "riotboot/flashwrite.h"
#include "riotboot/slot.h"
#include "hashes/sha256.h"
#include "bmi160.h"
#include "bmi160_params.h"
#include "periph/pm.h"
#include "rs485.h"
#include "dgram.h"

#define FW_UPDATE_KEY "mls160a-update"

static const rs485_params_t rs485_params = {
    .uart = UART_DEV(1),
    .dir_pin = GPIO_PIN(PORT_A, 1),
    .baudrate = 115200
};

#define CH_ACC_X (0b00000001)
#define CH_ACC_Y (0b00000010)
#define CH_ACC_Z (0b00000100)
#define CH_GYR_X (0b00001000)
#define CH_GYR_Y (0b00010000)
#define CH_GYR_Z (0b00100000)
#define CH_TEMP  (0b01000000)
#define CH_HUM   (0b10000000)

static dgram_t dgram;
static rs485_t rs485;

static void mode_measure (void)
{
    static bmi160_t bmi160;
    uint8_t cfg_active_sensors;
    uint16_t cfg_sample_rate;
    uint64_t next_measurment_us;
    uint32_t sample_period_us;

    /* Init BMI160 */
    bmi160_init(&bmi160, bmi160_params);

    /* Read configuration frame */
    dgram_reader_init(&dgram, 'A');
    dgram_reader_receive(&dgram, &rs485, 4);
    if (dgram_reader_finish(&dgram)) {
        puts("- Wrong CRC");
        return;
    }
    cfg_active_sensors = dgram_get_u8(&dgram, 1);
    cfg_sample_rate = dgram_get_u16(&dgram, 2);
    if (cfg_sample_rate == 0) {
        puts("- Sample rate must be greater than 0");
        return;
    }
    /* ToDo: Check validity of sample rate */

    /* Setup timer */
    sample_period_us = 1000000 / cfg_sample_rate;
    next_measurment_us = xtimer_now_usec64();

    puts("Starting measurement ...");

    LED0_ON;

    while (1) {
        const uint16_t empty = 0;
        bmi160_data_t acc, gyr;

        /* Meaure ... */
        bmi160_read(&bmi160, &gyr, &acc);

        /* Build paket */
        dgram_factory_init(&dgram, 'M');
        if (cfg_active_sensors & CH_ACC_X) {
            dgram_factory_append(&dgram, &acc.x, sizeof(acc.x));
        }
        if (cfg_active_sensors & CH_ACC_Y) {
            dgram_factory_append(&dgram, &acc.y, sizeof(acc.y));
        }
        if (cfg_active_sensors & CH_ACC_Z) {
            dgram_factory_append(&dgram, &acc.z, sizeof(acc.z));
        }
        if (cfg_active_sensors & CH_GYR_X) {
            dgram_factory_append(&dgram, &gyr.x, sizeof(gyr.x));
        }
        if (cfg_active_sensors & CH_GYR_Y) {
            dgram_factory_append(&dgram, &gyr.y, sizeof(gyr.y));
        }
        if (cfg_active_sensors & CH_GYR_Z) {
            dgram_factory_append(&dgram, &gyr.z, sizeof(gyr.z));
        }
        if (cfg_active_sensors & CH_TEMP) {
            dgram_factory_append(&dgram, &empty, sizeof(empty));
        }
        if (cfg_active_sensors & CH_HUM) {
            dgram_factory_append(&dgram, &empty, sizeof(empty));
        }
        dgram_factory_send(&dgram, &rs485);

        /* Spin lock until it's time to send the next packet */
        next_measurment_us += sample_period_us;
        do {
            if (rs485_try_get_one(&rs485) == 'Z') {
                return;
            }
        } while (next_measurment_us > xtimer_now_usec64());
    }
}

static void mode_update (void)
{
    static riotboot_flashwrite_t fw;
    uint8_t slot = riotboot_slot_other();
    uint32_t offset = RIOTBOOT_FLASHWRITE_SKIPLEN;
    bool more = true;
    hmac_context_t hmac;
    uint8_t hmac_digest[SHA256_DIGEST_LENGTH];
    size_t max_len;

    /* Begin FW update */
    hmac_sha256_init(&hmac, FW_UPDATE_KEY, strlen(FW_UPDATE_KEY));
    assert(RIOTBOOT_FLASHWRITE_SKIPLEN == 4);
    hmac_sha256_update(&hmac, "RIOT", RIOTBOOT_FLASHWRITE_SKIPLEN);
    riotboot_flashwrite_init(&fw, slot);
    max_len = riotboot_flashwrite_slotsize(&fw);
    assert(max_len);

    printf("Starting FW update in slot %d (max_len=%d)\n", slot, max_len);

    /* Receive chunks */
    while (more) {
        uint16_t len_req = 128;
        uint16_t len_rsp;

        if (offset + len_req > max_len) len_req = max_len - offset;

        printf("Requesting chunk from offset 0x%06lx with length %d\n", offset, len_req);

        /* Request chunk */
        dgram_factory_init(&dgram, 'R');
        dgram_factory_append(&dgram, &slot, sizeof(slot));
        dgram_factory_append(&dgram, &offset, sizeof(offset));
        dgram_factory_append(&dgram, &len_req, sizeof(len_req));
        dgram_factory_send(&dgram, &rs485);

        /* Receive chunk */
        while (1) {
            int c = rs485_try_get_one(&rs485);
            if (c == 'Z') return;
            if (c == 'P') break;
        }
        dgram_reader_init(&dgram, 'P');
        dgram_reader_receive(&dgram, &rs485, 1 + 4 + 2 + 1); // SLOT + OFFSET + LEN + CRC
        if (dgram_get_u8(&dgram, 1) != slot) continue;
        if (dgram_get_u32(&dgram, 2) != offset) continue;
        len_rsp = dgram_get_u16(&dgram, 6);
        if (len_rsp > len_req) continue;
        dgram_reader_receive(&dgram, &rs485, len_rsp);
        if (dgram_reader_finish(&dgram)) continue;
        more = len_req == len_rsp;

        printf("Received chunk from offset 0x%06lx with length %d\n", offset, len_rsp);
        LED0_TOGGLE;

        /* Write chunk */
        riotboot_flashwrite_putbytes(&fw, &dgram.buf[8], len_rsp, more);
        offset += len_rsp;
        hmac_sha256_update(&hmac, &dgram.buf[8], len_rsp);
    }

    /* Request HMAC */
    rs485_send(&rs485, (const uint8_t *) "V", 1);
    while (1) {
        int c = rs485_try_get_one(&rs485);
        if (c == 'Z') return;
        if (c == 'H') break;
    }
    dgram_reader_init(&dgram, 'H');
    dgram_reader_receive(&dgram, &rs485, 1 + SHA256_DIGEST_LENGTH);
    if (dgram_reader_finish(&dgram)) goto failed;

    /* Check HMAC */
    hmac_sha256_final(&hmac, hmac_digest);
    if (memcmp(hmac_digest, &dgram.buf[1], SHA256_DIGEST_LENGTH) != 0) goto failed;

    /* Enable slot */
    riotboot_flashwrite_finish(&fw);

    rs485_send(&rs485, (const uint8_t *) "Y", 1);
    puts("Finished successfully");
    pm_reboot();
    return;

failed:
    rs485_send(&rs485, (const uint8_t *) "N", 1);
    puts("Error!");
}

int main(void)
{
    unsigned slot;
    const riotboot_hdr_t * hdr;


    /* Info about current slot */
    slot = riotboot_slot_current();
    hdr = riotboot_slot_get_hdr(slot);
    printf("Current slot: %d\n", slot);
    riotboot_hdr_print(hdr);

    /* Get up RS485 interface */
    rs485_init(&rs485, &rs485_params);

    while (1) {
        puts("Waiting for instruction ...");

        while (1) {
            /* Blicky LED ... */
            if ((xtimer_now_usec() / 1000000) % 2) {
                LED0_OFF;
            }
            else {
                LED0_ON;
            }

            /* First byte decides where to go ... */
            switch (rs485_try_get_one(&rs485)) {
                case 'A': mode_measure(); break;
                case 'U': mode_update(); break;
            }
        }
    }

    return 0;
}
