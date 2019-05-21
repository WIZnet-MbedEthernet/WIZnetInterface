#ifndef MBED_EXTENDED_TESTS
    #error [NOT_SUPPORTED] Parallel tests are not supported by default
#endif

#include "mbed.h"
#include "TCPSocket.h"
#include "greentea-client/test_env.h"
#include "unity/unity.h"
#include "utest.h"

#include "WIZnetInterface.h"
using namespace utest::v1;


#ifndef MBED_CFG_TCP_CLIENT_ECHO_BUFFER_SIZE
#define MBED_CFG_TCP_CLIENT_ECHO_BUFFER_SIZE 64
#endif

#ifndef MBED_CFG_TCP_CLIENT_ECHO_THREADS
#define MBED_CFG_TCP_CLIENT_ECHO_THREADS 3
#endif

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x


WIZnetInterface net;
SocketAddress tcp_addr;
Mutex iomutex;

void prep_buffer(char *tx_buffer, size_t tx_size) {
    for (size_t i=0; i<tx_size; ++i) {
        tx_buffer[i] = (rand() % 10) + '0';
    }
}


// Each echo class is in charge of one parallel transaction
class Echo {
private:
    char tx_buffer[MBED_CFG_TCP_CLIENT_ECHO_BUFFER_SIZE];
    char rx_buffer[MBED_CFG_TCP_CLIENT_ECHO_BUFFER_SIZE];

    TCPSocket sock;
    Thread thread;

public:
    // Limiting stack size to 1k
    Echo(): thread(osPriorityNormal, 1024) {
    }

    void start() {
        osStatus status = thread.start(callback(this, &Echo::echo));
        TEST_ASSERT_EQUAL(osOK, status);
    }

    void join() {
        osStatus status = thread.join();
        TEST_ASSERT_EQUAL(osOK, status);
    }

    void echo() {
        int err = sock.open(&net);
        TEST_ASSERT_EQUAL(0, err);

        err = sock.connect(tcp_addr);
        TEST_ASSERT_EQUAL(0, err);

        iomutex.lock();
        printf("HTTP: Connected to %s:%d\r\n",
                tcp_addr.get_ip_address(), tcp_addr.get_port());
        printf("tx_buffer buffer size: %u\r\n", sizeof(tx_buffer));
        printf("rx_buffer buffer size: %u\r\n", sizeof(rx_buffer));
        iomutex.unlock();

        prep_buffer(tx_buffer, sizeof(tx_buffer));
        sock.send(tx_buffer, sizeof(tx_buffer));

        // Server will respond with HTTP GET's success code
        const int ret = sock.recv(rx_buffer, sizeof(rx_buffer));
        bool result = !memcmp(tx_buffer, rx_buffer, sizeof(tx_buffer));
        TEST_ASSERT_EQUAL(ret, sizeof(rx_buffer));
        TEST_ASSERT_EQUAL(true, result);

        err = sock.close();
        TEST_ASSERT_EQUAL(0, err);
    }
};

Echo *echoers[MBED_CFG_TCP_CLIENT_ECHO_THREADS];


void test_tcp_echo_parallel() {
	int err = net.connect();
    TEST_ASSERT_EQUAL(0, err);

    printf("MBED: TCPClient IP address is '%s'\n", net.get_ip_address());
    printf("MBED: TCPClient waiting for server IP and port...\n");

    greentea_send_kv("target_ip", net.get_ip_address());

    char recv_key[] = "host_port";
    char ipbuf[60] = {0};
    char portbuf[16] = {0};
    unsigned int port = 0;

    greentea_send_kv("host_ip", " ");
    greentea_parse_kv(recv_key, ipbuf, sizeof(recv_key), sizeof(ipbuf));

    greentea_send_kv("host_port", " ");
    greentea_parse_kv(recv_key, portbuf, sizeof(recv_key), sizeof(ipbuf));
    sscanf(portbuf, "%u", &port);

    printf("MBED: Server IP address received: %s:%d \n", ipbuf, port);
    tcp_addr.set_ip_address(ipbuf);
    tcp_addr.set_port(port);

    // Startup echo threads in parallel
    for (int i = 0; i < MBED_CFG_TCP_CLIENT_ECHO_THREADS; i++) {
        echoers[i] = new Echo;
        echoers[i]->start();
    }

    for (int i = 0; i < MBED_CFG_TCP_CLIENT_ECHO_THREADS; i++) {
        echoers[i]->join();
        delete echoers[i];
    }

    net.disconnect();
}

// Test setup
utest::v1::status_t test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(120, "tcp_echo");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("TCP echo parallel", test_tcp_echo_parallel),
};

Specification specification(test_setup, cases);

int main() {
    return !Harness::run(specification);
}

