/*
 * Out-of-tree libsigrok hardware driver for PSLab Pico.
 *
 * Drop this directory into libsigrok/src/hardware/pslab-pico and register the
 * driver from libsigrok's hardware driver list. The protocol implementation is
 * intentionally POSIX-serial based so it remains usable as an external driver
 * package while the firmware still exposes only USB CDC.
 */

#include <config.h>
#include <errno.h>
#include <glib.h>
#include <libsigrok/libsigrok.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_PREFIX "pslab-pico"

#include "libsigrok-internal.h"
#include "protocol.h"

static const uint32_t scanopts[] = {
    SR_CONF_CONN,
    SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
    SR_CONF_LOGIC_ANALYZER,
};

static const uint32_t devopts[] = {
    SR_CONF_CONN | SR_CONF_GET,
    SR_CONF_SERIALCOMM | SR_CONF_GET,
    SR_CONF_LIMIT_SAMPLES | SR_CONF_GET | SR_CONF_SET,
    SR_CONF_SAMPLERATE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
    SR_CONF_TRIGGER_MATCH | SR_CONF_LIST,
};

static const uint64_t samplerates[] = {
    SR_KHZ(1),
    SR_KHZ(2),
    SR_KHZ(5),
    SR_KHZ(10),
    SR_KHZ(20),
    SR_KHZ(50),
    SR_KHZ(100),
    SR_KHZ(200),
    SR_KHZ(500),
    SR_MHZ(1),
    SR_MHZ(2),
    SR_MHZ(5),
    SR_MHZ(10),
    SR_MHZ(15),
    SR_MHZ(25),
    SR_MHZ(30),
    SR_MHZ(50),
    SR_MHZ(75),
    SR_MHZ(150),
};

static const int32_t trigger_matches[] = {
    SR_TRIGGER_ZERO,
    SR_TRIGGER_ONE,
    SR_TRIGGER_RISING,
    SR_TRIGGER_FALLING,
};

static char const *const channel_names[] = {
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
};

static PslabPicoDevice *dev_context_new(char const *conn)
{
    PslabPicoDevice *devc = g_malloc0(sizeof(*devc));

    devc->fd = -1;
    devc->conn = g_strdup(conn);
    devc->sysclk_hz = PSLAB_PICO_DEFAULT_SYSCLK_HZ;
    devc->samplerate_hz = PSLAB_PICO_DEFAULT_SAMPLERATE;
    devc->sample_count = PSLAB_PICO_DEFAULT_SAMPLES;
    devc->pin_base = PSLAB_PICO_DEFAULT_PIN_BASE;
    devc->pin_count = PSLAB_PICO_DEFAULT_PIN_COUNT;
    devc->trigger_pin = PSLAB_PICO_DEFAULT_PIN_BASE;
    devc->trigger = PSLAB_PICO_TRIGGER_AUTO;
    return devc;
}

static void dev_context_clear(PslabPicoDevice *devc)
{
    if (!devc) {
        return;
    }

    pslab_pico_close(devc);
    g_free(devc->conn);
    devc->conn = NULL;
}

static struct sr_dev_inst *device_new(char const *conn, char const *idn)
{
    struct sr_dev_inst *sdi;
    struct sr_channel_group *cg;

    sdi = g_malloc0(sizeof(*sdi));
    sdi->status = SR_ST_INACTIVE;
    sdi->vendor = g_strdup("FOSSASIA");
    sdi->model = g_strdup("PSLab Pico");
    sdi->version = g_strdup(idn);
    sdi->serial_num = NULL;
    sdi->connection_id = g_strdup(conn);
    sdi->priv = dev_context_new(conn);

    cg = sr_channel_group_new(sdi, "Logic", NULL);
    for (size_t i = 0; i < G_N_ELEMENTS(channel_names); ++i) {
        struct sr_channel *ch =
            sr_channel_new(sdi, (int)i, SR_CHANNEL_LOGIC, i < 2, channel_names[i]);
        cg->channels = g_slist_append(cg->channels, ch);
    }

    return sdi;
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
    GSList *devices = NULL;
    char const *conn = NULL;
    char const *serialcomm = NULL;
    char conn_fallback[32];

    (void)di;
    for (GSList *l = options; l; l = l->next) {
        struct sr_config *src = l->data;
        if (src->key == SR_CONF_CONN) {
            conn = g_variant_get_string(src->data, NULL);
        } else if (src->key == SR_CONF_SERIALCOMM) {
            serialcomm = g_variant_get_string(src->data, NULL);
        }
    }

    if (!conn) {
        sr_info("pslab-pico needs conn=/dev/ttyACM* for scanning.");
        return NULL;
    }
    if (!strchr(conn, '/') && g_ascii_isdigit(conn[0])) {
        snprintf(conn_fallback, sizeof(conn_fallback), "/dev/ttyACM%s", conn);
        conn = conn_fallback;
    }
    (void)serialcomm;

    PslabPicoDevice probe = {
        .fd = -1,
        .conn = (char *)conn,
    };
    char idn[128];

    if (pslab_pico_open(&probe) < 0) {
        sr_warn("Failed to open %s.", conn);
        return NULL;
    }

    int ret = pslab_pico_probe(&probe, idn, sizeof(idn));
    pslab_pico_close(&probe);
    if (ret < 0) {
        sr_warn("%s is not a PSLab Pico SCPI device.", conn);
        return NULL;
    }

    devices = g_slist_append(devices, device_new(conn, idn));
    return std_scan_complete(di, devices);
}

static int dev_clear(const struct sr_dev_driver *di)
{
    return std_dev_clear_with_callback(
        di,
        (std_dev_clear_callback)dev_context_clear
    );
}

static int dev_open(struct sr_dev_inst *sdi)
{
    PslabPicoDevice *devc;

    if (!sdi || !(devc = sdi->priv)) {
        return SR_ERR_ARG;
    }

    if (pslab_pico_open(devc) < 0) {
        return SR_ERR;
    }

    return SR_OK;
}

static int dev_close(struct sr_dev_inst *sdi)
{
    if (!sdi) {
        return SR_ERR_ARG;
    }

    pslab_pico_close(sdi->priv);
    return SR_OK;
}

static uint32_t enabled_channel_count(struct sr_dev_inst const *sdi)
{
    uint32_t count = 0;

    for (GSList const *l = sdi->channels; l; l = l->next) {
        struct sr_channel const *ch = l->data;
        if (ch->enabled && ch->type == SR_CHANNEL_LOGIC) {
            count = (uint32_t)ch->index + 1u;
        }
    }

    if (count == 0) {
        count = 1;
    }
    if (count > PSLAB_PICO_MAX_CHANNELS) {
        count = PSLAB_PICO_MAX_CHANNELS;
    }
    return count;
}

static void apply_session_trigger(struct sr_dev_inst const *sdi)
{
    PslabPicoDevice *devc = sdi->priv;
    struct sr_trigger *trigger = sr_session_trigger_get(sdi->session);

    devc->trigger = PSLAB_PICO_TRIGGER_AUTO;
    devc->trigger_pin = devc->pin_base;

    if (!trigger || !trigger->stages) {
        return;
    }

    struct sr_trigger_stage *stage = trigger->stages->data;
    if (!stage || !stage->matches) {
        return;
    }

    struct sr_trigger_match *match = stage->matches->data;
    if (!match || !match->channel ||
        match->channel->type != SR_CHANNEL_LOGIC) {
        return;
    }

    devc->trigger_pin = devc->pin_base + (uint32_t)match->channel->index;

    switch (match->match) {
    case SR_TRIGGER_ZERO:
        devc->trigger = PSLAB_PICO_TRIGGER_LOW;
        break;
    case SR_TRIGGER_ONE:
        devc->trigger = PSLAB_PICO_TRIGGER_HIGH;
        break;
    case SR_TRIGGER_RISING:
        devc->trigger = PSLAB_PICO_TRIGGER_RISING;
        break;
    case SR_TRIGGER_FALLING:
        devc->trigger = PSLAB_PICO_TRIGGER_FALLING;
        break;
    default:
        devc->trigger = PSLAB_PICO_TRIGGER_AUTO;
        break;
    }
}

static int config_get(
    uint32_t key,
    GVariant **data,
    const struct sr_dev_inst *sdi,
    const struct sr_channel_group *cg
)
{
    PslabPicoDevice *devc;

    (void)cg;
    if (!sdi || !(devc = sdi->priv)) {
        return SR_ERR_ARG;
    }

    switch (key) {
    case SR_CONF_CONN:
        *data = g_variant_new_string(devc->conn);
        break;
    case SR_CONF_SERIALCOMM:
        *data = g_variant_new_string("115200/8n1");
        break;
    case SR_CONF_LIMIT_SAMPLES:
        *data = g_variant_new_uint64(devc->sample_count);
        break;
    case SR_CONF_SAMPLERATE:
        *data = g_variant_new_uint64(devc->samplerate_hz);
        break;
    default:
        return SR_ERR_NA;
    }

    return SR_OK;
}

static int config_set(
    uint32_t key,
    GVariant *data,
    const struct sr_dev_inst *sdi,
    const struct sr_channel_group *cg
)
{
    PslabPicoDevice *devc;

    (void)cg;
    if (!sdi || !(devc = sdi->priv)) {
        return SR_ERR_ARG;
    }

    switch (key) {
    case SR_CONF_LIMIT_SAMPLES: {
        uint64_t sample_count = g_variant_get_uint64(data);
        if (sample_count == 0 || sample_count > PSLAB_PICO_MAX_SAMPLES) {
            return SR_ERR_ARG;
        }
        devc->sample_count = (uint32_t)sample_count;
        break;
    }
    case SR_CONF_SAMPLERATE:
        devc->samplerate_hz = (uint32_t)g_variant_get_uint64(data);
        if (devc->samplerate_hz == 0 ||
            devc->samplerate_hz > devc->sysclk_hz) {
            return SR_ERR_ARG;
        }
        break;
    default:
        return SR_ERR_NA;
    }

    return SR_OK;
}

static int config_list(
    uint32_t key,
    GVariant **data,
    const struct sr_dev_inst *sdi,
    const struct sr_channel_group *cg
)
{
    (void)sdi;

    if (cg) {
        return SR_ERR_NA;
    }

    switch (key) {
    case SR_CONF_SCAN_OPTIONS:
    case SR_CONF_DEVICE_OPTIONS:
        return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
    case SR_CONF_SAMPLERATE:
        *data = std_gvar_samplerates(samplerates, G_N_ELEMENTS(samplerates));
        break;
    case SR_CONF_LIMIT_SAMPLES:
        *data = std_gvar_tuple_u64(1, PSLAB_PICO_MAX_SAMPLES);
        break;
    case SR_CONF_TRIGGER_MATCH:
        *data = std_gvar_array_i32(
            trigger_matches,
            G_N_ELEMENTS(trigger_matches)
        );
        break;
    default:
        return SR_ERR_NA;
    }

    return SR_OK;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
    PslabPicoDevice *devc;
    uint8_t *samples = NULL;
    size_t len = 0;

    if (!sdi || !(devc = sdi->priv)) {
        return SR_ERR_ARG;
    }

    devc->pin_count = enabled_channel_count(sdi);
    apply_session_trigger(sdi);

    std_session_send_df_header(sdi);
    std_session_send_df_frame_begin(sdi);

    int ret = pslab_pico_capture(devc, &samples, &len);
    if (ret < 0) {
        sr_err("PSLab Pico capture failed: %s.", g_strerror(-ret));
        std_session_send_df_frame_end(sdi);
        std_session_send_df_end(sdi);
        return SR_ERR;
    }

    struct sr_datafeed_logic logic = {
        .length = len,
        .unitsize = 1,
        .data = samples,
    };
    struct sr_datafeed_packet packet = {
        .type = SR_DF_LOGIC,
        .payload = &logic,
    };

    sr_session_send(sdi, &packet);
    std_session_send_df_frame_end(sdi);
    std_session_send_df_end(sdi);
    free(samples);
    return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
    (void)sdi;
    return SR_OK;
}

static struct sr_dev_driver pslab_pico_driver_info = {
    .name = "pslab-pico",
    .longname = "PSLab Pico SCPI logic analyser",
    .api_version = 1,
    .init = std_init,
    .cleanup = std_cleanup,
    .scan = scan,
    .dev_list = std_dev_list,
    .dev_clear = dev_clear,
    .config_get = config_get,
    .config_set = config_set,
    .config_list = config_list,
    .dev_open = dev_open,
    .dev_close = dev_close,
    .dev_acquisition_start = dev_acquisition_start,
    .dev_acquisition_stop = dev_acquisition_stop,
    .context = NULL,
};

SR_REGISTER_DEV_DRIVER(pslab_pico_driver_info);
