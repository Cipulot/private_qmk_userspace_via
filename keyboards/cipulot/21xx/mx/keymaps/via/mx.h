/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "quantum.h"
#include "util.h"

static const pin_t indicator_pins[7] = {
    B7, // F_ROW_LED_PIN
    C0, // MACRO_LED_R1_PIN
    F1, // MACRO_LED_R2_PIN
    C3, // MACRO_LED_R3_PIN
    C2, // MACRO_LED_R4_PIN
    C1, // MACRO_LED_R5_PIN
    A2  // CAPS_LOCK_LED_PIN
};

bool keypress;

typedef struct _indicator_config_t {
    uint8_t func;
    uint8_t index;
    bool    enabled;
} indicator_config;

typedef struct PACKED {
    indicator_config ind1; // F-row LED
    indicator_config ind2; // Macro Led R1
    indicator_config ind3; // Macro Led R2
    indicator_config ind4; // Macro Led R3
    indicator_config ind5; // Macro Led R4
    indicator_config ind6; // Macro Led R5
    indicator_config ind7; // Caps Lock Led
} eeprom_mx_config_t;

// Check if the size of the reserved persistent memory is the same as the size of struct eeprom_ec_config_t
_Static_assert(sizeof(eeprom_mx_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

extern eeprom_mx_config_t eeprom_mx_config;

void              indicators_init(void);
bool              indicators_callback(void);
extern uint8_t *pIndicators;
indicator_config *get_indicator_p(int index);
