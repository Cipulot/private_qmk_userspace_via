/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#define CIPULOT_EECONFIG_KB_DATA_BASE_SIZE 20
#define VIA_FIRMWARE_VERSION 4
#define FEATURE_CONFIG_VERSION 4

// Expand the STM32 wear-levelled logical EEPROM while retaining all four VIA layers.
#define WEAR_LEVELING_LOGICAL_SIZE 2048
#define WEAR_LEVELING_BACKING_SIZE 4096

#include "keyboards/cipulot/common/general/mx/runtime/mx_runtime_features_config.h"
