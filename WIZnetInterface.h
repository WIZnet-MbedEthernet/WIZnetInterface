/**
  ******************************************************************************
  * @file    WIZnetInterface.h
  * @author  Justin Kim (modified version from Sergei G (https://os.mbed.com/users/sgnezdov/))
  * @brief   Header file of the NetworkStack for the WIZnet Ethernet Device
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

#ifndef WIZNET_INTERFACE_H
#define WIZNET_INTERFACE_H

#include "mbed.h"
#include "wiznet.h"
#include "rtos.h"
#include "PinNames.h"
#include "DHCPClient.h"
#include "DNSClient.h"

/** wiznet_socket struct
 *  Wiznet socket 
 */
 
struct wiznet_socket {
   int   fd;
   nsapi_protocol_t proto;
   bool  connected;
   bool  server;
   ProtocolMode Protocol_Mode;
   void  (*callback)(void *);
   void  *callback_data;
}; 

/** W5500Interface class
 *  Implementation of the NetworkStack for the W5500
 */

class WIZnetInterface : public NetworkStack, public EthInterface
{
public:

#if (not defined TARGET_WIZwiki_W7500) && (not defined TARGET_WIZwiki_W7500P) && (not defined TARGET_WIZwiki_W7500ECO)
    //W5500Interface(SPI* spi, PinName cs, PinName reset);
    WIZnetInterface(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName reset);
#endif
    int init();
    int init(uint8_t * mac);
    int init(const char* ip, const char* mask, const char* gateway);
    int init(uint8_t * mac, const char* ip, const char* mask, const char* gateway);
 
    bool init_ipv6(const char *lla, const char *gua, const char *sn6, const char *gw6);
    bool set_dns6(uint8_t *dns6);
    /** Start the interface
     *  @return             0 on success, negative on failure
     */
    virtual int connect();
 
    /** Stop the interface
     *  @return             0 on success, negative on failure
     */
    virtual int disconnect();
 
    /** Get the internally stored IP address
     *  @return             IP address of the interface or null if not yet connected
     */
    virtual const char *get_ip_address();
     /** Get the local gateway
     *
     *  @return         Null-terminated representation of the local gateway
     *                  or null if no network mask has been recieved
     */
    virtual const char *get_gateway();

    /** Get the local network mask
     *
     *  @return         Null-terminated representation of the local network mask
     *                  or null if no network mask has been recieved
     */
    virtual const char *get_netmask();
 
    /** Get MAC address and fill mac with it. 
    */
    void get_mac(uint8_t mac[6]) ;
 
    /** Get the internally stored MAC address
     *  @return             MAC address of the interface
     */
    virtual const char *get_mac_address();

    bool setDnsServerIP(const char* ip_address);

    bool setMode(ProtocolMode mode);
    /** Translates a hostname to an IP address with specific version
     *
     *  The hostname may be either a domain name or an IP address. If the
     *  hostname is an IP address, no network transactions will be performed.
     *
     *  If no stack-specific DNS resolution is provided, the hostname
     *  will be resolve using a UDP socket on the stack.
     *
     *  @param address  Destination for the host SocketAddress
     *  @param host     Hostname to resolve
     *  @param version  IP version of address to resolve, NSAPI_UNSPEC indicates
     *                  version is chosen by the stack (defaults to NSAPI_UNSPEC)
     *  @return         0 on success, negative error code on failure
     */
    //using NetworkInterface::gethostbyname;
    virtual nsapi_error_t gethostbyname(const char *host,
            SocketAddress *address, nsapi_version_t version = NSAPI_UNSPEC, const char *interface_name = NULL);    

	int IPrenew(int timeout_ms = 15*1000);
	uint32_t str_to_ip(const char* str);
	
protected:
    /** Opens a socket
     *
     *  Creates a network socket and stores it in the specified handle.
     *  The handle must be passed to following calls on the socket.
     *
     *  A stack may have a finite number of sockets, in this case
     *  NSAPI_ERROR_NO_SOCKET is returned if no socket is available.
     *
     *  @param handle   Destination for the handle to a newly created socket
     *  @param proto    Protocol of socket to open, NSAPI_TCP or NSAPI_UDP
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t socket_open(nsapi_socket_t *handle, nsapi_protocol_t proto);
//    virtual int socket_open(void **handle, nsapi_protocol_t proto);

    /** Close the socket
     *
     *  Closes any open connection and deallocates any memory associated
     *  with the socket.
     *
     *  @param handle   Socket handle
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t socket_close(nsapi_socket_t handle);

    /** Bind a specific address to a socket
     *
     *  Binding a socket specifies the address and port on which to recieve
     *  data. If the IP address is zeroed, only the port is bound.
     *
     *  @param handle   Socket handle
     *  @param address  Local address to bind
     *  @return         0 on success, negative error code on failure.
     */
    virtual nsapi_error_t socket_bind(nsapi_socket_t handle, const SocketAddress &address);

    /** Listen for connections on a TCP socket
     *
     *  Marks the socket as a passive socket that can be used to accept
     *  incoming connections.
     *
     *  @param handle   Socket handle
     *  @param backlog  Number of pending connections that can be queued
     *                  simultaneously
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t socket_listen(nsapi_socket_t handle, int backlog);

    /** Connects TCP socket to a remote host
     *
     *  Initiates a connection to a remote server specified by the
     *  indicated address.
     *
     *  @param handle   Socket handle
     *  @param address  The SocketAddress of the remote host
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t socket_connect(nsapi_socket_t handle, const SocketAddress &address);

    /** Accepts a connection on a TCP socket
     *
     *  The server socket must be bound and set to listen for connections.
     *  On a new connection, creates a network socket and stores it in the
     *  specified handle. The handle must be passed to following calls on
     *  the socket.
     *
     *  A stack may have a finite number of sockets, in this case
     *  NSAPI_ERROR_NO_SOCKET is returned if no socket is available.
     *
     *  This call is non-blocking. If accept would block,
     *  NSAPI_ERROR_WOULD_BLOCK is returned immediately.
     *
     *  @param server   Socket handle to server to accept from
     *  @param handle   Destination for a handle to the newly created socket
     *  @param address  Destination for the remote address or NULL
     *  @return         0 on success, negative error code on failure
     */
    virtual nsapi_error_t socket_accept(nsapi_socket_t server,
            nsapi_socket_t *handle, SocketAddress *address=0);
     
    /** Send data over a TCP socket
     *
     *  The socket must be connected to a remote host. Returns the number of
     *  bytes sent from the buffer.
     *
     *  This call is non-blocking. If send would block,
     *  NSAPI_ERROR_WOULD_BLOCK is returned immediately.
     *
     *  @param handle   Socket handle
     *  @param data     Buffer of data to send to the host
     *  @param size     Size of the buffer in bytes
     *  @return         Number of sent bytes on success, negative error
     *                  code on failure
     */
    virtual nsapi_size_or_error_t socket_send(nsapi_socket_t handle,
            const void *data, nsapi_size_t size);

    /** Receive data over a TCP socket
     *
     *  The socket must be connected to a remote host. Returns the number of
     *  bytes received into the buffer.
     *
     *  This call is non-blocking. If recv would block,
     *  NSAPI_ERROR_WOULD_BLOCK is returned immediately.
     *
     *  @param handle   Socket handle
     *  @param data     Destination buffer for data received from the host
     *  @param size     Size of the buffer in bytes
     *  @return         Number of received bytes on success, negative error
     *                  code on failure
     */
    virtual nsapi_size_or_error_t socket_recv(nsapi_socket_t handle,
            void *data, nsapi_size_t size);

    /** Send a packet over a UDP socket
     *
     *  Sends data to the specified address. Returns the number of bytes
     *  sent from the buffer.
     *
     *  This call is non-blocking. If sendto would block,
     *  NSAPI_ERROR_WOULD_BLOCK is returned immediately.
     *
     *  @param handle   Socket handle
     *  @param address  The SocketAddress of the remote host
     *  @param data     Buffer of data to send to the host
     *  @param size     Size of the buffer in bytes
     *  @return         Number of sent bytes on success, negative error
     *                  code on failure
     */
    virtual nsapi_size_or_error_t socket_sendto(nsapi_socket_t handle, const SocketAddress &address,
            const void *data, nsapi_size_t size);

    /** Receive a packet over a UDP socket
     *
     *  Receives data and stores the source address in address if address
     *  is not NULL. Returns the number of bytes received into the buffer.
     *
     *  This call is non-blocking. If recvfrom would block,
     *  NSAPI_ERROR_WOULD_BLOCK is returned immediately.
     *
     *  @param handle   Socket handle
     *  @param address  Destination for the source address or NULL
     *  @param data     Destination buffer for data received from the host
     *  @param size     Size of the buffer in bytes
     *  @return         Number of received bytes on success, negative error
     *                  code on failure
     */
    virtual nsapi_size_or_error_t socket_recvfrom(nsapi_socket_t handle, SocketAddress *address,
            void *buffer, nsapi_size_t size);

    /** Register a callback on state change of the socket
     *
     *  The specified callback will be called on state changes such as when
     *  the socket can recv/send/accept successfully and on when an error
     *  occurs. The callback may also be called spuriously without reason.
     *
     *  The callback may be called in an interrupt context and should not
     *  perform expensive operations such as recv/send calls.
     *
     *  @param handle   Socket handle
     *  @param callback Function to call on state change
     *  @param data     Argument to pass to callback
     */
    virtual void socket_attach(nsapi_socket_t handle, void (*callback)(void *), void *data);
    
    virtual NetworkStack* get_stack() {return this;}

private:
	WIZnet_Chip _wiznet;

	Mutex _mutex;
	Thread thread_read_socket;

    char ip_string[20];
    char netmask_string[20];
    char gateway_string[20];
    char mac_string[20];
    bool ip_set;    
    ProtocolMode temp_WizMode;

    int listen_port;
    
    //void signal_event(nsapi_socket_t handle);
    void event();
    
    //wiznet socket management
    struct wiznet_socket wiznet_sockets[MAX_SOCK_NUM];
    wiznet_socket* get_sock(int fd, nsapi_protocol_t proto);
    void init_socks();

	DHCPClient dhcp;
	bool _dhcp_enable;
	DNSClient  dns;

	virtual void socket_check_read();

//	Thread *_daemon;
//    void daemon();

};

/* \brief print_MAC - print_MAC  - helper function to print out MAC address
 * in: network_interface - pointer to network i/f
 *     bool log-messages   print out logs or not
 * MAC address is print, if it can be acquired & log_messages is true.
 *
 */
void print_MAC(NetworkInterface* network_interface, bool log_messages);


/* \brief easy_connect - easy_connect function to connect the pre-defined network bearer,
 *                       config done via mbed_app.json (see README.md for details).
 * IN: bool  log_messages  print out diagnostics or not.
 */
NetworkInterface* easy_connect(bool log_messages = false);
#endif
