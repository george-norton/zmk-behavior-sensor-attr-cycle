/*
 * Copyright (c) 2025 George Norton
 *
 * SPDX-License-Identifier: MIT
 */
#define DT_DRV_COMPAT zmk_behavior_sensor_attr_cycle
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_sensor_attr_cycle_config {
    const struct device *sensor_device;
    int32_t attr;
    bool persistant;
    uint8_t length;
    int32_t values[];
};

struct behavior_sensor_attr_cycle_data {
   uint8_t index;
};

static int behavior_sensor_attr_cycle_init(const struct device *dev) {
    LOG_ERR("GN: behavior_sensor_attr_cycle_init");
    return 0;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_sensor_attr_cycle_data *data = dev->data;
    const struct behavior_sensor_attr_cycle_config *config = dev->config;

    struct sensor_value val = { val1: config->values[data->index], val2: 0 };
    sensor_attr_set(config->sensor_device, SENSOR_CHAN_ALL, config->attr, &val);
    data->index = (data->index + 1) % config->length;

    return 0;
}

static const struct behavior_driver_api behavior_sensor_attr_cycle_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
};

#define CYCLE_INST(n)                                                                              \
    static struct behavior_sensor_attr_cycle_data data##n = {};                                    \
    static const struct behavior_sensor_attr_cycle_config config##n = {                            \
        .sensor_device =  DEVICE_DT_GET(DT_INST_PHANDLE(n, sensor_device)),                        \
        .length = DT_PROP_LEN(DT_DRV_INST(n), values),                                             \
        .values = DT_PROP(DT_DRV_INST(n), values),                                                 \
        .attr = DT_PROP(DT_DRV_INST(n), attr),                                                     \
        .persistant = DT_PROP(DT_DRV_INST(n), persistant),                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_sensor_attr_cycle_init, NULL,                              \
                            &data##n, &config##n, POST_KERNEL,                                     \
                            CONFIG_INPUT_INIT_PRIORITY,                                            \
                            &behavior_sensor_attr_cycle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CYCLE_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */