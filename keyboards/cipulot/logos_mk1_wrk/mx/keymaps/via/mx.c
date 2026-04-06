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

#include "quantum.h"
#include "mx.h"

eeprom_mx_config_t eeprom_mx_config;
socd_cleaner_t     socd_opposing_pairs[4];

// EEPROM default initialization
void eeconfig_init_kb(void) {

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

    // Copy default SOCD pairs to EEPROM
    for (int i = 0; i < 4; i++) {
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].keys[0]    = socd_pairs[i].key1;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].keys[1]    = socd_pairs[i].key2;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].resolution = SOCD_CLEANER_OFF;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].held[0]    = false;
        eeprom_mx_config.eeprom_socd_opposing_pairs[i].held[1]    = false;
    }

    // Write to EEPROM entire datablock
    eeconfig_update_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);

    // Call user initialization
    eeconfig_init_user();
}

// Keyboard post-initialization
void keyboard_post_init_kb(void) {
    // Read the EEPROM data block
    eeconfig_read_kb_datablock(&eeprom_mx_config, 0, EECONFIG_KB_DATA_SIZE);

    // Copy SOCD cleaner pairs to runtime instance
    memcpy(socd_opposing_pairs, eeprom_mx_config.eeprom_socd_opposing_pairs, sizeof(socd_opposing_pairs));

    // Call user post-initialization
    keyboard_post_init_user();
}
