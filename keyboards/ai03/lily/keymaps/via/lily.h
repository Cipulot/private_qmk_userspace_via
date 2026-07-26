/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quantum.h"
#include "util.h"

typedef struct _indicator_config_t {
    uint8_t h;
    uint8_t s;
    uint8_t v;
    uint8_t func;
    uint8_t index;
    bool    enabled;
} indicator_config;

typedef struct PACKED {
    indicator_config ind1;
    indicator_config ind2;
    indicator_config ind3;
} eeprom_lily_config_t;

// Check if the size of the reserved persistent memory is the same as the size of struct eeprom_ec_config_t
_Static_assert(sizeof(eeprom_lily_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

extern eeprom_lily_config_t eeprom_lily_config;

bool              indicators_callback(void);
uint8_t          *pIndicators;
indicator_config *get_indicator_p(int index);
