/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "mx.h"
#include "action.h"
#include "print.h"
#include "via.h"
#include <string.h>

#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#    include "usb_descriptor.h"
#endif

#ifdef VIA_ENABLE

// Function prototypes
static void     factory_reset(void);
static uint16_t socd_pair_handler(bool mode, uint8_t pair_idx, uint8_t field, uint16_t value);

// Declaring enums for VIA config menu
enum via_enums {
    // clang-format off
    id_ind1_enabled = 1,
    id_ind1_brightness = 2,
    id_ind1_color = 3,
    id_ind1_func = 4,
    id_ind2_enabled = 5,
    id_ind2_brightness = 6,
    id_ind2_color = 7,
    id_ind2_func = 8,
    id_ind3_enabled = 9,
    id_ind3_brightness = 10,
    id_ind3_color = 11,
    id_ind3_func = 12,
    id_socd_pair_1_mode = 13,
    id_socd_pair_1_key_1 = 14,
    id_socd_pair_1_key_2 = 15,
    id_socd_pair_2_mode = 16,
    id_socd_pair_2_key_1 = 17,
    id_socd_pair_2_key_2 = 18,
    id_socd_pair_3_mode = 19,
    id_socd_pair_3_key_1 = 20,
    id_socd_pair_3_key_2 = 21,
    id_socd_pair_4_mode = 22,
    id_socd_pair_4_key_1 = 23,
    id_socd_pair_4_key_2 = 24,
    id_flash_mode = 25,
    id_factory_reset = 26,
    // clang-format on
};

// Indices helpers for indicator handling
int indi_index;
int data_index;

// Handle the data received by the keyboard from the VIA menus
void via_config_set_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);

// Forward the same data to the slave side in case of split keyboard
#    ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        transaction_rpc_send(RPC_ID_VIA_CMD, RAW_EPSIZE - 2, data);
    }
#    endif
    if ((*value_id) < id_socd_pair_1_mode) {
        indi_index                            = ((int)(*value_id) - 1) / 4;
        data_index                            = (int)(*value_id) - indi_index * 4;
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
                }
                break;
            }
            case 2: {
                current_indicator_p->v = value_data[0];
                if (indi_index == 0) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind1.v);
                } else if (indi_index == 1) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind2.v);
                } else if (indi_index == 2) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind3.v);
                }
                break;
            }
            case 3: {
                current_indicator_p->h = value_data[0];
                current_indicator_p->s = value_data[1];
                if (indi_index == 0) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind1.h);
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind1.s);
                } else if (indi_index == 1) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind2.h);
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind2.s);
                } else if (indi_index == 2) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind3.h);
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind3.s);
                }
                break;
            }
            case 4: {
                current_indicator_p->func = (current_indicator_p->func & 0xF0) | (uint8_t)value_data[0];
                if (indi_index == 0) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind1.func);
                } else if (indi_index == 1) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind2.func);
                } else if (indi_index == 2) {
                    eeconfig_update_kb_datablock_field(eeprom_mx_config, ind3.func);
                }
                break;
            }
            default: {
                // Unhandled value.
                break;
            }
        }
        indicators_callback();
    } else {
        switch (*value_id) {
            case id_socd_pair_1_mode:
                socd_pair_handler(1, 0, 0, value_data[0]);
                break;
            case id_socd_pair_1_key_1:
                socd_pair_handler(1, 0, 1, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_1_key_2:
                socd_pair_handler(1, 0, 2, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_2_mode:
                socd_pair_handler(1, 1, 0, value_data[0]);
                break;
            case id_socd_pair_2_key_1:
                socd_pair_handler(1, 1, 1, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_2_key_2:
                socd_pair_handler(1, 1, 2, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_3_mode:
                socd_pair_handler(1, 2, 0, value_data[0]);
                break;
            case id_socd_pair_3_key_1:
                socd_pair_handler(1, 2, 1, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_3_key_2:
                socd_pair_handler(1, 2, 2, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_4_mode:
                socd_pair_handler(1, 3, 0, value_data[0]);
                break;
            case id_socd_pair_4_key_1:
                socd_pair_handler(1, 3, 1, value_data[1] | (value_data[0] << 8));
                break;
            case id_socd_pair_4_key_2:
                socd_pair_handler(1, 3, 2, value_data[1] | (value_data[0] << 8));
                break;
            case id_flash_mode: {
                uint8_t value = value_data[0];
                if (value == 0) {
                    // Execute DFU Jump
                    reset_keyboard();
                }
                break;
            }
            case id_factory_reset: {
                uint8_t value = value_data[0];
                if (value == 0) {
                    // Factory reset the board to the original state
                    factory_reset();
                }
                break;
            }
            default: {
                // Unhandled value.
                break;
            }
        }
    }
}

// Handle the data sent by the keyboard to the VIA menus
void via_config_get_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);
    uint16_t socd_pair_result;

    if ((*value_id) < id_socd_pair_1_mode) {
        indi_index                            = ((int)(*value_id) - 1) / 4;
        data_index                            = (int)(*value_id) - indi_index * 4;
        indicator_config *current_indicator_p = get_indicator_p(indi_index);

        switch (data_index) {
            case 1: {
                value_data[0] = current_indicator_p->enabled;
                break;
            }
            case 2: {
                value_data[0] = current_indicator_p->v;
                break;
            }
            case 3: {
                value_data[0] = current_indicator_p->h;
                value_data[1] = current_indicator_p->s;
                break;
            }
            case 4: {
                value_data[0] = current_indicator_p->func & 0x0F;
                break;
            }
            default: {
                // Unhandled value.
                break;
            }
        }
    } else {
        switch (*value_id) {
            case id_socd_pair_1_mode:
                value_data[0] = socd_pair_handler(0, 0, 0, 0);
                break;
            case id_socd_pair_1_key_1:
                socd_pair_result = socd_pair_handler(0, 0, 1, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_1_key_2:
                socd_pair_result = socd_pair_handler(0, 0, 2, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_2_mode:
                value_data[0] = socd_pair_handler(0, 1, 0, 0);
                break;
            case id_socd_pair_2_key_1:
                socd_pair_result = socd_pair_handler(0, 1, 1, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_2_key_2:
                socd_pair_result = socd_pair_handler(0, 1, 2, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_3_mode:
                value_data[0] = socd_pair_handler(0, 2, 0, 0);
                break;
            case id_socd_pair_3_key_1:
                socd_pair_result = socd_pair_handler(0, 2, 1, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_3_key_2:
                socd_pair_result = socd_pair_handler(0, 2, 2, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_4_mode:
                value_data[0] = socd_pair_handler(0, 3, 0, 0);
                break;
            case id_socd_pair_4_key_1:
                socd_pair_result = socd_pair_handler(0, 3, 1, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            case id_socd_pair_4_key_2:
                socd_pair_result = socd_pair_handler(0, 3, 2, 0);
                value_data[0]    = socd_pair_result >> 8;
                value_data[1]    = socd_pair_result & 0xFF;
                break;
            default: {
                // Unhandled value.
                break;
            }
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

// Handle the SOCD pairs configuration
static uint16_t socd_pair_handler(bool mode, uint8_t pair_idx, uint8_t field, uint16_t value) {
    if (mode) { // set
        switch (field) {
            case 0: // mode/resolution
                socd_opposing_pairs[pair_idx].resolution                         = value;
                socd_opposing_pairs[pair_idx].held[0]                            = false;
                socd_opposing_pairs[pair_idx].held[1]                            = false;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].resolution = value;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].held[0]    = false;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].held[1]    = false;
                eeconfig_update_kb_datablock_field(eeprom_mx_config, eeprom_socd_opposing_pairs);
                return 0;
            case 1: // key 1
                socd_opposing_pairs[pair_idx].keys[0]                         = value;
                socd_opposing_pairs[pair_idx].held[0]                         = false;
                socd_opposing_pairs[pair_idx].held[1]                         = false;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].keys[0] = value;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].held[0] = false;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].held[1] = false;
                eeconfig_update_kb_datablock_field(eeprom_mx_config, eeprom_socd_opposing_pairs);
                return 0;
            case 2: // key 2
                socd_opposing_pairs[pair_idx].keys[1]                         = value;
                socd_opposing_pairs[pair_idx].held[0]                         = false;
                socd_opposing_pairs[pair_idx].held[1]                         = false;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].keys[1] = value;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].held[0] = false;
                eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].held[1] = false;
                eeconfig_update_kb_datablock_field(eeprom_mx_config, eeprom_socd_opposing_pairs);
                return 0;
            default:
                return 0;
        }
    } else { // get
        switch (field) {
            case 0: // mode/resolution
                return eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].resolution;
            case 1: // key 1
                return eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].keys[0];
            case 2: // key 2
                return eeprom_mx_config.eeprom_socd_opposing_pairs[pair_idx].keys[1];
            default:
                return 0;
        }
    }
}

// Factory reset the board (unplug/replug requirement is merely a way to have UI refresh from a new connection)
static void factory_reset(void) {
    // Clear the EEPROM data
    eeconfig_init_kb();

    // Reset the runtime values to the EEPROM values
    keyboard_post_init_kb();

    uprintf("###################################################################\n");
    uprintf("# Factory Reset Performed                                         #\n");
    uprintf("# Unplug the board and plug it back in to complete the procedure. #\n");
    uprintf("###################################################################\n");
}

// Slave handler for split keyboards
#    ifdef SPLIT_KEYBOARD
void via_cmd_slave_handler(uint8_t m2s_size, const void *m2s_buffer, uint8_t s2m_size, void *s2m_buffer) {
    if (m2s_size == (RAW_EPSIZE - 2)) {
        via_config_set_value((uint8_t *)m2s_buffer);
    } else {
        uprintf("Unexpected response in slave handler\n");
    }
}
#    endif

#endif // VIA_ENABLE
