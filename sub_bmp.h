#ifndef SUB_BMP_H
#define SUB_BMP_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#pragma pack(push, 2)
struct bmpheader
{
	uint16_t mark;
	uint32_t size;
	uint16_t Reserved0;
	uint16_t Reserved1;
	uint32_t offset;
	uint32_t tp;
	uint32_t w;
	uint32_t h;
	uint16_t planes;
	uint16_t bits;
	uint32_t compres;
	char bminfo[5];
};
#pragma pack(pop)

void bmp_max();
void bmp_next();
void bmp_draw(String pathsrc, unsigned long tm);
void bmp_draw(String path);
void bmp_net();
void bmp_wait(String path);
void bmp_rainbow();
struct bmpheader bmp_header(String path);
void bmp_wrheader(File f, uint32_t w, uint32_t h);
void bmp_rotate(String path);

#endif
