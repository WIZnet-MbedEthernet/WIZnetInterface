
#pragma once

//#include "mbed.h"
//#include "mbed_debug.h"
#define W5500       1
#define W6100       2

#if defined(TARGET_WIZwiki_W7500) || defined(TARGET_WIZwiki_W7500ECO) || defined(TARGET_WIZwiki_W7500P)
    #include "W7500x_toe.h"
#else

    #if MBED_CONF_WIZNET_INTERFACE == W5500
        #define USE_W5500  
        #include "W5500.h"
    #elif MBED_CONF_WIZNET_INTERFACE == W6100
        #define USE_W6100  
        #include "W6100.h"
    #endif
    //#define USE_W5100S // don't use this library

    #if defined MBED_CONF_WIZNET_MOSI
        #define WIZNET_MOSI MBED_CONF_WIZNET_MOSI
    #else
        #define WIZNET_MOSI SPI_MOSI
    #endif
    #if defined MBED_CONF_WIZNET_MISO
        #define WIZNET_MISO MBED_CONF_WIZNET_MISO
    #else
        #define WIZNET_MISO SPI_MISO
    #endif
    #if defined MBED_CONF_WIZNET_SCK
        #define WIZNET_SCK MBED_CONF_WIZNET_SCK
    #else
        #define WIZNET_SCK SPI_SCK
    #endif
    #if defined MBED_CONF_WIZNET_CS
        #define WIZNET_CS MBED_CONF_WIZNET_CS
    #else
        #define WIZNET_CS SPI_CS
    #endif
    #if defined MBED_CONF_WIZNET_RESET
        #define WIZNET_RESET MBED_CONF_WIZNET_RESET
    #else
        #define WIZNET_RESET D15
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

*/
