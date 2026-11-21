import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import button, datetime, number, switch, text_sensor

DEPENDENCIES = ["web_server", "wifi", "captive_portal"]

ALARM_ENTITIES = {
    "alarm_time": datetime.TimeEntity,
    "alarm_enabled": switch.Switch,
    "alarm_skip": switch.Switch,
    "alarm_status": text_sensor.TextSensor,
    "alarm_duration": number.Number,
    "snooze_minutes": number.Number,
    "buzzer_volume": number.Number,
    "buzzer_frequency": number.Number,
    "test_buzzer": button.Button,
    "silence_alarm": button.Button,
}

ClockAdmin = cg.esphome_ns.namespace("clock_admin").class_("ClockAdmin", cg.Component)
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(ClockAdmin),
    cv.Required("web_server_id"): cv.use_id(
        cg.esphome_ns.namespace("web_server").class_("WebServer")
    ),
    cv.Required("default_username"): cv.string_strict,
    cv.Required("default_timezone"): cv.string_strict,
    **{cv.Required(key): cv.use_id(entity) for key, entity in ALARM_ENTITIES.items()},
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    web_server = await cg.get_variable(config["web_server_id"])
    entities = [await cg.get_variable(config[key]) for key in ALARM_ENTITIES]
    var = cg.new_Pvariable(config[CONF_ID], web_server, *entities)
    cg.add(var.set_build_defaults(config["default_username"], config["default_timezone"]))
    await cg.register_component(var, config)
