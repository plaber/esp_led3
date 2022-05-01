#ifndef SUB_JSON_H
#define SUB_JSON_H

#include <Arduino_JSON.h>
#include <FS.h>
#include <LittleFS.h>

void json_save();
void json_load();
void mac_decode(String in, uint8_t *ans);
void json_del();

#endif
