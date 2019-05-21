// DNSClient.h 2013/4/5
#pragma once

#include "UDPSocket.h"

//#define DBG_DNS 1
 
class DNSClient {
public:
    DNSClient();
    DNSClient(NetworkStack *ns, const char* hostname = NULL);
    DNSClient(NetworkStack *ns, SocketAddress* pHost);
    
    int setup(NetworkStack *ns);

    virtual ~DNSClient();
    bool lookup(const char* hostname);
    bool set_server(const char* serverip, int port=53);
    uint32_t get_ip() {return m_ip;}
    const char* get_ip_address() {return m_ipaddr;}
    
protected:
    void poll();
    void callback();
    int response(uint8_t buf[], int size);
    int query(uint8_t buf[], int size, const char* hostname);
    void resolve(const char* hostname);
    uint8_t m_id[2];
    Timer m_interval;
    int m_retry;
    const char* m_hostname;

private:
    enum MyNetDnsState
    {
        MYNETDNS_START,
        MYNETDNS_PROCESSING, //Req has not completed
        MYNETDNS_NOTFOUND,
        MYNETDNS_ERROR,
        MYNETDNS_OK
    };
    MyNetDnsState m_state;
    UDPSocket *m_udp;
    NetworkStack *m_ns;

    uint32_t m_ip;
    char m_ipaddr[24];
};
