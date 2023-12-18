#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "SwTimer.h"
#include "UdpSocket.h"
#include "WifiConnect.h"

#define BLINK_GPIO   2

#define TASK_SLEEP_ms(ms)   vTaskDelay((ms)/portTICK_PERIOD_MS)

static int connect_success = -1;

static TaskHandle_t blinkThread;
static TaskHandle_t udpThread;

#define LED_ON     gpio_set_level(BLINK_GPIO, 1)
#define LED_OFF    gpio_set_level(BLINK_GPIO, 0)

static const char *TAG = "[app]";

#define LOGE_PRINT(fmt, ...) \
    ESP_LOGE(TAG, "(l:%u) " fmt, __LINE__, ##__VA_ARGS__)

#define LOGI_PRINT(fmt, ...) \
    ESP_LOGI(TAG, "(l:%u) " fmt, __LINE__, ##__VA_ARGS__)

//static SwTimer periodic_tim;
//static SwTimer interval_tim;
static SwTimer oneshot_tim;
static SwTimer oneshot_interval_tim;

static UdpSocket udp_sock;
static char rxbuf[512];

static WifiConfig wifi_config = { 0 };


static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT_OUTPUT);
}

#if 0
static void periodic_func(void)
{
    uint64_t delay = SwTimer_toc(&interval_tim);
    SwTimer_tic(&interval_tim);
    printf("hello world (%u)\r\n", (unsigned int)delay);
}
#endif

static void oneshot_func(void)
{
    uint64_t delay = SwTimer_toc(&oneshot_interval_tim);
    printf("one shot (%u)\r\n", (unsigned int)delay);
}

static void blink_led(void *p)
{
    SwTimer timer;
    configure_led();

    ESP_LOGI(TAG, "Starting task blink_led.");

    for (;;)
    {
        SwTimer_tic(&timer);

        if (connect_success == 0)
        {
            TASK_SLEEP_ms(700);
            LED_ON;
            TASK_SLEEP_ms(100);
            LED_OFF;
            TASK_SLEEP_ms(100);
            LED_ON;
            TASK_SLEEP_ms(100);
            LED_OFF;
        }
        else
        {
            TASK_SLEEP_ms(500);
            LED_ON;
            TASK_SLEEP_ms(500);
            LED_OFF;
        }
    }
}


static void udp_socket_server(void *p)
{
    int ret;
    char addr_str[128];

    ret = UdpSocket_init(&udp_sock, 13000, 10);
    if (ret < 0)
    {
        LOGE_PRINT("Error initializing socket: %d", ret);
        return;
    }

    while (1)
    {
        int len, ret;
        len = UdpSocket_read(&udp_sock, rxbuf, sizeof(rxbuf));
        if (len > 0)
        {
            UDPSOCKET_GET_ADDR(udp_sock.source_addr, addr_str);
            LOGI_PRINT("Received %d bytes from %s:", len, addr_str);

            ret = UdpSocket_write(&udp_sock, rxbuf, len);
            if (ret == 0)
            {
                LOGI_PRINT("Wrote %d bytes.", len);
            }
        }
        else if (len == 0)
        {
            continue;
        }
    }
}

static esp_err_t
get_string(
    nvs_handle_t handle,
    const char *key,
    char *target,
    uint16_t max,
    size_t *str_len)
{
    esp_err_t ret;

    target[0] = '\0';
    *str_len = 0;

    /* First get the string length. */
    ret = nvs_get_str(handle, key, NULL, str_len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) getting string length!", esp_err_to_name(ret));
        return ret;
    }

    if (*str_len == 0)
    {
        LOGI_PRINT("Empty string.");
        return ESP_OK;
    }
    else if (*str_len > max)
    {
        LOGE_PRINT("Error: string length too large for target!");
        return ESP_ERR_INVALID_SIZE;
    }

    ret = nvs_get_str(handle, key, target, str_len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) getting string!", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}


void app_main(void)
{
    esp_err_t ret;
    size_t str_len;
    nvs_handle_t handle;
    
    /** @brief Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open("config", NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        LOGE_PRINT("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        return;
    }
    LOGI_PRINT("The NVS handle successfully opened");

    uint8_t use_dhcp = 1;
    ret = nvs_get_u8(handle, "use_dhcp", &use_dhcp);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) reading config!\n", esp_err_to_name(ret));
        return;
    }
    wifi_config.use_dhcp = use_dhcp;

    ret = get_string(handle, "ssid", wifi_config.ssid,
        sizeof(wifi_config.ssid), &str_len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("get_string returned %d", ret);
        return;
    }

    ret = get_string(handle, "pass", wifi_config.password,
        sizeof(wifi_config.password), &str_len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("get_string returned %d", ret);
        return;
    }

    LOGI_PRINT("use_dhcp = %u", wifi_config.use_dhcp);
    LOGI_PRINT("Read ssid = %s", wifi_config.ssid);

    connect_success = WifiConnect_init(&wifi_config);

    //SwTimer_tic(&interval_tim);
    //SwTimer_create(&periodic_tim, &periodic_func, NULL);
    //SwTimer_start(&periodic_tim, SWTIMER_TYPE_PERIODIC, 5000000);

    SwTimer_tic(&oneshot_interval_tim);
    SwTimer_create(&oneshot_tim, &oneshot_func, NULL);
    SwTimer_start(&oneshot_tim, SWTIMER_TYPE_ONE_SHOT, 15000000);

    xTaskCreate(blink_led, "LED flash", 4*1024, NULL, 10, &blinkThread);
    xTaskCreate(udp_socket_server, "Udp sock", 4*1024, NULL, 10, &udpThread);
}
