import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_DECIBEL_MILLIWATT,
    UNIT_VOLT,
    UNIT_WATT,
    UNIT_WATT_HOUR,
)

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
            cv.Optional(CONF_VOLTAGE_CAL, default=230.0): cv.float_,
            cv.Optional(CONF_VOLTAGE_PHASE, default=1.7): cv.float_,
            cv.Optional(CONF_SAMPLE_COUNT, default=200): cv.positive_int,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    ct_pins = config.get(CONF_CT_PINS, [])
    ct_cal = config.get(CONF_CT_CAL, [])
    voltage_pin = config.get(CONF_VOLTAGE_PIN, -1)
    voltage_cal = config.get(CONF_VOLTAGE_CAL, 230.0)
    voltage_phase = config.get(CONF_VOLTAGE_PHASE, 1.7)
    sample_count = config.get(CONF_SAMPLE_COUNT, 200)
    ct_names = config.get(CONF_CT_NAMES, [])

    # Create the component instance (no ctor args)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    ct_pins = config.get(CONF_CT_PINS, [])
    ct_cal = config.get(CONF_CT_CAL, [])

    # Send each channel as simple ints/floats
    for i, pin in enumerate(ct_pins):
        cal = ct_cal[i] if i < len(ct_cal) else 1111.0
        cg.add(var.add_ct_channel(i, pin, cal))

    voltage_pin = config.get(CONF_VOLTAGE_PIN, -1)
    voltage_cal = config.get(CONF_VOLTAGE_CAL, 230.0)
    voltage_phase = config.get(CONF_VOLTAGE_PHASE, 1.7)
    sample_count = config.get(CONF_SAMPLE_COUNT, 200)

    cg.add(var.set_voltage_config(
        voltage_pin,
        voltage_cal,
        voltage_phase,
        sample_count
    ))

    # Auto-create sensors so they are exposed to HA without YAML declarations
    for i, pin in enumerate(ct_pins):
        base_name = ct_names[i] if i < len(ct_names) else f"CT{i}"

        current = await sensor.new_sensor({
            CONF_NAME: f"{base_name} Current",
            "unit_of_measurement": UNIT_AMPERE,
            "accuracy_decimals": 2,
            "device_class": DEVICE_CLASS_CURRENT,
            "state_class": STATE_CLASS_MEASUREMENT,
        })
        power = await sensor.new_sensor({
            CONF_NAME: f"{base_name} Power",
            "unit_of_measurement": UNIT_WATT,
            "accuracy_decimals": 1,
            "device_class": DEVICE_CLASS_POWER,
            "state_class": STATE_CLASS_MEASUREMENT,
        })
        energy_day = await sensor.new_sensor({
            CONF_NAME: f"{base_name} Energy Daily",
            "unit_of_measurement": UNIT_WATT_HOUR,
            "accuracy_decimals": 1,
            "device_class": DEVICE_CLASS_ENERGY,
            "state_class": STATE_CLASS_TOTAL_INCREASING,
        })
        energy_week = await sensor.new_sensor({
            CONF_NAME: f"{base_name} Energy Weekly",
            "unit_of_measurement": UNIT_WATT_HOUR,
            "accuracy_decimals": 1,
            "device_class": DEVICE_CLASS_ENERGY,
            "state_class": STATE_CLASS_TOTAL_INCREASING,
        })
        energy_month = await sensor.new_sensor({
            CONF_NAME: f"{base_name} Energy Monthly",
            "unit_of_measurement": UNIT_WATT_HOUR,
            "accuracy_decimals": 1,
            "device_class": DEVICE_CLASS_ENERGY,
            "state_class": STATE_CLASS_TOTAL_INCREASING,
        })

        cg.add(var.sensor_current_rms[i] = current)
        cg.add(var.sensor_power_w[i] = power)
        cg.add(var.sensor_energy_daily_wh[i] = energy_day)
        cg.add(var.sensor_energy_weekly_wh[i] = energy_week)
        cg.add(var.sensor_energy_monthly_wh[i] = energy_month)

    voltage_sensor = await sensor.new_sensor({
        CONF_NAME: "Mains Voltage",
        "unit_of_measurement": UNIT_VOLT,
        "accuracy_decimals": 1,
        "device_class": DEVICE_CLASS_VOLTAGE,
        "state_class": STATE_CLASS_MEASUREMENT,
    })
    wifi_rssi = await sensor.new_sensor({
        CONF_NAME: "WiFi RSSI",
        "unit_of_measurement": UNIT_DECIBEL_MILLIWATT,
        "accuracy_decimals": 0,
        "device_class": DEVICE_CLASS_SIGNAL_STRENGTH,
        "state_class": STATE_CLASS_MEASUREMENT,
    })
    cg.add(var.sensor_voltage_rms = voltage_sensor)
    cg.add(var.sensor_wifi_rssi = wifi_rssi)