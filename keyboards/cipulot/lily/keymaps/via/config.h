/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#define CIPULOT_EECONFIG_KB_DATA_BASE_SIZE 18
#define CIPULOT_EECONFIG_KB_DATA_RUNTIME_BASE_SIZE 38
#define VIA_FIRMWARE_VERSION 2
#define FEATURE_CONFIG_VERSION 2

// Lily retains four VIA layers by storing eight rather than sixteen Tap Dance slots.
#define FEATURE_TAP_DANCE_COUNT 8
#define FEATURE_CONFIG_EEPROM_SIZE 248

#include "keyboards/cipulot/common/general/mx/runtime/mx_runtime_features_config.h"

// Preserve indicator and static underglow control while recovering AVR flash for the shared runtime editors.
#undef RGBLIGHT_EFFECT_ALTERNATING
#undef RGBLIGHT_EFFECT_BREATHING
#undef RGBLIGHT_EFFECT_CHRISTMAS
#undef RGBLIGHT_EFFECT_KNIGHT
#undef RGBLIGHT_EFFECT_RAINBOW_MOOD
#undef RGBLIGHT_EFFECT_RAINBOW_SWIRL
#undef RGBLIGHT_EFFECT_RGB_TEST
#undef RGBLIGHT_EFFECT_SNAKE
#undef RGBLIGHT_EFFECT_STATIC_GRADIENT
#undef RGBLIGHT_EFFECT_TWINKLE
