#include "sub_jpg.h"
#include "conf.h"
#include "sub_bmp.h"
#include "sub_led.h"

extern FS* fileSystem;

static uint16_t bw, bh, sw, sh;
static File jpgtemp;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
	char buf[70];
	if (x == 0 && y == 0)
	{
		bw = w;
		bh = h;
		sw = 0;
		sh = 0;
	}
	if (x == 0)
	{
		sh += h;
		Serial.write('\n');
		if ((sh / bh) % 2 == 0)
		{
			led_setpx(1, 0, 0, 255);
			led_setpx(2, 0, 0, 0);
		}
		else
		{
			led_setpx(1, 0, 0, 0);
			led_setpx(2, 0, 0, 255);
		}
		led_show();
	}
	if (y == 0)
	{
		sw += w;
	}
	Serial.write('.');
	jpgtemp.write((char*)bitmap, w * h * 2);
	// Return 1 to decode next block
	return 1;
}

void jpg_seekpx(uint16_t w, uint16_t h, uint16_t l, uint16_t t)
{
	int rows = t / bh;
	int cols = l / bw;
	int rbh = t < h - (h % bh) ? bh : h % bh;
	int rbw = l < w - (w % bw) ? bw : w % bw;
	int sk = (rows * w * bh) + (cols * bw * rbh) + (t % rbh) * rbw + (l % rbw);
	jpgtemp.seek(sk * 2, SeekSet);
}

struct jpgheader jpg_header(String path, bool rundec)
{
	struct jpgheader ans = {0, 0, 1};
	File file = fileSystem->open(path, "r");
	if (file.size() == 0){
		file.close();
		led_setpx(1, 255, 0, 0);
		led_setpx(2, 255, 0, 0);
		led_show();
		delay(200);
		return ans;
	}
	TJpgDec.getFsJpgSize(&ans.w, &ans.h, path);
	
	FSInfo fs_info;
	fileSystem->info(fs_info);
	uint32_t spc = fs_info.totalBytes - fs_info.usedBytes;
	ans.nd = ans.w * ans.h * 2;
	
	if(ans.h >= conf.leds * 8) //256
	{
		ans.s = 2;
	}
	if(ans.h >= conf.leds * 16) //512
	{
		ans.s = 4;
	}
	if(ans.h >= conf.leds * 32) //1024
	{
		ans.s = 8;
	}
	ans.nd /= ans.s * ans.s;
	if (rundec)
	{
		if(spc < ans.nd + 8192 * 2)
		{
			Serial.print(F("No free space to convert "));
			Serial.println(path);
			led_setpx(1, 255, 0, 0);
			led_setpx(2, 0, 0, 0);
			led_show();
			delay(100);
			return ans;
		}
		TJpgDec.setJpgScale(ans.s);
		TJpgDec.setCallback(tft_output);
		jpgtemp = fileSystem->open("convert.tmp", "w");
		TJpgDec.drawFsJpg(0, 0, path);
		jpgtemp.close();
		Serial.println(F("reopen jpgtemp"));
		jpgtemp = fileSystem->open("convert.tmp", "r");
		String newname = path.substring(0, path.length() - 3);
		File newbmp = fileSystem->open(newname + "bma", "w");
		if (ans.h <= conf.leds)
		{
			bmp_wrheader(newbmp, sh, sw);
			bool odd = false;
			for (int i = 0; i < sw; i++)
			{
				for (int j = 0; j < sh; j++)
				{
					jpg_seekpx(sw, sh, i, j);
					uint16_t px;
					jpgtemp.read((uint8_t*)(&px), 2);
					uint16_t red = (px >> 11);
					uint16_t grn = ((px >> 6) & 0b11111);
					uint16_t blu = (px & 0b11111);
					uint16_t c = (red << 10) | (grn << 5) | (blu);
					newbmp.write((char*)&c, 2);
				}
				for (int k = 0; k < (sh * 2) % 4; k++) newbmp.write(0);
				odd = !odd;
				led_setpx(1, 0, 0, odd ? 255 : 0);
				led_setpx(2, 0, 0, odd ? 0 : 255);
				led_show();
			}
		}
		else
		{
			int divh = sh / conf.leds;
			float divhf = sh / (float)conf.leds;
			int neww = conf.leds * sw / sh;
			
			bmp_wrheader(newbmp, conf.leds, neww);
			bool odd = false;
			for (int i = 0; i < neww; i++)
			{
				for (int j = 0; j < conf.leds; j++)
				{
					int r = 0, g = 0, b = 0, p = 0;
					for (int m = 0; m < divh; m++)
					{
						for (int n = 0; n < divh; n++)
						{
							jpg_seekpx(sw, sh, floorf(i * divhf) + m, floorf(j * divhf) + n);
							uint16_t px;
							jpgtemp.read((uint8_t*)(&px), 2);
							r += (px >> 11);
							g += ((px >> 6) & 0b11111);
							b += (px & 0b11111);
							p++;
						}
					}
					uint16_t c = ((r / p) << 10) | ((g / p) << 5) | (b / p);
					newbmp.write((char*)&c, 2);
				}
				for (int k = 0; k < (conf.leds * 2) % 4; k++) newbmp.write(0);
				odd = !odd;
				led_setpx(1, 0, 0, odd ? 255 : 0);
				led_setpx(2, 0, 0, odd ? 0 : 255);
				led_show();
			}
		}
		newbmp.close();
		int jrs = jpgtemp.size();
		jpgtemp.close();
		Serial.println(F("close jpgtemp"));
		led_setpx(1, 0, 255, 0);
		led_setpx(2, 0, 0, 0);
		led_show();
	}
	return ans;
}

