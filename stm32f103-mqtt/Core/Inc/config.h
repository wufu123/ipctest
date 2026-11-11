#ifndef __CONFIG_H
#define __CONFIG_H

/* ============ WiFi 配置 ============ */
#define WIFI_SSID       "your_wifi_ssid"
#define WIFI_PASSWORD   "your_wifi_password"

/* ============ MQTT Broker 配置 ============ */
#define MQTT_HOST       "broker.emqx.io"   /* 公共测试服务器，可换成自己的 broker */
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "stm32f103_dev01"  /* 每个设备唯一 */
#define MQTT_USERNAME   ""                  /* 留空表示匿名连接 */
#define MQTT_PASSWORD   ""

/* ============ 主题 ============ */
#define TOPIC_SUB       "stm32/cmd"        /* 订阅：接收控制命令 */
#define TOPIC_PUB       "stm32/data"       /* 发布：周期上报数据 */

#endif /* __CONFIG_H */
