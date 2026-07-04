#ifndef LIBSIGROK_HARDWARE_PSLAB_PICO_PROTOCOL_H
#define LIBSIGROK_HARDWARE_PSLAB_PICO_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PSLAB_PICO_MAX_CHANNELS 8
#define PSLAB_PICO_MAX_SAMPLES 65536
#define PSLAB_PICO_DEFAULT_SYSCLK_HZ 150000000u
#define PSLAB_PICO_DEFAULT_PIN_BASE 16u
#define PSLAB_PICO_DEFAULT_PIN_COUNT 2u
#define PSLAB_PICO_DEFAULT_SAMPLES 1024u
#define PSLAB_PICO_DEFAULT_SAMPLERATE 1000000u
#define PSLAB_PICO_IO_TIMEOUT_MS 5000

typedef enum {
    PSLAB_PICO_TRIGGER_AUTO = 0,
    PSLAB_PICO_TRIGGER_HIGH,
    PSLAB_PICO_TRIGGER_LOW,
    PSLAB_PICO_TRIGGER_RISING,
    PSLAB_PICO_TRIGGER_FALLING,
} PslabPicoTrigger;

typedef struct {
    int fd;
    char *conn;
    uint32_t sysclk_hz;
    uint32_t samplerate_hz;
    uint32_t sample_count;
    uint32_t pin_base;
    uint32_t pin_count;
    uint32_t trigger_pin;
    PslabPicoTrigger trigger;
} PslabPicoDevice;

uint32_t pslab_pico_bits_per_word(uint32_t pin_count);
uint32_t pslab_pico_word_count(uint32_t pin_count, uint32_t sample_count);
bool pslab_pico_unpack_words(
    uint8_t *dst,
    size_t dst_len,
    uint32_t const *src,
    size_t src_words,
    uint32_t pin_count,
    uint32_t sample_count
);

int pslab_pico_open(PslabPicoDevice *dev);
void pslab_pico_close(PslabPicoDevice *dev);
int pslab_pico_probe(PslabPicoDevice *dev, char *idn, size_t idn_len);
int pslab_pico_capture(PslabPicoDevice *dev, uint8_t **samples, size_t *len);

#endif
