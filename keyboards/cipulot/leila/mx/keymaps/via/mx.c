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

static void set_indicator_defaults(void *storage, uint16_t size) {
    if (storage == NULL || size != sizeof(eeprom_mx_config.indicators)) {
        return;
    }

    cipulot_rgb_indicator_config_t *indicators = storage;
    indicators[0]                              = (cipulot_rgb_indicator_config_t){.h = 0, .s = 255, .v = 150, .func = 0x04, .index = 0, .enabled = true};
    indicators[1]                              = (cipulot_rgb_indicator_config_t){.h = 86, .s = 255, .v = 150, .func = 0x04, .index = 1, .enabled = true};
    indicators[2]                              = (cipulot_rgb_indicator_config_t){.h = 166, .s = 254, .v = 150, .func = 0x04, .index = 2, .enabled = true};
}

static bool validate_indicators(const void *storage, uint16_t size) {
    return cipulot_rgb_indicator_validate(storage, size, LEILA_MX_INDICATOR_COUNT);
}

static bool apply_indicators(const void *storage, uint16_t size) {
    if (storage == NULL || size != sizeof(eeprom_mx_config.indicators)) {
        return false;
    }

    rgblight_set_effect_range(3, 1);
    return cipulot_rgb_indicator_apply_rgblight(storage, LEILA_MX_INDICATOR_COUNT);
}

static const cipulot_extension_t leila_mx_indicator_extension = {
    .set_defaults = set_indicator_defaults,
    .validate     = validate_indicators,
    .apply        = apply_indicators,
    .split_config = {0},
};

bool leila_mx_indicator_extension_apply(void) {
    return cipulot_extension_apply(&leila_mx_indicator_extension, eeprom_mx_config.indicators, sizeof(eeprom_mx_config.indicators));
}

// EEPROM default initialization
void eeconfig_init_kb(void) {
    cipulot_extension_set_defaults(&leila_mx_indicator_extension, eeprom_mx_config.indicators, sizeof(eeprom_mx_config.indicators));

    // Initialize the SOCD cleaner pairs
    const struct {
        uint16_t key1;
        uint16_t key2;
    } socd_pairs[] = {
        {KC_A, KC_D},
        {KC_W, KC_S},
        {KC_Z, KC_X},
        {KC_LEFT, KC_RIGHT},
    };

    _Static_assert(ARRAY_SIZE(socd_pairs) == SOCD_PAIR_COUNT, "Leila MX must define defaults for every SOCD pair");

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

// Keyboard post-initialization
void keyboard_post_init_kb(void) {
    eeconfig_read_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);

    if (!cipulot_extension_validate(&leila_mx_indicator_extension, eeprom_mx_config.indicators, sizeof(eeprom_mx_config.indicators))) {
        cipulot_extension_set_defaults(&leila_mx_indicator_extension, eeprom_mx_config.indicators, sizeof(eeprom_mx_config.indicators));
        eeconfig_update_kb_datablock_field(eeprom_mx_config, indicators);
    }

    memcpy(socd_opposing_pairs, eeprom_mx_config.eeprom_socd_opposing_pairs, sizeof(socd_opposing_pairs));

    const socd_config_context_t socd_context = {
        .runtime_pairs = socd_opposing_pairs,
        .stored_pairs  = eeprom_mx_config.eeprom_socd_opposing_pairs,
        .pair_count    = SOCD_PAIR_COUNT,
        .save          = save_socd_config,
    };
    socd_config_init(&socd_context);

    leila_mx_indicator_extension_apply();
    keyboard_post_init_user();
}

bool led_update_kb(led_t led_state) {
    leila_mx_indicator_extension_apply();
    return true;
}

__attribute__((weak)) layer_state_t layer_state_set_user(layer_state_t state) {
    leila_mx_indicator_extension_apply();
    return state;
}
