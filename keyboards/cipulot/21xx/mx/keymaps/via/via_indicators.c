/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mx.h"
#include "print.h"
#include "via.h"

#ifdef VIA_ENABLE

// Declaring enums for VIA config menu
enum via_enums {
    // clang-format off
    id_ind1_enabled = 1,
    id_ind1_func = 2,
    id_ind2_enabled = 3,
    id_ind2_func = 4,
    id_ind3_enabled = 5,
    id_ind3_func = 6,
    id_ind4_enabled = 7,
    id_ind4_func = 8,
    id_ind5_enabled = 9,
    id_ind5_func = 10,
    id_ind6_enabled = 11,
    id_ind6_func = 12,
    id_ind7_enabled = 13,
    id_ind7_func = 14,
    // clang-format on
};

int indi_index;
int data_index;

// Handle the data received by the keyboard from the VIA menus
void via_config_set_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);

    indi_index                            = ((int)(*value_id) - 1) / 2;
    data_index                            = (int)(*value_id) - indi_index * 2;
    indicator_config *current_indicator_p = get_indicator_p(indi_index);

    switch (data_index) {
        case 1: {
            current_indicator_p->enabled = value_data[0];
            if (indi_index == 0) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind1.enabled);
            } else if (indi_index == 1) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind2.enabled);
            } else if (indi_index == 2) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind3.enabled);
            } else if (indi_index == 3) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind4.enabled);
            } else if (indi_index == 4) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind5.enabled);
            } else if (indi_index == 5) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind6.enabled);
            } else if (indi_index == 6) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind7.enabled);
            }
            break;
        }
        case 2: {
            current_indicator_p->func = (current_indicator_p->func & 0xF0) | (uint8_t)value_data[0];
            if (indi_index == 0) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind1.func);
            } else if (indi_index == 1) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind2.func);
            } else if (indi_index == 2) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind3.func);
            } else if (indi_index == 3) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind4.func);
            } else if (indi_index == 4) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind5.func);
            } else if (indi_index == 5) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind6.func);
            } else if (indi_index == 6) {
                eeconfig_update_kb_datablock_field(eeprom_mx_config, ind7.func);
            }
            break;
        }
        default: {
            // Unhandled value.
            break;
        }
    }
    indicators_callback();
}

// Handle the data sent by the keyboard to the VIA menus
void via_config_get_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);

    indi_index                            = ((int)(*value_id) - 1) / 2;
    data_index                            = (int)(*value_id) - indi_index * 2;
    indicator_config *current_indicator_p = get_indicator_p(indi_index);

    switch (data_index) {
        case 1: {
            value_data[0] = current_indicator_p->enabled;
            break;
        }
        case 2: {
            value_data[0] = current_indicator_p->func & 0x0F;
            break;
        }
        default: {
            // Unhandled value.
            break;
        }
    }
}

// Handle the commands sent and received by the keyboard with VIA
void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // data = [ command_id, channel_id, value_id, value_data ]
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id == id_custom_channel) {
        switch (*command_id) {
            case id_custom_set_value: {
                via_config_set_value(value_id_and_data);
                break;
            }
            case id_custom_get_value: {
                via_config_get_value(value_id_and_data);
                break;
            }
            case id_custom_save: {
                // Bypass the save function in favor of pinpointed saves
                break;
            }
            default: {
                // Unhandled message.
                *command_id = id_unhandled;
                break;
            }
        }
        return;
    }

    *command_id = id_unhandled;
}

#endif // VIA_ENABLE
