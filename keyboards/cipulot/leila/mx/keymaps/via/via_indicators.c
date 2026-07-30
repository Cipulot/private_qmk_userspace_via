/* Copyright 2026 Cipulot
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mx.h"
#include "action.h"
#include "print.h"
#include "via.h"

#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#    include "usb_descriptor.h"
#endif

#ifdef VIA_ENABLE

static void factory_reset(void);
static bool decode_socd_value_id(uint8_t value_id, uint8_t *pair_index, socd_config_field_t *field);

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

static const cipulot_rgb_indicator_via_ids_t indicator_ids[] = {
    {.enabled = id_ind1_enabled, .brightness = id_ind1_brightness, .color = id_ind1_color, .function = id_ind1_func},
    {.enabled = id_ind2_enabled, .brightness = id_ind2_brightness, .color = id_ind2_color, .function = id_ind2_func},
    {.enabled = id_ind3_enabled, .brightness = id_ind3_brightness, .color = id_ind3_color, .function = id_ind3_func},
};

static const cipulot_rgb_indicator_context_t indicator_context = {
    .indicators      = eeprom_mx_config.indicators,
    .count           = ARRAY_SIZE(indicator_ids),
    .ids             = indicator_ids,
    .kb_storage_base = &eeprom_mx_config,
    .apply           = leila_mx_indicator_extension_apply,
};

_Static_assert(ARRAY_SIZE(indicator_ids) == LEILA_MX_INDICATOR_COUNT, "Leila MX VIA map must cover every indicator");

void via_config_set_value(uint8_t *data) {
    uint8_t            *value_id   = &(data[0]);
    uint8_t            *value_data = &(data[1]);
    uint8_t             pair_index;
    socd_config_field_t socd_field;

#    ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        transaction_rpc_send(RPC_ID_VIA_CMD, RAW_EPSIZE - 2, data);
    }
#    endif

    if (cipulot_rgb_indicator_set_value(&indicator_context, *value_id, value_data)) {
        return;
    }

    if (decode_socd_value_id(*value_id, &pair_index, &socd_field)) {
        uint16_t value = socd_field == SOCD_CONFIG_FIELD_RESOLUTION ? value_data[0] : value_data[1] | ((uint16_t)value_data[0] << 8);
        socd_config_set_pair_field(pair_index, socd_field, value);
        return;
    }

    switch (*value_id) {
        case id_flash_mode:
            if (value_data[0] == 0) {
                reset_keyboard();
            }
            break;
        case id_factory_reset:
            if (value_data[0] == 0) {
                factory_reset();
            }
            break;
        default:
            break;
    }
}

void via_config_get_value(uint8_t *data) {
    uint8_t            *value_id   = &(data[0]);
    uint8_t            *value_data = &(data[1]);
    uint8_t             pair_index;
    uint16_t            socd_value;
    socd_config_field_t socd_field;

    if (cipulot_rgb_indicator_get_value(&indicator_context, *value_id, value_data)) {
        return;
    }

    if (decode_socd_value_id(*value_id, &pair_index, &socd_field) && socd_config_get_pair_field(pair_index, socd_field, &socd_value)) {
        if (socd_field == SOCD_CONFIG_FIELD_RESOLUTION) {
            value_data[0] = socd_value;
        } else {
            value_data[0] = socd_value >> 8;
            value_data[1] = socd_value & 0xFF;
        }
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id        = &(data[0]);
    uint8_t *channel_id        = &(data[1]);
    uint8_t *value_id_and_data = &(data[2]);

    if (*channel_id == id_custom_channel) {
        switch (*command_id) {
            case id_custom_set_value:
                via_config_set_value(value_id_and_data);
                break;
            case id_custom_get_value:
                via_config_get_value(value_id_and_data);
                break;
            case id_custom_save:
                break;
            default:
                *command_id = id_unhandled;
                break;
        }
        return;
    }

    *command_id = id_unhandled;
}

static bool decode_socd_value_id(uint8_t value_id, uint8_t *pair_index, socd_config_field_t *field) {
    if (value_id < id_socd_pair_1_mode || value_id > id_socd_pair_4_key_2) {
        return false;
    }

    uint8_t offset = value_id - id_socd_pair_1_mode;
    *pair_index    = offset / 3;
    *field         = (socd_config_field_t)(offset % 3);
    return true;
}

static void factory_reset(void) {
    eeconfig_init_kb();
    keyboard_post_init_kb();

    uprintf("###################################################################\n");
    uprintf("# Factory Reset Performed                                         #\n");
    uprintf("# Unplug the board and plug it back in to complete the procedure. #\n");
    uprintf("###################################################################\n");
}

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
