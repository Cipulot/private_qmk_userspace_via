VIA_ENABLE = yes

include keyboards/cipulot/common/extension.mk
include keyboards/cipulot/common/rgb_indicator.mk
include keyboards/cipulot/common/socd_config.mk

SRC += mx.c via_indicators.c
