
#pragma once

//#include "mbed.h"
//#include "mbed_debug.h"

#if defined(TARGET_WIZwiki_W7500) || defined(TARGET_WIZwiki_W7500ECO) || defined(TARGET_WIZwiki_W7500P)
    #include "W7500x_toe.h"
#else

    #if MBED_CONF_APP_NETWORK_INTERFACE == ETHERNET_W5500
        #define USE_W5500  // don't use this library
        #include "W5500.h"
    #elif MBED_CONF_APP_NETWORK_INTERFACE == ETHERNET_W6100
        #define USE_W6100  // don't use this library
    #endif
    //#define USE_W5100S // don't use this library

    #if defined MBED_CONF_APP_WIZCHIP_SPI_MOSI
        #define Wizchip_MOSI MBED_CONF_APP_WIZCHIP_SPI_MOSI
    #else
        #define Wizchip_MOSI SPI_MOSI
    #endif
    #if defined MBED_CONF_APP_WIZCHIP_MISO
        #define Wizchip_MISO MBED_CONF_APP_WIZCHIP_SPI_MISO
    #else
        #define Wizchip_MISO SPI_MISO
    #endif
    #if defined MBED_CONF_APP_WIZCHIP_SCK
        #define Wizchip_SCK MBED_CONF_APP_WIZCHIP_SPI_SCK
    #else
        #define Wizchip_SCK SPI_SCK
    #endif
    #if defined MBED_CONF_APP_WIZCHIP_CS
        #define Wizchip_CS MBED_CONF_APP_WIZCHIP_SPI_CS
    #else
        #define Wizchip_CS SPI_CS
    #endif
    #if defined MBED_CONF_APP_WIZCHIP_RESET
        #define Wizchip_RESET MBED_CONF_APP_WIZCHIP_SPI_RESET
    #else
        #define Wizchip_RESET D15
    #endif
#endif






// Arduino pin defaults for convenience
//#if !defined(W5500_SPI_MOSI)
//#define W5500_SPI_MOSI   D11
//#endif
//#if !defined(W5500_SPI_MISO)
//#define W5500_SPI_MISO   D12
//#endif
//#if !defined(W5500_SPI_SCLK)
//#define W5500_SPI_SCLK   D13
//#endif
//#if !defined(W5500_SPI_CS)
//#define W5500_SPI_CS     D10
//#endif
//#if !defined(W5500_SPI_RST)
//#define W5500_SPI_RST    NC
//#endif



/*
// current library do not support next chips.

#if defined(USE_W5100S)
#include "W5100S.h"
#endif

#if defined(USE_W6100)
#include "W6100.h"
#endif

//comming soon!
*/
