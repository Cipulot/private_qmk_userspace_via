VIA_ENABLE = yes
MOUSEKEY_ENABLE = no
SPACE_CADET_ENABLE = no

include keyboards/cipulot/common/mx_config.mk
include keyboards/cipulot/common/rgb_indicator.mk

AUTO_SHIFT_ENABLE = no

SRC += keyboards/cipulot/lily/lily_via.c
