/* USER CODE BEGIN Includes */
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "esp8266.h"
#include "mqtt.h"
/* USER CODE END Includes */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void Error_Handler(void);

static void dbg(const char *s);
static void on_message(const char *topic, const uint8_t *payload, uint16_t len);

static uint32_t last_pub  = 0;
static uint32_t pub_count = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    dbg("\r\n=== STM32F103 MQTT Terminal ===\r\n");

    esp8266_init();
    mqtt_set_callback(on_message);

    bool online = false;

    while (1)
    {
        if (!online || !mqtt_is_connected() || esp8266_is_closed())
        {
            online = false;
            mqtt_disconnect();
            esp8266_close();

            dbg("Connecting WiFi...\r\n");
            if (esp8266_join_ap(WIFI_SSID, WIFI_PASSWORD))
            {
                dbg("WiFi OK\r\n");
                if (esp8266_tcp_connect(MQTT_HOST, MQTT_PORT))
                {
                    dbg("TCP OK, MQTT handshake...\r\n");
                    if (mqtt_connect(MQTT_CLIENT_ID, MQTT_USERNAME,
                                     MQTT_PASSWORD, 60))
                    {
                        dbg("MQTT connected\r\n");
                        mqtt_subscribe(TOPIC_SUB, 1);
                        online = true;
                    }
                }
            }

            if (!online)
            {
                dbg("Reconnect in 3s...\r\n");
                HAL_Delay(3000);
                continue;
            }
        }

        /* 周期上报数据 */
        if (HAL_GetTick() - last_pub >= 5000)
        {
            char payload[128];
            snprintf(payload, sizeof(payload),
                     "{\"id\":\"%s\",\"uptime\":%lu,\"cnt\":%lu}",
                     MQTT_CLIENT_ID,
                     (unsigned long)(HAL_GetTick() / 1000),
                     (unsigned long)pub_count++);

            if (mqtt_publish(TOPIC_PUB, (uint8_t *)payload,
                             (uint16_t)strlen(payload), 1))
            {
                dbg("PUB: ");
                dbg(payload);
                dbg("\r\n");
            }
            last_pub = HAL_GetTick();
        }

        mqtt_loop();
        HAL_Delay(10);
    }
}

/* 收到订阅消息的回调 */
static void on_message(const char *topic, const uint8_t *payload, uint16_t len)
{
    dbg("RX [");
    dbg(topic);
    dbg("] ");

    char tmp[64];
    uint16_t n = (len < sizeof(tmp) - 1) ? len : (uint16_t)(sizeof(tmp) - 1);
    memcpy(tmp, payload, n);
    tmp[n] = 0;
    dbg(tmp);
    dbg("\r\n");

    /* 简单命令：ON 点亮 LED，OFF 熄灭 LED */
    if (strcmp(topic, TOPIC_SUB) == 0)
    {
        if (len == 2 && memcmp(payload, "ON", 2) == 0)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* LED 亮 */
        }
        else if (len == 3 && memcmp(payload, "OFF", 3) == 0)
        {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* LED 灭 */
        }
    }
}

static void dbg(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 1000);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(200);
    }
}

/* HSE 8MHz, PLL x9 = 72MHz */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.HSIState       = RCC_HSI_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                         RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* PC13 板载 LED（低电平点亮） */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin   = GPIO_PIN_13;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &gpio);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* 初始熄灭 */
}

/* USART1 调试串口 PA9=TX PA10=RX */
static void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USART2 接 ESP8266 PA2=TX PA3=RX（需在 CubeMX 中使能全局中断） */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}
