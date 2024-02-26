/*******************************************************************************
 *  @file: ws2812_controller.c
 *  
 *  @brief: Application for controlling a WS2812 LED strip.
*******************************************************************************/
#include <stdio.h>
#include "nvs_flash.h"
#include "ProtoRpc.h"
#include "PbGeneric.h"
#include "LogPrint.h"
#include "ProtoRpcApp.pb.h"
#include "Config.pb.h"
#include "mdns.h"

#include "RtosUtils.h"
#include "SwTimer.h"
#include "WifiConnect.h"
#include "UdpRpcServer.h"
#include "TcpRpcServer.h"
#include "CList.h"
#include "CheckCond.h"
#include "WS2812Led.h"

static const char *TAG = "[app]";

static Config config;
static TcpRpcServer tcp_rpc;

#include "TestRpc.h"
#include "RtosUtilsRpc.h"

/******************************************************************************/
/** @brief RPC Declarations */

static ProtoRpc_Resolver_Entry resolvers[] = {
    PROTORPC_ADD_CALLSET(RpcFrame_test_callset_tag, TestRpc_resolver),
    PROTORPC_ADD_CALLSET(RpcFrame_rtosutils_callset_tag, RtosUtilsRpc_resolver),
};

static ProtoRpc rpc = ProtoRpc_init(RpcFrame, resolvers);
/******************************************************************************/

#define RPCSERVER_STACK_SIZE    4*1024

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


WS2812Led_Strip led_strip = {
    .numPixels = 144,
    .loopDelay_ms = 10,
    .rmt_gpio = 12,
    .rmt_resolution_hz = 10000000,
    .taskStackSize = 4*1024,
    .taskName = "Led0",
    .taskPrio = 15
};

WS2812Led_Segment segment0 = {
    .startIdx = 0,
    .endIdx = 143,
    .taskName = "seg0",
    .taskStackSize = 2*1024,
    .taskPrio = 10,
    .loopDelay_ms = 10
};


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
    status = UdpRpcServer_Task_init(&rpc, RPCSERVER_STACK_SIZE);
    if (status < 0)
    {
        LOGPRINT_ERROR("Error initializing UdpRpcServer.");
    }

    /* Start TCP Rcp server. */
    status = TcpRpcServer_init(
        &tcp_rpc,
        &rpc,
        13001, 
        RPCSERVER_STACK_SIZE,
        20);
    if (status < 0)
    {
        LOGPRINT_ERROR("Error initializing TcpRpcServer.");
    }

    /* Use core 1 to mitigate preemption from wifi task. */
    WS2812Led_init(&led_strip, 1);
    WS2812Led_addSegment(&led_strip, &segment0);

    segment0.off(&segment0);
    WS2812Led_show(&led_strip);

    while (1)
    {
        int i, j;
        CHSV start, end, color;


        segment0.fire(&segment0, true, 120, 100, 10);
        RTOS_TASK_SLEEP_s(20);

        color = WS2812LED_COLOR(HUE_AQUA, 255, 240);
        segment0.dissolve(&segment0, true, &color, 80, 20, 50);
        RTOS_TASK_SLEEP_s(20);

        color = WS2812LED_COLOR(HUE_BLUE, 255, 240);
        segment0.meteor(&segment0, true, &color, 1, 80, true, 20);
        RTOS_TASK_SLEEP_s(20);

        segment0.sparkle(&segment0, true, &color, 1, 10);
        RTOS_TASK_SLEEP_s(10);

        segment0.twinkle(&segment0, true, 50, 10);
        RTOS_TASK_SLEEP_s(10);

        for (i = 0; i < 100; i++)
        {
            segment0.fill_random(&segment0, 255, 50);
            RTOS_TASK_SLEEP_ms(100);
        }

        for (i = 0; i < 1000; i++)
        {
            segment0.fill_rainbow(&segment0, i, 255, 70);
            RTOS_TASK_SLEEP_ms(10);
        }

#if 0
        for (j = 0; j < 255; j+=10)
        {
            for (i = 0; i < 200; i++)
            {
                uint8_t v;
                if (i < 100)
                {
                    v = i;
                }
                else
                {
                    v = 200 - i;
                }

                start = (CHSV){ .h = j, .s = 255, .v = v };
                segment0.fill_solid(&segment0, &start);
                RTOS_TASK_SLEEP_ms(10);
            }
        }

        start = (CHSV){ .h = HUE_BLUE, .s = 255, .v = 80 };
        end = (CHSV){ .h = HUE_BLUE-1, .s = 255, .v = 80 };
        segment0.blend(&segment0,
                       true,
                       &start,
                       &end,
                       GRAD_LONGEST,
                       800,
                       20);

        RTOS_TASK_SLEEP_s(15);
#endif
    }

}
