#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "Config.pb.h"
#include "PbGeneric.h"

static const char *TAG = "[app]";

#define LOGE_PRINT(fmt, ...) \
    ESP_LOGE(TAG, "(l:%u) " fmt, __LINE__, ##__VA_ARGS__)

#define LOGI_PRINT(fmt, ...) \
    ESP_LOGI(TAG, "(l:%u) " fmt, __LINE__, ##__VA_ARGS__)


static Config config;

void app_main(void)
{
    esp_err_t ret;
    WifiConfig *wifi_config = &config.wifi_config;
    NetConfig *net_config = &config.net_config;
    size_t len;
    nvs_handle_t handle;
    uint8_t blob[sizeof(config)];
    
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
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        return;
    }
    LOGI_PRINT("The NVS handle successfully opened");

    ret = nvs_get_blob(handle, "config.bin", NULL, &len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) getting blob len.\n", esp_err_to_name(ret));
        return;
    }
    LOGI_PRINT("blob len = %u", len);

    ret = nvs_get_blob(handle, "config.bin", blob, &len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) getting blob.\n", esp_err_to_name(ret));
        return;
    }

    Pb_unpack(blob, len, &config, (void *)Config_fields);
    LOGI_PRINT("ssid = %s", wifi_config->ssid);
    LOGI_PRINT("pass = %s", wifi_config->pass);
    LOGI_PRINT("use_dhcp = %u", net_config->use_dhcp);
    LOGI_PRINT("ip = %s", net_config->ip);
    LOGI_PRINT("netmask = %s", net_config->netmask);
    LOGI_PRINT("gw = %s", net_config->gw);
}
