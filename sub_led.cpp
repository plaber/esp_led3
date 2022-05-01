#include "sub_led.h"
#include "conf.h"
#include "sub_eep.h"

//NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(32, 3);
NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>* strip = NULL;


RgbColor black(0, 0, 0);
RgbColor white(255, 255, 255);
RgbColor red(255, 0, 0);
RgbColor dred(128, 0, 0);
RgbColor yellow(255, 255, 0);
RgbColor orange(255, 127, 0);
RgbColor green(0, 255, 0);
RgbColor wgreen(102, 255, 0);
RgbColor blue(0, 0, 255);

char cont[256];

void led_init()
{
	led_calccont();
	led_reinit(conf.leds);
}

void led_calccont()
{
	for (int i = 0; i < 256; i++)
	{
		if (i < conf.cont)
			cont[i] = 0;
		else if (i > 255 - conf.cont)
			cont[i] = 255;
		else
			cont[i] = prop(i, conf.cont, 255 - conf.cont, 0, 255);
	}
}

void led_brgn()
{
	if (strip) strip->SetBrightness(conf.brgn);
}

void led_brgn(int v)
{
	if (strip) strip->SetBrightness(v);
}

void led_clear()
{
	if (strip) strip->ClearTo(black);
}

void led_clear(HsbColor & color)
{
	if (strip) strip->ClearTo(color);
}

void led_clear(RgbColor & color)
{
	if (strip) strip->ClearTo(color);
}

void led_setpx(int i, int r, int g, int b)
{
	RgbColor color(cont[r], cont[g], cont[b]);
	if (conf.mode == true)
	{
		if (strip) strip->SetPixelColor(conf.leds - i - 1, color);
	}
	else
	{
		if (strip) strip->SetPixelColor(i, color);
	}
}

void led_setpx(int i, struct RgbColor & color)
{
	if (conf.mode == true)
	{
		if (strip) strip->SetPixelColor(conf.leds - i - 1, color);
	}
	else
	{
		if (strip) strip->SetPixelColor(i, color);
	}
}

void led_show()
{
	if (strip) strip->Show();
}

void led_reinit(uint16_t newCount)
{
	if (strip != NULL)
	{
		delete strip; // delete the previous dynamically created strip
	}
	strip = new NeoPixelBrightnessBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>(newCount, 3); // and recreate with new count
	strip->Begin();
	led_brgn();
}

void led_blink(int r, int g, int b)
{
	led_setpx(1, r, g, b);
	led_show();
	delay(300);
	led_setpx(1, 0, 0, 0);
	led_show();
	delay(300);
}

void led_blinkall()
{
	led_clear(red); led_show(); delay(500);
	led_clear(green); led_show(); delay(500);
	led_clear(blue); led_show(); delay(500);
	led_clear(); led_show();
}

void led_drawip(int d)
{
	int n1 = d % 10; //1
	int n2 = ((d - n1) / 10) % 10; //10
	int n3 = (d - n1 - n2 * 10) / 100; //100
	int i = 0, s = 0;
	if (n3 != 0)
	{
		for (i = 0; i < n3; i++) led_setpx(i, red);
		s += n3;
	}
	if (n2 != 0)
	{
		for (i = s; i < s + n2; i++) led_setpx(i, green);
		s += n2;
	}
	else
	{
		if (n3 != 0) {
			led_setpx(s, white);
			s++;
		}
	}
	if (n1 != 0)
	{
		for (i = s; i < s + n1; i++) led_setpx(i, blue);
		s += n1;
	}
	else
	{
		led_setpx(s, white);
		s++;
	}
	led_show();
}

void led_drawvcc()
{
	//draw vcc
	int v = getvcc();
	if (v > 3200) led_setpx(conf.leds - 1, dred);
	if (v > 3300) led_setpx(conf.leds - 1, red);
	if (v > 3400) led_setpx(conf.leds - 2, red);
	if (v > 3500) led_setpx(conf.leds - 3, yellow);
	if (v > 3600) led_setpx(conf.leds - 4, yellow);
	if (v > 3700) led_setpx(conf.leds - 5, yellow);
	if (v > 3800) led_setpx(conf.leds - 6, green);
	if (v > 3900) led_setpx(conf.leds - 7, green);
	if (v > 4000) led_setpx(conf.leds - 8, green);
	if (v > 4100) led_setpx(conf.leds - 9, wgreen);
	if (v > 4200) led_setpx(conf.leds - 10,wgreen);
	led_show();
}

int prop(int x, int x1, int x2, int y1, int y2)
{
	return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}
