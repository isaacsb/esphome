import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import core, pins
from esphome.components import display, spi
from esphome.const import (
    CONF_BUSY_PIN,
    CONF_CS_PIN,
    CONF_DC_PIN,
    CONF_FULL_UPDATE_EVERY,
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
    CONF_RESET_DURATION,
    CONF_RESET_PIN,
)

DEPENDENCIES = ["spi"]

m5stack_coreink_display_ns = cg.esphome_ns.namespace("m5stack_coreink_display")
M5StackCoreInkDisplay = m5stack_coreink_display_ns.class_(
    "M5StackCoreInkDisplay", cg.PollingComponent, spi.SPIDevice, display.DisplayBuffer
)


CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(M5StackCoreInkDisplay),
            cv.Optional(CONF_DC_PIN, default="GPIO15"): pins.gpio_output_pin_schema,
            cv.Optional(CONF_CS_PIN, default="GPIO9"): pins.gpio_output_pin_schema,
            cv.Optional(CONF_RESET_PIN, default="GPIO0"): pins.gpio_output_pin_schema,
            cv.Optional(CONF_BUSY_PIN, default="GPIO4"): pins.gpio_input_pin_schema,
            cv.Optional(CONF_FULL_UPDATE_EVERY, default=30): cv.uint32_t,
            cv.Optional(
                CONF_RESET_DURATION, default=core.TimePeriod(milliseconds=240)
            ): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(max=core.TimePeriod(milliseconds=1000)),
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema(False)),
    cv.has_at_most_one_key(CONF_PAGES, CONF_LAMBDA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    await cg.register_component(var, config)
    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    dc = await cg.gpio_pin_expression(config[CONF_DC_PIN])
    cg.add(var.set_dc_pin(dc))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayBufferRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))

    reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(reset))

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))

    cg.add(var.set_reset_duration(config[CONF_RESET_DURATION]))
