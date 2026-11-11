#ifndef __ESP8266_H
#define __ESP8266_H

#include <stdint.h>
#include <stdbool.h>

/* ESP8266 通过 USART2 (PA2=TX, PA3=RX) 连接 */
bool esp8266_init(void);
bool esp8266_join_ap(const char *ssid, const char *pass);
bool esp8266_tcp_connect(const char *host, uint16_t port);
void esp8266_close(void);

/* 向 TCP 连接发送原始字节（内部使用 AT+CIPSEND） */
bool esp8266_send(const uint8_t *data, uint16_t len);
/* 从 TCP 连接读取原始字节（自动解析 +IPD,<len>: 帧头） */
int  esp8266_read(uint8_t *buf, uint16_t len, uint32_t timeout_ms);

/* 连接是否被远端断开 */
bool esp8266_is_closed(void);

#endif /* __ESP8266_H */
