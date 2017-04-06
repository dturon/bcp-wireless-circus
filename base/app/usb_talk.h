#ifndef _USB_TALK_H
#define _USB_TALK_H

#include <bc_common.h>
#include <jsmn.h>
#include <bc_module_relay.h>

#define USB_TALK_INT_VALUE_NULL INT32_MIN

typedef struct
{
    const char *buffer;
    int token_count;
    jsmntok_t *tokens;

} usb_talk_payload_t;

typedef void (*usb_talk_sub_callback_t)(usb_talk_payload_t *payload, void *param);

void usb_talk_init(void);
void usb_talk_start(void);
void usb_talk_sub(const char *topic, usb_talk_sub_callback_t callback, void *param);
void usb_talk_send_string(const char *buffer);
void usb_talk_publish_led(const char *prefix, bool *state);
void usb_talk_publish_push_button(const char *prefix, uint16_t *event_count);
void usb_talk_publish_thermometer(const char *prefix, uint8_t *i2c, float *temperature);
void usb_talk_publish_humidity_sensor(const char *prefix, uint8_t *i2c, float *relative_humidity);
void usb_talk_publish_lux_meter(const char *prefix, uint8_t *i2c, float *illuminance);
void usb_talk_publish_barometer(const char *prefix, uint8_t *i2c, float *pascal, float *altitude);
void usb_talk_publish_co2_concentation(const char *prefix, int16_t *concentration);
void usb_talk_publish_light(const char *prefix, bool *state);
void usb_talk_publish_relay(const char *prefix, bool *state);
void usb_talk_publish_module_relay(const char *prefix, uint8_t *number, bc_module_relay_state_t *state);
void usb_talk_publish_led_strip_config(const char *prefix, const char *mode, int *count);

bool usb_talk_payload_get_bool(usb_talk_payload_t *payload, bool *value);
bool usb_talk_payload_get_key_bool(usb_talk_payload_t *payload, const char *key, bool *value);
bool usb_talk_payload_get_data(usb_talk_payload_t *payload, uint8_t *buffer, size_t *length);
bool usb_talk_payload_get_key_data(usb_talk_payload_t *payload, const char *key, uint8_t *buffer, size_t *length);
bool usb_talk_payload_get_enum(usb_talk_payload_t *payload, int *value, ...);
bool usb_talk_payload_get_key_enum(usb_talk_payload_t *payload, const char *key, int *value, ...);
bool usb_talk_payload_get_int(usb_talk_payload_t *payload, int *value);
bool usb_talk_payload_get_key_int(usb_talk_payload_t *payload, const char *key, int *value);
bool usb_talk_payload_get_string(usb_talk_payload_t *payload, char *buffer, size_t *length);
bool usb_talk_payload_get_key_string(usb_talk_payload_t *payload, const char *key, char *buffer, size_t *length);

bool usb_talk_is_string_token_equal(const char *buffer, jsmntok_t *token, const char *string);

#endif /* _USB_TALK_H */
