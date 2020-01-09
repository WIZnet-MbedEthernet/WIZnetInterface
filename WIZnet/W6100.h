#pragma once

#include "mbed.h"
#include "mbed_debug.h"

#include "SPI.h"
#include "DigitalOut.h"
#include "PinNames.h"

#define TEST_ASSERT(A) while(!(A)){debug("\n\n%s@%d %s ASSERT!\n\n",__PRETTY_FUNCTION__,__LINE__,#A);exit(1);};

#define DEFAULT_WAIT_RESP_TIMEOUT 500

enum ProtocolMode {
    CLOSED  = 0,
    TCP4    = 1,
    UDP4    = 2,
    IPRAW4  = 3,
    MACRAW  = 7,
    TCP6    = 9,
    UCP6    = 10,
    IPRAW6  = 11,
    TCPD    = 13,
    UDPD    = 14
};

enum Command {
    OPEN      = 0x01,
    LISTEN    = 0x02,
    CONNECT   = 0x04,
    DISCON    = 0x08,
    CLOSE     = 0x10,
    SEND      = 0x20,
    SEND_MAC  = 0x21, 
    SEND_KEEP = 0x22,
    RECV      = 0x40,
    
};

enum Interrupt {
    INT_CON     = 0x01,
    INT_DISCON  = 0x02,
    INT_RECV    = 0x04,
    INT_TIMEOUT = 0x08,
    INT_SEND_OK = 0x10,
};

enum Status {
    WIZ_SOCK_CLOSED      = 0x00,
    WIZ_SOCK_INIT        = 0x13,
    WIZ_SOCK_LISTEN      = 0x14,
    WIZ_SOCK_SYNSENT     = 0x15,
    WIZ_SOCK_ESTABLISHED = 0x17,
    WIZ_SOCK_CLOSE_WAIT  = 0x1c,
    WIZ_SOCK_UDP         = 0x22,
};

#define MAX_SOCK_NUM 8

//W6100 Common Register
#define CIDR          0x0000
#define VER           0x0002
#define SYSR          0x2000
#define SYCR          0x2004
#define TCNTR         0x2016
#define TCNTCLR       0x2020
#define IR            0x2100
#define SIR           0x2101
#define SLIR          0x2102
#define IMR           0x2104
#define IRCLR         0x2108
#define SIMR          0x2114
#define SLIMR         0x2124
#define SLIRCLR       0x2128
#define SLPSR         0x212C
#define SLCR          0x2130
#define PHYSR         0x3000
#define PHYRAR        0x3008
#define PHYDIR        0x300C
#define PHYDOR        0x3010
#define PHYACR        0x3014
#define PHYDIVR       0x3018
#define PHYCR         0x301C
#define NET4MR        0x4000
#define NET6MR        0x4004
#define NETMR         0x4008
#define NETMR2        0x4009
#define PTMR          0x4100
#define PMNR          0x4104
#define PHAR          0x4108
#define PSIDR         0x4110
#define PMRUR         0x4114
#define SHAR          0x4120
#define GAR           0x4130
#define SUBR          0x4134
#define SIPR          0x4138
#define LLAR          0x4140
#define GUAR          0x4150
#define SUB6R         0x4160
#define GA6R          0x4170
#define SLDIP6R       0x4180
#define SLDIR         0x418C
#define SLDHAR        0x4190
#define PINGIDR       0x4198
#define PINGSEQR      0x419C
#define UIPR          0x41A0
#define UPORTR        0x41A4
#define UIP6R         0x41B0
#define UPORT6R       0x41C0
#define INTPTMR       0x41C5
#define PLR           0x41D0
#define PFR           0x41D4
#define VLTR          0x41D8
#define PLTR          0x41DC
#define PAR           0x41E0
#define ICMP6BLKR     0x41F0
#define CHPLCKR       0x41F4
#define NETLCKR       0x41F5
#define PHYLCKR       0x41F6
#define RTR           0x4200
#define RCR           0x4204
#define SLRTR         0x4208
#define SLRCR         0x420c
#define SLHOPR        0x420F

//W6100 Socket Register
#define Sn_MR         0x0000
#define Sn_PSP        0x0004
#define Sn_CR         0x0010
#define Sn_IR         0x0020
#define Sn_IMR        0x0024
#define Sn_IRCLR      0x0028
#define Sn_SR         0x0030
#define Sn_ESR        0x0031
#define Sn_PNR        0x0100
#define Sn_TOSR       0x0104
#define Sn_TTLR       0x0108
#define Sn_FRGR       0x010C
#define Sn_MSSR       0x0110
#define Sn_PORTR      0x0114
#define Sn_DHAR       0x0118
#define Sn_DIPR       0x0120
#define Sn_DIP6R      0x0130
//#define Sn_DPORTR     0x0140
#define Sn_DPORT     0x0140
#define Sn_MR2        0x0144
#define Sn_RTR        0x0180
#define Sn_RCR        0x0184
#define Sn_KPALVTR    0x0188
#define Sn_TX_BSR     0x0200
#define Sn_TX_FSR     0x0204
#define Sn_TX_RD      0x0208
#define Sn_TX_WR      0x020C
#define Sn_RX_BSR     0x0220
#define Sn_RX_RSR     0x0224
#define Sn_RX_RD      0x0228
#define Sn_RX_WR      0x022C

//define protocol
#define Sn_MR_TCP            (0x01)
#define Sn_MR_UDP            (0x02)
#define Sn_MR_IPRAW          (0x03)
#define Sn_MR_MACRAW         (0x07)
#define Sn_MR_TCP6           (0x09)
#define Sn_MR_UDP6           (0x0A)
#define Sn_MR_IPRAW6         (0x0B)
#define Sn_MR_TCPD           (0x0D)
#define Sn_MR_UDPD           (0x0E)

//to do modify w6100 version
class WIZnet_Chip {
public:
    /*
    * Constructor
    *
    * @param spi spi class
    * @param cs cs of the W5500
    * @param reset reset pin of the W5500
    */
    WIZnet_Chip(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset);
    //WIZnet_Chip(SPI* spi, PinName cs, PinName reset);

    virtual ~WIZnet_Chip();

    /*
    * check link set interrupt mask check version
    *
    * @return true if connected, false otherwise
    */ 
    bool init_chip();

    /*
    * Set MAC Address to W5500
    *
    * @return true if connected, false otherwise
    */ 
    bool setmac();

    /*
    * Set Network Informations (SrcIP, Netmask, Gataway)
    *
    * @return true if connected, false otherwise
    */ 
    bool setip();

    bool setip6();
    /*
    * Disconnect the connection
    *
    * @ returns true 
    */
    bool disconnect(int socket);

    /*
    * Open a tcp connection with the specified host on the specified port
    *
    * @param host host (can be either an ip address or a name. If a name is provided, a dns request will be established)
    * @param port port
    * @ returns true if successful
    */
    bool connect(int socket, const char * host, int port, int timeout_ms = 10*1000);

    /*
    * Set the mode (UDP or TCP)
    * //to do modify script
    * @param p WizMode
    * @ returns true if successful
    */
    bool setProtocolMode(int socket, ProtocolMode p);

    /*
    * Set local port number
    *
    * @return true if connected, false otherwise
    */ 
	bool setLocalPort(int socket, uint16_t port);

    /*
    * Reset the W6100
    */
    void reset();
   
    int wait_readable(int socket, int wait_time_ms, int req_size = 0);

    int wait_writeable(int socket, int wait_time_ms, int req_size = 0);

    /*
    * Check if a tcp link is active
    *
    * @returns true if successful
    */
    bool is_connected(int socket);

    /*
    * Open a socket
    *
    * @ returns true if successful
    */
    bool open(int socket, int proto);

    /*
    * Close a tcp connection
    *
    * @ returns true if successful
    */
    bool close(int socket);

    /*
    * Check if status of socket is closed.
    *
    * Used by automatically open socket (bind to local address).
    *
    * @ returns true if socket is closed.
    */
    bool is_closed(int socket);

    /*
    * Check status of socket.
    *
    * @ returns 1byte socket status.
    */
    uint8_t socket_status(int socket);

    /*
    * @param str string to be sent
    * @param len string length
    */
    int send(int socket, const char * str, int len);

    int recv(int socket, char* buf, int len);

    /*
    * Return true if the module is using dhcp
    *
    * @returns true if the module is using dhcp
    */
    bool isDHCP() {
        return dhcp;
    }

    bool gethostbyname(const char* host, uint32_t* ip);

    static WIZnet_Chip * getInstance() {
        return inst;
    };

    int new_socket();
    uint16_t new_port();
    void scmd(int socket, Command cmd);

    template<typename T>
    void sreg(int socket, uint16_t addr, T data) {
        reg_wr<T>(addr, (0x0C + (socket << 5)), data);
    }

    template<typename T>
    T sreg(int socket, uint16_t addr) {
        return reg_rd<T>(addr, (0x08 + (socket << 5)));
    }

    template<typename T>
    void reg_wr(uint16_t addr, T data) {
        return reg_wr(addr, 0x04, data);
    }
    
    template<typename T>
    void reg_wr(uint16_t addr, uint8_t cb, T data) {
        uint8_t buf[sizeof(T)];
        *reinterpret_cast<T*>(buf) = data;
        for(int i = 0; i < sizeof(buf)/2; i++) { //  Little Endian to Big Endian
            uint8_t t = buf[i];
            buf[i] = buf[sizeof(buf)-1-i];
            buf[sizeof(buf)-1-i] = t;
        }
        spi_write(addr, cb, buf, sizeof(buf));
    }

    template<typename T>
    T reg_rd(uint16_t addr) {
        return reg_rd<T>(addr, 0x00);
    }

    template<typename T>
    T reg_rd(uint16_t addr, uint8_t cb) {
        uint8_t buf[sizeof(T)];
        spi_read(addr, cb, buf, sizeof(buf));
        for(int i = 0; i < sizeof(buf)/2; i++) { // Big Endian to Little Endian
            uint8_t t = buf[i];
            buf[i] = buf[sizeof(buf)-1-i];
            buf[sizeof(buf)-1-i] = t;
        }
        return *reinterpret_cast<T*>(buf);
    }

    void reg_rd_mac(uint16_t addr, uint8_t* data) {
        spi_read(addr, 0x00, data, 6);
    }

    void reg_wr_ip(uint16_t addr, uint8_t cb, const char* ip) {
        uint8_t buf[4];
        char* p = (char*)ip;
        for(int i = 0; i < 4; i++) {
            buf[i] = atoi(p);
            p = strchr(p, '.');
            if (p == NULL) {
                break;
            }
            p++;
        }
        spi_write(addr, cb, buf, sizeof(buf));
    }

    void reg_wr_ip6(uint16_t addr, const uint8_t *data)
    {
        spi_write(addr, 0x04, data, 16);
    }
    void reg_rd_ip6(uint16_t addr, uint8_t* data) {
        spi_read(addr, 0x00, data, 16);
    }

    void sreg_ip(int socket, uint16_t addr, const char* ip) {
        reg_wr_ip(addr, (0x0C + (socket << 5)), ip);
    }

    uint8_t mac[6];
    uint32_t ip;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dnsaddr;
    bool dhcp;
    uint8_t     lla[16];   ///< Source Link Local Address
    uint8_t     gua[16];   ///< Source Global Unicast Address
    uint8_t     sn6[16];   ///< IPv6 Prefix
    uint8_t     gw6[16];   ///< Gateway IPv6 Address
    uint8_t     dns6[16];  ///< DNS server IPv6 Address

protected:
    static WIZnet_Chip* inst;

    void reg_wr_mac(uint16_t addr, uint8_t* data) {
        spi_write(addr, 0x04, data, 6);
    }

    void spi_write(uint16_t addr, uint8_t cb, const uint8_t *buf, uint16_t len);
    void spi_read(uint16_t addr, uint8_t cb, uint8_t *buf, uint16_t len);
    SPI* spi;
    DigitalOut cs;
    DigitalOut reset_pin;
};

extern uint32_t str_to_ip(const char* str);
extern void printfBytes(char* str, uint8_t* buf, int len);
extern void printHex(uint8_t* buf, int len);
extern void debug_hex(uint8_t* buf, int len);
