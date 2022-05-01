#ifndef SUB_ENOW_H
#define SUB_ENOW_H

#include <WifiEspNowBroadcast.h>

void enow_begin();
void enow_end();
void enow_poll();
void enow_receive(const uint8_t mac[6], const uint8_t * buf, size_t count, void * cbarg);
void enow_send(char txt[]);
int enow_getorder();
void enow_wakeup2();
void enow_wakeup();

#endif
