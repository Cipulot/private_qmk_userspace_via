/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include QMK_KEYBOARD_H
#include "keyboards/cipulot/common/special/ec/global/ec_global_switch_matrix.h"
#include "keyboards/cipulot/common/general/ec/runtime/ec_runtime_features_keymap.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    [0] = LAYOUT(
        KC_ESC,  _______,  _______,  MO(1),
        KC_Z,    KC_X,     KC_C,     KC_SPACE),

    [1] = LAYOUT(
        KC_VOLD,  KC_VOLU,  KC_MUTE,  _______,
        _______,  _______,  _______,  MO(2)),

    [2] = LAYOUT(
        QK_BOOT,  NK_TOGG,  SE_TOGG,  _______,
        _______,  _______,  _______,  _______)
    // clang-format on
};
