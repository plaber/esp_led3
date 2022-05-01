#ifndef SUB_JPG_H
#define SUB_JPG_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>
#include <TJpg_Decoder.h>

struct jpgheader
{
	uint16_t w;
	uint16_t h;
	uint8_t s;
	uint32_t nd;
};

struct jpgheader jpg_header(String path, bool rundec);

#endif
