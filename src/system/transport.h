#ifndef PSLAB_TRANSPORT_H
#define PSLAB_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRANSPORT_MODE_USB = 0,
    TRANSPORT_MODE_WIFI = 1,
    TRANSPORT_MODE_AUTO = 2,
} TransportMode;

typedef enum {
    TRANSPORT_INSTRUMENT_LA = 1,
    TRANSPORT_INSTRUMENT_DSO = 2,
} TransportInstrument;

typedef enum {
    TRANSPORT_DATA_FORMAT_DEFAULT = 0,
    TRANSPORT_DATA_FORMAT_LA_U32_PACKED = 1,
    TRANSPORT_DATA_FORMAT_DSO_U16_LE = 2,
} TransportDataFormat;

typedef struct {
    uint32_t sample_rate_hz;
    uint32_t sample_count;
    uint32_t channel_count;
    uint32_t pin_base_or_channel;
    uint32_t trigger_mode;
    uint32_t data_format;
} TransportCaptureMeta;

typedef void (*TransportYieldCallback)(void);

void transport_init(void);
void transport_set_yield_callback(TransportYieldCallback callback);
void transport_set_mode(TransportMode mode);
TransportMode transport_get_mode(void);
char const *transport_get_mode_name(void);
bool transport_wifi_is_effective(void);
bool transport_send_capture(
    TransportInstrument instrument,
    uint32_t capture_sequence,
    TransportCaptureMeta const *meta,
    uint8_t const *data,
    size_t len
);
uint32_t transport_get_sent_frames(void);
uint32_t transport_get_dropped_frames(void);
uint32_t transport_get_timeouts(void);

#ifdef __cplusplus
}
#endif

#endif
