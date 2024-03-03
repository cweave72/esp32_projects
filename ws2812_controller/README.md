# WS2812 Controller Application

This project controls a single LED strip containing WS2812-type LEDs.

## NVS Configuration

Configuration data is stored in NVS partition of flash as a Protobuf-packed blob
which is read from NVS and decoded on boot.

NVS configuration CSV: `nvs_config.csv`
```
key,type,encoding,value
config,namespace,,
config.bin,file,binary,/tmp/espressif/build_tmp/config.bin
```

Protobuf encoding and decoding on the ESP32 side is accomplished using the
`nanopb` library. The configuration structure is defined by `main/Config.proto`:

Config.proto:
```
syntax = "proto3";

import "nanopb.proto";

message WifiConfig {
    string ssid = 1 [(nanopb).max_size = 32];
    string pass = 2 [(nanopb).max_size = 64];
}

message NetConfig {
    bool use_dhcp = 1;
    string ip = 2 [(nanopb).max_size = 32];
    string netmask = 3 [(nanopb).max_size = 32];
    string gw = 4 [(nanopb).max_size = 32];
    string hostname = 5 [(nanopb).max_size = 64];
}

message Config {
    WifiConfig wifi_config = 1;
    NetConfig net_config = 2;
}

```

Configuration data is specified via `config.yaml` file:
```yaml
wifi_config:
  ssid: "myssid"
  pass: "mypassword"

net_config:
  use_dhcp: true
  ip: "192.168.1.190"
  netmask: "255.255.255.0"
  gw: "192.168.1.1"
  hostname: "myhost"
```

## Adding a dependency

This app requires the espressif/mdns managed component.  To add this component,
follow these steps:

1. `idf.py idf.py add-dependency espressif/mdns`
2. `idf.py reconfigure`

Step 1 adds the file `idf_component.yml` to `main/`. Step 2 adds the directory
`managed_components/espressif_mdns`.

## Building and Flashing

Build: `./buildall make`

Flash NVS: `buildall flash-nvs`

Flash and monitor: `./buildall flashmon`

## Building the python RPC libs

`./gen_rpc_python.sh`    (output in `python/rpc/lib`)
