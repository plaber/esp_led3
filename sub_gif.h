#ifndef SUB_GIF_H
#define SUB_GIF_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WebServer.h>

struct gifheader
{
	char gif[3];
	char mod[3];
	uint16_t w;
	uint16_t h;
	uint8_t flag;
	uint8_t bkgr;
	uint8_t ratio;
	int cdp;
	uint8_t fb;
	uint16_t row;
	uint16_t col;
	uint16_t wb;
	uint16_t hb;
	uint8_t flagb;
	uint8_t lzw;
	uint8_t sz;
	int fsz;
};

struct gifheader gif_header(String path, bool rundec);
void gif_dict(bool frz);

#endif
