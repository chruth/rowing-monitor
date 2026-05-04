from esphome import pins
import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DURATION,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_EMPTY,
    UNIT_METER,
    UNIT_SECOND,
)

DEPENDENCIES = ["api"]
AUTO_LOAD = ["sensor"]

rowing_monitor_ns = cg.esphome_ns.namespace("rowing_monitor")
CustomAPIDevice = cg.esphome_ns.namespace("api").class_("CustomAPIDevice")
RowingMonitorComponent = rowing_monitor_ns.class_(
    "RowingMonitorComponent", cg.Component, CustomAPIDevice
)

CONF_STEP1_PIN = "step1_pin"
CONF_STEP2_PIN = "step2_pin"
CONF_RESET_PIN = "reset_pin"

CONF_SPM_SENSOR = "spm_sensor"
CONF_DISTANCE_SENSOR = "distance_sensor"
CONF_TOTAL_STROKES_SENSOR = "total_strokes_sensor"
CONF_ACTIVE_TIME_SENSOR = "active_time_sensor"
CONF_UPTIME_SENSOR = "uptime_sensor"
CONF_VALID_STROKES_SENSOR = "valid_strokes_sensor"
CONF_SHORT_STROKES_SENSOR = "short_strokes_sensor"
CONF_MICRO_STROKES_SENSOR = "micro_strokes_sensor"


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(RowingMonitorComponent),
        cv.Required(CONF_STEP1_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_STEP2_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_RESET_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_SPM_SENSOR): sensor.sensor_schema(
            unit_of_measurement="SPM",
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Required(CONF_DISTANCE_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_METER,
            accuracy_decimals=4,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Required(CONF_TOTAL_STROKES_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Required(CONF_ACTIVE_TIME_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Required(CONF_UPTIME_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Required(CONF_VALID_STROKES_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Required(CONF_SHORT_STROKES_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Required(CONF_MICRO_STROKES_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    p1 = await cg.gpio_pin_expression(config[CONF_STEP1_PIN])
    cg.add(var.set_step1_pin(p1))
    p2 = await cg.gpio_pin_expression(config[CONF_STEP2_PIN])
    cg.add(var.set_step2_pin(p2))
    pr = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(pr))

    s = await sensor.new_sensor(config[CONF_SPM_SENSOR])
    cg.add(var.set_spm_sensor(s))
    s = await sensor.new_sensor(config[CONF_DISTANCE_SENSOR])
    cg.add(var.set_distance_sensor(s))
    s = await sensor.new_sensor(config[CONF_TOTAL_STROKES_SENSOR])
    cg.add(var.set_total_strokes_sensor(s))
    s = await sensor.new_sensor(config[CONF_ACTIVE_TIME_SENSOR])
    cg.add(var.set_active_time_sensor(s))
    s = await sensor.new_sensor(config[CONF_UPTIME_SENSOR])
    cg.add(var.set_uptime_sensor(s))
    s = await sensor.new_sensor(config[CONF_VALID_STROKES_SENSOR])
    cg.add(var.set_valid_strokes_sensor(s))
    s = await sensor.new_sensor(config[CONF_SHORT_STROKES_SENSOR])
    cg.add(var.set_short_strokes_sensor(s))
    s = await sensor.new_sensor(config[CONF_MICRO_STROKES_SENSOR])
    cg.add(var.set_micro_strokes_sensor(s))
