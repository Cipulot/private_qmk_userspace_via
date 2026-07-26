/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include QMK_KEYBOARD_H
#include "keyboards/cipulot/ec_keychain/ec_switch_matrix.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    [0] = LAYOUT_ortho_1x1(
        KC_ENT
    )
    // clang-format on
};
