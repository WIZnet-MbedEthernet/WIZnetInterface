// modified 08/08/2018 by Bongjun Hur

#include "mbed.h"
#include "mbed_debug.h"
#include "wiznet.h"

#ifdef USE_W5500

//Debug is disabled by default
#define w5500_DBG 0

#if w5500_DBG
#define DBG(...) do{debug("%p %d %s ", this,__LINE__,__PRETTY_FUNCTION__); debug(__VA_ARGS__); } while(0);
//#define DBG(x, ...) debug("[W5500:DBG]"x"\r\n", ##__VA_ARGS__);
#define WARN(x, ...) debug("[W5500:WARN]"x"\r\n", ##__VA_ARGS__);
#define ERR(x, ...) debug("[W5500:ERR]"x"\r\n", ##__VA_ARGS__);
#define INFO(x, ...) debug("[W5500:INFO]"x"\r\n", ##__VA_ARGS__);
#else
#define DBG(x, ...)
#define WARN(x, ...)
#define ERR(x, ...)
#define INFO(x, ...)
#endif

#define DBG_SPI 0

#if !defined(MBED_CONF_W5500_SPI_SPEED)
#define MBED_CONF_W5500_SPI_SPEED   100000
#endif


WIZnet_Chip* WIZnet_Chip::inst;

WIZnet_Chip::WIZnet_Chip(PinName mosi, PinName miso, PinName sclk, PinName _cs, PinName _reset):
    cs(_cs), reset_pin(_reset)
{
    spi = new SPI(mosi, miso, sclk);
    DBG("SPI interface init...\n");
    spi->format(32, 0);
    spi->frequency(MBED_CONF_W5500_SPI_SPEED);
    cs = 1;
    reset_pin = 1;
    inst = this;
    dhcp = false;
}

/*
WIZnet_Chip::WIZnet_Chip(SPI* spi, PinName _cs, PinName _reset):
    cs(_cs), reset_pin(_reset)
{
    this->spi = spi;
    cs = 1;
    reset_pin = 1;
    inst = this;
    dhcp = false;
}
*/

WIZnet_Chip::~WIZnet_Chip()
{
    delete spi;
}

bool WIZnet_Chip::setmac()
{
    reg_wr_mac(SHAR, mac);
    return true;
}

// Set the IP
bool WIZnet_Chip::setip()
{
    reg_wr<uint32_t>(SIPR, ip);
    reg_wr<uint32_t>(GAR, gateway);
    reg_wr<uint32_t>(SUBR, netmask);
    return true;
}

bool WIZnet_Chip::setProtocolMode(int socket, ProtocolMode p)
{
    if (socket < 0) {
        return false;
    }
    sreg<uint8_t>(socket, Sn_MR, p);
    return true;
}

bool WIZnet_Chip::setLocalPort(int socket, uint16_t port)
{
    if (socket < 0) {
        return false;
    }
    sreg<uint16_t>(socket, Sn_PORT, port);
    return true;
}

bool WIZnet_Chip::connect(int socket, const char * host, int port, int timeout_ms)
{
    if (socket < 0) {
        return false;
    }
    sreg<uint8_t>(socket, Sn_MR, TCP);
    scmd(socket, OPEN);
    sreg_ip(socket, Sn_DIPR, host);
    sreg<uint16_t>(socket, Sn_DPORT, port);
    sreg<uint16_t>(socket, Sn_PORT, new_port());
    scmd(socket, CONNECT);
    Timer t;
    t.reset();
    t.start();
    while(!is_connected(socket)) {
        if ((timeout_ms) && (t.read_ms() > timeout_ms)) {
            return false;
        }
    }
    return true;
}

bool WIZnet_Chip::gethostbyname(const char* host, uint32_t* ip)
{
#if 0
    uint32_t addr = str_to_ip(host);
    char buf[17];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", (addr>>24)&0xff, (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff);
    if (strcmp(buf, host) == 0) {
        *ip = addr;
        return true;
    }
    DNSClient client;
    if(client.lookup(host)) {
        *ip = client.ip;
        return true;
    }
#endif
    return false;
}

bool WIZnet_Chip::disconnect()
{
    return true;
}

bool WIZnet_Chip::is_connected(int socket)
{
    uint8_t tmpSn_SR;
    tmpSn_SR = sreg<uint8_t>(socket, Sn_SR);
    // packet sending is possible, when state is SOCK_CLOSE_WAIT.
    if ((tmpSn_SR == WIZ_SOCK_ESTABLISHED) || (tmpSn_SR == WIZ_SOCK_CLOSE_WAIT)) {
        return true;
    }
    return false;
}

// Reset the chip & set the buffer
void WIZnet_Chip::reset()
{
//    reset_pin = 1;
    reset_pin = 0;
    thread_sleep_for(1); // 500us (w5500)
    reset_pin = 1;
    thread_sleep_for(400); // 400ms (w5500)

#if defined(USE_WIZ550IO_MAC)
    //reg_rd_mac(SHAR, mac); // read the MAC address inside the module
#endif
    //reg_wr_mac(SHAR, mac);
    
    // set RX and TX buffer size
    for (int socket = 0; socket < MAX_SOCK_NUM; socket++) {
        sreg<uint8_t>(socket, Sn_RXBUF_SIZE, 2);
        sreg<uint8_t>(socket, Sn_TXBUF_SIZE, 2);
    }
    
    reg_rd_mac(SHAR, mac); // read the MAC address inside the module
}


bool WIZnet_Chip::close(int socket)
{
    if (socket < 0) {
        return false;
    }
    // if not connected, return
    if (sreg<uint8_t>(socket, Sn_SR) == WIZ_SOCK_CLOSED) {
        return true;
    }
    if (sreg<uint8_t>(socket, Sn_MR) == TCP) {
        scmd(socket, DISCON);
    }
    scmd(socket, CLOSE);
    sreg<uint8_t>(socket, Sn_IR, 0xff);
    return true;
}

bool WIZnet_Chip::is_closed(int socket)
{
    if (sreg<uint8_t>(socket, Sn_SR) == WIZ_SOCK_CLOSED) {
        return true;
    }
    return false;
}

int WIZnet_Chip::wait_readable(int socket, int wait_time_ms, int req_size)
{
    if (socket < 0) {
        return -1;
    }
    
    Timer t;
    t.reset();
    t.start();
    while(1) {
        //int size = sreg<uint16_t>(socket, Sn_RX_RSR);
        int size1, size2;
        // during the reading Sn_RX_RSR, it has the possible change of this register.
        // so read twice and get same value then use size information.
        while (1) {
            size1 = sreg<uint16_t>(socket, Sn_RX_RSR);
            size2 = sreg<uint16_t>(socket, Sn_RX_RSR);
            
            if (size1 == size2) {
                break;
            }
            
            if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms) {
               return -1;
            }
            
            if (!is_connected(socket)) {
            return NSAPI_ERROR_NO_CONNECTION;
            }
        }
        
        if ((size1 > req_size) || (wait_time_ms != (-1) && t.read_ms() > wait_time_ms)) 
        {
            return size1;
        }
    }
    return NSAPI_ERROR_WOULD_BLOCK;
}

int WIZnet_Chip::wait_writeable(int socket, int wait_time_ms, int req_size)
{
    if (socket < 0) {
        return -1;
    }
    Timer t;
    t.reset();
    t.start();
    t.start();
        
    while(1) {
        //int size = sreg<uint16_t>(socket, Sn_TX_FSR);
        int size1, size2;
        // during the reading Sn_TX_FSR, it has the possible change of this register.
        // so read twice and get same value then use size information.
        do {
            size1 = sreg<uint16_t>(socket, Sn_TX_FSR);
            size2 = sreg<uint16_t>(socket, Sn_TX_FSR);
            DBG("The time taken was %d %d %f seconds\n", wait_time_ms, t.read_ms(), t.read());
            
            if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms) {
                
                return NSAPI_ERROR_WOULD_BLOCK;
            }        
        } while (size1 != size2);
        if (size1 > req_size) {
            return size1;
        }
        if (wait_time_ms != (-1) && t.read_ms() > wait_time_ms) {
            break;
        }
    }
    return NSAPI_ERROR_WOULD_BLOCK;
}

int WIZnet_Chip::send(int socket, const char * str, int len)
{
    if (socket < 0) {
        return -1;
    }
    uint16_t ptr = sreg<uint16_t>(socket, Sn_TX_WR);
    uint8_t cntl_byte = (0x14 + (socket << 5));
    spi_write(ptr, cntl_byte, (uint8_t*)str, len);
    sreg<uint16_t>(socket, Sn_TX_WR, ptr + len);
    scmd(socket, SEND);
    uint8_t tmp_Sn_IR;
    while (( (tmp_Sn_IR = sreg<uint8_t>(socket, Sn_IR)) & INT_SEND_OK) != INT_SEND_OK) {
        // @Jul.10, 2014 fix contant name, and udp sendto function.
        switch (sreg<uint8_t>(socket, Sn_SR)) {
            case WIZ_SOCK_CLOSED :
                close(socket);
                return 0;
                //break;
            case WIZ_SOCK_UDP :
                // ARP timeout is possible.
                if ((tmp_Sn_IR & INT_TIMEOUT) == INT_TIMEOUT) {
                    sreg<uint8_t>(socket, Sn_IR, INT_TIMEOUT);
                    return 0;
                }
                break;
            default :
                break;
        }
    }
    sreg<uint8_t>(socket, Sn_IR, INT_SEND_OK);

    return len;
}

int WIZnet_Chip::recv(int socket, char* buf, int len)
{
    if (socket < 0) {
        return -1;
    }
    uint16_t ptr = sreg<uint16_t>(socket, Sn_RX_RD);
    uint8_t cntl_byte = (0x18 + (socket << 5));
    spi_read(ptr, cntl_byte, (uint8_t*)buf, len);
    sreg<uint16_t>(socket, Sn_RX_RD, ptr + len);
    scmd(socket, RECV);
    return len;
}

int WIZnet_Chip::new_socket()
{
    for(int s = 0; s < MAX_SOCK_NUM; s++) {
        if (sreg<uint8_t>(s, Sn_SR) == WIZ_SOCK_CLOSED) {
            return s;
        }
    }
    return -1;
}

uint16_t WIZnet_Chip::new_port()
{
    uint16_t port = rand();
    port |= 49152;
    return port;
}

void WIZnet_Chip::scmd(int socket, Command cmd)
{
    sreg<uint8_t>(socket, Sn_CR, cmd);
    while(sreg<uint8_t>(socket, Sn_CR));
}

void WIZnet_Chip::spi_write(uint16_t addr, uint8_t cb, const uint8_t *buf, uint16_t len)
{
    spi->lock();
    cs = 0;
    spi->write(addr >> 8);
    spi->write(addr & 0xff);
    spi->write(cb);
    for(int i = 0; i < len; i++) {
        spi->write(buf[i]);
    }
    cs = 1;

#if DBG_SPI
    debug("[SPI]W %04x(%02x %d)", addr, cb, len);
    for(int i = 0; i < len; i++) {
        debug(" %02x", buf[i]);
        if (i > 16) {
            debug(" ...");
            break;
        }
    }
    debug("\r\n");
#endif
    spi->unlock();

}

void WIZnet_Chip::spi_read(uint16_t addr, uint8_t cb, uint8_t *buf, uint16_t len)
{
    spi->lock();
    cs = 0;
    spi->write(addr >> 8);
    spi->write(addr & 0xff);
    spi->write(cb);
    for(int i = 0; i < len; i++) {
        buf[i] = spi->write(0);
    }
    cs = 1;

#if DBG_SPI
    debug("[SPI]R %04x(%02x %d)", addr, cb, len);
    for(int i = 0; i < len; i++) {
        debug(" %02x", buf[i]);
        if (i > 16) {
            debug(" ...");
            break;
        }
    }
    debug("\r\n");
    if ((addr&0xf0ff)==0x4026 || (addr&0xf0ff)==0x4003) {
        thread_sleep_for(200);
    }
#endif
    spi->unlock();
}

uint32_t str_to_ip(const char* str)
{
    uint32_t ip = 0;
    char* p = (char*)str;
    for(int i = 0; i < 4; i++) {
        ip |= atoi(p);
        p = strchr(p, '.');
        if (p == NULL) {
            break;
        }
        ip <<= 8;
        p++;
    }
    return ip;
}

void printfBytes(char* str, uint8_t* buf, int len)
{
    printf("%s %d:", str, len);
    for(int i = 0; i < len; i++) {
        printf(" %02x", buf[i]);
    }
    printf("\n");
}

void printHex(uint8_t* buf, int len)
{
    for(int i = 0; i < len; i++) {
        if ((i%16) == 0) {
            printf("%p", buf+i);
        }
        printf(" %02x", buf[i]);
        if ((i%16) == 15) {
            printf("\n");
        }
    }
    printf("\n");
}

void debug_hex(uint8_t* buf, int len)
{
    for(int i = 0; i < len; i++) {
        if ((i%16) == 0) {
            debug("%p", buf+i);
        }
        debug(" %02x", buf[i]);
        if ((i%16) == 15) {
            debug("\n");
        }
    }
    debug("\n");
}

#endif
