#include <application.h>
#include <jsmn.h>
#include <bc_usb_cdc.h>

#define PREFIX_REMOTE "remote"
#define PREFIX_BASE "base"
#define UPDATE_INTERVAL 10000

#define MAX_PIXELS 150
#define APPLICATION_TASK_ID 0

static bc_led_t led;

static bool light;

static uint8_t pixels[MAX_PIXELS * 4];
static size_t pixels_length;

static uint32_t _dma_buffer[MAX_PIXELS * 4 * 2];

static bc_led_strip_buffer_t led_strip_buffer =
{
    .type = BC_LED_STRIP_TYPE_RGBW,
    .count = MAX_PIXELS,
    .buffer = _dma_buffer
};

static bc_led_strip_t led_strip;
static int led_strip_count = MAX_PIXELS;

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
static void radio_event_handler(bc_radio_event_t event, void *event_param);
static void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param);
static void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param);
static void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param);
static void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param);

static void _light_set(bool state);
static void light_set(usb_talk_payload_t *payload);
static void light_get(usb_talk_payload_t *payload);
static void relay_set(usb_talk_payload_t *payload);
static void relay_get(usb_talk_payload_t *payload);
static void led_strip_set(usb_talk_payload_t *payload);
static void led_strip_set(usb_talk_payload_t *payload);
static void led_strip_config_set(usb_talk_payload_t *payload);
static void led_strip_config_get(usb_talk_payload_t *payload);

void application_init(void)
{

    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_module_power_init();

    bc_led_strip_init(&led_strip, bc_module_power_get_led_strip_driver(), &led_strip_buffer);

    usb_talk_init();

    usb_talk_sub(PREFIX_BASE "/light/-/set", light_set);
    usb_talk_sub(PREFIX_BASE "/light/-/get", light_get);
    usb_talk_sub(PREFIX_BASE "/relay/-/set", relay_set);
    usb_talk_sub(PREFIX_BASE "/relay/-/get", relay_get);
    usb_talk_sub(PREFIX_BASE "/led-strip/-/set", led_strip_set);
    usb_talk_sub(PREFIX_BASE "/led-strip/-/config/set", led_strip_config_set);
    usb_talk_sub(PREFIX_BASE "/led-strip/-/config/get", led_strip_config_get);

    static bc_button_t button;
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize radio
    bc_radio_init();
    bc_radio_set_event_handler(radio_event_handler, NULL);
    bc_radio_listen();

    // Tags
    static bc_tag_temperature_t temperature_tag_0_48;
    bc_tag_temperature_init(&temperature_tag_0_48, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT);
    bc_tag_temperature_set_update_interval(&temperature_tag_0_48, UPDATE_INTERVAL);
    static uint8_t temperature_tag_0_48_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT;
    bc_tag_temperature_set_event_handler(&temperature_tag_0_48, temperature_tag_event_handler, &temperature_tag_0_48_i2c);

    static bc_tag_temperature_t temperature_tag_0_49;
    bc_tag_temperature_init(&temperature_tag_0_49, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature_tag_0_49, UPDATE_INTERVAL);
    static uint8_t temperature_tag_0_49_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE;
    bc_tag_temperature_set_event_handler(&temperature_tag_0_49, temperature_tag_event_handler, &temperature_tag_0_49_i2c);

    static bc_tag_temperature_t temperature_tag_1_48;
    bc_tag_temperature_init(&temperature_tag_1_48, BC_I2C_I2C1, BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT);
    bc_tag_temperature_set_update_interval(&temperature_tag_1_48, UPDATE_INTERVAL);
    static uint8_t temperature_tag_1_48_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT;
    bc_tag_temperature_set_event_handler(&temperature_tag_1_48, temperature_tag_event_handler,&temperature_tag_1_48_i2c);

    static bc_tag_temperature_t temperature_tag_1_49;
    bc_tag_temperature_init(&temperature_tag_1_49, BC_I2C_I2C1, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature_tag_1_49, UPDATE_INTERVAL);
    static uint8_t temperature_tag_1_49_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE;
    bc_tag_temperature_set_event_handler(&temperature_tag_1_49, temperature_tag_event_handler,&temperature_tag_1_49_i2c);

    //----------------------------

    static bc_tag_humidity_t humidity_tag_r2_0_40;
    bc_tag_humidity_init(&humidity_tag_r2_0_40, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_0_40, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_0_40_i2c = (BC_I2C_I2C0 << 7) | 0x40;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_0_40, humidity_tag_event_handler, &humidity_tag_r2_0_40_i2c);

    static bc_tag_humidity_t humidity_tag_r2_0_41;
    bc_tag_humidity_init(&humidity_tag_r2_0_41, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_ALTERNATE);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_0_41, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_0_41_i2c = (BC_I2C_I2C0 << 7) | 0x41;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_0_41, humidity_tag_event_handler, &humidity_tag_r2_0_41_i2c);

    static bc_tag_humidity_t humidity_tag_r1_0_5f;
    bc_tag_humidity_init(&humidity_tag_r1_0_5f, BC_TAG_HUMIDITY_REVISION_R1, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r1_0_5f, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r1_0_5f_i2c = (BC_I2C_I2C0 << 7) | 0x5f;
    bc_tag_humidity_set_event_handler(&humidity_tag_r1_0_5f, humidity_tag_event_handler, &humidity_tag_r1_0_5f_i2c);

    static bc_tag_humidity_t humidity_tag_r2_1_40;
    bc_tag_humidity_init(&humidity_tag_r2_1_40, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_1_40, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_1_40_i2c = (BC_I2C_I2C1 << 7) | 0x40;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_1_40, humidity_tag_event_handler, &humidity_tag_r2_1_40_i2c);

    static bc_tag_humidity_t humidity_tag_r2_1_41;
    bc_tag_humidity_init(&humidity_tag_r2_1_41, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_ALTERNATE);
    bc_tag_humidity_set_update_interval(&humidity_tag_r2_1_41, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r2_1_41_i2c = (BC_I2C_I2C1 << 7) | 0x41;
    bc_tag_humidity_set_event_handler(&humidity_tag_r2_1_41, humidity_tag_event_handler, &humidity_tag_r2_1_41_i2c);

    static bc_tag_humidity_t humidity_tag_r1_1_5f;
    bc_tag_humidity_init(&humidity_tag_r1_1_5f, BC_TAG_HUMIDITY_REVISION_R1, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag_r1_1_5f, UPDATE_INTERVAL);
    static uint8_t humidity_tag_r1_1_5f_i2c = (BC_I2C_I2C1 << 7) | 0x5f;
    bc_tag_humidity_set_event_handler(&humidity_tag_r1_1_5f, humidity_tag_event_handler, &humidity_tag_r1_1_5f_i2c);

    //----------------------------

    static bc_tag_lux_meter_t lux_meter_0_44;
    bc_tag_lux_meter_init(&lux_meter_0_44, BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT);
    bc_tag_lux_meter_set_update_interval(&lux_meter_0_44, UPDATE_INTERVAL);
    static uint8_t lux_meter_0_44_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT;
    bc_tag_lux_meter_set_event_handler(&lux_meter_0_44, lux_meter_event_handler, &lux_meter_0_44_i2c);

    static bc_tag_lux_meter_t lux_meter_0_45;
    bc_tag_lux_meter_init(&lux_meter_0_45, BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE);
    bc_tag_lux_meter_set_update_interval(&lux_meter_0_45, UPDATE_INTERVAL);
    static uint8_t lux_meter_0_45_i2c = (BC_I2C_I2C0 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE;
    bc_tag_lux_meter_set_event_handler(&lux_meter_0_45, lux_meter_event_handler, &lux_meter_0_45_i2c);

    static bc_tag_lux_meter_t lux_meter_1_44;
    bc_tag_lux_meter_init(&lux_meter_1_44, BC_I2C_I2C1, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT);
    bc_tag_lux_meter_set_update_interval(&lux_meter_1_44, UPDATE_INTERVAL);
    static uint8_t lux_meter_1_44_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT;
    bc_tag_lux_meter_set_event_handler(&lux_meter_1_44, lux_meter_event_handler, &lux_meter_1_44_i2c);

    static bc_tag_lux_meter_t lux_meter_1_45;
    bc_tag_lux_meter_init(&lux_meter_1_45, BC_I2C_I2C1, BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE);
    bc_tag_lux_meter_set_update_interval(&lux_meter_1_45, UPDATE_INTERVAL);
    static uint8_t lux_meter_1_45_i2c = (BC_I2C_I2C1 << 7) | BC_TAG_LUX_METER_I2C_ADDRESS_ALTERNATE;
    bc_tag_lux_meter_set_event_handler(&lux_meter_1_45, lux_meter_event_handler, &lux_meter_1_45_i2c);

    //----------------------------

    static bc_tag_barometer_t barometer_tag_0;
    bc_tag_barometer_init(&barometer_tag_0, BC_I2C_I2C0);
    bc_tag_barometer_set_update_interval(&barometer_tag_0, UPDATE_INTERVAL);
    static uint8_t barometer_tag_0_i2c = (BC_I2C_I2C0 << 7) | 0x60;
    bc_tag_barometer_set_event_handler(&barometer_tag_0, barometer_tag_event_handler, &barometer_tag_0_i2c);

    static bc_tag_barometer_t barometer_tag_1;
    bc_tag_barometer_init(&barometer_tag_1, BC_I2C_I2C1);
    bc_tag_barometer_set_update_interval(&barometer_tag_1, UPDATE_INTERVAL);
    static uint8_t barometer_tag_1_i2c = (BC_I2C_I2C1 << 7) | 0x60;
    bc_tag_barometer_set_event_handler(&barometer_tag_1, barometer_tag_event_handler, &barometer_tag_1_i2c);
}

void application_task(void)
{
    if (bc_led_strip_write(&led_strip))
    {
        bc_scheduler_plan_current_relative(50);
    }
    else
    {
        bc_scheduler_plan_current_now();
    }
}

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        static uint16_t event_count = 0;
        usb_talk_publish_push_button(PREFIX_BASE, &event_count);
        event_count++;
        _light_set(!light);
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enrollment_start();
        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
}


static void radio_event_handler(bc_radio_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_RADIO_EVENT_PAIR_SUCCESS)
    {
        //bc_radio_enrollment_stop();

        bc_led_pulse(&led, 1000);

        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
}

void bc_radio_on_push_button(uint32_t *peer_device_address, uint16_t *event_count)
{
    (void) peer_device_address;

    _light_set(!light);

    usb_talk_publish_push_button(PREFIX_REMOTE, event_count);
}

void bc_radio_on_thermometer(uint32_t *peer_device_address, uint8_t *i2c, float *temperature)
{
    (void) peer_device_address;

    usb_talk_publish_thermometer(PREFIX_REMOTE, i2c, temperature);
}


void bc_radio_on_humidity(uint32_t *peer_device_address, uint8_t *i2c, float *percentage)
{
    (void) peer_device_address;

    usb_talk_publish_humidity_sensor(PREFIX_REMOTE, i2c, percentage);

}

void bc_radio_on_lux_meter(uint32_t *peer_device_address, uint8_t *i2c, float *illuminance)
{
    (void) peer_device_address;

    usb_talk_publish_lux_meter(PREFIX_REMOTE, i2c, illuminance);
}

void bc_radio_on_barometer(uint32_t *peer_device_address, uint8_t *i2c, float *pressure, float *altitude)
{
    (void) peer_device_address;

    usb_talk_publish_barometer(PREFIX_REMOTE, i2c, pressure, altitude);
}


static void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    float value;

    if (event != BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_temperature_get_temperature_celsius(self, &value))
    {
        usb_talk_publish_thermometer(PREFIX_BASE, (uint8_t *)event_param, &value);
    }
}

static void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
    float value;

    if (event != BC_TAG_HUMIDITY_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_humidity_get_humidity_percentage(self, &value))
    {
        usb_talk_publish_humidity_sensor(PREFIX_BASE, (uint8_t *)event_param, &value);
    }
}

static void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param)
{
    float value;

    if (event != BC_TAG_LUX_METER_EVENT_UPDATE)
    {
        return;
    }

    if (bc_tag_lux_meter_get_luminosity_lux(self, &value))
    {
        usb_talk_publish_lux_meter(PREFIX_BASE, (uint8_t *)event_param, &value);
    }
}

static void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param)
{
    float pascal;
    float meter;

    if (event != BC_TAG_BAROMETER_EVENT_UPDATE)
    {
        return;
    }

    if (!bc_tag_barometer_get_pressure_pascal(self, &pascal))
    {
        return;
    }

    if (!bc_tag_barometer_get_altitude_meter(self, &meter))
    {
        return;
    }

    usb_talk_publish_barometer(PREFIX_BASE, (uint8_t *)event_param, &pascal, &meter);

}

static void _light_set(bool state)
{
    light = state;

    bc_led_set_mode(&led, light ? BC_LED_MODE_ON : BC_LED_MODE_OFF);

    if (light)
    {
        bc_led_strip_set_rgbw_framebuffer(&led_strip, pixels, pixels_length);
    }
    else
    {
        bc_led_strip_fill(&led_strip, 0x00000000);
    }

    bc_scheduler_plan_now(APPLICATION_TASK_ID);

    usb_talk_publish_light(PREFIX_BASE, &light);
}

static void light_set(usb_talk_payload_t *payload)
{
    bool new_state;

    if (!usb_talk_payload_get_bool(payload, "state", &new_state))
    {
        return;
    }

    _light_set(new_state);
}

static void light_get(usb_talk_payload_t *payload)
{
    (void) payload;

    usb_talk_publish_light(PREFIX_BASE, &light);
}

static void relay_set(usb_talk_payload_t *payload)
{
    bool state;

    if (!usb_talk_payload_get_bool(payload, "state", &state))
    {
        return;
    }

    bc_module_power_relay_set_state(state);
}

static void relay_get(usb_talk_payload_t *payload)
{
    (void) payload;

    bool state = bc_module_power_relay_get_state();

    usb_talk_publish_relay(PREFIX_BASE, &state);
}

static void led_strip_set(usb_talk_payload_t *payload)
{
    size_t length = led_strip_count * led_strip_buffer.type;

    if (usb_talk_payload_get_data(payload, "pixels", pixels, &length))
    {
        pixels_length = length;

        if (light)
        {
            bc_led_strip_set_rgbw_framebuffer(&led_strip, pixels, pixels_length);
        }

        bc_scheduler_plan_now(APPLICATION_TASK_ID);

        usb_talk_send_string("[\"base/led-strip/-/set/ok\", {}]\n");
    }

}

static void led_strip_config_set(usb_talk_payload_t *payload)
{
    int mode;
    int count;

    if (!usb_talk_payload_get_enum(payload, "mode", &mode, "rgb", "rgbw"))
    {
        return;
    }

    if (!usb_talk_payload_get_uint(payload, "count", &count))
    {
        return;
    }

    if (count > MAX_PIXELS)
    {
        return;
    }

    bc_led_strip_effect_stop(&led_strip);

    bc_led_strip_fill(&led_strip, 0x00000000);

    led_strip_count = count;
    led_strip_buffer.type = mode == 0 ? BC_LED_STRIP_TYPE_RGB : BC_LED_STRIP_TYPE_RGBW;

    bc_led_strip_fill(&led_strip, 0x00000000);

    while(!bc_led_strip_write(&led_strip)) { }

    usb_talk_publish_led_strip_config(PREFIX_BASE, "/set/ok", led_strip_buffer.type == BC_LED_STRIP_TYPE_RGB ? "rgb" : "rgbw", &led_strip_count);

}

static void led_strip_config_get(usb_talk_payload_t *payload)
{
    (void) payload;

    usb_talk_publish_led_strip_config(PREFIX_BASE, "", led_strip_buffer.type == BC_LED_STRIP_TYPE_RGB ? "rgb" : "rgbw", &led_strip_count);

}
