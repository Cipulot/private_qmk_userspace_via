/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quantum.h"
#include "keyboards/cipulot/common/shared/config/socd_config.h"
#include "util.h"

typedef struct PACKED {
    socd_cleaner_t eeprom_socd_opposing_pairs[SOCD_PAIR_COUNT]; // SOCD cleaner pairs
} eeprom_mx_config_t;

_Static_assert(sizeof(eeprom_mx_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

extern eeprom_mx_config_t eeprom_mx_config;
