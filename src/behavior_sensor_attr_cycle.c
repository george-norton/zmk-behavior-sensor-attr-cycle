/*
 * Copyright (c) 2025, George Norton
 *
 * SPDX-License-Identifier: MIT
 */
#define DT_DRV_COMPAT zmk_behavior_sensor_attr_cycle
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zephyr/settings/settings.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define MAX_SETTINGS_LENGTH 16
#define SETTINGS_PREFIX "attr_cycle"

struct behavior_sensor_attr_cycle_config {
    const struct device *sensor_device;
    const char settings_key[MAX_SETTINGS_LENGTH];
    int32_t attr;
    int32_t save_delay;
    int32_t load_delay;
    bool persistant;
    uint8_t length;
    int32_t values[];
};

// Everything in here will get stored in flash and persisted if CONFIG_SETTINGS is enabled and correctly configured
struct behavior_sensor_attr_cycle_persistant_state {
    uint8_t index;
};

struct behavior_sensor_attr_cycle_data {
    const struct device *dev;
#if IS_ENABLED(CONFIG_SETTINGS)
    struct k_work_delayable load_work;
    struct k_work_delayable save_work;
#endif
    struct behavior_sensor_attr_cycle_persistant_state state;
};

#if IS_ENABLED(CONFIG_SETTINGS)
static void save_work_callback(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct behavior_sensor_attr_cycle_data *data = CONTAINER_OF(dwork, struct behavior_sensor_attr_cycle_data, save_work);
    const struct device *dev = data->dev;
    const struct behavior_sensor_attr_cycle_config *config = dev->config;

    int err = settings_save_one(config->settings_key, &data->state, sizeof(struct behavior_sensor_attr_cycle_persistant_state));
    if (err < 0) {
        LOG_ERR("Failed to save settings %d", err);
    }
}

static void load_work_callback(struct k_work *work) {
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct behavior_sensor_attr_cycle_data *data = CONTAINER_OF(dwork, struct behavior_sensor_attr_cycle_data, load_work);
    const struct device *dev = data->dev;
    const struct behavior_sensor_attr_cycle_config *config = dev->config;

    struct sensor_value val = { val1: config->values[data->state.index], val2: 0 };
    sensor_attr_set(config->sensor_device, SENSOR_CHAN_ALL, config->attr, &val);
}
#endif

static int behavior_sensor_attr_cycle_init(const struct device *dev) {
    struct behavior_sensor_attr_cycle_data *data = dev->data;
    const struct behavior_sensor_attr_cycle_config *config = dev->config;
    data->dev = dev;
    
#if IS_ENABLED(CONFIG_SETTINGS)
    if (config->persistant) {
        k_work_init_delayable (&data->save_work, save_work_callback);
    }
#endif
    return 0;
};

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_sensor_attr_cycle_data *data = dev->data;
    const struct behavior_sensor_attr_cycle_config *config = dev->config;

    // Update the index, then send the new value to the sensor
    data->state.index = (data->state.index + 1) % config->length;
    struct sensor_value val = { val1: config->values[data->state.index], val2: 0 };
    sensor_attr_set(config->sensor_device, SENSOR_CHAN_ALL, config->attr, &val);

#if IS_ENABLED(CONFIG_SETTINGS)
    if (config->persistant) {
        // We try to limit flash writes. It seems likley the user will activate the behaviour multiple times
        // looking for a specific value, so delay writing a little bit.
        k_work_reschedule(&data->save_work, K_MSEC(config->save_delay));
    }
#endif
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
        .save_delay = DT_PROP(DT_DRV_INST(n), save_delay),                                         \
        .load_delay = DT_PROP(DT_DRV_INST(n), load_delay),                                         \
        .persistant = DT_PROP(DT_DRV_INST(n), persistant),                                         \
        .settings_key = SETTINGS_PREFIX "/" #n,                                                    \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_sensor_attr_cycle_init, NULL,                              \
                            &data##n, &config##n, POST_KERNEL,                                     \
                            CONFIG_INPUT_INIT_PRIORITY,                                            \
                            &behavior_sensor_attr_cycle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CYCLE_INST)

#if IS_ENABLED(CONFIG_SETTINGS)
#define SETTINGS_INST(n) \
    case n: { \
        data = &data##n;\
        config = &config##n; \
        break; \
    }

// This is called once at startup when we have read our settings
static int sensor_attr_cycle_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    struct behavior_sensor_attr_cycle_data *data = NULL;
    const struct behavior_sensor_attr_cycle_config *config = NULL;
    char *endptr;
    long identifier = strtol(name, &endptr, 10);
    int err = 0;

    if (endptr == name) {
        return -ENOENT;
    }

    // The identifier is the instance index, this switch statement initializes our data and config pointers
    // to point at the right structures.
    switch (identifier) {
        DT_INST_FOREACH_STATUS_OKAY(SETTINGS_INST)
        default:
            return -ENOENT;
    }

    if (config->persistant) {
        err = read_cb(cb_arg, &data->state, sizeof(struct behavior_sensor_attr_cycle_persistant_state));
        if (err >= 0) {
            if (data->state.index >= config->length) {
                data->state.index = 0;
            }
            else {
                k_work_init_delayable(&data->load_work, load_work_callback);
                k_work_schedule(&data->load_work, K_MSEC(config->load_delay));
            }
        }
        else {
            LOG_ERR("Failed to load settings %d", err);
        }
    }
    return MIN(err, 0);
}

SETTINGS_STATIC_HANDLER_DEFINE(sensor_attr_cycle, SETTINGS_PREFIX, NULL, sensor_attr_cycle_settings_load_cb, NULL, NULL);
#endif

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */