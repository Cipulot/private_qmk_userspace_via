/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quantum.h"
#include "util.h"
#include "socd_cleaner.h"

// EEPROM configuration structure definitions
typedef struct PACKED {
    socd_cleaner_t   eeprom_socd_opposing_pairs[4]; // SOCD cleaner pairs
} eeprom_mx_config_t;

// Compile-time check for EECONFIG_KB_DATA_SIZE
_Static_assert(sizeof(eeprom_mx_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

// Extern declarations
extern eeprom_mx_config_t eeprom_mx_config;
// Runtime SOCD cleaner pairs
// For now it can't be part of runtime_ec_config_t due to how the submodule checks for the existance of the structure
extern socd_cleaner_t socd_opposing_pairs[4];
