# Special hybrid VIA template

This buildable version-1 example pairs with
`keyboards/cipulot/special_hybrid_template` in QMK and
`v3/cipulot/special_hybrid_template/special_hybrid_template.json` in the VIA
definition repository.

`via_hybrid.c` demonstrates a board-local channel-0 binding for twelve
individually configurable special positions plus a global group. It delegates
VIA dispatch, shared runtime channels, SOCD storage, calibration, protocol
encoding, and System actions to the common Cipulot services.

Before using it for a production board, replace the example hardware/storage
assumptions, assign a unique VID/PID pair, synchronize special-position order
with the JSON groups, and define the board's channel-0 compatibility boundary.
Do not reuse template PID `0x6C05`.
