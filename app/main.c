#include <stdio.h>
#include <string.h>
#include "periph/gpio.h"
#include "xtimer.h"
#include "bmi160.h"
#include "bmi160_params.h"
#include "crc8.h"
#include "rs485.h"

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

int main(void)
{
    static rs485_t rs485;
    static bmi160_t bmi160;
    static uint8_t buf[18];

    rs485_init(&rs485, &rs485_params);

    bmi160_init(&bmi160, bmi160_params);

    while (1) {
        bool run;
        static uint8_t cfg_active_sensors;
        static uint16_t cfg_sample_rate;
        static uint64_t next_measurment_us;
        static uint32_t sample_period_us;

        puts("Waiting for configuration ...");

        /* Empfange Konfigurationsstartbyte */
        while (1) {
            int c = rs485_try_get_one(&rs485);
            if (c == 'A') break;
            if ((xtimer_now_usec() / 1000000) % 2) {
                LED0_OFF;
            }
            else {
                LED0_ON;
            }
        }

        /* Warte auf Konfiguration */
        buf[0] = 'A';
        rs485_get(&rs485, buf + 1, 4);
        if (crc8(buf, 4) != buf[4]) {
            puts("- Wrong CRC");
            continue;
        }
        cfg_active_sensors = *((uint8_t *) (buf + 1));
        cfg_sample_rate = *((uint16_t *) (buf + 2));
        if (cfg_sample_rate == 0) {
            puts("- Sample rate must be greater than 0");
            continue;
        }
        /* ToDo: Check validity of sample rate */

        /* Einstellung der timer */
        sample_period_us = 1000000 / cfg_sample_rate;
        next_measurment_us = xtimer_now_usec64();

        puts("Starting measurement ...");

        LED0_OFF;

        run = true;
        while (run) {
            static bmi160_data_t acc, gyr;
            int i = 0;

            /* Messung */
            bmi160_read(&bmi160, &gyr, &acc);

            /* Paket backen */
            buf[i] = 'M';
            i += 1;
            if (cfg_active_sensors & CH_ACC_X) {
                memcpy(&buf[i], &acc.x, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_ACC_Y) {
                memcpy(&buf[i], &acc.y, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_ACC_Z) {
                memcpy(&buf[i], &acc.z, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_GYR_X) {
                memcpy(&buf[i], &gyr.x, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_GYR_Y) {
                memcpy(&buf[i], &gyr.y, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_GYR_Z) {
                memcpy(&buf[i], &gyr.z, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_TEMP) {
                buf[i + 0] = 0;
                buf[i + 1] = 0;
                i += 2;
            }
            if (cfg_active_sensors & CH_HUM) {
                buf[i + 0] = 0;
                buf[i + 1] = 0;
                i += 2;
            }
            buf[i] = crc8(buf, i);
            i += 1;

            /* Paket senden */
            rs485_send(&rs485, buf, i);

            next_measurment_us += sample_period_us;
            do {
                if (rs485_try_get_one(&rs485) == 'Z') {
                    run = false;
                    break;
                }
            } while (next_measurment_us > xtimer_now_usec64());
        }
    }

    return 0;
}
