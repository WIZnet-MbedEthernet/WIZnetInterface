
#pragma once

//#include "mbed.h"
//#include "mbed_debug.h"

#if defined(TARGET_WIZwiki_W7500) || defined(TARGET_WIZwiki_W7500ECO) || defined(TARGET_WIZwiki_W7500P)
    #include "W7500x_toe.h"
#else

    #define USE_W5500  // don't use this library
    //#define USE_W5100S // don't use this library
    //#define USE_W6100  // don't use this library

    #if defined(USE_W5500)
    #include "W5500.h"
    //#define USE_WIZ550IO_MAC    // want to use the default MAC address stored in the WIZ550io
    #endif

#endif



//#define ETHERNET          1
//#define ETERNET_WIZTOE     2



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
