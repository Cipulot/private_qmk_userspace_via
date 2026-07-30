/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>

#include "mx.h"

eeprom_mx_config_t eeprom_mx_config;
socd_cleaner_t     socd_opposing_pairs[SOCD_PAIR_COUNT];

static void save_socd_config(void) {
    eeconfig_update_kb_datablock_field(eeprom_mx_config, eeprom_socd_opposing_pairs);
}

void eeconfig_init_kb(void) {
    const struct {
        uint16_t key1;
        uint16_t key2;
    } socd_pairs[] = {
        {KC_A, KC_D},
        {KC_W, KC_S},
        {KC_Z, KC_X},
        {KC_LEFT, KC_RIGHT},
    };

    _Static_assert(ARRAY_SIZE(socd_pairs) == SOCD_PAIR_COUNT, "Logos MX must define defaults for every SOCD pair");

    for (uint8_t i = 0; i < SOCD_PAIR_COUNT; i++) {
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].keys[0]    = socd_pairs[i].key1;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].keys[1]    = socd_pairs[i].key2;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].resolution = SOCD_CLEANER_OFF;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].held[0]    = false;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].held[1]    = false;
    }

    eeconfig_update_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);
    eeconfig_init_user();
}

void keyboard_post_init_kb(void) {
    eeconfig_read_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);
    memcpy(socd_opposing_pairs, eeprom_mx_config.eeprom_socd_opposing_pairs, sizeof(socd_opposing_pairs));

    const socd_config_context_t socd_context = {
        .runtime_pairs = socd_opposing_pairs,
        .stored_pairs  = eeprom_mx_config.eeprom_socd_opposing_pairs,
        .pair_count    = SOCD_PAIR_COUNT,
        .save          = save_socd_config,
    };
    socd_config_init(&socd_context);

    keyboard_post_init_user();
}
