#include <stdio.h>
#include <string.h>
#include "periph/gpio.h"
#include "xtimer.h"
#include "bmi160.h"
#include "bmi160_params.h"
#include "uart_half_duplex.h"

#define RS485_DIR       GPIO_PIN(PORT_A, 1)
#define RS485_UART      UART_DEV(1)
#define RS485_BAUDRATE  (115200L)
#include "crc8.h"

static void rs485_dir_init(uart_t uart)
{
    (void)uart;
    gpio_init(RS485_DIR, GPIO_OUT);
}

static void rs485_dir_enable_tx(uart_t uart)
{
    (void)uart;
    gpio_set(RS485_DIR);
}

static void rs485_dir_disable_tx(uart_t uart)
{
    (void)uart;
    gpio_clear(RS485_DIR);
}

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
    static uart_half_duplex_t rs485;
    static uart_half_duplex_params_t rs485_params = {
        .uart = RS485_UART,
        .baudrate = RS485_BAUDRATE,
        .dir = {
            .init = rs485_dir_init,
            .enable_tx = rs485_dir_enable_tx,
            .disable_tx = rs485_dir_disable_tx
        }
    };
    static uint8_t rs485_buffer[128];
    static bmi160_t bmi160;

    uart_half_duplex_init(&rs485, rs485_buffer, sizeof(rs485_buffer), &rs485_params);

    bmi160_init(&bmi160, bmi160_params);

    while (1) {
        static size_t received_bytes;
        static uint8_t cfg_active_sensors;
        static uint16_t cfg_sample_rate;
        static uint64_t next_measurment_us;
        static uint32_t sample_period_us;

        LED0_OFF;

        puts("Waiting for configuration ...");

        /* Empfange Konfigurationsstartbyte */
        while (1) {
            received_bytes = uart_half_duplex_recv(&rs485, 1);
            if (received_bytes < 1) continue;
            if (rs485.buffer[0] == 'A') break;
            rs485.size = 0;
        }

        /* Warte auf Konfiguration */
        received_bytes = uart_half_duplex_recv(&rs485, 5);
        if (received_bytes < 5) {
            puts("- Timeout");
            continue;
        }
        if (crc8(rs485.buffer, 4) != rs485.buffer[4]) {
            puts("- Wrong CRC");
            continue;
        }
        cfg_active_sensors = *((uint8_t *) (rs485.buffer + 1));
        cfg_sample_rate = *((uint16_t *) (rs485.buffer + 2));
        if (cfg_sample_rate == 0) {
            puts("- Sample rate must be greater than 0");
            continue;
        }
        /* ToDo: Check validity of sample rate */

        /* Einstellung der timer */
        sample_period_us = 1000000 / cfg_sample_rate;
        next_measurment_us = xtimer_now_usec64();

        puts("Starting measurement ...");

        LED0_ON;

        do {
            static bmi160_data_t acc, gyr;
            int i = 0;

            /* Clear receiver buffer */
            rs485.size = 0;

            /* Messung */
            bmi160_read(&bmi160, &gyr, &acc);

            /* Paket backen */
            rs485.buffer[i] = 'M';
            i += 1;
            if (cfg_active_sensors & CH_ACC_X) {
                memcpy(&rs485.buffer[i], &acc.x, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_ACC_Y) {
                memcpy(&rs485.buffer[i], &acc.y, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_ACC_Z) {
                memcpy(&rs485.buffer[i], &acc.z, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_GYR_X) {
                memcpy(&rs485.buffer[i], &gyr.x, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_GYR_Y) {
                memcpy(&rs485.buffer[i], &gyr.y, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_GYR_Z) {
                memcpy(&rs485.buffer[i], &gyr.z, 2);
                i += 2;
            }
            if (cfg_active_sensors & CH_TEMP) {
                rs485.buffer[i + 0] = 0;
                rs485.buffer[i + 1] = 0;
                i += 2;
            }
            if (cfg_active_sensors & CH_HUM) {
                rs485.buffer[i + 0] = 0;
                rs485.buffer[i + 1] = 0;
                i += 2;
            }
            rs485.buffer[i] = crc8(rs485.buffer, i);
            i += 1;

            /* Paket senden */
            uart_half_duplex_send(&rs485, i);

            next_measurment_us += sample_period_us;
            while (next_measurment_us > xtimer_now_usec64()) {
                if (rs485.size > 0 && rs485.buffer[0] == 'Z') break;
            }
        } while (rs485.size < 1 || rs485.buffer[0] != 'Z');
    }

    return 0;
}
