VIA_ENABLE = yes
CONSOLE_ENABLE = yes

include keyboards/cipulot/common/runtime_features.mk
include keyboards/cipulot/common/analog_runtime_features.mk

SRC += keyboards/cipulot/common/shared/runtime/feature_config.c
SRC += keyboards/cipulot/common/shared/config/actuation_calibration.c
SRC += keyboards/cipulot/common/shared/config/socd_config.c
SRC += keyboards/cipulot/common/special/hybrid/via_hybrid_per_key_adapter.c
SRC += keyboards/cipulot/hybrid_enso_e/1_3_0/via_hybrid.c
