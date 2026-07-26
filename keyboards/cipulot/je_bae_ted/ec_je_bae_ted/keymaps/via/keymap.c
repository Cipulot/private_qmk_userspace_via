/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include QMK_KEYBOARD_H
#include "keyboards/cipulot/common/special/ec/global/ec_global_switch_matrix.h"
#include "keyboards/cipulot/common/general/ec/runtime/ec_runtime_features_keymap.h"
#include "socd_cleaner.h"

// The shared EC value layer links the SOCD service, while this reduced board
// intentionally publishes no SOCD controls. Keep one disabled module slot so
// QMK's module introspection remains well-formed.
socd_cleaner_t socd_opposing_pairs[1];

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    [0] = LAYOUT(KC_BSLS,  KC_ENTER)
    // clang-format on
};
