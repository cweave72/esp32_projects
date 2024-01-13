#include <stdio.h>
#include "nvs_flash.h"
#include "ProtoRpc.h"
#include "PbGeneric.h"
#include "LogPrint.h"
#include "ProtoRpcApp.pb.h"
#include "Config.pb.h"

#include "SwTimer.h"
#include "WifiConnect.h"
#include "UdpRpcServer.h"

static const char *TAG = "[app]";
static Config config;

#include "TestRpc.h"
#include "RtosUtilsRpc.h"

/******************************************************************************/
/** @brief RPC Declarations */

static ProtoRpc_Resolver_Entry resolvers[] = {
    PROTORPC_ADD_CALLSET(RpcFrame_test_callset_tag, TestRpc_resolver),
    PROTORPC_ADD_CALLSET(RpcFrame_rtosutils_callset_tag, RtosUtilsRpc_resolver),
};

#define NUM_RESOLVERS   PROTORPC_ARRAY_LENGTH(resolvers)

static ProtoRpc_info rpc_info = { 
    .header_offset = offsetof(RpcFrame, header),
    .which_callset_offset = offsetof(RpcFrame, which_callset),
    .callset_offset = offsetof(RpcFrame, callset),
    .frame_fields = RpcFrame_fields,
};
/******************************************************************************/

#define RPCSERVER_STACK_SIZE    10*1024

/******************************************************************************
    get_config
*//**
    @brief Gets the config object.
******************************************************************************/
static int
get_config(void)
{
    esp_err_t ret;
    WifiConfig *wifi_config = &config.wifi_config;
    NetConfig *net_config = &config.net_config;
    size_t len;
    nvs_handle_t handle;
    uint8_t blob[sizeof(config)];

    ret = nvs_open("config", NVS_READONLY, &handle);
    if (ret != ESP_OK)
    {
        LOGPRINT_ERROR("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
        return -1;
    }
    LOGPRINT_INFO("The NVS handle successfully opened");

    ret = nvs_get_blob(handle, "config.bin", NULL, &len);
    if (ret != ESP_OK)
    {
        LOGPRINT_ERROR("Error (%s) getting blob len.\n", esp_err_to_name(ret));
        return -1;
    }
    LOGPRINT_INFO("blob len = %u", len);

    ret = nvs_get_blob(handle, "config.bin", blob, &len);
    if (ret != ESP_OK)
    {
        LOGPRINT_ERROR("Error (%s) getting blob.\n", esp_err_to_name(ret));
        return -1;
    }

    Pb_unpack(blob, len, &config, (void *)Config_fields);
    LOGPRINT_INFO("ssid = %s", wifi_config->ssid);
    LOGPRINT_INFO("pass = %s", wifi_config->pass);
    LOGPRINT_INFO("use_dhcp = %u", net_config->use_dhcp);
    LOGPRINT_INFO("ip = %s", net_config->ip);
    LOGPRINT_INFO("netmask = %s", net_config->netmask);
    LOGPRINT_INFO("gw = %s", net_config->gw);

    return 0;
}

void app_main(void)
{
    esp_err_t ret;
    int status;

    /** @brief Initialize NVS */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    status = get_config();
    if (status != 0)
    {
        return;
    }
    
    ret = WifiConnect_init(
        config.wifi_config.ssid,
        config.wifi_config.pass,
        config.net_config.use_dhcp,
        config.net_config.ip,
        config.net_config.netmask,
        config.net_config.gw);
    if (ret != ESP_OK)
    {
        return;
    }

    status = UdpRpcServer_Task_init(
        &rpc_info,
        resolvers,
        NUM_RESOLVERS,
        RPCSERVER_STACK_SIZE);
    if (status < 0)
    {
        LOGPRINT_ERROR("Error initializing UdpRpcServer.");
    }
}
