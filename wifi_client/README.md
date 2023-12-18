
## wifi_client

This example project demonstrates how to connect to a wifi AP.

It is assumed you have cloned the esp32_components repo in $HOME/esp/esp32_components

### Configuration

The project is configured by adding a file `nvs_config.csv` to the project root
with contents:

```
key,type,encoding,value
config,namespace,,
ssid,data,string,"<your ssid>"
pass,data,string,"<your password>"
use_dhcp,data,u8,1
ip,data,string,"192.168.1.190"
netmask,data,string,"255.255.255.0"
gw,data,string,"192.168.1.1"
```

### Building

Upon opening a new terminal shell, enable the environment by sourcing the
`export.sh` script. (subsequent builds don't need to do this):

```
> . $HOME/esp/esp-idf/export.sh
```

Then:

```
> ./buildall make
```

### Flashing

Plug the ESP32 dev board into a usb port.  This assumes it connects as
/dev/ttyUSB0 (note that you should be in the `dialout` group for this to work).

First flash the NV config:
```
> ./buildall nvs-flash
```

Then flash the app
```
> ./buildall flash
```

### Monitoring

Run the built-in serial monitor:
```
> ./buildall monitor
```

Note that subsequent rebuild/flash/monitor can be combined as:
```
> ./buildall flashmon
```
