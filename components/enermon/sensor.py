import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome import pins
from esphome.const import CONF_ID

cg.add_library(
    name="EmonLib",
    repository="https://github.com/openenergymonitor/EmonLib.git",
    version=None,
)

DEPENDENCIES = []
AUTO_LOAD = []

enermon_ns = cg.esphome_ns.namespace("enermon")
Enermon = enermon_ns.class_("Enermon", cg.Component)

CONF_CT_PINS = "ct_pins"
CONF_CT_CAL = "ct_cal"
CONF_CT_NAMES = "ct_names"
CONF_VOLTAGE_PIN = "voltage_pin"
CONF_VOLTAGE_CAL = "voltage_cal"
CONF_VOLTAGE_PHASE = "voltage_phase"
CONF_SAMPLE_COUNT = "sample_count"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Enermon),
            cv.Optional(CONF_CT_PINS, default=[]): cv.ensure_list(cv.int_),
            cv.Optional(CONF_CT_CAL, default=[]): cv.ensure_list(cv.float_),
            cv.Optional(CONF_CT_NAMES, default=[]): cv.ensure_list(cv.string),
            cv.Optional(CONF_VOLTAGE_PIN, default=None): cv.int_,
            cv.Optional(CONF_VOLTAGE_CAL, default=23426.0): cv.float_,
            cv.Optional(CONF_VOLTAGE_PHASE, default=0): cv.int_,
            cv.Optional(CONF_SAMPLE_COUNT, default=200): cv.positive_int,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    ct_pins = config.get(CONF_CT_PINS, [])
    ct_cal = config.get(CONF_CT_CAL, [])
    voltage_pin = config.get(CONF_VOLTAGE_PIN, -1)
    voltage_cal = config.get(CONF_VOLTAGE_CAL, 23426.0)
    voltage_phase = config.get(CONF_VOLTAGE_PHASE, 0)
    sample_count = config.get(CONF_SAMPLE_COUNT, 200)

    int_vector = cg.std_vector(cg.int_)
    float_vector = cg.std_vector(cg.float_)

    c_ct_pins = int_vector()
    for p in ct_pins:
        c_ct_pins.push_back(p)

    c_ct_cal = float_vector()
    for cc in ct_cal:
        c_ct_cal.push_back(cc)

    var = cg.new_Pvariable(config[CONF_ID], c_ct_pins, c_ct_cal, voltage_pin, voltage_cal, voltage_phase, sample_count)
    cg.add(var)
