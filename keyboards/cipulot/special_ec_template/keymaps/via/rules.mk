VIA_ENABLE = yes

OPT_DEFS += -DCIPULOT_EC_RUNTIME_FEATURES_ENABLE
OPT_DEFS += -DCIPULOT_EC_SWITCH_MATRIX_HEADER=\"keyboards/cipulot/special_ec_template/ec_switch_matrix.h\"

include keyboards/cipulot/common/config_services.mk
include keyboards/cipulot/common/runtime_features.mk

SRC += keyboards/cipulot/common/general/ec/ec_config.c
SRC += keyboards/cipulot/common/general/ec/runtime/ec_runtime_features.c
SRC += keyboards/cipulot/common/shared/runtime/feature_config.c
SRC += keyboards/cipulot/common/shared/via/via_ec_adapter.c
SRC += keyboards/cipulot/common/shared/via/via_ec_values.c
SRC += keyboards/cipulot/special_ec_template/via_special_ec_template.c
