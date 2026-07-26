# Special EC template VIA keymap

This keymap demonstrates a special EC board with twelve board-channel values
for three configurable indicators followed by shifted EC/SOCD/System IDs.
Channels 7-10 remain owned by the shared runtime-feature implementation.

For a new board, change the channel-0 enum and explicit `indicator_ids[]` table
to match the board's published protocol. The shared indicator service owns
lookup, GET/SET, precise keyboard-EEPROM persistence, and standard rendering;
the board owns indicator defaults, LED indices, count, and RGB effect range. Do
not renumber an already published control. If the board needs user EEPROM beyond
`feature_config_t`, define one combined layout and update the load/save callbacks
deliberately.
