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

#include "ec_switch_matrix.h"
#include "generated_calibration_layout.h"
#include "action.h"
#include "print.h"
#include "via.h"
#include "version.h"
#include "usb_descriptor.h"
#include <assert.h>
#include <string.h>

#ifdef VIA_ENABLE

#    ifndef RAW_EPSIZE
#        error RAW_EPSIZE must be defined to size VIA text value responses
#    endif

#    define EC_VIA_TEXT_VALUE_MAX_LEN (RAW_EPSIZE - 4)

_Static_assert(EC_VIA_TEXT_VALUE_MAX_LEN > 0, "RAW_EPSIZE is too small for VIA text value responses");
_Static_assert(EC_VIA_TEXT_VALUE_MAX_LEN <= 29, "Unexpected RAW_EPSIZE for VIA text value responses; review app-side text decoding limits");

void ec_rescale_values(uint8_t item);
static void ec_get_firmware_version_text(uint8_t *value_data) {
    uint32_t value = (VIA_FIRMWARE_VERSION == 0) ? 1 : VIA_FIRMWARE_VERSION;
    char     text[11];
    uint8_t  text_len = 0;

    if (value == 0) {
        text[text_len++] = '0';
    } else {
        while (value > 0 && text_len < sizeof(text)) {
            text[text_len++] = '0' + (value % 10);
            value /= 10;
        }
    }

    uint8_t out_len = 0;
    while (text_len > 0 && out_len < EC_VIA_TEXT_VALUE_MAX_LEN) {
        value_data[out_len++] = text[--text_len];
    }
    value_data[out_len] = 0;
}

static void ec_get_build_text(uint8_t *value_data) {
    strncpy((char *)value_data, QMK_BUILDDATE, EC_VIA_TEXT_VALUE_MAX_LEN);
    value_data[EC_VIA_TEXT_VALUE_MAX_LEN] = 0;
}

void ec_save_threshold_data(uint8_t option);
void ec_save_bottoming_reading(void);
void ec_show_calibration_data(void);
void ec_clear_bottoming_calibration_data(void);


static void     ec_get_firmware_version_text(uint8_t *value_data);
static void     ec_get_build_text(uint8_t *value_data);

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
    switch (field) {
        case CAL_PRINT_APC_ACTUATION:
            return ec_config.rescaled_mode_0_actuation_threshold;
        case CAL_PRINT_APC_RELEASE:
            return ec_config.rescaled_mode_0_release_threshold;
        case CAL_PRINT_RT_INITIAL_DEADZONE:
            return ec_config.rescaled_mode_1_initial_deadzone_offset;
        case CAL_PRINT_NOISE_FLOOR:
            return ec_config.noise_floor;
        case CAL_PRINT_BOTTOMING:
            return eeprom_ec_config.bottoming_reading;
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
    id_mode_0_actuation_threshold = 2,
    id_mode_0_release_threshold = 3,
    id_save_threshold_data = 4,
    id_mode_1_initial_deadzone_offset = 5,
    id_mode_1_actuation_offset = 6,
    id_mode_1_release_offset = 7,
    id_bottoming_calibration = 8,
    id_noise_floor_calibration = 9,
    id_show_calibration_data = 10,
    id_clear_bottoming_calibration_data = 11,
    id_firmware_version_text = 12,
    id_firmware_build_text = 13,
    // clang-format on
};

// Handle the data received by the keyboard from the VIA menus
void via_config_set_value(uint8_t *data) {
    // data = [ value_id, value_data ]
    uint8_t *value_id   = &(data[0]);
    uint8_t *value_data = &(data[1]);

    switch (*value_id) {
        case id_actuation_mode: {
            eeprom_ec_config.actuation_mode = value_data[0];
            ec_config.actuation_mode        = eeprom_ec_config.actuation_mode;
            if (ec_config.actuation_mode == 0) {
                uprintf("#########################\n");
                uprintf("#  Actuation Mode: APC  #\n");
                uprintf("#########################\n");
            } else if (ec_config.actuation_mode == 1) {
                uprintf("#################################\n");
                uprintf("# Actuation Mode: Rapid Trigger #\n");
                uprintf("#################################\n");
            }
            eeconfig_update_kb_datablock_field(eeprom_ec_config, actuation_mode);
            break;
        }
        case id_mode_0_actuation_threshold: {
            ec_config.mode_0_actuation_threshold = value_data[1] | (value_data[0] << 8);
            uprintf("APC Mode Actuation Threshold: %d\n", ec_config.mode_0_actuation_threshold);
            break;
        }
        case id_mode_0_release_threshold: {
            ec_config.mode_0_release_threshold = value_data[1] | (value_data[0] << 8);
            uprintf("APC Mode Release Threshold: %d\n", ec_config.mode_0_release_threshold);
            break;
        }
        case id_mode_1_initial_deadzone_offset: {
            ec_config.mode_1_initial_deadzone_offset = value_data[1] | (value_data[0] << 8);
            uprintf("Rapid Trigger Mode Initial Deadzone Offset: %d\n", ec_config.mode_1_initial_deadzone_offset);
            break;
        }
        case id_mode_1_actuation_offset: {
            ec_config.mode_1_actuation_offset = value_data[0];
            uprintf("Rapid Trigger Mode Actuation Offset: %d\n", ec_config.mode_1_actuation_offset);
            break;
        }
        case id_mode_1_release_offset: {
            ec_config.mode_1_release_offset = value_data[0];
            uprintf("Rapid Trigger Mode Release Offset: %d\n", ec_config.mode_1_release_offset);
            break;
        }
        case id_bottoming_calibration: {
            if (value_data[0] == 1) {
                ec_config.bottoming_calibration = true;
                uprintf("##############################\n");
                uprintf("# Calibration In Progress #\n");
                uprintf("##############################\n");
            } else {
                ec_config.bottoming_calibration = false;
                ec_save_bottoming_reading();
                uprintf("## Calibration Completed ##\n");
                ec_show_calibration_data();
            }
            break;
        }
        case id_save_threshold_data: {
            ec_save_threshold_data(value_data[0]);
            break;
        }
        case id_noise_floor_calibration: {
            if (value_data[0] == 0) {
                ec_noise_floor();
                ec_rescale_values(0);
                ec_rescale_values(1);
                ec_rescale_values(2);
                ec_rescale_values(3);
                ec_rescale_values(4);
                uprintf("#############################\n");
                uprintf("# Resting Position Readings Acquired #\n");
                uprintf("#############################\n");
                break;
            }
        }
        case id_show_calibration_data: {
            if (value_data[0] == 0) {
                ec_show_calibration_data();
            }
            break;
        }
        case id_clear_bottoming_calibration_data: {
            if (value_data[0] == 0) {
                ec_clear_bottoming_calibration_data();
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

    switch (*value_id) {
        case id_actuation_mode: {
            value_data[0] = eeprom_ec_config.actuation_mode;
            break;
        }
        case id_mode_0_actuation_threshold: {
            value_data[0] = eeprom_ec_config.mode_0_actuation_threshold >> 8;
            value_data[1] = eeprom_ec_config.mode_0_actuation_threshold & 0xFF;
            break;
        }
        case id_mode_0_release_threshold: {
            value_data[0] = eeprom_ec_config.mode_0_release_threshold >> 8;
            value_data[1] = eeprom_ec_config.mode_0_release_threshold & 0xFF;
            break;
        }
        case id_mode_1_initial_deadzone_offset: {
            value_data[0] = eeprom_ec_config.mode_1_initial_deadzone_offset >> 8;
            value_data[1] = eeprom_ec_config.mode_1_initial_deadzone_offset & 0xFF;
            break;
        }
        case id_mode_1_actuation_offset: {
            value_data[0] = eeprom_ec_config.mode_1_actuation_offset;
            break;
        }
        case id_mode_1_release_offset: {
            value_data[0] = eeprom_ec_config.mode_1_release_offset;
            break;
        }
        case id_firmware_version_text:
            ec_get_firmware_version_text(value_data);
            break;
        case id_firmware_build_text:
            ec_get_build_text(value_data);
            break;
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

// Rescale the values received by VIA to fit the new range
void ec_rescale_values(uint8_t item) {
    switch (item) {
        // Rescale the APC mode actuation thresholds
        case 0:
            ec_config.rescaled_mode_0_actuation_threshold = rescale(ec_config.mode_0_actuation_threshold, ec_config.noise_floor, eeprom_ec_config.bottoming_reading);
            break;
        // Rescale the APC mode release thresholds
        case 1:
            ec_config.rescaled_mode_0_release_threshold = rescale(ec_config.mode_0_release_threshold, ec_config.noise_floor, eeprom_ec_config.bottoming_reading);
            break;
        // Rescale the Rapid Trigger mode initial deadzone offsets
        case 2:
            ec_config.rescaled_mode_1_initial_deadzone_offset = rescale(ec_config.mode_1_initial_deadzone_offset, ec_config.noise_floor, eeprom_ec_config.bottoming_reading);
            break;
        // Rescale the Rapid Trigger mode actuation offsets
        case 3:
            ec_config.rescaled_mode_1_actuation_offset = rescale(ec_config.mode_1_actuation_offset, ec_config.noise_floor, eeprom_ec_config.bottoming_reading);
            break;
        // Rescale the Rapid Trigger mode release offsets
        case 4:
            ec_config.rescaled_mode_1_release_offset = rescale(ec_config.mode_1_release_offset, ec_config.noise_floor, eeprom_ec_config.bottoming_reading);
            break;

        default:
            // Unhandled item.
            break;
    }
}

void ec_save_threshold_data(uint8_t option) {
    // Save APC mode thresholds and rescale them for runtime usage
    if (option == 0) {
        eeprom_ec_config.mode_0_actuation_threshold = ec_config.mode_0_actuation_threshold;
        eeprom_ec_config.mode_0_release_threshold   = ec_config.mode_0_release_threshold;
        ec_rescale_values(0);
        ec_rescale_values(1);
    }
    // Save Rapid Trigger mode thresholds and rescale them for runtime usage
    else if (option == 1) {
        eeprom_ec_config.mode_1_initial_deadzone_offset = ec_config.mode_1_initial_deadzone_offset;
        eeprom_ec_config.mode_1_actuation_offset        = ec_config.mode_1_actuation_offset;
        eeprom_ec_config.mode_1_release_offset          = ec_config.mode_1_release_offset;
        ec_rescale_values(2);
        ec_rescale_values(3);
        ec_rescale_values(4);
    }
    eeconfig_update_kb_datablock(&eeprom_ec_config, 0, EECONFIG_KB_DATA_SIZE);
    uprintf("####################################\n");
    uprintf("# New thresholds applied and saved #\n");
    uprintf("####################################\n");
}

// Save the bottoming reading
void ec_save_bottoming_reading(void) {
    // If the calibration starter flag is still set on the key, it indicates that the key was skipped during the scan because it is not physically present.
    // If the flag is not set, it means a bottoming reading was taken. If this reading doesn't exceed the noise floor by the BOTTOMING_CALIBRATION_THRESHOLD, it likely indicates one of the following:
    // 1. The key is part of an alternative layout and is not being pressed.
    // 2. The key is in the current layout but is not being pressed.
    // In both conditions we should set the bottoming reading to the maximum value to avoid false positives.
    if (ec_config.bottoming_calibration_starter || ec_config.bottoming_reading < (ec_config.noise_floor + BOTTOMING_CALIBRATION_THRESHOLD)) {
        eeprom_ec_config.bottoming_reading = 1023;
    } else {
        eeprom_ec_config.bottoming_reading = ec_config.bottoming_reading;
    }
    // Rescale the values to fit the new range for runtime usage
    ec_rescale_values(0);
    ec_rescale_values(1);
    ec_rescale_values(2);
    ec_rescale_values(3);
    ec_rescale_values(4);
    eeconfig_update_kb_datablock(&eeprom_ec_config, 0, EECONFIG_KB_DATA_SIZE);
}

// Show the calibration data
void ec_show_calibration_data(void) {
    uprintf("\n###############\n");
    uprintf("# Resting Position Readings #\n");
    uprintf("###############\n");
    calibration_print_layout_field(CAL_PRINT_NOISE_FLOOR);

    uprintf("\n######################\n");
    uprintf("# Calibration Readings #\n");
    uprintf("######################\n");
    calibration_print_layout_field(CAL_PRINT_BOTTOMING);

    uprintf("\n######################################\n");
    uprintf("# Rescaled APC Mode Actuation Points #\n");
    uprintf("######################################\n");
    uprintf("Original APC Mode Actuation Point: %4d\n", ec_config.mode_0_actuation_threshold);
    calibration_print_layout_field(CAL_PRINT_APC_ACTUATION);

    uprintf("\n######################################\n");
    uprintf("# Rescaled APC Mode Release Points   #\n");
    uprintf("######################################\n");
    uprintf("Original APC Mode Release Point: %4d\n", ec_config.mode_0_release_threshold);
    calibration_print_layout_field(CAL_PRINT_APC_RELEASE);

    uprintf("\n#######################################################\n");
    uprintf("# Rescaled Rapid Trigger Mode Initial Deadzone Offset #\n");
    uprintf("#######################################################\n");
    uprintf("Original Rapid Trigger Mode Initial Deadzone Offset: %4d\n", ec_config.mode_1_initial_deadzone_offset);
    calibration_print_layout_field(CAL_PRINT_RT_INITIAL_DEADZONE);
    print("\n");
}

// Clear the calibration data
void ec_clear_bottoming_calibration_data(void) {
    // Clear the EEPROM data
    eeconfig_init_kb();

    // Reset the runtime values to the EEPROM values
    keyboard_post_init_kb();

    uprintf("######################################\n");
    uprintf("# Calibration Data Cleared #\n");
    uprintf("######################################\n");
}

#endif // VIA_ENABLE
