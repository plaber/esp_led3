#ifndef SUB_HTTP_H
#define SUB_HTTP_H

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <FS.h>
#include <LittleFS.h>
#include <SimpleFTPServer.h>

void http_begin();
void http_beginap();
void http_poll();
void ftp_poll();

#endif
