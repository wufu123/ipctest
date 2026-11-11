#include "stm32f1xx_hal.h"
#include "esp8266.h"
#include "uart_ring.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

#define ESP_UART_TIMEOUT 1000U

static ring_buffer_t esp_rx_ring;
static uint8_t       rx_byte;

/* 已解析出的 TCP 净荷缓冲（去掉了 +IPD 帧头） */
#define TCP_BUF_SIZE 1024
static uint8_t  tcp_buf[TCP_BUF_SIZE];
static uint16_t tcp_head = 0;
static uint16_t tcp_tail = 0;

static bool esp_closed = false;

/* ---------- TCP 净荷缓冲操作 ---------- */
static uint16_t tcp_available(void)
{
    return (uint16_t)((tcp_head - tcp_tail + TCP_BUF_SIZE) % TCP_BUF_SIZE);
}

static bool tcp_push(uint8_t c)
{
    uint16_t next = (uint16_t)((tcp_head + 1) % TCP_BUF_SIZE);
    if (next == tcp_tail) {
        return false;
    }
    tcp_buf[tcp_head] = c;
    tcp_head = next;
    return true;
}

static bool tcp_pop(uint8_t *c)
{
    if (tcp_head == tcp_tail) {
        return false;
    }
    *c = tcp_buf[tcp_tail];
    tcp_tail = (uint16_t)((tcp_tail + 1) % TCP_BUF_SIZE);
    return true;
}

/* ---------- 解析一条 +IPD,<len>:<data> 帧 ---------- */
static bool decode_ipd(void)
{
    uint16_t idx;
    if (!ring_find(&esp_rx_ring, "+IPD,", &idx)) {
        return false;
    }

    uint32_t len = 0;
    uint16_t scan = (uint16_t)(idx + 5);        /* 跳过 "+IPD," */
    bool     colon = false;

    /* 解析长度数字，直到 ':' */
    while (1) {
        uint8_t c;
        if (!ring_peek(&esp_rx_ring, scan, &c)) {
            return false;                       /* 数据尚未完整到达 */
        }
        if (c == ':') {
            colon = true;
            break;
        }
        if (c >= '0' && c <= '9') {
            len = len * 10U + (uint32_t)(c - '0');
            scan++;
        } else {
            return false;                       /* 格式异常 */
        }
    }

    uint16_t data_pos = (uint16_t)(scan + 1);   /* 首个数据字节的位置 */

    if (ring_available(&esp_rx_ring) < data_pos + len) {
        return false;                           /* 数据体还没收全，等待 */
    }

    /* 将数据体搬到 TCP 净荷缓冲 */
    for (uint16_t i = 0; i < (uint16_t)len; i++) {
        uint8_t c;
        ring_peek(&esp_rx_ring, (uint16_t)(data_pos + i), &c);
        if (!tcp_push(c)) {
            break;                              /* 净荷缓冲满，丢弃剩余 */
        }
    }

    /* 连同帧头前的杂散字节一并丢弃 */
    ring_discard(&esp_rx_ring, (uint16_t)(data_pos + len));
    return true;
}

static void check_closed(void)
{
    uint16_t idx;
    if (ring_find(&esp_rx_ring, "CLOSED", &idx)) {
        esp_closed = true;
        ring_clear(&esp_rx_ring);
    }
}

/* ---------- AT 命令辅助 ---------- */
static bool esp_wait_for(const char *str, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (1) {
        uint16_t idx;
        if (ring_find(&esp_rx_ring, str, &idx)) {
            ring_discard(&esp_rx_ring, (uint16_t)(idx + strlen(str)));
            return true;
        }
        if (HAL_GetTick() - start > timeout_ms) {
            return false;
        }
    }
}

static bool esp_at_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    ring_clear(&esp_rx_ring);
    if (HAL_UART_Transmit(&huart2, (uint8_t *)cmd, (uint16_t)strlen(cmd),
                          ESP_UART_TIMEOUT) != HAL_OK) {
        return false;
    }
    return esp_wait_for(expect, timeout_ms);
}

static bool esp8266_config(void)
{
    if (!esp_at_cmd("ATE0\r\n", "OK", 2000)) return false;
    if (!esp_at_cmd("AT+CWMODE=1\r\n", "OK", 3000)) return false;
    if (!esp_at_cmd("AT+CIPMUX=0\r\n", "OK", 2000)) return false;
    return true;
}

/* ---------- 对外接口 ---------- */
bool esp8266_init(void)
{
    ring_init(&esp_rx_ring);
    ring_clear(&esp_rx_ring);
    tcp_head = 0;
    tcp_tail = 0;
    esp_closed = false;

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);

    for (int i = 0; i < 5; i++) {
        if (esp_at_cmd("AT\r\n", "OK", 1000)) {
            return esp8266_config();
        }
        HAL_Delay(500);
    }
    return false;
}

bool esp8266_join_ap(const char *ssid, const char *pass)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pass);
    return esp_at_cmd(cmd, "OK", 20000);
}

bool esp8266_tcp_connect(const char *host, uint16_t port)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",
             host, (unsigned)port);
    if (!esp_at_cmd(cmd, "OK", 20000)) {
        return false;
    }
    ring_clear(&esp_rx_ring);
    tcp_head = 0;
    tcp_tail = 0;
    esp_closed = false;
    return true;
}

void esp8266_close(void)
{
    esp_at_cmd("AT+CIPCLOSE\r\n", "OK", 2000);
    esp_closed = false;
}

bool esp8266_send(const uint8_t *data, uint16_t len)
{
    char cmd[32];
    int  n = snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", (unsigned)len);

    ring_clear(&esp_rx_ring);
    if (HAL_UART_Transmit(&huart2, (uint8_t *)cmd, (uint16_t)n,
                          ESP_UART_TIMEOUT) != HAL_OK) {
        return false;
    }
    if (!esp_wait_for(">", 3000)) {
        return false;
    }
    if (HAL_UART_Transmit(&huart2, data, len, 3000) != HAL_OK) {
        return false;
    }
    return true;
}

int esp8266_read(uint8_t *buf, uint16_t len, uint32_t timeout_ms)
{
    uint16_t got = 0;
    uint32_t start = HAL_GetTick();

    while (got < len) {
        check_closed();
        if (esp_closed) {
            break;
        }
        while (decode_ipd()) {
            /* 尽量解析出所有完整帧 */
        }
        while (got < len && tcp_available() > 0) {
            uint8_t c;
            if (tcp_pop(&c)) {
                buf[got++] = c;
            }
        }
        if (got >= len) {
            break;
        }
        if (HAL_GetTick() - start > timeout_ms) {
            break;
        }
    }
    return (int)got;
}

bool esp8266_is_closed(void)
{
    return esp_closed;
}

/* USART2 接收中断回调：字节入环形缓冲，并重新武装 1 字节接收 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        ring_put(&esp_rx_ring, rx_byte);
        HAL_UART_Receive_IT(huart, &rx_byte, 1);
    }
}
