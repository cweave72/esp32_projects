#include <stdio.h>
#include "nvs_flash.h"
#include "ProtoRpc.h"
#include "PbGeneric.h"
#include "SwTimer.h"
#include "LogPrint.h"
#include "ProtoRpcApp.pb.h"
#include "Config.pb.h"
#include "mdns.h"
#include "CheckCond.h"

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

static uint8_t rpc_call_frame[RpcFrame_size]  = { 0 };
static uint8_t rpc_reply_frame[RpcFrame_size] = { 0 };

/******************************************************************************/
/** @brief RPC Declarations */
#include "TestRpc.h"
#include "RtosUtilsRpc.h"
#include "Lfs_PartRpc.h"

#define RPCSERVER_STACK_SIZE    8*1024

static ProtoRpc_Resolver_Entry resolvers[] = {
    PROTORPC_ADD_CALLSET(RpcFrame_test_callset_tag, TestRpc_resolver),
    PROTORPC_ADD_CALLSET(RpcFrame_rtosutils_callset_tag, RtosUtilsRpc_resolver),
    PROTORPC_ADD_CALLSET(RpcFrame_lfs_callset_tag, Lfs_PartRpc_resolver),
};

static ProtoRpc rpc = ProtoRpc_init(RpcFrame,
                                    rpc_call_frame,
                                    rpc_reply_frame,
                                    resolvers);

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

#include "Lfs_Part.h"
#include "lfs_helpers.h"
static Lfs_Part_t lpfs = { 0 };

static void
ls(lfs_t *lfs, const char *path)
{
    int ret;
    lfs_dir_t dir;
    struct lfs_info info;

    ret = lfs_dir_open(lfs, &dir, path);
    if (ret < 0)
    {
        LOGPRINT_ERROR("Failed open dir %s", path);
        return;
    }

    LOGPRINT_INFO("Directory listing: %s", path);
    while (1)
    {
        ret = lfs_dir_read(lfs, &dir, &info);
        if (ret > 0)
        {
            LOGPRINT_INFO("%c %u: %s",
                (info.type == LFS_TYPE_REG) ? 'f' : 'd',
                (unsigned int)info.size,
                info.name);
        }
        else 
        {
            lfs_dir_close(lfs, &dir);
            break;
        }
    }
}

static void
update_bootcount(Lfs_Part_t *lpfs)
{
    lfs_t *lfs = &lpfs->lfs;
    lfs_file_t fp;
    int ret;
    unsigned int boot_count;

    if (lfs_exists(lfs, "boot_count.txt", NULL) != LFS_ERR_OK)
    {
        LOGPRINT_INFO("Creating boot_count.txt");

        ret = lfs_file_open(lfs, &fp, "boot_count.txt",
            LFS_O_RDWR | LFS_O_CREAT);
        CHECK_COND_VOID_RETURN_MSG(ret < 0, "Failed to open file.");

        boot_count = 0;

        ret = lfs_file_write(lfs, &fp, &boot_count, sizeof(boot_count));
        if (ret < 0)
        {
            LOGPRINT_ERROR("Failed on write: %d", ret);
            lfs_file_close(lfs, &fp);
            return;
        }

        lfs_file_close(lfs, &fp);
    }

    ret = lfs_file_open(lfs, &fp, "boot_count.txt", LFS_O_RDWR);
    CHECK_COND_VOID_RETURN_MSG(ret < 0, "Failed to open file (exists).");

    ret = lfs_file_read(lfs, &fp, &boot_count, sizeof(boot_count));
    CHECK_COND_VOID_RETURN_MSG(ret < 0, "Failed to read.");

    LOGPRINT_INFO("boot_count = %u", boot_count);

    boot_count++;

    lfs_file_rewind(lfs, &fp);
    ret = lfs_file_write(lfs, &fp, &boot_count, sizeof(boot_count));
    CHECK_COND_VOID_RETURN_MSG(ret < 0, "Failed to write.");

    ret = lfs_file_close(lfs, &fp);
    CHECK_COND_VOID_RETURN_MSG(ret < 0, "Failed close.");
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


    ret = Lfs_Part_init(&lpfs, "littlefs", &status);
    if (ret == LFS_ERR_OK)
    {
        Lfs_Part_register(&lpfs);
        Lfs_PartRpc_init(&lpfs);
        update_bootcount(&lpfs);
    }

    ls(&lpfs.lfs, "/");
    
    //ret = lfs_format(&lpfs.lfs, &lpfs.cfg);
    //LOGPRINT_INFO("ret = %d", ret);
    
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
