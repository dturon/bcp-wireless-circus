#include <application.h>
#include <usb_talk.h>

#define PREFIX_REMOTE "remote"
#define PREFIX_BASE "base"
#define UPDATE_INTERVAL 5000

#define MAX_PIXELS 150
#define APPLICATION_TASK_ID 0

static bc_led_t led;
static bool led_state;

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

static struct {
    bc_tick_t next_update;
    bool mqtt;
    struct {
        float_t temperature;
        float_t humidity;
        float_t illuminance;
        float_t pressure;
        float_t altitude;
        float_t co2_concentation;
    } base;
    struct {
        float_t temperature;
        float_t humidity;
        float_t illuminance;
        float_t pressure;
        float_t altitude;
        float_t co2_concentation;
    } remote;
    uint8_t cnt;
    uint8_t page;

} lcd;

static const struct {
    char *title;
    char *name0;
    char *format0;
    float_t *value0;
    char *unit0;
    char *name1;
    char *format1;
    float_t *value1;
    char *unit1;

} pages[] = {
        {"Base",
            "Temperature   ", "%.1f", &lcd.base.temperature, "\xb0" "C",
            "Humidity      ", "%.1f", &lcd.base.humidity, "%"},
        {"Base",
            "CO2           ", "%.0f", &lcd.base.co2_concentation, "ppm",
            "Illuminance   ", "%.1f", &lcd.base.illuminance, "lux"},
        {"Base",
            "Pressure      ", "%.0f", &lcd.base.pressure, "hPa",
            "Altitude      ", "%.1f", &lcd.base.altitude, "m"},
        {"Remote",
            "Temperature   ", "%.1f", &lcd.remote.temperature, "\xb0" "C",
            "Humidity      ", "%.1f", &lcd.remote.humidity, "%"},
        {"Remote",
            "CO2           ", "%.0f", &lcd.remote.co2_concentation, "ppm",
            "Illuminance   ", "%.1f", &lcd.remote.illuminance, "lux"},
        {"Remote",
            "Pressure      ", "%.0f", &lcd.remote.pressure, "hPa",
            "Altitude      ", "%.1f", &lcd.remote.altitude, "m"}
};


static bc_module_relay_t relay_0_0;
static bc_module_relay_t relay_0_1;

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
static void radio_event_handler(bc_radio_event_t event, void *event_param);
static void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param);
static void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param);
static void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param);
static void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param);
static void co2_event_handler(bc_module_co2_event_t event, void *event_param);
static void encoder_event_handler(bc_module_encoder_event_t event, void *param);
static void set_default_pixels(void);

static void led_state_set(usb_talk_payload_t *payload, void *param);
static void led_state_get(usb_talk_payload_t *payload, void *param);
static void _light_set(bool state);
static void light_state_set(usb_talk_payload_t *payload, void *param);
static void light_state_get(usb_talk_payload_t *payload, void *param);
static void relay_state_set(usb_talk_payload_t *payload, void *param);
static void relay_state_get(usb_talk_payload_t *payload, void *param);
static void module_relay_state_set(usb_talk_payload_t *payload, void *param);
static void module_relay_state_get(usb_talk_payload_t *payload, void *param);
static void led_strip_framebuffer_set(usb_talk_payload_t *payload, void *param);
static void led_strip_framebuffer_set(usb_talk_payload_t *payload, void *param);
static void led_strip_config_set(usb_talk_payload_t *payload, void *param);
static void led_strip_config_get(usb_talk_payload_t *payload, void *param);
static void lcd_text_set(usb_talk_payload_t *payload, void *param);

void application_init(void)
{
    usb_talk_init();

    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_module_power_init();

    set_default_pixels();

    bc_led_strip_init(&led_strip, bc_module_power_get_led_strip_driver(), &led_strip_buffer);

    bc_module_lcd_init(&_bc_module_lcd_framebuffer);
    bc_module_lcd_clear();
    bc_module_lcd_update();

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

    //----------------------------

    bc_module_co2_init();
    bc_module_co2_set_update_interval(30000);
    bc_module_co2_set_event_handler(co2_event_handler, NULL);

    // ---------------------------

    bc_module_encoder_init();
    bc_module_encoder_set_event_handler(encoder_event_handler, NULL);

    //----------------------------

    bc_module_relay_init(&relay_0_0, BC_MODULE_RELAY_I2C_ADDRESS_DEFAULT);
    bc_module_relay_init(&relay_0_1, BC_MODULE_RELAY_I2C_ADDRESS_ALTERNATE);


    usb_talk_sub(PREFIX_BASE "/led/-/state/set", led_state_set, NULL);
    usb_talk_sub(PREFIX_BASE "/led/-/state/get", led_state_get, NULL);
    usb_talk_sub(PREFIX_BASE "/light/-/state/set", light_state_set, NULL);
    usb_talk_sub(PREFIX_BASE "/light/-/state/get", light_state_get, NULL);
    usb_talk_sub(PREFIX_BASE "/led-strip/-/framebuffer/set", led_strip_framebuffer_set, NULL);
    usb_talk_sub(PREFIX_BASE "/led-strip/-/config/set", led_strip_config_set, NULL);
    usb_talk_sub(PREFIX_BASE "/led-strip/-/config/get", led_strip_config_get, NULL);
    usb_talk_sub(PREFIX_BASE "/relay/-/state/set", relay_state_set, NULL);
    usb_talk_sub(PREFIX_BASE "/relay/-/state/get", relay_state_get, NULL);
    usb_talk_sub(PREFIX_BASE "/relay/0:0/state/set", module_relay_state_set, &relay_0_0);
    usb_talk_sub(PREFIX_BASE "/relay/0:0/state/get", module_relay_state_get, &relay_0_0);
    usb_talk_sub(PREFIX_BASE "/relay/0:1/state/set", module_relay_state_set, &relay_0_1);
    usb_talk_sub(PREFIX_BASE "/relay/0:1/state/get", module_relay_state_get, &relay_0_1);
    usb_talk_sub(PREFIX_BASE "/lcd/-/text/set", lcd_text_set, NULL);

    memset(&lcd.base, 0xff, sizeof(lcd.base));
    memset(&lcd.remote, 0xff, sizeof(lcd.remote));
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

    bc_tick_t now = bc_tick_get();
    if (lcd.next_update < now)
    {
        if (!lcd.mqtt && (lcd.cnt++ > 2))
        {
            char str[32];
            int w;

            bc_module_lcd_clear();

            bc_module_lcd_set_font(&bc_font_ubuntu_24);
            bc_module_lcd_draw_string(5, 5, pages[lcd.page].title);

            bc_module_lcd_set_font(&bc_font_ubuntu_15);
            bc_module_lcd_draw_string(10, 30, pages[lcd.page].name0);
            bc_module_lcd_set_font(&bc_font_ubuntu_24);

            snprintf(str, sizeof(str), pages[lcd.page].format0, *pages[lcd.page].value0);
            w = bc_module_lcd_draw_string(15, 50, str);
            bc_module_lcd_set_font(&bc_font_ubuntu_15);
            w = bc_module_lcd_draw_string(w, 60, pages[lcd.page].unit0);

            bc_module_lcd_set_font(&bc_font_ubuntu_15);
            bc_module_lcd_draw_string(10, 80, pages[lcd.page].name1);
            bc_module_lcd_set_font(&bc_font_ubuntu_24);

            snprintf(str, sizeof(str), pages[lcd.page].format1, *pages[lcd.page].value1);
            w = bc_module_lcd_draw_string(15, 100, str);
            bc_module_lcd_set_font(&bc_font_ubuntu_15);

            bc_module_lcd_draw_string(w, 110, pages[lcd.page].unit1);

            lcd.cnt = 0;
            if (++lcd.page == (sizeof(pages) / sizeof(pages[0])))
            {
                lcd.page = 0;
            }
        }

        bc_module_lcd_update();
        lcd.next_update = now + 500;
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
    lcd.remote.temperature = *temperature;
}


void bc_radio_on_humidity(uint32_t *peer_device_address, uint8_t *i2c, float *percentage)
{
    (void) peer_device_address;

    usb_talk_publish_humidity_sensor(PREFIX_REMOTE, i2c, percentage);
    lcd.remote.humidity = *percentage;
}

void bc_radio_on_lux_meter(uint32_t *peer_device_address, uint8_t *i2c, float *illuminance)
{
    (void) peer_device_address;

    usb_talk_publish_lux_meter(PREFIX_REMOTE, i2c, illuminance);
    lcd.remote.illuminance = *illuminance;
}

void bc_radio_on_barometer(uint32_t *peer_device_address, uint8_t *i2c, float *pressure, float *altitude)
{
    (void) peer_device_address;

    usb_talk_publish_barometer(PREFIX_REMOTE, i2c, pressure, altitude);
    lcd.remote.pressure = *pressure / 100;
    lcd.remote.altitude = *altitude;
}

void bc_radio_on_co2(uint32_t *peer_device_address, float *concentration)
{
    (void) peer_device_address;

    usb_talk_publish_co2_concentation(PREFIX_REMOTE, concentration);
    lcd.remote.co2_concentation = *concentration;
}

void bc_radio_on_buffer(uint32_t *peer_device_address, uint8_t *buffer, size_t *length)
{
    (void) peer_device_address;

    if (*length < 1 + sizeof(int))
    {
        return;
    }

    if (buffer[0] == 0x00)
    {
        int increment;
        memcpy(&increment, &buffer[1], sizeof(increment));

        usb_talk_publish_encoder(PREFIX_REMOTE, &increment);
    }
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
        lcd.base.temperature = value;
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
        lcd.base.humidity = value;
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
        lcd.base.illuminance = value;
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
    lcd.base.pressure = pascal / 100;
    lcd.base.altitude = meter;

}

void co2_event_handler(bc_module_co2_event_t event, void *event_param)
{
    (void) event_param;
    float value;

    if (event == BC_MODULE_CO2_EVENT_UPDATE)
    {
        if (bc_module_co2_get_concentration(&value))
        {
            usb_talk_publish_co2_concentation(PREFIX_BASE, &value);
            lcd.base.co2_concentation = value;
        }
    }
}

static void encoder_event_handler(bc_module_encoder_event_t event, void *param)
{
    (void)param;

    if (event == BC_MODULE_ENCODER_EVENT_ROTATION)
    {
        int increment = bc_module_encoder_get_increment();
        usb_talk_publish_encoder(PREFIX_BASE, &increment);
    }
}

static void set_default_pixels(void)
{
    pixels_length = led_strip_buffer.type * led_strip_count;

    memset(pixels, 0x00, sizeof(pixels));

    int tmp = 0;

    for (int i = 0; i < (int)pixels_length; i += led_strip_buffer.type)
    {
        pixels[i + (tmp++ % led_strip_buffer.type)] = 0x10;
    }
}

static void led_state_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;

    if (!usb_talk_payload_get_bool(payload, &led_state))
    {
        return;
    }

    bc_led_set_mode(&led, led_state ? BC_LED_MODE_ON : BC_LED_MODE_OFF);

    usb_talk_publish_led(PREFIX_BASE, &led_state);
}

static void led_state_get(usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    usb_talk_publish_led(PREFIX_BASE, &led_state);
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

    led_state = state;
    usb_talk_publish_led(PREFIX_BASE, &led_state);
}

static void light_state_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;
    bool new_state;

    if (!usb_talk_payload_get_bool(payload, &new_state))
    {
        return;
    }

    _light_set(new_state);
}

static void light_state_get(usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    usb_talk_publish_light(PREFIX_BASE, &light);
}

static void relay_state_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;
    bool state;

    if (!usb_talk_payload_get_bool(payload, &state))
    {
        return;
    }

    bc_module_power_relay_set_state(state);

    usb_talk_publish_relay(PREFIX_BASE, &state);
}

static void relay_state_get(usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    bool state = bc_module_power_relay_get_state();

    usb_talk_publish_relay(PREFIX_BASE, &state);
}

static void module_relay_state_set(usb_talk_payload_t *payload, void *param)
{
    (void) payload;

    bc_module_relay_t *relay = (bc_module_relay_t *)param;

    bool state;

    if (!usb_talk_payload_get_bool(payload, &state))
    {
        return;
    }

    bc_module_relay_set_state(relay, state);

    uint8_t number = (&relay_0_0 == relay) ? 0 : 1;
    bc_module_relay_state_t relay_state = state ? BC_MODULE_RELAY_STATE_TRUE : BC_MODULE_RELAY_STATE_FALSE;

    usb_talk_publish_module_relay(PREFIX_BASE, &number, &relay_state);
}

static void module_relay_state_get(usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    bc_module_relay_t *relay = (bc_module_relay_t *)param;

    bc_module_relay_state_t state = bc_module_relay_get_state(relay);

    uint8_t number = (&relay_0_0 == relay) ? 0 : 1;

    usb_talk_publish_module_relay(PREFIX_BASE, &number, &state);
}

static void led_strip_framebuffer_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;

    size_t length = led_strip_count * led_strip_buffer.type;

    if (usb_talk_payload_get_data(payload, pixels, &length))
    {
        pixels_length = length;

        if (light)
        {
            bc_led_strip_set_rgbw_framebuffer(&led_strip, pixels, pixels_length);
        }

        bc_scheduler_plan_now(APPLICATION_TASK_ID);

        usb_talk_send_string("[\"" PREFIX_BASE "/led-strip/-/framebuffer/set/ok\", null]\n");
    }

}

static void led_strip_config_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;

    int type;
    int count;

    if (!usb_talk_payload_get_key_enum(payload, "type", &type, "rgb", "rgbw"))
    {
        return;
    }

    if (!usb_talk_payload_get_key_int(payload, "count", &count))
    {
        return;
    }

    if ((count > MAX_PIXELS) || (count < 1))
    {
        return;
    }

    bc_led_strip_effect_stop(&led_strip);

    bc_led_strip_fill(&led_strip, 0x00000000);

    while(!bc_led_strip_write(&led_strip)) { }
    while(!bc_led_strip_write(&led_strip)) { }

    led_strip_count = count;
    led_strip_buffer.type = type == 0 ? BC_LED_STRIP_TYPE_RGB : BC_LED_STRIP_TYPE_RGBW;

    set_default_pixels();
    if (light)
    {
        bc_led_strip_set_rgbw_framebuffer(&led_strip, pixels, pixels_length);
    }

    while(!bc_led_strip_write(&led_strip)) { }

    usb_talk_publish_led_strip_config(PREFIX_BASE, led_strip_buffer.type == BC_LED_STRIP_TYPE_RGB ? "rgb" : "rgbw", &led_strip_count);

}

static void led_strip_config_get(usb_talk_payload_t *payload, void *param)
{
    (void) payload;
    (void) param;

    usb_talk_publish_led_strip_config(PREFIX_BASE, led_strip_buffer.type == BC_LED_STRIP_TYPE_RGB ? "rgb" : "rgbw", &led_strip_count);

}

static void lcd_text_set(usb_talk_payload_t *payload, void *param)
{
    (void) param;
    int x;
    int y;
    int font_size = 0;
    char text[32];
    size_t length = sizeof(text);

    memset(text, 0, length);
    if (!usb_talk_payload_get_key_int(payload, "x", &x))
    {
        return;
    }
    if (!usb_talk_payload_get_key_int(payload, "y", &y))
    {
        return;
    }
    if (!usb_talk_payload_get_key_string(payload, "text", text, &length))
    {
        return;
    }

    if (!lcd.mqtt)
    {
        bc_module_lcd_clear();
        lcd.mqtt = true;
    }

    usb_talk_payload_get_key_int(payload, "font", &font_size);
    switch (font_size) {
        case 11:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_11);
            break;
        }
        case 13:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_13);
            break;
        }
        case 15:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_15);
            break;
        }
        case 24:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_24);
            break;
        }
        case 28:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_28);
            break;
        }
        case 33:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_33);
            break;
        }
        default:
        {
            bc_module_lcd_set_font(&bc_font_ubuntu_15);
            break;
        }
    }

    bc_module_lcd_draw_string(x, y, text);
}
