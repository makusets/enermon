import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome import pins
from esphome.const import (
    CONF_ID,
)

DEPENDENCIES = []
AUTO_LOAD = ["sensor"]

enermon_ns = cg.esphome_ns.namespace("enermon")
Enermon = enermon_ns.class_("Enermon", cg.Component)

CONF_CT_PINS = "ct_pins"
CONF_A_PER_V = "a_per_v"
CONF_CAL_GAIN = "cal_gain"
CONF_CAL_OFFSET = "cal_offset"
CONF_CT_CAL = "ct_cal"
CONF_CT_NAMES = "ct_names"
CONF_VOLTAGE_PIN = "voltage_pin"
CONF_VOLTAGE_DIVIDER = "voltage_divider"
CONF_VOLTAGE_CAL = "voltage_cal"
CONF_VOLTAGE_PHASE = "voltage_phase"
CONF_SAMPLE_COUNT = "sample_count"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Enermon),
            cv.Optional(CONF_CT_PINS, default=[]): cv.ensure_list(cv.int_),
            cv.Optional(CONF_A_PER_V, default=[]): cv.ensure_list(cv.float_),
            cv.Optional(CONF_CAL_GAIN, default=[]): cv.ensure_list(cv.float_),
            cv.Optional(CONF_CAL_OFFSET, default=[]): cv.ensure_list(cv.float_),
            cv.Optional(CONF_CT_CAL, default=[]): cv.ensure_list(cv.float_),
            cv.Optional(CONF_CT_NAMES, default=[]): cv.ensure_list(cv.string),
            cv.Optional(CONF_VOLTAGE_PIN, default=None): cv.int_,
            cv.Optional(CONF_VOLTAGE_DIVIDER, default=80.0): cv.float_,
            cv.Optional(CONF_VOLTAGE_CAL, default=23426.0): cv.float_,
            cv.Optional(CONF_VOLTAGE_PHASE, default=0): cv.int_,
            cv.Optional(CONF_SAMPLE_COUNT, default=200): cv.positive_int,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    ct_pins = config.get(CONF_CT_PINS, [])
    a_per_v = config.get(CONF_A_PER_V, [])
    cal_gain = config.get(CONF_CAL_GAIN, [])
    cal_offset = config.get(CONF_CAL_OFFSET, [])
    ct_cal = config.get(CONF_CT_CAL, [])
    ct_names = config.get(CONF_CT_NAMES, [])
    voltage_pin = config.get(CONF_VOLTAGE_PIN, -1)
    voltage_divider = config.get(CONF_VOLTAGE_DIVIDER, 80.0)
    voltage_cal = config.get(CONF_VOLTAGE_CAL, 23426.0)
    voltage_phase = config.get(CONF_VOLTAGE_PHASE, 0)
    sample_count = config.get(CONF_SAMPLE_COUNT, 200)

    int_vector = cg.std_vector_template(cg.int_)
    float_vector = cg.std_vector_template(cg.float_)

    c_ct_pins = int_vector()
    for p in ct_pins:
        c_ct_pins.push_back(p)

    c_a_per_v = float_vector()
    for v in a_per_v:
        c_a_per_v.push_back(v)

    c_cal_gain = float_vector()
    for g in cal_gain:
        c_cal_gain.push_back(g)

    c_cal_offset = float_vector()
    for o in cal_offset:
        c_cal_offset.push_back(o)

    c_ct_cal = float_vector()
    for cc in ct_cal:
        c_ct_cal.push_back(cc)

    var = cg.new_Pvariable(config[CONF_ID], c_ct_pins, c_a_per_v, c_cal_gain, c_cal_offset, c_ct_cal, voltage_pin, voltage_divider, voltage_cal, voltage_phase, sample_count)
    await cg.register_component(var, config)

    for i in range(4):
        name_base = f"CT {i+1}"
        if i < len(ct_names) and ct_names[i]:
            name_base = ct_names[i]

        current = sensor.new_sensor(config)
        current.set_parent(var)
        current.set_name(f"{name_base} Current RMS")
        current.set_unit_of_measurement("A")
        cg.add(var.sensor_current_rms[i].assign(current))

        power = sensor.new_sensor(config)
        power.set_parent(var)
        power.set_name(f"{name_base} Active Power")
        power.set_unit_of_measurement("W")
        cg.add(var.sensor_power_w[i].assign(power))

        ed = sensor.new_sensor(config)
        ed.set_parent(var)
        ed.set_name(f"{name_base} Energy Today")
        ed.set_unit_of_measurement("Wh")
        cg.add(var.sensor_energy_daily_wh[i].assign(ed))

        ew = sensor.new_sensor(config)
        ew.set_parent(var)
        ew.set_name(f"{name_base} Energy Week")
        ew.set_unit_of_measurement("Wh")
        cg.add(var.sensor_energy_weekly_wh[i].assign(ew))

        em = sensor.new_sensor(config)
        em.set_parent(var)
        em.set_name(f"{name_base} Energy Month")
        em.set_unit_of_measurement("Wh")
        cg.add(var.sensor_energy_monthly_wh[i].assign(em))

    voltage = sensor.new_sensor(config)
    voltage.set_parent(var)
    voltage.set_name("Line Voltage RMS")
    voltage.set_unit_of_measurement("V")
    cg.add(var.sensor_voltage_rms.assign(voltage))

    wifi = sensor.new_sensor(config)
    wifi.set_parent(var)
    wifi.set_name("WiFi RSSI")
    wifi.set_unit_of_measurement("dBm")
    cg.add(var.sensor_wifi_rssi.assign(wifi))