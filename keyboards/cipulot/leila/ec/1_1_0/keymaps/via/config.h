/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#define VIA_EEPROM_LAYOUT_OPTIONS_SIZE 2
#define VIA_FIRMWARE_VERSION 1
#define FEATURE_CONFIG_VERSION 1

#include "keyboards/cipulot/common/general/ec/runtime/ec_runtime_features_config.h"

// Keep the released indicator, calibration, SOCD, and runtime-feature offsets.
#define CIPULOT_EC_SOLENOID_ENABLE
#define CIPULOT_EC_RGB_BANK_MODE_ENABLE
#undef EECONFIG_KB_DATA_SIZE
#define EECONFIG_KB_DATA_SIZE (CIPULOT_EECONFIG_KB_DATA_BASE_SIZE + FEATURE_CONFIG_EEPROM_SIZE + 18)
