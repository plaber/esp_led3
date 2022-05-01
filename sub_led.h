#ifndef SUB_LED_H
#define SUB_LED_H

#include <SPI.h>
#include <NeoPixelBrightnessBus.h>

void led_init();
void led_calccont();
void led_brgn();
void led_brgn(int v);
void led_clear();
void led_clear(HsbColor & color);
void led_clear(RgbColor & color);
void led_setpx(int i, int r, int g, int b);
void led_setpx(int i, struct RgbColor & color);
void led_show();
void led_reinit(uint16_t newCount);
void led_blink(int r, int g, int b);
void led_blinkall();
void led_drawip(int d);
void led_drawvcc();
int prop(int x, int x1, int x2, int y1, int y2);


#endif
