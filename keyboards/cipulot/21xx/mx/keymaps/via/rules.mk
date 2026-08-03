VIA_ENABLE = yes

include keyboards/cipulot/common/mx_config.mk
include keyboards/cipulot/common/gpio_indicator.mk
include keyboards/cipulot/common/solenoid_via.mk

SRC += keyboards/cipulot/21xx/mx/21xx_mx_via.c
