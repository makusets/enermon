import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_VOLT,
    UNIT_WATT,
)

cg.add_library(
    name="EmonLib",
    repository="https://github.com/openenergymonitor/EmonLib.git",
    version=None,
)

DEPENDENCIES = []
AUTO_LOAD = ["sensor"]

enermon_ns = cg.esphome_ns.namespace("enermon")
Enermon = enermon_ns.class_("Enermon", cg.Component)

CONF_VOLTAGE_PIN = "voltage_pin"
CONF_VOLTAGE_CAL = "voltage_cal"
CONF_VOLTAGE_PHASE = "voltage_phase"
CONF_SAMPLE_COUNT = "sample_count"
CONF_SENSORS = "sensors"

# Sensor configuration for each CT
CONF_CT_PIN = "ct_pin"
CONF_CT_CAL = "ct_cal"
CONF_CURRENT = "current"
CONF_POWER = "power"
CONF_ENERGY_DAILY = "energy_daily"
CONF_ENERGY_WEEKLY = "energy_weekly"
CONF_ENERGY_MONTHLY = "energy_monthly"

CONF_VOLTAGE = "voltage"

CT_SENSOR_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_CT_PIN): cv.int_,
        cv.Optional(CONF_CT_CAL, default=30.0): cv.float_,
        cv.Optional(CONF_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_ENERGY_DAILY): sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_ENERGY_WEEKLY): sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_ENERGY_MONTHLY): sensor.sensor_schema(
            unit_of_measurement="Wh",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Enermon),
        cv.Optional(CONF_VOLTAGE_PIN): cv.int_,
        cv.Optional(CONF_VOLTAGE_CAL, default=230.0): cv.float_,
        cv.Optional(CONF_VOLTAGE_PHASE, default=1.7): cv.float_,
        cv.Optional(CONF_SAMPLE_COUNT, default=200): cv.positive_int,
        cv.Optional(CONF_SENSORS, default=[]): cv.ensure_list(CT_SENSOR_SCHEMA),
        cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Voltage config
    voltage_pin = config.get(CONF_VOLTAGE_PIN, -1)
    voltage_cal = config.get(CONF_VOLTAGE_CAL, 230.0)
    voltage_phase = config.get(CONF_VOLTAGE_PHASE, 1.7)
    sample_count = config.get(CONF_SAMPLE_COUNT, 200)
    
    cg.add(
        var.set_voltage_config(
            voltage_pin,
            voltage_cal,
            voltage_phase,
            sample_count,
        )
    )

    # Set up CT sensors - each with its own pin and calibration
    for ct_index, sensor_config in enumerate(config.get(CONF_SENSORS, [])):
        ct_pin = sensor_config[CONF_CT_PIN]
        ct_cal = sensor_config[CONF_CT_CAL]
        
        # Configure this CT channel
        cg.add(var.add_ct_channel(ct_index, ct_pin, ct_cal))
        
        # Set up current sensor if defined
        if CONF_CURRENT in sensor_config:
            sens = await sensor.new_sensor(sensor_config[CONF_CURRENT])
            cg.add(var.set_sensor_current(ct_index, sens))
            
        # Set up power sensor if defined
        if CONF_POWER in sensor_config:
            sens = await sensor.new_sensor(sensor_config[CONF_POWER])
            cg.add(var.set_sensor_power(ct_index, sens))
            
        # Set up energy daily sensor if defined
        if CONF_ENERGY_DAILY in sensor_config:
            sens = await sensor.new_sensor(sensor_config[CONF_ENERGY_DAILY])
            cg.add(var.set_sensor_energy_daily(ct_index, sens))
            
        # Set up energy weekly sensor if defined
        if CONF_ENERGY_WEEKLY in sensor_config:
            sens = await sensor.new_sensor(sensor_config[CONF_ENERGY_WEEKLY])
            cg.add(var.set_sensor_energy_weekly(ct_index, sens))
            
        # Set up energy monthly sensor if defined
        if CONF_ENERGY_MONTHLY in sensor_config:
            sens = await sensor.new_sensor(sensor_config[CONF_ENERGY_MONTHLY])
            cg.add(var.set_sensor_energy_monthly(ct_index, sens))

    # Voltage sensor
    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_sensor_voltage(sens))