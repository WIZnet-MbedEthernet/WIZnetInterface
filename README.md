
## WIZnetInterface Library 

### Examples
- UDP Example - https://github.com/WIZnet-MbedEthernet/mbed-os-example-udp_WIZnetInterface

### Program setting for using WiznetInterface in mbed_app.json. 
 - Define SPI interface pin.
 - Define Reset Pin for module reset.

```
{
    "name": "WIZNET",
    "config": {
        "sck": {
            "help": "sck pin for spi connection. defaults to SPI_SCK",
            "value": "SPI_SCK"
        },
        "cs": {
            "help": "cs pin for spi connection. defaults to SPI_CS",
            "value": "SPI_CS"
        },
        "miso": {
            "help": "miso pin for spi connection. defaults to SPI_MISO",
            "value": "SPI_MISO"
        },
        "mosi": {
            "help": "mosi pin for spi connection. defaults to SPI_MOSI",
            "value": "SPI_MOSI"
        },
        "rst": {
            "help": "RESET pin for spi connection. defaults to D15",
            "value": "D15"
        },
        "debug": {
            "help": "Enable debug logs. [true/false]",
            "value": true
        },
        "provide-default": {
            "help": "Provide default WifiInterface. [true/false]",
            "value": false
        },
        "socket-bufsize": {
            "help": "Max socket data heap usage",
            "value": 8192
        }
    },
    "target_overrides": {
    }
}
```
