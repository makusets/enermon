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
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    ct_pins = config.get(CONF_CT_PINS, [])
    ct_cal = config.get(CONF_CT_CAL, [])
    ct_names = config.get(CONF_CT_NAMES, [])
    voltage_pin = config.get(CONF_VOLTAGE_PIN, -1)
    voltage_cal = config.get(CONF_VOLTAGE_CAL, 230.0)
    voltage_phase = config.get(CONF_VOLTAGE_PHASE, 1.7)
    sample_count = config.get(CONF_SAMPLE_COUNT, 200)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Configure CT channels
    for i, pin in enumerate(ct_pins):
        cal = ct_cal[i] if i < len(ct_cal) else 30.0
        cg.add(var.add_ct_channel(i, pin, cal))

    # Voltage config
    cg.add(
        var.set_voltage_config(
            voltage_pin,
            voltage_cal,
            voltage_phase,
            sample_count,
        )
    )

    # Auto-create per-CT sensors so they are exposed to HA without YAML declarations
    for i, pin in enumerate(ct_pins):
        base_name = ct_names[i] if i < len(ct_names) else f"CT{i}"

        current_conf = sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        )({
            CONF_ID: cg.allocate_id(sensor.Sensor),
            CONF_NAME: f"{base_name} Current",
        })

        power_conf = sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        )({
            CONF_ID: cg.allocate_id(sensor.Sensor),
            CONF_NAME: f"{base_name} Power",
        })

        energy_day_conf = sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        )({
            CONF_ID: cg.allocate_id(sensor.Sensor),
            CONF_NAME: f"{base_name} Energy Daily",
        })

        energy_week_conf = sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        )({
            CONF_ID: cg.allocate_id(sensor.Sensor),
            CONF_NAME: f"{base_name} Energy Weekly",
        })

        energy_month_conf = sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        )({
            CONF_ID: cg.allocate_id(sensor.Sensor),
            CONF_NAME: f"{base_name} Energy Monthly",
        })

        current = await sensor.new_sensor(current_conf)
        power = await sensor.new_sensor(power_conf)
        energy_day = await sensor.new_sensor(energy_day_conf)
        energy_week = await sensor.new_sensor(energy_week_conf)
        energy_month = await sensor.new_sensor(energy_month_conf)

        cg.add(var.set_sensor_current(i, current))
        cg.add(var.set_sensor_power(i, power))
        cg.add(var.set_sensor_energy_daily(i, energy_day))
        cg.add(var.set_sensor_energy_weekly(i, energy_week))
        cg.add(var.set_sensor_energy_monthly(i, energy_month))

    # Voltage and WiFi sensors
    voltage_conf = sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    )({
        CONF_ID: cg.allocate_id(sensor.Sensor),
        CONF_NAME: "Mains Voltage",
    })

    wifi_conf = sensor.sensor_schema(
        unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
        state_class=STATE_CLASS_MEASUREMENT,
    )({
        CONF_ID: cg.allocate_id(sensor.Sensor),
        CONF_NAME: "WiFi RSSI",
    })

    voltage_sensor = await sensor.new_sensor(voltage_conf)
    wifi_rssi = await sensor.new_sensor(wifi_conf)

    cg.add(var.set_sensor_voltage(voltage_sensor))
    cg.add(var.set_sensor_wifi_rssi(wifi_rssi))
