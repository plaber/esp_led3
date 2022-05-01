#ifndef SUB_UDP_H
#define SUB_UDP_H

#include <WiFiUdp.h>

void udp_begin();
void udp_poll();
void udp_sendmac();
void udp_sendip();
void udp_echo(char *buf);

#endif
