#ifndef SUB_WIFI_H
#define SUB_WIFI_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266NetBIOS.h>

void wifi_init();
void wifi_initap();
void wifi_conn();
bool wifi_wps();

#endif
