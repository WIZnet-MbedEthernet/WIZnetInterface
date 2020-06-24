/**
  ******************************************************************************
  * @file    WIZnetInterface.h
  * @author  Justin Kim (modified version from Sergei G (https://os.mbed.com/users/sgnezdov/))
  * @brief   Implementation file of the NetworkStack for the WIZnet Ethernet Device
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, WIZnet SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2019 WIZnet Co.,Ltd.</center></h2>
  ******************************************************************************
  */

#include "mbed.h"
#include "WIZnetInterface.h"
#include "ip6string.h"

static uint8_t WIZNET_DEFAULT_TESTMAC[6] = {0x00, 0x08, 0xdc, 0x19, 0x85, 0xa8};
//static int udp_local_port = 0;

#define SKT(h) ((wiznet_socket*)h)
#define WIZNET_WAIT_TIMEOUT   400
#define WIZNET_WAIT_TIMEOUT1   1000
#define WIZNET_ACCEPT_TIMEOUT 300000 //5 mins timeout, retrun NSAPI_ERROR_WOULD_BLOCK if there is no connection during 5 mins

#if MBED_CONF_WIZNET_DEBUG
#define DBG(...) do{debug("[%s:%d] \n", __PRETTY_FUNCTION__,__LINE__);debug(__VA_ARGS__);} while(0);
#else
#define DBG(...) while(0);
#define INFO(...) do{debug("[%s:%d]", __PRETTY_FUNCTION__,__LINE__);debug(__VA_ARGS__);} while(0);
#endif

#include "DHCPClient.h"
DHCPClient dhcp;
/**
 * @brief   Defines a custom MAC address
 * @note    Have to be unique within the connected network!
 *          Modify the mac array items as needed.
 * @param   mac A 6-byte array defining the MAC address
 * @retval
 */
/* Interface implementation */
#if defined(TARGET_WIZwiki_W7500) || defined(TARGET_WIZwiki_W7500ECO) || defined(TARGET_WIZwiki_W7500P)
    NetworkInterface *NetworkInterface::get_default_instance()
    {
        WIZnetInterface eth;
        return &eth;
    }
#else
#if defined MBED_CONF_WIZNET_DEFAULT_NETWORK
    NetworkInterface *NetworkInterface::get_default_instance()
    {
        //static WIZnetInterface eth(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS, D15);
        static WIZnetInterface eth(WIZNET_MOSI, WIZNET_MISO, WIZNET_SCK, WIZNET_CS, WIZNET_RESET);
        return &eth;
    }
#endif
    WIZnetInterface::WIZnetInterface(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset) :
        _wiznet(mosi, miso, sclk, cs, reset)
        {
            _wiznet.reset();
            ip_set = false;
            _dhcp_enable = true;
            thread_read_socket.start(callback(this, &WIZnetInterface::socket_check_read));
        }

#endif

wiznet_socket* WIZnetInterface::get_sock(int fd, nsapi_protocol_t proto)
{
    for (int i=0; i<MAX_SOCK_NUM ; i++) {
        if (wiznet_sockets[i].fd == -1) {
            wiznet_sockets[i].fd            = fd;
            wiznet_sockets[i].proto         = proto;
            wiznet_sockets[i].connected     = false;
            wiznet_sockets[i].callback      = NULL;
            wiznet_sockets[i].callback_data = NULL;
            return &wiznet_sockets[i];
        }
    }
    return NULL;
}

void WIZnetInterface::init_socks()
{
    for (int i=0; i<MAX_SOCK_NUM ; i++) {
        wiznet_sockets[i].fd            = -1;
        wiznet_sockets[i].proto         = NSAPI_TCP;
        wiznet_sockets[i].connected     = false;
        wiznet_sockets[i].callback      = NULL;
        wiznet_sockets[i].callback_data = NULL;
    }

    dns.setup(get_stack());
    //initialize the socket isr
    //_daemon = new Thread(osPriorityNormal, 1024);
    //_daemon->start(callback(this, &W5500Interface::daemon));
}

uint32_t WIZnetInterface:: str_to_ip(const char* str)
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

int WIZnetInterface::init()
{
    _dhcp_enable = true;
    _wiznet.reg_wr<uint32_t>(SIPR, 0x00000000); // local ip "0.0.0.0"
    //_w5500.reg_wr<uint8_t>(SIMR, 0xFF); //
    for (int i =0; i < 6; i++) _wiznet.mac[i] = WIZNET_DEFAULT_TESTMAC[i];
    _wiznet.setmac();
    init_socks();
    return 0;
}

int WIZnetInterface::init(uint8_t * mac)
{
    _dhcp_enable = true;
    _wiznet.reg_wr<uint32_t>(SIPR, 0x00000000); // local ip "0.0.0.0"
    // should set the mac address and keep the value in this class
    for (int i =0; i < 6; i++) _wiznet.mac[i] = mac[i];
    _wiznet.setmac();
    init_socks();
    return 0;
}

// add this function, because sometimes no needed MAC address in init calling.
int WIZnetInterface::init(const char* ip, const char* mask, const char* gateway)
{
    _dhcp_enable = false;
    
    _wiznet.ip = str_to_ip(ip);
    strcpy(ip_string, ip);
    ip_set = true;
    _wiznet.netmask = str_to_ip(mask);
    _wiznet.gateway = str_to_ip(gateway);

    // @Jul. 8. 2014 add code. should be called to write chip.
    _wiznet.setip();
    init_socks();

    return 0;
}

int WIZnetInterface::init(uint8_t * mac, const char* ip, const char* mask, const char* gateway)
{
    _dhcp_enable = false;
    //
    for (int i =0; i < 6; i++) _wiznet.mac[i] = mac[i];
    //
    _wiznet.ip = str_to_ip(ip);
    strcpy(ip_string, ip);
    ip_set = true;
    _wiznet.netmask = str_to_ip(mask);
    _wiznet.gateway = str_to_ip(gateway);

    #ifdef USE_W6100
    _wiznet.init_chip();
    #endif

    // @Jul. 8. 2014 add code. should be called to write chip.
    _wiznet.setmac();
    _wiznet.setip();
    init_socks();

    return 0;
}

#ifdef USE_W6100
bool WIZnetInterface::init_ipv6(const char *lla, const char *gua, const char *sn6, const char *gw6)
{
    //to do
    if(stoip6(lla, strlen(lla), _wiznet.lla) == false)
    {
        DBG("[Error] lla trans data error \n");
    }
    if(stoip6(gua, strlen(gua), _wiznet.gua) == false)
    {
        DBG("[Error] gua trans data error \n");
    }
    if(stoip6(sn6, strlen(sn6), _wiznet.sn6) == false)
    {
        DBG("[Error] sn6 trans data error \n");
    }
    if(stoip6(gw6, strlen(gw6), _wiznet.gw6) == false)
    {
        DBG("[Error] gw6 trans data error \n");
    }
    _wiznet.setip6();
    return true;
}
bool WIZnetInterface::set_dns6(uint8_t *dns6)
{
    //to do
    return true;
}
#endif
void WIZnetInterface::socket_check_read()
{
    while (1) {
        for (int i = 0; i < MAX_SOCK_NUM; i++) {
            _mutex.lock();
                for (int i=0; i<MAX_SOCK_NUM ; i++) {
                    if (wiznet_sockets[i].fd >= 0 && wiznet_sockets[i].callback) {
                    	int size = _wiznet.sreg<uint16_t>(wiznet_sockets[i].fd, Sn_RX_RSR);
                        if (size > 0) {
                            //led1 = !led1;
                            wiznet_sockets[i].callback(wiznet_sockets[i].callback_data);
                        }
                    }
            }
            _mutex.unlock();
        }
        thread_sleep_for(1);
    }
}

int WIZnetInterface::IPrenew(int timeout_ms)
{
    DBG("[EasyConnect] DHCP start\n");
    int err = dhcp.setup(get_stack(), _wiznet.mac, timeout_ms);
    if (err == (-1)) {
        DBG("[EasyConnect] Timeout.\n");
        return NSAPI_ERROR_DHCP_FAILURE;
    }
    DBG("[EasyConnect] DHCP completed\n");
    DBG("[EasyConnect] Connected, IP: %d.%d.%d.%d\r\n", dhcp.yiaddr[0], dhcp.yiaddr[1], dhcp.yiaddr[2], dhcp.yiaddr[3]);

    char ip[24], gateway[24], netmask[24], dnsaddr[24];
    sprintf(ip,      "%d.%d.%d.%d", dhcp.yiaddr[0],  dhcp.yiaddr[1],  dhcp.yiaddr[2],  dhcp.yiaddr[3]);
    sprintf(gateway, "%d.%d.%d.%d", dhcp.gateway[0], dhcp.gateway[1], dhcp.gateway[2], dhcp.gateway[3]);
    sprintf(netmask, "%d.%d.%d.%d", dhcp.netmask[0], dhcp.netmask[1], dhcp.netmask[2], dhcp.netmask[3]);
    sprintf(dnsaddr, "%d.%d.%d.%d", dhcp.dnsaddr[0], dhcp.dnsaddr[1], dhcp.dnsaddr[2], dhcp.dnsaddr[3]);

    init(ip, netmask, gateway);
    setDnsServerIP(dnsaddr);

    _dhcp_enable = true; // because this value was changed in init(ip, netmask, gateway).

    return 0;
}


int WIZnetInterface::connect()
{
    if (_dhcp_enable) {
        init(); // init default mac address
        int err = IPrenew(15000);
		if (err < 0) return err;
    }

    if (_wiznet.setip() == false) return NSAPI_ERROR_DHCP_FAILURE;
    return 0;
}

bool WIZnetInterface::setDnsServerIP(const char* ip_address)
{
    return dns.set_server(ip_address);
}

bool WIZnetInterface::setMode(ProtocolMode mode)
{
    temp_WizMode = mode;
    DBG("Protocol Mode: %d \n", temp_WizMode);
    return true;
}

int WIZnetInterface::disconnect()
{
    _wiznet.disconnect();
    return 0;
}

const char *WIZnetInterface::get_ip_address()
{
    uint32_t ip = _wiznet.reg_rd<uint32_t>(SIPR);
    snprintf(ip_string, sizeof(ip_string), "%d.%d.%d.%d", (int)((ip>>24)&0xff), (int)((ip>>16)&0xff), (int)((ip>>8)&0xff), (int)(ip&0xff));
    return ip_string;
}

const char *WIZnetInterface::get_netmask()
{
    uint32_t netmask = _wiznet.reg_rd<uint32_t>(SUBR);
    snprintf(netmask_string, sizeof(netmask_string), "%d.%d.%d.%d", (int)((netmask>>24)&0xff), (int)((netmask>>16)&0xff), (int)((netmask>>8)&0xff), (int)(netmask&0xff));
    return netmask_string;
}

const char *WIZnetInterface::get_gateway()
{
    uint32_t gateway = _wiznet.reg_rd<uint32_t>(GAR);
    snprintf(gateway_string, sizeof(gateway_string), "%d.%d.%d.%d", (int)((gateway>>24)&0xff), (int)((gateway>>16)&0xff), (int)((gateway>>8)&0xff), (int)(gateway&0xff));
    return gateway_string;
}

const char *WIZnetInterface::get_mac_address()
{
    uint8_t mac[6];
    _wiznet.reg_rd_mac(SHAR, mac);
    snprintf(mac_string, sizeof(mac_string), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return mac_string;
}

void WIZnetInterface::get_mac(uint8_t mac[6])
{
    _wiznet.reg_rd_mac(SHAR, mac);
}

nsapi_error_t WIZnetInterface::socket_open(nsapi_socket_t *handle, nsapi_protocol_t proto)
{
    //a socket is created the same way regardless of the protocol
    int temp_port;
    int sock_fd = _wiznet.new_socket();
    if (sock_fd < 0) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    wiznet_socket *h = get_sock(sock_fd, proto);

    if (!h) {
        return NSAPI_ERROR_NO_SOCKET;
    }

    h->proto         = proto;
    h->connected     = false;
    h->server        = false;
    h->callback      = NULL;
    h->callback_data = NULL;

    //new up an int to store the socket fd
    h->Protocol_Mode = (ProtocolMode)(proto + 1);

    #ifdef USE_W6100
    if(temp_WizMode != 0)
    {
        h->Protocol_Mode = temp_WizMode;
        temp_WizMode = CLOSED;
    }
    #endif
    *handle = h;
    temp_port = _wiznet.new_port();
    DBG("Protocol Mode : %d , %d \n", SKT(*handle)->Protocol_Mode, temp_WizMode);
    _wiznet.setLocalPort(SKT(*handle)->fd, temp_port);
    _wiznet.setProtocolMode(SKT(*handle)->fd, SKT(*handle)->Protocol_Mode);
    while(_wiznet.is_closed(SKT(*handle)->fd) == true)
    {
        _wiznet.scmd(SKT(*handle)->fd, OPEN);
    }
    DBG("fd: %d, port: %d \n", SKT(*handle)->fd, temp_port);
    return 0;
}

//void W5500Interface::signal_event(nsapi_socket_t handle)
//{
////    DBG("fd: %d\n", SKT(handle)->fd);
////    if (SKT(handle)->callback != NULL) {
////        SKT(handle)->callback(SKT(handle)->callback_data);
////    }
//	if (handle == NULL) return;
//    w5500_socket *socket = (w5500_socket *)handle;
//    w5500_sockets[socket->fd].callback(w5500_sockets[socket->fd].callback_data);
//}

nsapi_error_t WIZnetInterface::socket_close(nsapi_socket_t handle)
{
    if (handle == NULL) return 0;
    DBG("fd: %d\n", SKT(handle)->fd);
    _wiznet.close(SKT(handle)->fd);

    SKT(handle)->fd = -1;

    return 0;
}

nsapi_error_t WIZnetInterface::socket_bind(nsapi_socket_t handle, const SocketAddress &address)
{
    if ((int)handle < 0) {
        return NSAPI_ERROR_DEVICE_ERROR;
    }
    DBG("fd: %d, port: %d\n", SKT(handle)->fd, address.get_port());
    if(_wiznet.is_closed(SKT(handle)->fd) == false)
    {
        _wiznet.close(SKT(handle)->fd); 
    }
    
    switch (SKT(handle)->proto) {
        case NSAPI_UDP:
            // set local port
            if (address.get_port() != 0) {
                _wiznet.setLocalPort( SKT(handle)->fd, address.get_port() );
            } else {
                //udp_local_port++;
                _wiznet.setLocalPort( SKT(handle)->fd, _wiznet.new_port() );
            }
            // set udp protocol
            _wiznet.setProtocolMode(SKT(handle)->fd, SKT(handle)->Protocol_Mode);
            _wiznet.scmd(SKT(handle)->fd, OPEN);
            SKT(handle)->connected = true;
            /*
                        uint8_t tmpSn_SR;
                		tmpSn_SR = _w5500.sreg<uint8_t>(SKT(handle)->fd, Sn_SR);
            		    DBG("open socket status: %2x\n", tmpSn_SR);
            */
            return 0;
        case NSAPI_TCP:
            listen_port = address.get_port();
            // set TCP protocol
            _wiznet.setProtocolMode(SKT(handle)->fd, SKT(handle)->Protocol_Mode);
            // set local port
            _wiznet.setLocalPort( SKT(handle)->fd, address.get_port() );
            // connect the network
            _wiznet.scmd(SKT(handle)->fd, OPEN);
            return 0;
    }

    return NSAPI_ERROR_DEVICE_ERROR;
}

nsapi_error_t WIZnetInterface::socket_listen(nsapi_socket_t handle, int backlog)
{
    DBG("fd: %d\n", SKT(handle)->fd);
    if (SKT(handle)->fd < 0) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    /*    if (backlog != 1) {
            return NSAPI_ERROR_NO_SOCKET;
        }
    */
    SKT(handle)->server = true;
    _mutex.lock();
    _wiznet.scmd(SKT(handle)->fd, LISTEN);
    _mutex.unlock();
    return 0;
}

nsapi_size_or_error_t WIZnetInterface::socket_connect(nsapi_socket_t handle, const SocketAddress &address)
{
    DBG("fd: %d\n", SKT(handle)->fd);
    //check for a valid socket
    _mutex.lock();

    if (SKT(handle)->fd < 0) {
        _mutex.unlock();
        return NSAPI_ERROR_NO_SOCKET;
    }

    DBG("Wiz socket_connect Addres[%s] Port: %d \n", address.get_ip_address(), address.get_port());
    //before we attempt to connect, we are not connected
    SKT(handle)->connected = false;

    //try to connect
    if (!_wiznet.connect(SKT(handle)->fd, address.get_ip_address(), address.get_port(), WIZNET_WAIT_TIMEOUT1)) {
        _mutex.unlock();
        return -1;
    }

    //we are now connected
    SKT(handle)->connected = true;
    _mutex.unlock();

    return 0;
}

nsapi_error_t WIZnetInterface::socket_accept(nsapi_socket_t server, nsapi_socket_t *handle, SocketAddress *address)
{
    SocketAddress _addr;

    DBG("fd: %d\n", SKT(handle)->fd);
    if (SKT(server)->fd < 0) {
        return NSAPI_ERROR_NO_SOCKET;
    }

    SKT(server)->connected = false;

    Timer t;
    t.reset();
    t.start();

    while(1) {
        if (t.read_ms() > WIZNET_ACCEPT_TIMEOUT) {
            DBG("WIZnetInterface::socket_accept, timed out\r\n");
            return NSAPI_ERROR_WOULD_BLOCK;
        }
        if (_wiznet.is_connected(SKT(server)->fd)) break;
    }

    //get socket for the connection
    *handle = get_sock(SKT(server)->fd, SKT(server)->proto);

    if (!(*handle)) {
        error("No more sockets for binding");
        return NSAPI_ERROR_NO_SOCKET;
    }

    //give it all of the socket info from the server
    SKT(*handle)->proto     = SKT(server)->proto;
    SKT(*handle)->connected = true;

    if (address) {
        uint32_t ip = _wiznet.sreg<uint32_t>(SKT(*handle)->fd, Sn_DIPR);
        char host[17];
        snprintf(host, sizeof(host), "%d.%d.%d.%d", (int)((ip>>24)&0xff), (int)((ip>>16)&0xff), (int)((ip>>8)&0xff), (int)(ip&0xff));
        int port = _wiznet.sreg<uint16_t>(SKT(*handle)->fd, Sn_DPORT);

        _addr.set_ip_address(host);
        _addr.set_port(port);
        *address = _addr;
    }


    //create a new tcp socket for the server
    SKT(server)->fd = _wiznet.new_socket();
    if (SKT(server)->fd < 0) {
        //error("No more sockets for listening");
        //return NSAPI_ERROR_NO_SOCKET;
        // already accepted socket, so return 0, but there is no listen socket anymore.
        return 0;
    }

    SKT(server)->proto     = NSAPI_TCP;
    SKT(server)->connected = false;

    _addr.set_port(listen_port);

    // and then, for the next connection, server socket should be assigned new one.
    if (socket_bind(server, _addr) < 0) {
        //error("No more sockets for listening");
        //return NSAPI_ERROR_NO_SOCKET;
        // already accepted socket, so return 0, but there is no listen socket anymore.
        return 0;
    }

    if (socket_listen(server, 1) < 0) {
        //error("No more sockets for listening");
        // already accepted socket, so return 0, but there is no listen socket anymore.
        return 0;
    }

    return 0;
}

nsapi_size_or_error_t WIZnetInterface::socket_send(nsapi_socket_t handle, const void *data, nsapi_size_t size)
{
    DBG("fd: %d\n", SKT(handle)->fd);
    //INFO("fd: %d\n", SKT(handle)->fd);

    nsapi_size_t writtenLen = 0;
    int ret;
    _mutex.lock();
    while (writtenLen < size) {
        int _size =  _wiznet.wait_writeable(SKT(handle)->fd, WIZNET_WAIT_TIMEOUT);
        if (_size < 0) {
            _mutex.unlock();
            return NSAPI_ERROR_WOULD_BLOCK;
        }
        if (_size > (size-writtenLen)) {
            _size = (size-writtenLen);
        }
        ret = _wiznet.send(SKT(handle)->fd, (char*)((uint32_t *)data+writtenLen), (int)_size);
        if (ret < 0) {
            DBG("returning error -1\n");
            _mutex.unlock();
            return -1;
        }
        writtenLen += ret;
    }
    _mutex.unlock();
    return writtenLen;
}

nsapi_size_or_error_t WIZnetInterface::socket_recv(nsapi_socket_t handle, void *data, nsapi_size_t size)
{
    int recved_size = 0;
    //int idx;
    int _size;
    nsapi_size_or_error_t retsize;
    uint8_t socket_status;

    #ifdef USE_W6100 
    DBG("fd: %d port: %d\n", SKT(handle)->fd, _wiznet.sreg<uint16_t>(SKT(handle)->fd, Sn_PORTR));
    #endif
    //INFO("fd: %d\n", SKT(handle)->fd);
    // add to cover exception.
    _mutex.lock();

    if(SKT(handle)->connected == false)
    {
        SKT(handle)->connected = true;
    }
    #if 0//#ifdef USE_W6100
    bool temp_server = SKT(handle)->server;
    uint16_t temp_port;
    switch(_wiznet.socket_status(SKT(handle)->fd))
    {
        case WIZ_SOCK_CLOSED :
        break;
        case WIZ_SOCK_INIT:
            if(SKT(handle)->server == true)
            {
                _mutex.lock();
                _wiznet.scmd(SKT(handle)->fd, LISTEN);
                _mutex.unlock();
            }
        break;
        case WIZ_SOCK_LISTEN:
        break;
        case WIZ_SOCK_SYNSENT:
        break;
        case WIZ_SOCK_ESTABLISHED:
            if(SKT(handle)->connected == false)
            {
                SKT(handle)->connected = true;
            }
        break;
        case WIZ_SOCK_CLOSE_WAIT:
        _wiznet.disconnect(SKT(handle)->fd);
        /*
            temp_port = _wiznet.sreg<uint16_t>(SKT(handle)->fd, Sn_PORTR);
            DBG("fd: port %d \n", temp_port);
            _wiznet.close(SKT(handle)->fd);
            _wiznet.setLocalPort(SKT(handle)->fd, temp_port);
            _wiznet.setProtocol(SKT(handle)->fd, (Protocol)(SKT(handle)->proto + 1));
            _wiznet.scmd(SKT(handle)->fd, OPEN);
            DBG("fd: server %d || socket status [%X]\n", SKT(handle)->server, _wiznet.socket_status(SKT(handle)->fd));
            if(temp_server == true)
            {
                SKT(handle)->server = true;
                _mutex.lock();
                _wiznet.scmd(SKT(handle)->fd, LISTEN);
                _mutex.unlock();
            }
            */
        break;
        default :
        break;
    }
    #endif
    #ifdef USE_W6100
    socket_status = _wiznet.socket_status(SKT(handle)->fd);
    DBG("fd: connected is %d || socket status [%X]\n", SKT(handle)->connected, socket_status);
    #endif

    if ((SKT(handle)->fd < 0) || !SKT(handle)->connected) {
        _mutex.unlock();
        return NSAPI_ERROR_NO_CONNECTION;
    }
    else if(socket_status == 0x1c)
    {
        _mutex.unlock();
        return NSAPI_ERROR_CONNECTION_LOST;
    }
 
     while(1) {
        _size = _wiznet.wait_readable(SKT(handle)->fd, WIZNET_WAIT_TIMEOUT);
        DBG("fd: _size %d recved_size %d\n", _size, recved_size);

        if (_size <= 0) {
            if(recved_size >= 0){
                retsize = recved_size;
                //INFO("recved_size : %d\n",recved_size);
                break;
            }
            _mutex.unlock();
            return NSAPI_ERROR_WOULD_BLOCK;
        }

        if(_size > size)
        {
            _size = size;
        }

        retsize = _wiznet.recv(SKT(handle)->fd, (char*)((uint32_t *)data + recved_size), (int)_size);

        DBG("rv: %d\n", retsize);
        recved_size += _size;

        //if(recved_size >= size)
            break;
    }
#if WIZNET_INTF_DBG
    if (retsize > 0) {
        debug("[socket_recv] buffer:");
        for(int i = 0; i < retsize; i++) {
            if ((i%16) == 0) {
                debug("\n");
            }
            debug(" %02x", ((uint8_t*)data)[i]);
        }
        if ((retsize-1%16) != 0) {
            debug("\n");
        }
    }
#endif
    _mutex.unlock();
    return retsize;
}

nsapi_size_or_error_t WIZnetInterface::socket_sendto(nsapi_socket_t handle, const SocketAddress &address,
        const void *data, nsapi_size_t size)
{
    _mutex.lock();
    DBG("fd: %d, ip: %s:%d\n", SKT(handle)->fd, address.get_ip_address(), address.get_port());
    if (_wiznet.is_closed(SKT(handle)->fd)) {
        return NSAPI_ERROR_NO_SOCKET;
    }
    //compare with original: int size = eth->wait_writeable(_sock_fd, _blocking ? -1 : _timeout, length-1);
    int len = _wiznet.wait_writeable(SKT(handle)->fd, WIZNET_WAIT_TIMEOUT, size-1);
    if (len < 0) {
        DBG("error: NSAPI_ERROR_WOULD_BLOCK\n");
        _mutex.unlock();
        return NSAPI_ERROR_WOULD_BLOCK;
    }

    // set remote host
    _wiznet.sreg_ip(SKT(handle)->fd, Sn_DIPR, address.get_ip_address());
    // set remote port
    _wiznet.sreg<uint16_t>(SKT(handle)->fd, Sn_DPORT, address.get_port());

    nsapi_size_or_error_t err = _wiznet.send(SKT(handle)->fd, (const char*)data, size);
    DBG("rv: %d, size: %d\n", err, size);

#if WIZNET_INTF_DBG
    if (err > 0) {
        debug("[socket_sendto] data: ");
        for(int i = 0; i < err; i++) {
            if ((i%16) == 0) {
                debug("\n");
            }
            debug(" %02x", ((uint8_t*)data)[i]);
        }
        if ((err-1%16) != 0) {
            debug("\n");
        }
    }
#endif
    _mutex.unlock();
    return err;
}

nsapi_size_or_error_t WIZnetInterface::socket_recvfrom(nsapi_socket_t handle, SocketAddress *address,
        void *buffer, nsapi_size_t size)
{
    DBG("fd: %d\n", SKT(handle)->fd);
    //check for null pointers
    if (buffer == NULL) {
        DBG("buffer is NULL; receive is ABORTED\n");
        return -1;
    }

    _mutex.lock();
    uint8_t info[8];
    int len = _wiznet.wait_readable(SKT(handle)->fd, WIZNET_WAIT_TIMEOUT, sizeof(info));
     DBG("Recieve len %d \n", len);
    if(len == 0 )
    {
        _mutex.unlock();
        return 0;
    }
    else if (len < 0) {
        DBG("error: NSAPI_ERROR_WOULD_BLOCK\n");
        _mutex.unlock();
        return NSAPI_ERROR_WOULD_BLOCK;
    }

    
    #ifdef USE_W6100
    uint8_t Temp_Head[2];
    uint16_t port;

    _wiznet.recv(SKT(handle)->fd, (char*)Temp_Head, sizeof(Temp_Head));
    DBG("Head : %X%X\n", Temp_Head[0], Temp_Head[1]);
    nsapi_size_t udp_size = ((Temp_Head[0]&0x07)<<8)|Temp_Head[1];

    if (udp_size > (len-sizeof(info))) {
        DBG("error: udp_size[%d] > (len[%d]-sizeof(info)[%d])\n", udp_size, len, sizeof(info) );
        _mutex.unlock();
        return -1;
    }

    if(Temp_Head[0]&0x80)
    {
        //ipv6
        uint8_t Temp_info[18];
        _wiznet.recv(SKT(handle)->fd, (char*)Temp_info, sizeof(Temp_info));
        //to do next address and port
    }
    else
    {
        //ipv4
        char addr[17];
        uint8_t Temp_info[6];
        _wiznet.recv(SKT(handle)->fd, (char*)Temp_info, sizeof(Temp_info));
        snprintf(addr, sizeof(addr), "%d.%d.%d.%d", Temp_info[0], Temp_info[1], Temp_info[2], Temp_info[3]);
        uint16_t port = Temp_info[4]<<8|Temp_info[5];

        if (address != NULL) {
        //DBG("[socket_recvfrom] warn: addressis NULL");
        address->set_ip_address(addr);
        address->set_port(port);
        }
    }
    
    #else
    //receive endpoint information
    _wiznet.recv(SKT(handle)->fd, (char*)info, sizeof(info));

    char addr[17];
    snprintf(addr, sizeof(addr), "%d.%d.%d.%d", info[0], info[1], info[2], info[3]);
    uint16_t port = info[4]<<8|info[5];
    // original behavior was to terminate execution if address is NULL
    if (address != NULL) {
        //DBG("[socket_recvfrom] warn: addressis NULL");
        address->set_ip_address(addr);
        address->set_port(port);
    }

    nsapi_size_t udp_size = info[6]<<8|info[7];

    if (udp_size > (len-sizeof(info))) {
        DBG("error: udp_size > (len-sizeof(info))\n");
        _mutex.unlock();
        return -1;
    }
    #endif
    //receive from socket
    nsapi_size_or_error_t err = _wiznet.recv(SKT(handle)->fd, (char*)buffer, udp_size);
    DBG("rv: %d\n", err);

#if WIZNET_INTF_DBG
    if (err > 0) {
        debug("[socket_recvfrom] buffer:");
        for(int i = 0; i < err; i++) {
            if ((i%16) == 0) {
                debug("\n");
            }
            debug(" %02x", ((uint8_t*)buffer)[i]);
        }
        if ((err-1%16) != 0) {
            debug("\n");
        }
    }
#endif

    _mutex.unlock();
    return  err;
}

void WIZnetInterface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
//	if (handle == NULL) return;
//	DBG("fd: %d, callback: %p\n", SKT(handle)->fd, callback);
//	SKT(handle)->callback       = callback;
//	SKT(handle)->callback_data  = data;

    if (handle == NULL) return;
    _mutex.lock();
    wiznet_socket *socket = (wiznet_socket *)handle;
    wiznet_sockets[socket->fd].callback = callback;


    wiznet_sockets[socket->fd].callback_data = data;
    _mutex.unlock();
}

void WIZnetInterface::event()
{
    for(int i=0; i<MAX_SOCK_NUM; i++){
        if (wiznet_sockets[i].callback) {
            wiznet_sockets[i].callback(wiznet_sockets[i].callback_data);
        }
    }
}

nsapi_error_t WIZnetInterface::gethostbyname(const char *host,
        SocketAddress *address, nsapi_version_t version, const char *interface_name)
{
    DBG("DNS process %s \n", host);
    bool isOK = dns.lookup(host);
    if (isOK) {
        DBG("is ok\n");
        DBG(" IP: %s\n", dns.get_ip_address());
    } else {
        DBG(" IP is not ok\n");
        return NSAPI_ERROR_DNS_FAILURE;
    }

    if ( !address->set_ip_address(dns.get_ip_address()) ) {
        return NSAPI_ERROR_DNS_FAILURE;
    }
    return NSAPI_ERROR_OK;
}

