
## WIZnetInterface Library 

### Program setting for using WiznetInterface in mbed_app.json. 
 - Define SPI interface pin.
 - Define Reset Pin for module reset.

```
{
    "config": {
        "network-interface":{
            "help": "options are ETHERNET, ETHERNET_W5500, ETHERNET_W6100",
            "value": "ETHERNET_W5500"
        },
    "WIZchip-SPI-MOSI": {
            "help": "Pin used as SPI MOSI (connects to WIZNET CHIP SPI MOSI)",
            "value": "SPI_MOSI"
        },
    "WIZchip-SPI-MISO": {
            "help": "Pin used as SPI MOSI (connects to WIZNET CHIP SPI MISO)",
            "value": "SPI_MISO"
        },
    "WIZchip-SPI-SCK": {
            "help": "Pin used as SPI MOSI (connects to WIZNET CHIP SPI SCK)",
            "value": "SPI_SCK"
        },
    "WIZchip-SPI-CS": {
            "help": "Pin used as SPI MOSI (connects to WIZNET CHIP SPI CS)",
            "value": "SPI_CS"
        },
    "WIZchip-SPI-RESET": {
            "help": "Pin used as SPI MOSI (connects to WIZNET CHIP SPI RESET)",
            "value": "D15"
        }
    },
    "target_overrides": {
        "*": {
            "platform.stdio-baud-rate": 115200,
            "platform.stdio-convert-newlines": true,
            "mbed-trace.enable": 0
        }
    }
}
```
