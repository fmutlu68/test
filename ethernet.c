// w5500_ethernet_udp.c
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "w5500.h"

#define LED_PIN 25
#define RESET_PIN 15
#define CS_PIN 17

#define SPI_PORT spi0
#define SPI_SCK 18
#define SPI_TX 19
#define SPI_RX 16

#define SOCK_UDP 0
#define PORT_UDP 5005
#define BUF_SIZE 1024

uint8_t buf[BUF_SIZE];

wiz_NetInfo netinfo = {
    .mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
    .ip = {192, 168, 1, 100},
    .sn = {255, 255, 255, 0},
    .gw = {192, 168, 1, 1}
};

void cs_select()   { gpio_put(CS_PIN, 0); }
void cs_deselect() { gpio_put(CS_PIN, 1); }

uint8_t spi_read() {
    uint8_t rx;
    spi_read_blocking(SPI_PORT, 0x00, &rx, 1);
    return rx;
}

void spi_write(uint8_t tx) {
    spi_write_blocking(SPI_PORT, &tx, 1);
}

void spi_read_buf(uint8_t* buf, uint16_t len) {
    spi_read_blocking(SPI_PORT, 0x00, buf, len);
}

void spi_write_buf(uint8_t* buf, uint16_t len) {
    spi_write_blocking(SPI_PORT, buf, len);
}

void wizchip_setup() {
    // SPI init
    spi_init(SPI_PORT, 1 * 1000 * 1000);
    gpio_set_function(SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX, GPIO_FUNC_SPI);

    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    cs_deselect();

    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_put(RESET_PIN, 0);
    sleep_ms(100);
    gpio_put(RESET_PIN, 1);
    sleep_ms(100);

    reg_wizchip_cs_cbfunc(cs_select, cs_deselect);
    reg_wizchip_spi_cbfunc(spi_read, spi_write);
    reg_wizchip_spiburst_cbfunc(spi_read_buf, spi_write_buf);

    uint8_t tmp[2] = {0};
    wizchip_init(tmp, tmp);
    wizchip_setnetinfo(&netinfo);
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    wizchip_setup();

    printf("Pico UDP Server started on 192.168.1.100:%d\n", PORT_UDP);

    socket(SOCK_UDP, Sn_MR_UDP, PORT_UDP, 0);

    while (1) {
        int32_t recv_len;
        uint8_t sender_ip[4];
        uint16_t sender_port;

        recv_len = recvfrom(SOCK_UDP, buf, BUF_SIZE, sender_ip, &sender_port);
        if (recv_len > 0) {
            buf[recv_len] = '\0';
            printf("Received from %u.%u.%u.%u:%u: %s\n", sender_ip[0], sender_ip[1], sender_ip[2], sender_ip[3], sender_port, buf);

            if (strcmp((char*)buf, "LED_ON") == 0) {
                gpio_put(LED_PIN, 1);
                sendto(SOCK_UDP, (uint8_t*)"ACK: LED_ON", 12, sender_ip, sender_port);
            } else if (strcmp((char*)buf, "LED_OFF") == 0) {
                gpio_put(LED_PIN, 0);
                sendto(SOCK_UDP, (uint8_t*)"ACK: LED_OFF", 13, sender_ip, sender_port);
            } else if (strcmp((char*)buf, "PING") == 0) {
                sendto(SOCK_UDP, (uint8_t*)"PONG", 4, sender_ip, sender_port);
            } else if (strcmp((char*)buf, "STATUS") == 0) {
                const char* msg = gpio_get(LED_PIN) ? "STATUS: LED ON" : "STATUS: LED OFF";
                sendto(SOCK_UDP, (uint8_t*)msg, strlen(msg), sender_ip, sender_port);
            } else {
                sendto(SOCK_UDP, (uint8_t*)"ERR: Unknown Command", 22, sender_ip, sender_port);
            }
        }
        sleep_ms(100);
    }
}
