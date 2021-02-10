#pragma once

#include <stdint.h>
#include "config.h"

#define WOOT_PKT_MAGIC 83

typedef struct {
    uint8_t magic;
    uint16_t seq;
} WootPktHeader;

typedef struct {
    WootPktHeader header;
    int16_t samples[NETBUFF_SAMPLES];
} WootPkt;