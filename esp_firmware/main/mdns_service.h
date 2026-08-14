#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Publishes the PSLab Pico bridge as pslab-pico.local and exposes its
 * TCP SCPI endpoint through the _pslab._tcp service.
 *
 * Call this after the station interface has received an IP address.
 */
bool mdns_service_start(uint16_t scpi_port);

#endif
