/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quantum.h"
#include "util.h"
#include "socd_cleaner.h"

// Indicator configuration structure definitions
typedef struct PACKED {
    uint8_t h;
    uint8_t s;
    uint8_t v;
    uint8_t func;
    uint8_t index;
    bool    enabled;
} indicator_config;

// EEPROM configuration structure definitions
typedef struct PACKED {
    indicator_config ind1;
    indicator_config ind2;
    socd_cleaner_t   eeprom_socd_opposing_pairs[4]; // SOCD cleaner pairs
} eeprom_mx_hhkb_config_t;

// Compile-time check for EECONFIG_KB_DATA_SIZE
_Static_assert(sizeof(eeprom_mx_hhkb_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

// Extern declarations
extern eeprom_mx_hhkb_config_t eeprom_mx_hhkb_config;
// Runtime SOCD cleaner pairs
// For now it can't be part of runtime_ec_config_t due to how the submodule checks for the existance of the structure
extern socd_cleaner_t socd_opposing_pairs[4];

extern uint8_t   *pIndicators;
indicator_config *get_indicator_p(int index);
bool              indicators_callback(void);
