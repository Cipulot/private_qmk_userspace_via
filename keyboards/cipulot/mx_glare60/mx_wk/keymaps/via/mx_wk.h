/* Copyright 2026 Cipulot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
} eeprom_mx_wk_config_t;

// Compile-time check for EECONFIG_KB_DATA_SIZE
_Static_assert(sizeof(eeprom_mx_wk_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

// Extern declarations
extern eeprom_mx_wk_config_t eeprom_mx_wk_config;
// Runtime SOCD cleaner pairs
// For now it can't be part of runtime_ec_config_t due to how the submodule checks for the existance of the structure
extern socd_cleaner_t socd_opposing_pairs[4];

extern uint8_t   *pIndicators;
indicator_config *get_indicator_p(int index);
bool              indicators_callback(void);
