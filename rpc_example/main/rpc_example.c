#include <stdio.h>
#include "nvs_flash.h"
#include "ProtoRpc.h"
#include "PbGeneric.h"
#include "SwTimer.h"
#include "LogPrint.h"
#include "ProtoRpcApp.pb.h"
#include "Config.pb.h"
#include "mdns.h"

#include "WifiConnect.h"
#include "UdpRpcServer.h"
#include "TcpRpcServer.h"
#include "TcpEcho.h"
#include "UdpEcho.h"

static const char *TAG = "[app]";
static Config config;

static TcpEcho tcp_echo;
static UdpEcho udp_echo;
static TcpRpcServer tcp_rpc;
static UdpRpcServer udp_rpc;

/******************************************************************************/
/** @brief RPC Declarations */
#include "TestRpc.h"
#include "RtosUtilsRpc.h"

#define RPCSERVER_STACK_SIZE    4*1024

static ProtoRpc_Resolver_Entry resolvers[] = {
    PROTORPC_ADD_CALLSET(RpcFrame_test_callset_tag, TestRpc_resolver),
    PROTORPC_ADD_CALLSET(RpcFrame_rtosutils_callset_tag, RtosUtilsRpc_resolver),
};

static ProtoRpc rpc = ProtoRpc_init(RpcFrame, resolvers);
/******************************************************************************/


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
    LOGPRINT_INFO("hostname = %s", net_config->hostname);

    return 0;
}

void app_main(void)
{
    esp_err_t ret;
    int status;

    /** @brief Allow component-scope log level control.
        See LogPrint_local.h
     */
    esp_log_level_set("*", ESP_LOG_DEBUG);

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
    if (status != 0) return;
    
    ret = WifiConnect_init(
        config.wifi_config.ssid,
        config.wifi_config.pass,
        config.net_config.use_dhcp,
        config.net_config.ip,
        config.net_config.netmask,
        config.net_config.gw);
    if (ret != ESP_OK)  return;

    ret = mdns_init();
    if (ret == ESP_OK)
    {
        mdns_hostname_set(config.net_config.hostname);
        //mdns_instance_name_set("First esp32 board");
    }
    else
    {
        LOGPRINT_ERROR("mdns init failed (ret=%d).", (int) ret);
    }

    /* Start UDP Rcp server. */
    status = UdpRpcServer_init(
        &udp_rpc,
        &rpc,
        13000, 
        RPCSERVER_STACK_SIZE,
        20);
    if (status < 0) LOGPRINT_ERROR("Error initializing UdpRpcServer.");

    /* Start TCP Rcp server. */
    status = TcpRpcServer_init(
        &tcp_rpc,
        &rpc,
        13001, 
        RPCSERVER_STACK_SIZE,
        20);
    if (status < 0) LOGPRINT_ERROR("Error initializing TcpRpcServer.");

    status = UdpEcho_init(
        &udp_echo,
        12000,
        NULL,
        2*1024,
        4*1024,
        "UDP Echo",
        20, 5);
    if (status < 0) LOGPRINT_ERROR("Error initializing Udp Echo server.");

    status = TcpEcho_init(
        &tcp_echo,
        12001,
        NULL,
        1024,
        4*1024,
        "TCP Echo",
        20);
    if (status < 0) LOGPRINT_ERROR("Error initializing Tcp Echo server.");

}
