#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "ProtoRpc.h"
#include "TestRpc.h"
#include "ProtoRpcApp.pb.h"

static const char *TAG = "[app]";

#define LOGE_PRINT(fmt, ...) \
    ESP_LOGE(TAG, "(l:%u) " fmt, __LINE__, ##__VA_ARGS__)

#define LOGI_PRINT(fmt, ...) \
    ESP_LOGI(TAG, "(l:%u) " fmt, __LINE__, ##__VA_ARGS__)

static ProtoRpc_Resolver_Entry resolvers[] = {
    PROTORPC_ADD_CALLSET(RpcFrame_test_callset_tag, TestRpc_resolver)
};

#define NUM_RESOLVERS   PROTORPC_ARRAY_LENGTH(resolvers)

static ProtoRpc_info rpc_info = { 
    .header_offset = offsetof(RpcFrame, header),
    .which_callset_offset = offsetof(RpcFrame, which_callset),
    .callset_offset = offsetof(RpcFrame, callset),
    .frame_fields = RpcFrame_fields,
};

static uint8_t rcv_msg[2*sizeof(RpcFrame)];
static uint8_t reply_msg[2*sizeof(RpcFrame)];

void app_main(void)
{
    esp_err_t ret;
    size_t len;
    nvs_handle_t handle;
    uint32_t reply_size;

    /** @brief Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nvs_open("rpc", NVS_READONLY, &handle);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        return;
    }
    LOGI_PRINT("The NVS handle successfully opened");

    ret = nvs_get_blob(handle, "rpc_msg.bin", NULL, &len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) getting rcv_msg len.\n", esp_err_to_name(ret));
        return;
    }
    LOGI_PRINT("rcv_msg len = %u", len);

    ret = nvs_get_blob(handle, "rpc_msg.bin", rcv_msg, &len);
    if (ret != ESP_OK)
    {
        LOGE_PRINT("Error (%s) getting rcv_msg.\n", esp_err_to_name(ret));
        return;
    }

    ProtoRpc_server(
        &rpc_info,
        resolvers,
        NUM_RESOLVERS,
        rcv_msg,
        len,
        reply_msg,
        sizeof(reply_msg),
        &reply_size);

    LOGI_PRINT("reply_size = %u", (unsigned int)reply_size);
    if (reply_size)
    {
        ESP_LOG_BUFFER_HEXDUMP(TAG, reply_msg, reply_size, ESP_LOG_INFO);
    }
    LOGI_PRINT("hello from rpc_example!");
}
