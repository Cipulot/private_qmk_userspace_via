/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "quantum.h"
#include "mx.h"

extern haptic_config_t haptic_config;
eeprom_mx_config_t eeprom_mx_config;

void eeconfig_init_kb(void) {
    // Default values
    eeprom_mx_config.ind1.func    = 0x06;
    eeprom_mx_config.ind1.index   = 0;
    eeprom_mx_config.ind1.enabled = true;

    eeprom_mx_config.ind2.func    = 0x06;
    eeprom_mx_config.ind2.index   = 1;
    eeprom_mx_config.ind2.enabled = true;

    eeprom_mx_config.ind3.func    = 0x06;
    eeprom_mx_config.ind3.index   = 2;
    eeprom_mx_config.ind3.enabled = true;

    eeprom_mx_config.ind4.func    = 0x06;
    eeprom_mx_config.ind4.index   = 3;
    eeprom_mx_config.ind4.enabled = true;

    eeprom_mx_config.ind5.func    = 0x06;
    eeprom_mx_config.ind5.index   = 4;
    eeprom_mx_config.ind5.enabled = true;

    eeprom_mx_config.ind6.func    = 0x06;
    eeprom_mx_config.ind6.index   = 5;
    eeprom_mx_config.ind6.enabled = true;

    eeprom_mx_config.ind7.func    = 0x06;
    eeprom_mx_config.ind7.index   = 6;
    eeprom_mx_config.ind7.enabled = true;

    // Write default value to EEPROM now
    eeconfig_update_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);

    eeconfig_init_user();
}

// On Keyboard startup
void keyboard_post_init_kb(void) {
    // Initialize the indicator pins
    indicators_init();

    // Read custom menu variables from memory
    eeconfig_read_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);

    // Call the indicator callback to set the indicator color
    indicators_callback();

    keyboard_post_init_user();
}

// This function gets called when caps, num, scroll change
bool led_update_kb(led_t led_state) {
    indicators_callback();
    return true;
}

// This function is called when layers change
__attribute__((weak)) layer_state_t layer_state_set_user(layer_state_t state) {
    indicators_callback();
    return state;
}

bool func_switch(uint8_t func) {
    switch (func) {
        case 0x00: {
            return false;
            break;
        }
        case 0x01: // If indicator is set as caps lock
        {
            if (host_keyboard_led_state().caps_lock) return true;
            break;
        }
        case 0x02: // If indicator is set as num lock
        {
            if (host_keyboard_led_state().num_lock) return true;
            break;
        }
        case 0x03: // If indicator is set as scroll lock
        {
            if (host_keyboard_led_state().scroll_lock) return true;
            break;
        }
        case 0x04: // If keypress indicator is set
        {
            if (keypress) return true;
            break;
        }
        case 0x05: // If haptic feedback is set
        {
            if (haptic_config.enable) {
                return true;
            } else {
                return false;
            }
            break;
        }
        case 0x06: // If indicator is set as layer 0
        case 0x07: // layer 1
        case 0x08: // layer 2
        case 0x09: // layer 3
        {
            if (IS_LAYER_ON((int)(func)-6)) return true;
            break;
        }
        default: {
            return false;
        }
    }
    return false;
}

bool set_indicator(indicator_config indicator) {
    if (!indicator.enabled) return false;
    return func_switch(indicator.func & 0x0F);
}

void indicators_init(void) {
    // Set the indicator pins as output
    keypress = false;
    for (int i = 0; i < 7; i++) {
        gpio_set_pin_output(indicator_pins[i]);
        gpio_write_pin_low(indicator_pins[i]);
    }
}

bool indicators_callback(void) {
    // Basic functioning: for each indicator, set_indicator is used to decide if the current indicator should be lit or off.
    indicator_config *current_indicator_p;
    int               index;
    for (index = 0; index < 7; index++) {
        current_indicator_p = get_indicator_p(index);
        if (set_indicator(*(current_indicator_p))) {
            // Turn on the indicator
            gpio_write_pin_high(indicator_pins[index]);
        } else
            // Turn off the indicator
            gpio_write_pin_low(indicator_pins[index]);
    }
    return true;
}

uint8_t *pIndicators = (uint8_t *)&eeprom_mx_config;

indicator_config *get_indicator_p(int index) {
    return (indicator_config *)(pIndicators + index * sizeof(indicator_config));
}
