/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include QMK_KEYBOARD_H
#include "keyboards/cipulot/common/general/ec/runtime/ec_runtime_features_keymap.h"
#include "keyboards/cipulot/common/general/ec/ec_switch_matrix.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // clang-format off
    [0] = LAYOUT_all(
        KC_ESC, KC_TAB,  KC_BSPC, MO(1),
        KC_NUM, KC_PSLS, KC_PAST, KC_PEQL,
        KC_P7,  KC_P8,   KC_P9,   KC_PMNS,
        KC_P4,  KC_P5,   KC_P6,   KC_PPLS,
        KC_P1,  KC_P2,   KC_P3,   KC_PENT,
        KC_P0,  KC_P0,   KC_PDOT, KC_PENT),

    [1] = LAYOUT_all(
        UG_TOGG, UG_VALD, UG_VALU, _______,
        _______, _______, _______, NK_TOGG,
        _______, _______, _______, _______,
        _______, _______, _______, _______,
        _______, _______, _______, _______,
        _______, _______, QK_BOOT, _______)
    // clang-format on
};
