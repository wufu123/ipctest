#include "stm32f1xx_hal.h"
#include "mqtt.h"
#include "esp8266.h"
#include <string.h>

static mqtt_msg_cb cb = NULL;
static bool        connected = false;
static uint32_t    last_tx = 0;
static uint32_t    keepalive_ms = 60000;

static uint8_t txbuf[512];

/* ---------- 剩余长度编解码 ---------- */
static uint16_t encode_remaining_length(uint8_t *buf, uint32_t len)
{
    uint16_t n = 0;
    do {
        uint8_t d = (uint8_t)(len % 128);
        len /= 128;
        if (len > 0) {
            d |= 0x80;
        }
        buf[n++] = d;
    } while (len > 0);
    return n;
}

static uint32_t decode_remaining_length(const uint8_t *buf, uint16_t *n)
{
    uint32_t value = 0;
    uint32_t mult  = 1;
    uint16_t i     = 0;
    uint8_t  b;
    do {
        b = buf[i++];
        value += (uint32_t)(b & 0x7F) * mult;
        mult  *= 128;
        if (i > 4) {
            break;
        }
    } while (b & 0x80);

    if (n) {
        *n = i;
    }
    return value;
}

/* ---------- 组包发送 ---------- */
static bool send_packet(uint8_t type, const uint8_t *body, uint16_t body_len)
{
    txbuf[0] = type;
    uint16_t rl = encode_remaining_length(&txbuf[1], body_len);
    memcpy(&txbuf[1 + rl], body, body_len);

    if (!esp8266_send(txbuf, (uint16_t)(1 + rl + body_len))) {
        return false;
    }
    last_tx = HAL_GetTick();
    return true;
}

/* 读取一个完整 MQTT 报文；>0 长度，0 无数据，-1 错误 */
static int mqtt_read_packet(uint8_t *buf, uint16_t maxlen, uint32_t timeout)
{
    uint8_t hdr;
    if (esp8266_read(&hdr, 1, timeout) != 1) {
        return 0;                                   /* 超时无数据 */
    }
    buf[0] = hdr;

    uint8_t  rl[4];
    uint16_t rl_n  = 0;
    uint32_t rem   = 0;
    uint32_t mult  = 1;
    do {
        if (esp8266_read(&rl[rl_n], 1, timeout) != 1) {
            return -1;                              /* 中途断流 */
        }
        rem += (uint32_t)(rl[rl_n] & 0x7F) * mult;
        mult *= 128;
        rl_n++;
        if (rl_n > 4) {
            return -1;
        }
    } while (rl[rl_n - 1] & 0x80);

    uint16_t total = (uint16_t)(1 + rl_n + rem);
    if (total > maxlen) {
        return -1;
    }
    memcpy(&buf[1], rl, rl_n);
    if (rem > 0) {
        if (esp8266_read(&buf[1 + rl_n], (uint16_t)rem, timeout) != (int)rem) {
            return -1;
        }
    }
    return (int)total;
}

/* ---------- 处理下行 PUBLISH ---------- */
static void handle_publish(uint8_t *pkt, int len)
{
    uint8_t  qos = (uint8_t)((pkt[0] >> 1) & 0x03);
    uint16_t rl_n;
    decode_remaining_length(&pkt[1], &rl_n);

    uint16_t pos  = (uint16_t)(1 + rl_n);
    uint16_t tlen = (uint16_t)((pkt[pos] << 8) | pkt[pos + 1]);
    pos += 2;

    char topic[64];
    uint16_t tl = (tlen < sizeof(topic) - 1) ? tlen : (uint16_t)(sizeof(topic) - 1);
    memcpy(topic, &pkt[pos], tl);
    topic[tl] = 0;
    pos += tlen;

    uint8_t pid_hi = 0, pid_lo = 0;
    if (qos > 0) {
        pid_hi = pkt[pos];
        pid_lo = pkt[pos + 1];
        pos += 2;
    }

    uint16_t plen = (uint16_t)(len - pos);
    if (cb) {
        cb(topic, &pkt[pos], plen);
    }

    if (qos == 1) {
        uint8_t body[2] = { pid_hi, pid_lo };
        send_packet(0x40, body, 2);                 /* PUBACK */
    }
}

static void process_packet(uint8_t *pkt, int len)
{
    switch (pkt[0] & 0xF0) {
    case 0x30:                                      /* PUBLISH */
        handle_publish(pkt, len);
        break;
    case 0xD0:                                      /* PINGRESP */
    default:
        break;
    }
}

/* ---------- 对外接口 ---------- */
void mqtt_set_callback(mqtt_msg_cb c)
{
    cb = c;
}

bool mqtt_connect(const char *client_id, const char *user, const char *pass,
                  uint16_t keepalive_s)
{
    uint8_t body[256];
    uint16_t n = 0;

    /* 协议名 "MQTT" */
    body[n++] = 0x00; body[n++] = 0x04;
    body[n++] = 'M'; body[n++] = 'Q'; body[n++] = 'T'; body[n++] = 'T';
    /* 协议级别 4 = MQTT 3.1.1 */
    body[n++] = 0x04;

    /* 连接标志：Clean Session + 可选用户名/密码 */
    uint8_t flags = 0x02;
    bool has_user = (user != NULL) && (user[0] != '\0');
    bool has_pass = (pass != NULL) && (pass[0] != '\0');
    if (has_user) flags |= 0x80;
    if (has_pass) flags |= 0x40;
    body[n++] = flags;

    /* 保活时间（秒） */
    body[n++] = (uint8_t)(keepalive_s >> 8);
    body[n++] = (uint8_t)(keepalive_s & 0xFF);

    /* Client ID */
    uint16_t idlen = (uint16_t)strlen(client_id);
    body[n++] = (uint8_t)(idlen >> 8);
    body[n++] = (uint8_t)(idlen & 0xFF);
    memcpy(&body[n], client_id, idlen);
    n += idlen;

    if (has_user) {
        uint16_t ulen = (uint16_t)strlen(user);
        body[n++] = (uint8_t)(ulen >> 8);
        body[n++] = (uint8_t)(ulen & 0xFF);
        memcpy(&body[n], user, ulen);
        n += ulen;
    }
    if (has_pass) {
        uint16_t plen = (uint16_t)strlen(pass);
        body[n++] = (uint8_t)(plen >> 8);
        body[n++] = (uint8_t)(plen & 0xFF);
        memcpy(&body[n], pass, plen);
        n += plen;
    }

    if (!send_packet(0x10, body, n)) {              /* CONNECT */
        return false;
    }

    /* 等待 CONNACK */
    uint8_t pkt[8];
    int len = mqtt_read_packet(pkt, sizeof(pkt), 10000);
    if (len < 4) {
        return false;
    }
    if (pkt[0] != 0x20 || pkt[2] != 0x00) {         /* 0x00 = 连接被接受 */
        return false;
    }

    keepalive_ms = (uint32_t)keepalive_s * 1000U;
    connected = true;
    last_tx = HAL_GetTick();
    return true;
}

bool mqtt_subscribe(const char *topic, uint8_t qos)
{
    static uint16_t pid = 1;
    pid++;

    uint8_t body[128];
    uint16_t n = 0;
    body[n++] = (uint8_t)(pid >> 8);
    body[n++] = (uint8_t)(pid & 0xFF);

    uint16_t tlen = (uint16_t)strlen(topic);
    body[n++] = (uint8_t)(tlen >> 8);
    body[n++] = (uint8_t)(tlen & 0xFF);
    memcpy(&body[n], topic, tlen);
    n += tlen;
    body[n++] = qos;

    if (!send_packet(0x82, body, n)) {              /* SUBSCRIBE */
        return false;
    }

    uint8_t pkt[8];
    int len = mqtt_read_packet(pkt, sizeof(pkt), 10000);
    if (len < 5 || (pkt[0] & 0xF0) != 0x90) {       /* SUBACK */
        return false;
    }
    return true;
}

bool mqtt_publish(const char *topic, const uint8_t *payload, uint16_t plen,
                  uint8_t qos)
{
    uint8_t body[256];
    uint16_t n = 0;

    uint16_t tlen = (uint16_t)strlen(topic);
    uint16_t need = (uint16_t)(2 + tlen + (qos ? 2 : 0) + plen);
    if (need > sizeof(body)) {
        return false;                               /* 载荷过大 */
    }

    body[n++] = (uint8_t)(tlen >> 8);
    body[n++] = (uint8_t)(tlen & 0xFF);
    memcpy(&body[n], topic, tlen);
    n += tlen;

    if (qos > 0) {
        static uint16_t pid = 1;
        pid++;
        body[n++] = (uint8_t)(pid >> 8);
        body[n++] = (uint8_t)(pid & 0xFF);
    }

    memcpy(&body[n], payload, plen);
    n += plen;

    uint8_t type = (uint8_t)(0x30 | (qos << 1));
    return send_packet(type, body, n);
}

void mqtt_disconnect(void)
{
    if (connected) {
        uint8_t body[1];
        send_packet(0xE0, body, 0);                 /* DISCONNECT */
    }
    connected = false;
}

bool mqtt_is_connected(void)
{
    return connected;
}

void mqtt_loop(void)
{
    if (!connected) {
        return;
    }

    if (esp8266_is_closed()) {
        connected = false;
        return;
    }

    /* 保活：空闲超时发送 PINGREQ */
    if (HAL_GetTick() - last_tx >= keepalive_ms) {
        uint8_t body[1];
        send_packet(0xC0, body, 0);
        last_tx = HAL_GetTick();
    }

    /* 处理下行报文（非阻塞，1ms 超时） */
    uint8_t pkt[256];
    int len = mqtt_read_packet(pkt, sizeof(pkt), 1);
    while (len > 0) {
        process_packet(pkt, len);
        len = mqtt_read_packet(pkt, sizeof(pkt), 1);
    }
}
