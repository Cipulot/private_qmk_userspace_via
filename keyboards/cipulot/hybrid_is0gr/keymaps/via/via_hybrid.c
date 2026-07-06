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
#include "quantum.h"
#include <string.h>
#include "generated_calibration_layout.h"

#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#    include "usb_descriptor.h"
#endif

#ifdef VIA_ENABLE

// Function prototypes
static void     hybrid_save_threshold_data(uint8_t option);
static void     hybrid_save_bottoming_calibration_reading(void);
static void     hybrid_show_calibration_data(void);
static void     hybrid_clear_bottoming_calibration_data(void);
static void     factory_reset(void);


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
    id_actuation_mode = 1,
    id_apc_actuation_threshold = 2,
    id_apc_release_threshold = 3,
    id_save_threshold_data = 4,
    id_rt_initial_deadzone_offset = 5,
    id_rt_actuation_offset = 6,
    id_rt_release_offset = 7,
    id_bottoming_calibration = 8,
    id_noise_floor_calibration = 9,
    id_show_calibration_data = 10,
    id_clear_bottoming_calibration_data = 11,
    id_switch_type = 12,
    id_flash_mode = 13,
    id_factory_reset = 14,
    // clang-format on
};

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

    switch (*value_id) {
        case id_switch_type: {
            uint8_t value = value_data[0];
            // Update only the per-key switch_type field in runtime and EEPROM (shared offset)
            update_keys_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, switch_type), 0, &value, sizeof(uint8_t));
            eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
            if (value == 0) {
                uprintf("###################\n");
                uprintf("# Switch Type: EC #\n");
                uprintf("###################\n");
            } else if (value == 1) {
                uprintf("###################\n");
                uprintf("# Switch Type: MX #\n");
                uprintf("###################\n");
            }
            break;
        }
        case id_actuation_mode: {
            uint8_t value = value_data[0];
            // Update only the per-key actuation_mode field in runtime and EEPROM (shared offset)
            update_keys_field(HYBRID_UPDATE_SHARED_OFFSET, offsetof(runtime_key_state_t, actuation_mode), 0, &value, sizeof(uint8_t));
            eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
            if (value == 0) {
                uprintf("#######################\n");
                uprintf("# Actuation Mode: APC #\n");
                uprintf("#######################\n");
            } else if (value == 1) {
                uprintf("#################################\n");
                uprintf("# Actuation Mode: Rapid Trigger #\n");
                uprintf("#################################\n");
            }
            break;
        }
        case id_apc_actuation_threshold: {
            uint16_t value = value_data[1] | (value_data[0] << 8);
            update_keys_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, apc_actuation_threshold), 0, &value, sizeof(uint16_t));
            uprintf("APC Mode Actuation Threshold: %d\n", value);
            break;
        }
        case id_apc_release_threshold: {
            uint16_t value = value_data[1] | (value_data[0] << 8);
            update_keys_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, apc_release_threshold), 0, &value, sizeof(uint16_t));
            uprintf("APC Mode Release Threshold: %d\n", value);
            break;
        }
        case id_rt_initial_deadzone_offset: {
            uint16_t value = value_data[1] | (value_data[0] << 8);
            update_keys_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_initial_deadzone_offset), 0, &value, sizeof(uint16_t));
            uprintf("Rapid Trigger Mode Initial Deadzone Offset: %d\n", value);
            break;
        }
        case id_rt_actuation_offset: {
            uint8_t value = value_data[0];
            update_keys_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_actuation_offset), 0, &value, sizeof(uint8_t));
            uprintf("Rapid Trigger Mode Actuation Offset: %d\n", value);
            break;
        }
        case id_rt_release_offset: {
            uint8_t value = value_data[0];
            update_keys_field(HYBRID_UPDATE_RUNTIME_ONLY, offsetof(runtime_key_state_t, rt_release_offset), 0, &value, sizeof(uint8_t));
            uprintf("Rapid Trigger Mode Release Offset: %d\n", value);
            break;
        }
        case id_bottoming_calibration: {
            uint8_t value = value_data[0];
            // 0: stop calibration and save, 1: start calibration
            if (value == 1) {
                // Set the bottoming calibration flag to true
                runtime_hybrid_config.bottoming_calibration = true;
                clear_keyboard();
                uprintf("###########################\n");
                uprintf("# Calibration In Progress #\n");
                uprintf("###########################\n");
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
                uprintf("######################################\n");
                uprintf("# Resting Position Readings Acquired #\n");
                uprintf("######################################\n");
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

// Handle the data sent by the keyboard to the VIA menus
void via_config_get_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);
    // Pointer to the first key's runtime state
    // Hardcoded to [0][0] as for now every key has the same config
    runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[0][0];

    switch (*value_id) {
        case id_switch_type: {
            value_data[0] = key_runtime->switch_type;
            break;
        }
        case id_actuation_mode: {
            value_data[0] = key_runtime->actuation_mode;
            break;
        }
        case id_apc_actuation_threshold: {
            value_data[0] = key_runtime->apc_actuation_threshold >> 8;
            value_data[1] = key_runtime->apc_actuation_threshold & 0xFF;
            break;
        }
        case id_apc_release_threshold: {
            value_data[0] = key_runtime->apc_release_threshold >> 8;
            value_data[1] = key_runtime->apc_release_threshold & 0xFF;
            break;
        }
        case id_rt_initial_deadzone_offset: {
            value_data[0] = key_runtime->rt_initial_deadzone_offset >> 8;
            value_data[1] = key_runtime->rt_initial_deadzone_offset & 0xFF;
            break;
        }
        case id_rt_actuation_offset: {
            value_data[0] = key_runtime->rt_actuation_offset;
            break;
        }
        case id_rt_release_offset: {
            value_data[0] = key_runtime->rt_release_offset;
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
    }
    // Save to EEPROM the eeprom_key_state field
    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
    uprintf("####################################\n");
    uprintf("# New thresholds applied and saved #\n");
    uprintf("####################################\n");
}

// Handle the application of the bottoming calibration data and save to EEPROM
static void hybrid_save_bottoming_calibration_reading(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            // Get pointer to key state in runtime and EEPROM
            runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[row][col];
            eeprom_key_state_t  *key_eeprom  = &eeprom_hybrid_config.eeprom_key_state[row][col];

            // Validate bottoming calibration reading before saving:
            // 1. If starter flag is still true: key never exceeded noise_floor + threshold during calibration
            //    → Key was not pressed or is physically absent → save 1023 (max ADC value)
            // 2. If starter flag is false but reading is below noise_floor + threshold: weak/invalid reading
            //    → Likely unpressed alternative layout key or noise spike during init → save 1023
            // 3. Otherwise: valid bottom-out peak captured → save actual reading
            // Setting 1023 for invalid keys ensures their rescaled thresholds don't become unreasonably low
            if (key_runtime->bottoming_calibration_starter || key_runtime->bottoming_calibration_reading < (key_runtime->noise_floor + BOTTOMING_CALIBRATION_THRESHOLD)) {
                // Save max ADC value for invalid/no-press keys
                key_runtime->bottoming_calibration_reading = 1023;
                key_eeprom->bottoming_calibration_reading  = 1023;
                // Rescale thresholds based on max bottoming value
                bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_ALL);
            } else {
                // Save the captured bottoming calibration reading
                key_eeprom->bottoming_calibration_reading = key_runtime->bottoming_calibration_reading;
                // Rescale all key thresholds based on new bottoming reading
                bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_ALL);
            }
        }
    }
    // Save to EEPROM the eeprom_key_state field
    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);
}

// Show the calibration data
static void hybrid_show_calibration_data(void) {
    uprintf("##################\n");
    uprintf("# Actuation Mode #\n");
    uprintf("##################\n");
    calibration_print_layout_field(CAL_PRINT_ACTUATION_MODE);

    uprintf("#############################\n");
    uprintf("# Resting Position Readings #\n");
    uprintf("#############################\n");
    calibration_print_layout_field(CAL_PRINT_NOISE_FLOOR);

    uprintf("############\n");
    uprintf("# Extremum #\n");
    uprintf("############\n");
    calibration_print_layout_field(CAL_PRINT_EXTREMUM);

    uprintf("########################\n");
    uprintf("# Calibration Readings #\n");
    uprintf("########################\n");
    calibration_print_layout_field(CAL_PRINT_BOTTOMING);

    uprintf("################################\n");
    uprintf("# APC Mode Actuation Threshold #\n");
    uprintf("################################\n");
    uprintf("Original Value: %4d\n", eeprom_hybrid_config.eeprom_key_state[0][0].apc_actuation_threshold);
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_APC_ACTUATION);

    uprintf("##############################\n");
    uprintf("# APC Mode Release Threshold #\n");
    uprintf("##############################\n");
    uprintf("Original Value: %4d\n", eeprom_hybrid_config.eeprom_key_state[0][0].apc_release_threshold);
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_APC_RELEASE);

    uprintf("##############################################\n");
    uprintf("# Rapid Trigger Mode Initial Deadzone Offset #\n");
    uprintf("##############################################\n");
    uprintf("Original Value: %4d\n", eeprom_hybrid_config.eeprom_key_state[0][0].rt_initial_deadzone_offset);
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_RT_INITIAL_DEADZONE);

    uprintf("#######################################\n");
    uprintf("# Rapid Trigger Mode Actuation Offset #\n");
    uprintf("#######################################\n");
    uprintf("Original Value: %4d\n", eeprom_hybrid_config.eeprom_key_state[0][0].rt_actuation_offset);
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_RT_ACTUATION);

    uprintf("#####################################\n");
    uprintf("# Rapid Trigger Mode Release Offset #\n");
    uprintf("#####################################\n");
    uprintf("Original Value: %4d\n", eeprom_hybrid_config.eeprom_key_state[0][0].rt_release_offset);
    uprintf("Rescaled Values:\n");
    calibration_print_layout_field(CAL_PRINT_RT_RELEASE);
    print("\n");
}

// Clear the calibration data readings
static void hybrid_clear_bottoming_calibration_data(void) {
    // Clear the EEPROM Calibration Data Readings only
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            // Get pointer to key state in runtime and EEPROM
            runtime_key_state_t *key_runtime = &runtime_hybrid_config.runtime_key_state[row][col];
            eeprom_key_state_t  *key_eeprom  = &eeprom_hybrid_config.eeprom_key_state[row][col];

            // Default the runtime and eeprom values to the default bottoming value
            key_runtime->bottoming_calibration_reading = DEFAULT_BOTTOMING_CALIBRATION_READING;
            key_runtime->bottoming_calibration_starter = DEFAULT_CALIBRATION_STARTER;
            key_eeprom->bottoming_calibration_reading  = DEFAULT_BOTTOMING_CALIBRATION_READING;

            // Rescale thresholds based on default bottoming value
            bulk_rescale_key_thresholds(key_runtime, key_eeprom, RESCALE_MODE_ALL);
        }
    }
    // Save to EEPROM the eeprom_key_state field
    eeconfig_update_kb_datablock_field(eeprom_hybrid_config, eeprom_key_state);

    uprintf("############################\n");
    uprintf("# Calibration Data Cleared #\n");
    uprintf("############################\n");
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
