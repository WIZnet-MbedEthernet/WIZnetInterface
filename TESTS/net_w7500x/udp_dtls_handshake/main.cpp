#include "mbed.h"
#include "UDPSocket.h"
#include "greentea-client/test_env.h"
#include "unity/unity.h"
#include "utest.h"

#include "WIZnetInterface.h"
using namespace utest::v1;


#ifndef MBED_CFG_UDP_DTLS_HANDSHAKE_BUFFER_SIZE
#define MBED_CFG_UDP_DTLS_HANDSHAKE_BUFFER_SIZE 512
#endif

#ifndef MBED_CFG_UDP_DTLS_HANDSHAKE_RETRIES
#define MBED_CFG_UDP_DTLS_HANDSHAKE_RETRIES 16
#endif

#ifndef MBED_CFG_UDP_DTLS_HANDSHAKE_PATTERN
#define MBED_CFG_UDP_DTLS_HANDSHAKE_PATTERN 112, 384, 200, 219, 25
#endif

#ifndef MBED_CFG_UDP_DTLS_HANDSHAKE_TIMEOUT
#define MBED_CFG_UDP_DTLS_HANDSHAKE_TIMEOUT 1500
#endif

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x

uint8_t buffer[MBED_CFG_UDP_DTLS_HANDSHAKE_BUFFER_SIZE] = {0};
int udp_dtls_handshake_pattern[] = {MBED_CFG_UDP_DTLS_HANDSHAKE_PATTERN};
const int udp_dtls_handshake_count = sizeof(udp_dtls_handshake_pattern) / sizeof(int);

void test_udp_dtls_handshake() {
	WIZnetInterface net;
    int err = net.connect();
    TEST_ASSERT_EQUAL(0, err);

    printf("MBED: UDPClient IP address is '%s'\n", net.get_ip_address());
    printf("MBED: UDPClient waiting for server IP and port...\n");

    greentea_send_kv("target_ip", net.get_ip_address());

    bool result = false;

    char recv_key[] = "host_port";
    char ipbuf[60] = {0};
    char portbuf[16] = {0};
    unsigned int port = 0;

    greentea_send_kv("host_ip", " ");
    greentea_parse_kv(recv_key, ipbuf, sizeof(recv_key), sizeof(ipbuf));

    greentea_send_kv("host_port", " ");
    greentea_parse_kv(recv_key, portbuf, sizeof(recv_key), sizeof(ipbuf));
    sscanf(portbuf, "%u", &port);

    printf("MBED: UDP Server IP address received: %s:%d \n", ipbuf, port);

    // align each size to 4-bits
    for (int i = 0; i < udp_dtls_handshake_count; i++) {
        udp_dtls_handshake_pattern[i] = (~0xf & udp_dtls_handshake_pattern[i]) + 0x10;
    }

    printf("MBED: DTLS pattern [");
    for (int i = 0; i < udp_dtls_handshake_count; i++) {
        printf("%d", udp_dtls_handshake_pattern[i]);
        if (i != udp_dtls_handshake_count-1) {
            printf(", ");
        }
    }
    printf("]\r\n");

    UDPSocket sock;
    SocketAddress udp_addr(ipbuf, port);
    sock.set_timeout(MBED_CFG_UDP_DTLS_HANDSHAKE_TIMEOUT);

    for (int attempt = 0; attempt < MBED_CFG_UDP_DTLS_HANDSHAKE_RETRIES; attempt++) {
        err = sock.open(&net);
        TEST_ASSERT_EQUAL(0, err);

        for (int i = 0; i < udp_dtls_handshake_count; i++) {
            buffer[i] = udp_dtls_handshake_pattern[i] >> 4;
        }

        err = sock.sendto(udp_addr, buffer, udp_dtls_handshake_count);
        printf("UDP: tx -> %d\r\n", err);
        TEST_ASSERT_EQUAL(udp_dtls_handshake_count, err);

        int step = 0;
        while (step < udp_dtls_handshake_count) {
            err = sock.recvfrom(NULL, buffer, sizeof(buffer));
            printf("UDP: rx <- %d ", err);

            // check length
            if (err != udp_dtls_handshake_pattern[step]) {
                printf("x (expected %d)\r\n", udp_dtls_handshake_pattern[step]);
                break;
            }

            // check quick xor of packet
            uint8_t check = 0;
            for (int j = 0; j < udp_dtls_handshake_pattern[step]; j++) {
                check ^= buffer[j];
            }

            if (check != 0) {
                printf("x (checksum 0x%02x)\r\n", check);
                break;
            }

            // successfully got a packet
            printf("\r\n");
            step += 1;
        }

        err = sock.close();
        TEST_ASSERT_EQUAL(0, err);

        // got through all steps, test passed
        if (step == udp_dtls_handshake_count) {
            result = true;
            break;
        }
    }

    net.disconnect();
    TEST_ASSERT_EQUAL(true, result);
}


// Test setup
utest::v1::status_t test_setup(const size_t number_of_cases) {
    GREENTEA_SETUP(120, "udp_shotgun");
    return verbose_test_setup_handler(number_of_cases);
}

Case cases[] = {
    Case("UDP DTLS handshake", test_udp_dtls_handshake),
};

Specification specification(test_setup, cases);

int main() {
    return !Harness::run(specification);
}

