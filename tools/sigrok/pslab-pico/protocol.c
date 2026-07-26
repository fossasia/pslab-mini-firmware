#include "protocol.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

uint32_t pslab_pico_bits_per_word(uint32_t pin_count)
{
    if (pin_count == 0 || pin_count > 32) {
        return 0;
    }

    return 32u - (32u % pin_count);
}

uint32_t pslab_pico_word_count(uint32_t pin_count, uint32_t sample_count)
{
    uint32_t bits_per_word = pslab_pico_bits_per_word(pin_count);
    if (bits_per_word == 0) {
        return 0;
    }

    return (sample_count * pin_count + bits_per_word - 1u) / bits_per_word;
}

bool pslab_pico_unpack_words(
    uint8_t *dst,
    size_t dst_len,
    uint32_t const *src,
    size_t src_words,
    uint32_t pin_count,
    uint32_t sample_count
)
{
    uint32_t bits_per_word = pslab_pico_bits_per_word(pin_count);
    uint32_t word_count = pslab_pico_word_count(pin_count, sample_count);

    if (!dst || !src || pin_count == 0 || pin_count > PSLAB_PICO_MAX_CHANNELS ||
        dst_len < sample_count || src_words < word_count) {
        return false;
    }

    size_t sample = 0;
    uint32_t mask = (1u << pin_count) - 1u;

    for (size_t word_index = 0; word_index < word_count && sample < sample_count;
         ++word_index) {
        uint32_t word = src[word_index];

        for (uint32_t shift = 0; shift + pin_count <= bits_per_word &&
                               sample < sample_count;
             shift += pin_count) {
            dst[sample++] = (uint8_t)((word >> shift) & mask);
        }
    }

    return sample == sample_count;
}

static int set_blocking_raw_serial(int fd)
{
    struct termios tio;

    if (tcgetattr(fd, &tio) < 0) {
        return -errno;
    }

    tio.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                     ICRNL | IXON);
    tio.c_oflag &= ~OPOST;
    tio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
    tio.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef CRTSCTS
    tio.c_cflag &= ~CRTSCTS;
#endif
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        return -errno;
    }

    tcflush(fd, TCIOFLUSH);
    return 0;
}

int pslab_pico_open(PslabPicoDevice *dev)
{
    if (!dev || !dev->conn) {
        return -EINVAL;
    }

    dev->fd = open(dev->conn, O_RDWR | O_NOCTTY);
    if (dev->fd < 0) {
        return -errno;
    }
    fcntl(dev->fd, F_SETFD, FD_CLOEXEC);

    int ret = set_blocking_raw_serial(dev->fd);
    if (ret < 0) {
        pslab_pico_close(dev);
        return ret;
    }

    return 0;
}

void pslab_pico_close(PslabPicoDevice *dev)
{
    if (!dev || dev->fd < 0) {
        return;
    }

    close(dev->fd);
    dev->fd = -1;
}

static int64_t monotonic_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int write_all(int fd, void const *buf, size_t len)
{
    uint8_t const *ptr = buf;

    while (len > 0) {
        ssize_t written = write(fd, ptr, len);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -errno;
        }

        ptr += written;
        len -= (size_t)written;
    }

    return 0;
}

static int scpi_write(PslabPicoDevice *dev, char const *fmt, ...)
{
    char line[128];
    va_list ap;

    va_start(ap, fmt);
    int len = vsnprintf(line, sizeof(line) - 2, fmt, ap);
    va_end(ap);

    if (len < 0 || (size_t)len > sizeof(line) - 2) {
        return -EINVAL;
    }

    line[len++] = '\n';
    return write_all(dev->fd, line, (size_t)len);
}

static int read_byte_timeout(int fd, uint8_t *byte, int timeout_ms)
{
    int64_t deadline = monotonic_ms() + timeout_ms;
    struct timespec wait_time = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };

    while (monotonic_ms() < deadline) {
        ssize_t got = read(fd, byte, 1);
        if (got == 1) {
            return 0;
        }
        if (got < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            return -errno;
        }
        nanosleep(&wait_time, NULL);
    }

    return -ETIMEDOUT;
}

static int read_exact_timeout(int fd, uint8_t *buf, size_t len, int timeout_ms)
{
    int64_t deadline = monotonic_ms() + timeout_ms;
    size_t pos = 0;
    struct timespec wait_time = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };

    while (pos < len && monotonic_ms() < deadline) {
        ssize_t got = read(fd, buf + pos, len - pos);
        if (got > 0) {
            pos += (size_t)got;
            continue;
        }
        if (got < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
            return -errno;
        }
        nanosleep(&wait_time, NULL);
    }

    return pos == len ? 0 : -ETIMEDOUT;
}

static int read_line(PslabPicoDevice *dev, char *line, size_t line_len)
{
    size_t pos = 0;

    if (!line || line_len == 0) {
        return -EINVAL;
    }

    while (pos + 1 < line_len) {
        uint8_t byte;
        int ret = read_byte_timeout(dev->fd, &byte, PSLAB_PICO_IO_TIMEOUT_MS);
        if (ret < 0) {
            return ret;
        }
        if (byte == '\n') {
            break;
        }
        if (byte != '\r') {
            line[pos++] = (char)byte;
        }
    }

    line[pos] = '\0';
    return (int)pos;
}

int pslab_pico_probe(PslabPicoDevice *dev, char *idn, size_t idn_len)
{
    int ret = scpi_write(dev, "*IDN?");
    if (ret < 0) {
        return ret;
    }

    ret = read_line(dev, idn, idn_len);
    if (ret < 0) {
        return ret;
    }

    return strstr(idn, "FOSSASIA") && strstr(idn, "PSLab Pico") ? 0 : -ENODEV;
}

static int read_scpi_block(PslabPicoDevice *dev, uint8_t **payload, size_t *len)
{
    uint8_t header;
    int ret;

    do {
        ret = read_byte_timeout(dev->fd, &header, PSLAB_PICO_IO_TIMEOUT_MS);
        if (ret < 0) {
            return ret;
        }
    } while (header == '\r' || header == '\n');

    if (header != '#') {
        return -EPROTO;
    }

    uint8_t digits_ch;
    ret = read_byte_timeout(dev->fd, &digits_ch, PSLAB_PICO_IO_TIMEOUT_MS);
    if (ret < 0) {
        return ret;
    }
    if (!isdigit(digits_ch) || digits_ch == '0') {
        return -EPROTO;
    }

    unsigned digits = (unsigned)(digits_ch - '0');
    char len_text[10];
    if (digits >= sizeof(len_text)) {
        return -EPROTO;
    }

    ret = read_exact_timeout(
        dev->fd,
        (uint8_t *)len_text,
        digits,
        PSLAB_PICO_IO_TIMEOUT_MS
    );
    if (ret < 0) {
        return ret;
    }

    len_text[digits] = '\0';
    char *end = NULL;
    unsigned long payload_len = strtoul(len_text, &end, 10);
    if (!end || *end != '\0' || payload_len == 0 ||
        payload_len > PSLAB_PICO_MAX_BLOCK_BYTES) {
        return -EPROTO;
    }

    uint8_t *buf = malloc(payload_len);
    if (!buf) {
        return -ENOMEM;
    }

    ret = read_exact_timeout(dev->fd, buf, payload_len, PSLAB_PICO_IO_TIMEOUT_MS);
    if (ret < 0) {
        free(buf);
        return ret;
    }

    *payload = buf;
    *len = payload_len;
    return 0;
}

static uint32_t le32_load(uint8_t const *bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint32_t divider_for(PslabPicoDevice const *dev)
{
    uint32_t samplerate = dev->samplerate_hz ? dev->samplerate_hz
                                             : PSLAB_PICO_DEFAULT_SAMPLERATE;
    uint32_t sysclk = dev->sysclk_hz ? dev->sysclk_hz
                                     : PSLAB_PICO_DEFAULT_SYSCLK_HZ;
    uint32_t divider = sysclk / samplerate;

    return divider == 0 ? 1 : divider;
}

static int configure_trigger(PslabPicoDevice *dev)
{
    int ret;

    switch (dev->trigger) {
    case PSLAB_PICO_TRIGGER_HIGH:
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:LEVel 1");
        if (ret < 0) {
            return ret;
        }
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:PIN %u", dev->trigger_pin);
        return ret < 0 ? ret : scpi_write(dev, "LA:CONFigure:TRIGger:MODE LEVEL");
    case PSLAB_PICO_TRIGGER_LOW:
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:LEVel 0");
        if (ret < 0) {
            return ret;
        }
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:PIN %u", dev->trigger_pin);
        return ret < 0 ? ret : scpi_write(dev, "LA:CONFigure:TRIGger:MODE LEVEL");
    case PSLAB_PICO_TRIGGER_RISING:
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:LEVel 1");
        if (ret < 0) {
            return ret;
        }
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:PIN %u", dev->trigger_pin);
        return ret < 0 ? ret : scpi_write(dev, "LA:CONFigure:TRIGger:MODE EDGE");
    case PSLAB_PICO_TRIGGER_FALLING:
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:LEVel 0");
        if (ret < 0) {
            return ret;
        }
        ret = scpi_write(dev, "LA:CONFigure:TRIGger:PIN %u", dev->trigger_pin);
        return ret < 0 ? ret : scpi_write(dev, "LA:CONFigure:TRIGger:MODE EDGE");
    case PSLAB_PICO_TRIGGER_AUTO:
    default:
        return scpi_write(dev, "LA:CONFigure:TRIGger:MODE AUTO");
    }
}

int pslab_pico_capture(PslabPicoDevice *dev, uint8_t **samples, size_t *len)
{
    uint8_t *raw = NULL;
    size_t raw_len = 0;
    int ret;

    if (!dev || !samples || !len || dev->pin_count == 0 ||
        dev->pin_count > PSLAB_PICO_MAX_CHANNELS ||
        dev->sample_count == 0 || dev->sample_count > PSLAB_PICO_MAX_SAMPLES) {
        return -EINVAL;
    }

    ret = scpi_write(dev, "LA:CONFigure:PINBase %u", dev->pin_base);
    if (ret < 0) {
        return ret;
    }
    ret = scpi_write(dev, "LA:CONFigure:PINCount %u", dev->pin_count);
    if (ret < 0) {
        return ret;
    }
    ret = scpi_write(dev, "LA:CONFigure:DIVider %u", divider_for(dev));
    if (ret < 0) {
        return ret;
    }
    ret = scpi_write(dev, "LA:CONFigure:SAMPles %u", dev->sample_count);
    if (ret < 0) {
        return ret;
    }
    ret = configure_trigger(dev);
    if (ret < 0) {
        return ret;
    }

    ret = scpi_write(dev, "LA:READ?");
    if (ret < 0) {
        return ret;
    }
    ret = read_scpi_block(dev, &raw, &raw_len);
    if (ret < 0) {
        return ret;
    }

    size_t word_count = pslab_pico_word_count(dev->pin_count, dev->sample_count);
    if (raw_len != word_count * sizeof(uint32_t)) {
        free(raw);
        return -EPROTO;
    }

    uint32_t *words = calloc(word_count, sizeof(*words));
    uint8_t *unpacked = malloc(dev->sample_count);
    if (!words || !unpacked) {
        free(raw);
        free(words);
        free(unpacked);
        return -ENOMEM;
    }

    for (size_t i = 0; i < word_count; ++i) {
        words[i] = le32_load(&raw[i * sizeof(uint32_t)]);
    }

    bool ok = pslab_pico_unpack_words(
        unpacked,
        dev->sample_count,
        words,
        word_count,
        dev->pin_count,
        dev->sample_count
    );

    free(raw);
    free(words);
    if (!ok) {
        free(unpacked);
        return -EPROTO;
    }

    *samples = unpacked;
    *len = dev->sample_count;
    return 0;
}
