/* Copyright 2026 Cipulot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "hybrid_switch_matrix.h"
#include "action.h"
#include "print.h"
#include "via.h"
#include <string.h>
#include "generated_calibration_layout.h"

#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#    include "usb_descriptor.h"
#endif

#ifdef VIA_ENABLE

// Function prototypes
static void     hybrid_save_threshold_data(uint8_t option);
static void     hybrid_prepare_bottoming_calibration(void);
static void     hybrid_save_bottoming_calibration_reading(void);
static void     hybrid_show_calibration_data(void);
static void     hybrid_clear_bottoming_calibration_data(void);
static void     factory_reset(void);
static uint16_t socd_pair_handler(bool mode, uint8_t pair_idx, uint8_t field, uint16_t value);
static void     hybrid_update_main_cluster_field(update_mode_t mode, size_t runtime_offset, size_t eeprom_offset, const void *value, size_t field_size);

typedef enum {
    CAL_PRINT_SWITCH_TYPE,
    CAL_PRINT_ACTUATION_MODE,
    CAL_PRINT_APC_ACTUATION,
    CAL_PRINT_APC_RELEASE,
    CAL_PRINT_RT_INITIAL_DEADZONE,
    CAL_PRINT_RT_ACTUATION,
    CAL_PRINT_RT_RELEASE,
    CAL_PRINT_NOISE_FLOOR,
    CAL_PRINT_EXTREMUM,
    CAL_PRINT_BOTTOMING,
} calibration_print_field_t;

static bool hybrid_key_is_effective_ec(uint8_t row, uint8_t col) {
    if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
        return true;
    }

    if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID) {
        return runtime_hybrid_config.runtime_key_state[row][col].switch_type == SWITCH_TYPE_EC;
    }

    return false;
}

static void hybrid_update_main_cluster_field(update_mode_t mode, size_t runtime_offset, size_t eeprom_offset, const void *value, size_t field_size) {
    if (runtime_hybrid_config.board_mode != BOARD_MODE_HYBRID) {
        update_keys_field(mode, runtime_offset, eeprom_offset, value, field_size);
        return;
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
#ifdef SPECIAL_POSITIONS_LIST
            if (is_special_position(row, col)) {
                continue;
            }
#endif
            update_single_key_field(mode, runtime_offset, eeprom_offset, value, field_size, row, col);
        }
    }
}

static uint16_t calibration_get_print_value(calibration_print_field_t field, uint8_t row, uint8_t col) {
    runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[row][col];
    eeprom_key_state_t  *key_eeprom  = &eeprom_hybrid_config.eeprom_key_state[row][col];

    switch (field) {
        case CAL_PRINT_SWITCH_TYPE:
            return key_eeprom->switch_type;
        case CAL_PRINT_ACTUATION_MODE:
            return key_eeprom->actuation_mode;
        case CAL_PRINT_APC_ACTUATION:
            return key_runtime->rescaled_apc_actuation_threshold;
        case CAL_PRINT_APC_RELEASE:
            return key_runtime->rescaled_apc_release_threshold;
        case CAL_PRINT_RT_INITIAL_DEADZONE:
            return key_runtime->rescaled_rt_initial_deadzone_offset;
        case CAL_PRINT_RT_ACTUATION:
            return key_runtime->rescaled_rt_actuation_offset;
        case CAL_PRINT_RT_RELEASE:
            return key_runtime->rescaled_rt_release_offset;
        case CAL_PRINT_NOISE_FLOOR:
            return key_runtime->noise_floor;
        case CAL_PRINT_EXTREMUM:
            return key_runtime->extremum;
        case CAL_PRINT_BOTTOMING:
            return key_runtime->bottoming_calibration_reading;
        default:
            return 0;
    }
}

static void calibration_print_spaces(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        uprintf(" ");
    }
}

static void calibration_print_dashes(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        uprintf("-");
    }
}

static void calibration_print_to_x(uint8_t *cursor, uint8_t x) {
    while (*cursor < x) {
        uprintf(" ");
        (*cursor)++;
    }
}

static void calibration_print_key_border_row(uint8_t layout_row) {
    uint8_t cursor = 0;

    for (uint8_t idx = 0; idx < ARRAY_SIZE(calibration_print_layout[layout_row]); idx++) {
        calibration_layout_key_t key = calibration_print_layout[layout_row][idx];

        if (key.row == CALIBRATION_LAYOUT_END) {
            break;
        }

        calibration_print_to_x(&cursor, key.x);
        uprintf("+");
        calibration_print_dashes(key.w - 2);
        uprintf("+");
        cursor = key.x + key.w;
    }
    uprintf("\n");
}

static void calibration_print_key_value_row(calibration_print_field_t field, uint8_t layout_row) {
    uint8_t cursor = 0;

    for (uint8_t idx = 0; idx < ARRAY_SIZE(calibration_print_layout[layout_row]); idx++) {
        calibration_layout_key_t key = calibration_print_layout[layout_row][idx];

        if (key.row == CALIBRATION_LAYOUT_END) {
            break;
        }

        uint8_t inner_width  = key.w - 2;
        uint8_t left_spaces  = (inner_width - 4) / 2;
        uint8_t right_spaces = inner_width - 4 - left_spaces;

        calibration_print_to_x(&cursor, key.x);
        uprintf("|");
        calibration_print_spaces(left_spaces);
        uprintf("%4d", calibration_get_print_value(field, key.row, key.col));
        calibration_print_spaces(right_spaces);
        uprintf("|");
        cursor = key.x + key.w;
    }
    uprintf("\n");
}

static void calibration_print_layout_field(calibration_print_field_t field) {
    for (uint8_t layout_row = 0; layout_row < ARRAY_SIZE(calibration_print_layout); layout_row++) {
        calibration_print_key_border_row(layout_row);
        calibration_print_key_value_row(field, layout_row);
        calibration_print_key_border_row(layout_row);
    }
}

// Declaring enums for VIA config menu
enum via_enums {
    // clang-format off
    // Special Key 1
    id_switch_type_key_1 = 1,
    id_actuation_mode_key_1 = 2,
    id_apc_actuation_threshold_key_1 = 3,
    id_apc_release_threshold_key_1 = 4,
    id_rt_initial_deadzone_offset_key_1 = 5,
    id_rt_actuation_offset_key_1 = 6,
    id_rt_release_offset_key_1 = 7,
    // Special Key 2
    id_switch_type_key_2 = 8,
    id_actuation_mode_key_2 = 9,
    id_apc_actuation_threshold_key_2 = 10,
    id_apc_release_threshold_key_2 = 11,
    id_rt_initial_deadzone_offset_key_2 = 12,
    id_rt_actuation_offset_key_2 = 13,
    id_rt_release_offset_key_2 = 14,
    // Special Key 3
    id_switch_type_key_3 = 15,
    id_actuation_mode_key_3 = 16,
    id_apc_actuation_threshold_key_3 = 17,
    id_apc_release_threshold_key_3 = 18,
    id_rt_initial_deadzone_offset_key_3 = 19,
    id_rt_actuation_offset_key_3 = 20,
    id_rt_release_offset_key_3 = 21,
    // Special Key 4
    id_switch_type_key_4 = 22,
    id_actuation_mode_key_4 = 23,
    id_apc_actuation_threshold_key_4 = 24,
    id_apc_release_threshold_key_4 = 25,
    id_rt_initial_deadzone_offset_key_4 = 26,
    id_rt_actuation_offset_key_4 = 27,
    id_rt_release_offset_key_4 = 28,
    // Special Key 5
    id_switch_type_key_5 = 29,
    id_actuation_mode_key_5 = 30,
    id_apc_actuation_threshold_key_5 = 31,
    id_apc_release_threshold_key_5 = 32,
    id_rt_initial_deadzone_offset_key_5 = 33,
    id_rt_actuation_offset_key_5 = 34,
    id_rt_release_offset_key_5 = 35,
    // Special Key 6
    id_switch_type_key_6 = 36,
    id_actuation_mode_key_6 = 37,
    id_apc_actuation_threshold_key_6 = 38,
    id_apc_release_threshold_key_6 = 39,
    id_rt_initial_deadzone_offset_key_6 = 40,
    id_rt_actuation_offset_key_6 = 41,
    id_rt_release_offset_key_6 = 42,
    // Special Key 7
    id_switch_type_key_7 = 43,
    id_actuation_mode_key_7 = 44,
    id_apc_actuation_threshold_key_7 = 45,
    id_apc_release_threshold_key_7 = 46,
    id_rt_initial_deadzone_offset_key_7 = 47,
    id_rt_actuation_offset_key_7 = 48,
    id_rt_release_offset_key_7 = 49,
    // Special Key 8
    id_switch_type_key_8 = 50,
    id_actuation_mode_key_8 = 51,
    id_apc_actuation_threshold_key_8 = 52,
    id_apc_release_threshold_key_8 = 53,
    id_rt_initial_deadzone_offset_key_8 = 54,
    id_rt_actuation_offset_key_8 = 55,
    id_rt_release_offset_key_8 = 56,
    // Special Key 9
    id_switch_type_key_9 = 57,
    id_actuation_mode_key_9 = 58,
    id_apc_actuation_threshold_key_9 = 59,
    id_apc_release_threshold_key_9 = 60,
    id_rt_initial_deadzone_offset_key_9 = 61,
    id_rt_actuation_offset_key_9 = 62,
    id_rt_release_offset_key_9 = 63,
    // Special Key 10
    id_switch_type_key_10 = 64,
    id_actuation_mode_key_10 = 65,
    id_apc_actuation_threshold_key_10 = 66,
    id_apc_release_threshold_key_10 = 67,
    id_rt_initial_deadzone_offset_key_10 = 68,
    id_rt_actuation_offset_key_10 = 69,
    id_rt_release_offset_key_10 = 70,
    // Special Key 11
    id_switch_type_key_11 = 71,
    id_actuation_mode_key_11 = 72,
    id_apc_actuation_threshold_key_11 = 73,
    id_apc_release_threshold_key_11 = 74,
    id_rt_initial_deadzone_offset_key_11 = 75,
    id_rt_actuation_offset_key_11 = 76,
    id_rt_release_offset_key_11 = 77,
    // Special Key 12
    id_switch_type_key_12 = 78,
    id_actuation_mode_key_12 = 79,
    id_apc_actuation_threshold_key_12 = 80,
    id_apc_release_threshold_key_12 = 81,
    id_rt_initial_deadzone_offset_key_12 = 82,
    id_rt_actuation_offset_key_12 = 83,
    id_rt_release_offset_key_12 = 84,
    // Full Board or General Cluster
    id_actuation_mode = 85,
    id_apc_actuation_threshold = 86,
    id_apc_release_threshold = 87,
    id_save_threshold_data = 88,
    id_rt_initial_deadzone_offset = 89,
    id_rt_actuation_offset = 90,
    id_rt_release_offset = 91,
    id_bottoming_calibration = 92,
    id_noise_floor_calibration = 93,
    id_show_calibration_data = 94,
    id_clear_bottoming_calibration_data = 95,
    id_switch_type = 96,
    id_socd_pair_1_mode = 97,
    id_socd_pair_1_key_1 = 98,
    id_socd_pair_1_key_2 = 99,
    id_socd_pair_2_mode = 100,
    id_socd_pair_2_key_1 = 101,
    id_socd_pair_2_key_2 = 102,
    id_socd_pair_3_mode = 103,
    id_socd_pair_3_key_1 = 104,
    id_socd_pair_3_key_2 = 105,
    id_socd_pair_4_mode = 106,
    id_socd_pair_4_key_1 = 107,
    id_socd_pair_4_key_2 = 108,
    id_board_mode = 109,
    id_flash_mode = 110,
    id_factory_reset = 111,
    // clang-format on
};

// Indices helpers for enum handling
int enum_index;
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

    if ((*value_id) < id_actuation_mode) {
        enum_index = ((int)(*value_id) - 1) / 7;
        data_index = (int)(*value_id) - enum_index * 7;

        switch (data_index) {
            case 1: {
                uint8_t value = value_data[0];
                // Update only the per-key switch_type field in runtime and EEPROM (shared offset)
                update_single_key_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, switch_type), 0, &value, sizeof(uint8_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
                if (value == 0) {
                    uprintf("#####################\n");
                    uprintf("#  Switch Type: EC  #\n");
                    uprintf("#####################\n");
                } else if (value == 1) {
                    uprintf("###################\n");
                    uprintf("#  Switch Type: MX  #\n");
                    uprintf("###################\n");
                }
                break;
            }
            case 2: {
                uint8_t value = value_data[0];
                // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
                update_single_key_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, actuation_mode), 0, &value, sizeof(uint8_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
                if (value == 0) {
                    uprintf("#########################\n");
                    uprintf("#  Actuation Mode: APC  #\n");
                    uprintf("#########################\n");
                } else if (value == 1) {
                    uprintf("#################################\n");
                    uprintf("# Actuation Mode: Rapid Trigger #\n");
                    uprintf("#################################\n");
                }
                break;
            }
            case 3: {
                uint16_t value = value_data[1] | (value_data[0] << 8);
                update_single_key_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, apc_actuation_threshold), 0, &value, sizeof(uint16_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                uprintf("APC Mode Actuation Threshold: %d\n", value);
                break;
            }
            case 4: {
                uint16_t value = value_data[1] | (value_data[0] << 8);
                update_single_key_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, apc_release_threshold), 0, &value, sizeof(uint16_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                uprintf("APC Mode Release Threshold: %d\n", value);
                break;
            }
            case 5: {
                uint16_t value = value_data[1] | (value_data[0] << 8);
                update_single_key_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_initial_deadzone_offset), 0, &value, sizeof(uint16_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                uprintf("Rapid Trigger Mode Initial Deadzone Offset: %d\n", value);
                break;
            }
            case 6: {
                uint8_t value = value_data[0];
                update_single_key_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_actuation_offset), 0, &value, sizeof(uint8_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                uprintf("Rapid Trigger Mode Actuation Offset: %d\n", value);
                break;
            }
            case 7: {
                uint8_t value = value_data[0];
                update_single_key_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_release_offset), 0, &value, sizeof(uint8_t), SPECIAL_POSITIONS[enum_index][0], SPECIAL_POSITIONS[enum_index][1]);
                uprintf("Rapid Trigger Mode Release Offset: %d\n", value);
                break;
            }
            default: {
                // Unhandled value.
                break;
            }
        }
    } else {
        switch (*value_id) {
            case id_board_mode: {
                uint8_t value = value_data[0];
                // Update board mode in runtime and EEPROM
                runtime_hybrid_config.board_mode = value;
                eeprom_hybrid_config.board_mode  = value;
                if (value == BOARD_MODE_EC || value == BOARD_MODE_MX) {
                    uint8_t switch_type                     = value == BOARD_MODE_EC ? SWITCH_TYPE_EC : SWITCH_TYPE_MX;
                    runtime_hybrid_config.board_switch_type = switch_type;
                    eeprom_hybrid_config.board_switch_type  = switch_type;
                    update_keys_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, switch_type), 0, &switch_type, sizeof(uint8_t));
                    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, board_switch_type);
                    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
                }
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, board_mode);
                if (value == 0) {
                    uprintf("#########################\n");
                    uprintf("#  Board Mode: Full EC  #\n");
                    uprintf("#########################\n");
                } else if (value == 1) {
                    uprintf("#########################\n");
                    uprintf("#  Board Mode: Full MX  #\n");
                    uprintf("#########################\n");
                } else if (value == 2) {
                    uprintf("########################\n");
                    uprintf("#  Board Mode: Hybrid  #\n");
                    uprintf("########################\n");
                }
                break;
            }
            case id_switch_type: {
                uint8_t value = value_data[0];
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_switch_type = value;
                eeprom_hybrid_config.board_switch_type  = value;
                // Update only the per-key switch_type field in runtime and EEPROM (shared offset)
                hybrid_update_main_cluster_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, switch_type), 0, &value, sizeof(uint8_t));
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, board_switch_type);
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
                if (value == 0) {
                    uprintf("##################################\n");
                    uprintf("#  Main Cluster Switch Type: EC  #\n");
                    uprintf("##################################\n");
                } else if (value == 1) {
                    uprintf("##################################\n");
                    uprintf("#  Main Cluster Switch Type: MX  #\n");
                    uprintf("##################################\n");
                }
                break;
            }
            case id_actuation_mode: {
                uint8_t value = value_data[0];
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_actuation_mode = value;
                eeprom_hybrid_config.board_actuation_mode  = value;
                // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
                hybrid_update_main_cluster_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, actuation_mode), 0, &value, sizeof(uint8_t));
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, board_actuation_mode);
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
                if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
                    if (value == 0) {
                        uprintf("######################################\n");
                        uprintf("#  Full Board Actuation Mode: APC  #\n");
                        uprintf("######################################\n");
                    } else if (value == 1) {
                        uprintf("##############################################\n");
                        uprintf("# Full Board Actuation Mode: Rapid Trigger #\n");
                        uprintf("##############################################\n");
                    }
                } else if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID && runtime_hybrid_config.board_switch_type == SWITCH_TYPE_EC) {
                    if (value == 0) {
                        uprintf("######################################\n");
                        uprintf("#  Main Cluster Actuation Mode: APC  #\n");
                        uprintf("######################################\n");
                    } else if (value == 1) {
                        uprintf("##############################################\n");
                        uprintf("# Main Cluster Actuation Mode: Rapid Trigger #\n");
                        uprintf("##############################################\n");
                    }
                }
                break;
            }
            case id_apc_actuation_threshold: {
                uint16_t value = value_data[1] | (value_data[0] << 8);
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_apc_actuation_threshold = value;
                eeprom_hybrid_config.board_apc_actuation_threshold  = value;
                // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
                hybrid_update_main_cluster_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, apc_actuation_threshold), 0, &value, sizeof(uint16_t));
                if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
                    if (value == 0) {
                        uprintf("Full Board Actuation Threshold: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Full Board Actuation Threshold: %d\n", value);
                    }
                } else if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID && runtime_hybrid_config.board_switch_type == SWITCH_TYPE_EC) {
                    if (value == 0) {
                        uprintf("Main Cluster Actuation Threshold: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Main Cluster Actuation Threshold: %d\n", value);
                    }
                }
                break;
            }
            case id_apc_release_threshold: {
                uint16_t value = value_data[1] | (value_data[0] << 8);
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_apc_release_threshold = value;
                eeprom_hybrid_config.board_apc_release_threshold  = value;
                hybrid_update_main_cluster_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, apc_release_threshold), 0, &value, sizeof(uint16_t));
                if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
                    if (value == 0) {
                        uprintf("Full Board Release Threshold: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Full Board Release Threshold: %d\n", value);
                    }
                } else if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID && runtime_hybrid_config.board_switch_type == SWITCH_TYPE_EC) {
                    if (value == 0) {
                        uprintf("Main Cluster Release Threshold: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Main Cluster Release Threshold: %d\n", value);
                    }
                }
                break;
            }
            case id_rt_initial_deadzone_offset: {
                uint16_t value = value_data[1] | (value_data[0] << 8);
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_rt_initial_deadzone_offset = value;
                eeprom_hybrid_config.board_rt_initial_deadzone_offset  = value;
                // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
                hybrid_update_main_cluster_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_initial_deadzone_offset), 0, &value, sizeof(uint16_t));
                if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
                    if (value == 0) {
                        uprintf("Full Board Rapid Trigger Initial Deadzone Offset: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Full Board Rapid Trigger Initial Deadzone Offset: %d\n", value);
                    }
                } else if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID && runtime_hybrid_config.board_switch_type == SWITCH_TYPE_EC) {
                    if (value == 0) {
                        uprintf("Main Cluster Rapid Trigger Initial Deadzone Offset: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Main Cluster Rapid Trigger Initial Deadzone Offset: %d\n", value);
                    }
                }
                break;
            }
            case id_rt_actuation_offset: {
                uint8_t value = value_data[0];
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_rt_actuation_offset = value;
                eeprom_hybrid_config.board_rt_actuation_offset  = value;
                // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
                hybrid_update_main_cluster_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_actuation_offset), 0, &value, sizeof(uint8_t));
                if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
                    if (value == 0) {
                        uprintf("Full Board Rapid Trigger Actuation Offset: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Full Board Rapid Trigger Actuation Offset: %d\n", value);
                    }
                } else if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID && runtime_hybrid_config.board_switch_type == SWITCH_TYPE_EC) {
                    if (value == 0) {
                        uprintf("Main Cluster Rapid Trigger Actuation Offset: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Main Cluster Rapid Trigger Actuation Offset: %d\n", value);
                    }
                }
                break;
            }
            case id_rt_release_offset: {
                uint8_t value = value_data[0];
                // Update switch type in runtime and EEPROM for full board
                runtime_hybrid_config.board_rt_release_offset = value;
                eeprom_hybrid_config.board_rt_release_offset  = value;
                // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
                hybrid_update_main_cluster_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_release_offset), 0, &value, sizeof(uint8_t));
                if (runtime_hybrid_config.board_mode == BOARD_MODE_EC) {
                    if (value == 0) {
                        uprintf("Full Board Rapid Trigger Release Offset: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Full Board Rapid Trigger Release Offset: %d\n", value);
                    }
                } else if (runtime_hybrid_config.board_mode == BOARD_MODE_HYBRID && runtime_hybrid_config.board_switch_type == SWITCH_TYPE_EC) {
                    if (value == 0) {
                        uprintf("Main Cluster Rapid Trigger Release Offset: %d\n", value);
                    } else if (value == 1) {
                        uprintf("Main Cluster Rapid Trigger Release Offset: %d\n", value);
                    }
                }
                break;
            }
            case id_bottoming_calibration: {
                uint8_t value = value_data[0];
                // 0: stop calibration and save, 1: start calibration
                if (value == 1) {
                    hybrid_prepare_bottoming_calibration();
                    runtime_hybrid_config.bottoming_calibration = true;
                    clear_keyboard();
                    uprintf("##############################\n");
                    uprintf("# Calibration In Progress    #\n");
                    uprintf("##############################\n");
                } else {
                    // Set the bottoming calibration flag to false and save readings
                    runtime_hybrid_config.bottoming_calibration = false;
                    clear_keyboard();
                    hybrid_save_bottoming_calibration_reading();
                    uprintf("## Calibration Completed ##\n");
                    hybrid_show_calibration_data();
                }
                break;
            }
            case id_save_threshold_data: {
                uint8_t value = value_data[0];
                // 0: APC thresholds, 1: RT thresholds
                hybrid_save_threshold_data(value);
                break;
            }
            case id_noise_floor_calibration: {
                uint8_t value = value_data[0];
                if (value == 0) {
                    // Perform resting position calibration
                    hybrid_noise_floor_calibration(); // Note: resting position calibration already rescales thresholds
                    uprintf("#############################\n");
                    uprintf("# Resting Position Readings Acquired #\n");
                    uprintf("#############################\n");
                    break;
                }
                break;
            }
            case id_show_calibration_data: {
                uint8_t value = value_data[0];
                if (value == 0) {
                    // Show calibration data
                    hybrid_show_calibration_data();
                }
                break;
            }
            case id_clear_bottoming_calibration_data: {
                uint8_t value = value_data[0];
                if (value == 0) {
                    // Clear bottoming calibration data
                    hybrid_clear_bottoming_calibration_data();
                }
                break;
            }
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

    if ((*value_id) < id_actuation_mode) {
        enum_index = ((int)(*value_id) - 1) / 7;
        data_index = (int)(*value_id) - enum_index * 7;
        // Pointer to the specific special key's runtime state
        runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[SPECIAL_POSITIONS[enum_index][0]][SPECIAL_POSITIONS[enum_index][1]];
        switch (data_index) {
            case 1: {
                value_data[0] = key_runtime->switch_type;
                break;
            }
            case 2: {
                value_data[0] = key_runtime->actuation_mode;
                break;
            }
            case 3: {
                value_data[0] = key_runtime->apc_actuation_threshold >> 8;
                value_data[1] = key_runtime->apc_actuation_threshold & 0xFF;
                break;
            }
            case 4: {
                value_data[0] = key_runtime->apc_release_threshold >> 8;
                value_data[1] = key_runtime->apc_release_threshold & 0xFF;
                break;
            }
            case 5: {
                value_data[0] = key_runtime->rt_initial_deadzone_offset >> 8;
                value_data[1] = key_runtime->rt_initial_deadzone_offset & 0xFF;
                break;
            }
            case 6: {
                value_data[0] = key_runtime->rt_actuation_offset;
                break;
            }
            case 7: {
                value_data[0] = key_runtime->rt_release_offset;
                break;
            }
            default: {
                // Unhandled value.
                break;
            }
        }
    } else {
        switch (*value_id) {
            case id_board_mode: {
                value_data[0] = runtime_hybrid_config.board_mode;
                break;
            }
            case id_switch_type: {
                value_data[0] = runtime_hybrid_config.board_switch_type;
                break;
            }
            case id_actuation_mode: {
                value_data[0] = runtime_hybrid_config.board_actuation_mode;
                break;
            }
            case id_apc_actuation_threshold: {
                value_data[0] = runtime_hybrid_config.board_apc_actuation_threshold >> 8;
                value_data[1] = runtime_hybrid_config.board_apc_actuation_threshold & 0xFF;
                break;
            }
            case id_apc_release_threshold: {
                value_data[0] = runtime_hybrid_config.board_apc_release_threshold >> 8;
                value_data[1] = runtime_hybrid_config.board_apc_release_threshold & 0xFF;
                break;
            }
            case id_rt_initial_deadzone_offset: {
                value_data[0] = runtime_hybrid_config.board_rt_initial_deadzone_offset >> 8;
                value_data[1] = runtime_hybrid_config.board_rt_initial_deadzone_offset & 0xFF;
                break;
            }
            case id_rt_actuation_offset: {
                value_data[0] = runtime_hybrid_config.board_rt_actuation_offset;
                break;
            }
            case id_rt_release_offset: {
                value_data[0] = runtime_hybrid_config.board_rt_release_offset;
                break;
            }
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

// Handle the application of new threshold data and save to EEPROM
static void hybrid_save_threshold_data(uint8_t option) {
    // Save APC mode thresholds and rescale them for runtime usage
    if (option == 0) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                // Get pointer to key state in runtime and EEPROM
                runtime_key_state_t *key_runtime    = &runtime_hybrid_config.runtime_key_state[row][col];
                eeprom_key_state_t  *key_eeprom     = &eeprom_hybrid_config.eeprom_key_state[row][col];
                key_eeprom->apc_actuation_threshold = key_runtime->apc_actuation_threshold;
                key_eeprom->apc_release_threshold   = key_runtime->apc_release_threshold;
                // Rescale key thresholds based on new APC values
                bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_APC);
            }
        }
        // Save to EEPROM the eeprom_key_state field
        eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
    }
    // Save Rapid Trigger mode thresholds and rescale them for runtime usage
    else if (option == 1) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                // Get pointer to key state in runtime and EEPROM
                runtime_key_state_t *key_runtime       = &runtime_hybrid_config.runtime_key_state[row][col];
                eeprom_key_state_t  *key_eeprom        = &eeprom_hybrid_config.eeprom_key_state[row][col];
                key_eeprom->rt_initial_deadzone_offset = key_runtime->rt_initial_deadzone_offset;
                key_eeprom->rt_actuation_offset        = key_runtime->rt_actuation_offset;
                key_eeprom->rt_release_offset          = key_runtime->rt_release_offset;
                // Rescale key thresholds based on new RT values
                bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_RT);
            }
        }
        // Save to EEPROM the eeprom_key_state field
        eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
    }
    uprintf("####################################\n");
    uprintf("# New thresholds applied and saved #\n");
    uprintf("####################################\n");
}

static void hybrid_prepare_bottoming_calibration(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!hybrid_key_is_effective_ec(row, col)) {
                continue;
            }

            runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[row][col];
            eeprom_key_state_t  *key_eeprom  = &eeprom_hybrid_config.eeprom_key_state[row][col];

            key_runtime->bottoming_calibration_starter = true;
            key_runtime->bottoming_calibration_reading = key_eeprom->bottoming_calibration_reading;
        }
    }
}

// Handle the application of the bottoming calibration data and save to EEPROM
static void hybrid_save_bottoming_calibration_reading(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!hybrid_key_is_effective_ec(row, col)) {
                continue;
            }

            runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[row][col];
            eeprom_key_state_t  *key_eeprom  = &eeprom_hybrid_config.eeprom_key_state[row][col];

            if (key_runtime->bottoming_calibration_starter || key_runtime->bottoming_calibration_reading < (key_runtime->noise_floor + BOTTOMING_CALIBRATION_THRESHOLD)) {
                key_runtime->bottoming_calibration_reading = key_eeprom->bottoming_calibration_reading;
                bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_ALL);
                continue;
            }

            key_eeprom->bottoming_calibration_reading = key_runtime->bottoming_calibration_reading;
            bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_ALL);
        }
    }
    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
}

// Show the calibration data
static void hybrid_show_calibration_data(void) {
    uprintf("\n#######################\n");
    uprintf("# Board Wide Settings #\n");
    uprintf("#######################\n\n");
    uprintf("Board Mode: %s\n", runtime_hybrid_config.board_mode == 0 ? "BOARD_MODE_EC" : runtime_hybrid_config.board_mode == 1 ? "BOARD_MODE_MX" : runtime_hybrid_config.board_mode == 2 ? "BOARD_MODE_HYBRID" : "UNKNOWN");
    uprintf("Board Switch Type: %s\n", runtime_hybrid_config.board_switch_type == 0 ? "SWITCH_TYPE_EC" : runtime_hybrid_config.board_switch_type == 1 ? "SWITCH_TYPE_MX" : "UNKNOWN");
    uprintf("Board Actuation Mode: %s\n", runtime_hybrid_config.board_actuation_mode == 0 ? "ACTUATION_MODE_APC" : runtime_hybrid_config.board_actuation_mode == 1 ? "ACTUATION_MODE_RAPID_TRIGGER" : "UNKNOWN");
    uprintf("Board APC Actuation Threshold: %d\n", runtime_hybrid_config.board_apc_actuation_threshold);
    uprintf("Board APC Release Threshold: %d\n", runtime_hybrid_config.board_apc_release_threshold);
    uprintf("Board RT Initial Deadzone Offset: %d\n", runtime_hybrid_config.board_rt_initial_deadzone_offset);
    uprintf("Board RT Actuation Offset: %d\n", runtime_hybrid_config.board_rt_actuation_offset);
    uprintf("Board RT Release Offset: %d\n", runtime_hybrid_config.board_rt_release_offset);

    uprintf("\n###############################################################################\n");

    uprintf("\n####################\n");
    uprintf("# Per-key Settings #\n");
    uprintf("####################\n\n");

    uprintf("\n#######################\n");
    uprintf("# Per-key Switch Type #\n");
    uprintf("#######################\n");
    uprintf("Switch Type: 0 -> EC | 1 -> MX\n");
    calibration_print_layout_field(CAL_PRINT_SWITCH_TYPE);

    uprintf("\n##########################\n");
    uprintf("# Per-key Actuation Mode #\n");
    uprintf("##########################\n");
    uprintf("Actuation Mode: 0 -> APC | 1 -> Rapid Trigger\n");
    calibration_print_layout_field(CAL_PRINT_ACTUATION_MODE);

    uprintf("\n###################################\n");
    uprintf("# Per-key APC Actuation Threshold #\n");
    uprintf("###################################\n");
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_APC_ACTUATION);

    uprintf("\n#################################\n");
    uprintf("# Per-key APC Release Threshold #\n");
    uprintf("#################################\n");
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_APC_RELEASE);

    uprintf("\n######################################\n");
    uprintf("# Per-key RT Initial Deadzone Offset #\n");
    uprintf("######################################\n");
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_RT_INITIAL_DEADZONE);

    uprintf("\n###############################\n");
    uprintf("# Per-key RT Actuation Offset #\n");
    uprintf("###############################\n");
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_RT_ACTUATION);

    uprintf("\n#############################\n");
    uprintf("# Per-key RT Release Offset #\n");
    uprintf("#############################\n");
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_RT_RELEASE);

    uprintf("\n#############################\n");
    uprintf("# Resting Position Readings #\n");
    uprintf("#############################\n");
    calibration_print_layout_field(CAL_PRINT_NOISE_FLOOR);

    uprintf("\n############\n");
    uprintf("# Extremum #\n");
    uprintf("############\n");
    calibration_print_layout_field(CAL_PRINT_EXTREMUM);

    uprintf("\n########################\n");
    uprintf("# Calibration Readings #\n");
    uprintf("########################\n");
    calibration_print_layout_field(CAL_PRINT_BOTTOMING);
    print("\n\n\n");
}

// Clear the calibration data readings
static void hybrid_clear_bottoming_calibration_data(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!hybrid_key_is_effective_ec(row, col)) {
                continue;
            }

            runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[row][col];
            eeprom_key_state_t  *key_eeprom  = &eeprom_hybrid_config.eeprom_key_state[row][col];

            key_runtime->bottoming_calibration_reading = DEFAULT_BOTTOMING_CALIBRATION_READING;
            key_runtime->bottoming_calibration_starter = DEFAULT_CALIBRATION_STARTER;
            key_eeprom->bottoming_calibration_reading  = DEFAULT_BOTTOMING_CALIBRATION_READING;

            bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_ALL);
        }
    }
    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);

    uprintf("############################\n");
    uprintf("# Calibration Data Cleared #\n");
    uprintf("############################\n");
}

// Handle the SOCD pairs configuration
static uint16_t socd_pair_handler(bool mode, uint8_t pair_idx, uint8_t field, uint16_t value) {
    if (mode) { // set
        switch (field) {
            case 0: // mode/resolution
                socd_opposing_pairs[pair_idx].resolution                             = value;
                socd_opposing_pairs[pair_idx].held[0]                                = false;
                socd_opposing_pairs[pair_idx].held[1]                                = false;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].resolution = value;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].held[0]    = false;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].held[1]    = false;
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_socd_opposing_pairs);
                return 0;
            case 1: // key 1
                socd_opposing_pairs[pair_idx].keys[0]                             = value;
                socd_opposing_pairs[pair_idx].held[0]                             = false;
                socd_opposing_pairs[pair_idx].held[1]                             = false;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].keys[0] = value;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].held[0] = false;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].held[1] = false;
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_socd_opposing_pairs);
                return 0;
            case 2: // key 2
                socd_opposing_pairs[pair_idx].keys[1]                             = value;
                socd_opposing_pairs[pair_idx].held[0]                             = false;
                socd_opposing_pairs[pair_idx].held[1]                             = false;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].keys[1] = value;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].held[0] = false;
                eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].held[1] = false;
                eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_socd_opposing_pairs);
                return 0;
            default:
                return 0;
        }
    } else { // get
        switch (field) {
            case 0: // mode/resolution
                return eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].resolution;
            case 1: // key 1
                return eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].keys[0];
            case 2: // key 2
                return eeprom_hybrid_config.eeprom_socd_opposing_pairs[pair_idx].keys[1];
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
