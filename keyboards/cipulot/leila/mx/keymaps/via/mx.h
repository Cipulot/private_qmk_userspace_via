/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <stddef.h>

#include "quantum.h"
#include "keyboards/cipulot/common/extensions/indicators/rgb_indicator_config.h"
#include "keyboards/cipulot/common/shared/config/socd_config.h"
#include "keyboards/cipulot/common/shared/extension.h"
#include "util.h"

#define LEILA_MX_INDICATOR_COUNT 3

// EEPROM configuration structure definitions
typedef struct PACKED {
    cipulot_rgb_indicator_config_t indicators[LEILA_MX_INDICATOR_COUNT];
    socd_cleaner_t                 eeprom_socd_opposing_pairs[SOCD_PAIR_COUNT];
} eeprom_mx_config_t;

// Compile-time checks for the released EEPROM layout
_Static_assert(sizeof(eeprom_mx_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");
_Static_assert(offsetof(eeprom_mx_config_t, indicators) == 0, "Leila MX indicator EEPROM offset must remain stable");
_Static_assert(offsetof(eeprom_mx_config_t, eeprom_socd_opposing_pairs) == 18, "Leila MX SOCD EEPROM offset must remain stable");

extern eeprom_mx_config_t eeprom_mx_config;

bool leila_mx_indicator_extension_apply(void);
