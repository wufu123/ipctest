#ifndef __MQTT_H
#define __MQTT_H

#include <stdint.h>
#include <stdbool.h>

/* 收到 PUBLISH 消息时回调 */
typedef void (*mqtt_msg_cb)(const char *topic, const uint8_t *payload, uint16_t len);

void mqtt_set_callback(mqtt_msg_cb cb);

bool mqtt_connect(const char *client_id, const char *user, const char *pass,
                  uint16_t keepalive_s);
bool mqtt_subscribe(const char *topic, uint8_t qos);
bool mqtt_publish(const char *topic, const uint8_t *payload, uint16_t len,
                  uint8_t qos);
void mqtt_disconnect(void);

bool mqtt_is_connected(void);

/* 周期性调用：处理保活 PING 与下行消息 */
void mqtt_loop(void);

#endif /* __MQTT_H */
