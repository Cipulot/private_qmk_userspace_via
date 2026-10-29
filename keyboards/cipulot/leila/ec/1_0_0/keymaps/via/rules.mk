VIA_ENABLE = yes
EC_CUSTOM_VIA_ADAPTER = yes

include keyboards/cipulot/common/ec_runtime_features.mk

SRC += keyboards/cipulot/common/extensions/ec/via_ec_3rgb.c
