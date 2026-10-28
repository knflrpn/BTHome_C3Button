import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.esp32 import add_idf_sdkconfig_option

try:
    from esphome.components.esp32 import include_builtin_idf_component
except ImportError:  # older ESPHome didn't exclude IDF components
    include_builtin_idf_component = None

DEPENDENCIES = ["esp32"]
CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    if include_builtin_idf_component is not None:
        include_builtin_idf_component("bt")
    add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
    add_idf_sdkconfig_option("CONFIG_BT_CONTROLLER_ONLY", True)
    cg.add_global(
        cg.RawStatement('#include "esphome/components/bthome_c3button/bthome_c3button.h"')
    )