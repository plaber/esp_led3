#include "sub_bmp.h"
#include "conf.h"
#include "sub_btn.h"
#include "sub_eep.h"
#include "sub_enow.h"
#include "sub_http.h"
#include "sub_led.h"
#include "sub_lis.h"
#include "sub_udp.h"

extern Dir rootFold;
extern FS* fileSystem;

void bmp_max()
{
	int st = millis();
	Dir rt = fileSystem->openDir("/");
	int p = 0, pm = 0;
	char fnd = 0;
	while (rt.next())
	{
		String f = rt.fileName();
		if (f.endsWith(exbmp) || f.endsWith(exgif) || f.endsWith(exjpg)) p++;
		if (f.startsWith("prog") && f.endsWith(extxt))
		{
			if (f == stat.progname)
			{
				stat.currprog = pm;
				fnd = 1;
			}
			if (pm < 15)
			{
				f.toCharArray(stat.proglist[pm], 32);
				pm++;
			}
		}
	}
	if (fnd == 0 && pm > 0)
	{
		stat.progname = String(stat.proglist[0]);
		stat.currprog = 0;
	}
	Serial.printf("maxbmp %d prog %d %dms\n", p, pm, millis()-st);
	stat.maxprog = pm;
	stat.maxbmp = p;
}

void bmp_next()
{
	if (stat.maxbmp == 0)
	{
		stat.currname = "no_bmp";
		return;
	}
	startlook:
	String fn = "";
	if (rootFold.next())
	{
		fn = rootFold.fileName();
		if (fn.endsWith(exbmp) || fn.endsWith(exgif) || fn.endsWith(exjpg))
		{
			stat.currbmp += 1;
			stat.currname = fn;
		}
		else
		{
			goto startlook;
		}
	}
	else
	{
		stat.maxbmp = stat.currbmp;
		rootFold = fileSystem->openDir("/"); //rewind
		stat.currbmp = 0;
		bmp_next();
	}
}

void bmp_draw(String pathsrc,  unsigned long tm)
{
	Serial.print("draw " + pathsrc + "\t" + String(tm, DEC));
	unsigned long stm = millis();
	String path = "[empty]";
	if (pathsrc.endsWith(exbmp) || pathsrc.endsWith(exgif) || pathsrc.endsWith(exjpg))
	{
		path = pathsrc.substring(0, pathsrc.length() - 3) + "bma";
	}
	File f = fileSystem->open(path, "r");
	if (f)
	{
		//Serial.print("\t" + String(millis() - stm));
		struct bmpheader h;
		f.seek(0, SeekSet);
		f.read((uint8_t*)(&h), 14);
		f.read((uint8_t*)(&h.tp), 20);
		//if (h.size != f.size()) goto bma_error;
		int t = 0;
		uint32_t rsiz = h.w * (h.bits / 8);
		if (rsiz % 4 != 0) rsiz += 4 - (rsiz % 4);
		char row[rsiz];
		
		while ((t = (millis() - stm)) < tm || tm == -1)
		{
			if (pathsrc != stat.currname || stat.go == false) break;
			f.seek(h.offset, SeekSet);
			
			for (int j = 0; j < h.h; j++)
			{
				if (pathsrc != stat.currname || stat.go == false || ((millis() - stm) >= tm && tm != -1)) break;
				f.readBytes(row, rsiz);
				
				for (int i = 0; i < h.w; i++)
				{
					if (pathsrc != stat.currname || stat.go == false || i >= conf.leds) break;
					uint8_t bR, bG, bB;
					uint16_t bC;
					switch (h.bits)
					{
						case 24:
							bB = row[i * 3];
							bG = row[i * 3 + 1];
							bR = row[i * 3 + 2];
							break;
						case 16:
							bC = row[i * 2] | (row[i * 2 + 1] << 8);
							bB = ((bC & 0x001F)) << 3;
							bG = ((bC & 0x03E0) >> 5) << 3;
							bR = ((bC & 0x7C00) >> 10) << 3;
					}
					led_setpx(i, bR, bG, bB);
				}
				if (conf.leds > h.w)
				{
					for (int dp = h.w; dp < conf.leds; dp++)
					{
						if (pathsrc != stat.currname || stat.go == false) break;
						led_setpx(dp, 0, 0, 0);
					}
				}
				led_show();
				bmp_wait(pathsrc);
			}
			if (stat.uptime > 0)
			{
				unsigned long now = millis();
				if(now > stat.uptime + 60000)
				{
					File batlog = fileSystem->open("batlog_" + String(conf.brgn, DEC) + ".txt","a");
					batlog.printf("%d\n", getvcc());
					batlog.close();
					stat.uptime = millis();
				}
			}
		}
		f.close();
		led_clear();
		led_show();
		Serial.println("\t" + String(t - tm, DEC));
	}
	else
	{
		bma_error:
		Serial.print(F(" no bma file "));
		Serial.println(path);
		while ((millis() - stm) < tm || tm == -1)
		{
			if (pathsrc != stat.currname || stat.go == false) break;
			bmp_rainbow();
			bmp_wait(pathsrc);
		}
	}
}

void bmp_draw(String path)
{
	if (path.endsWith(exbma) && fileSystem->exists(path))
	{
		File f = fileSystem->open(path, "r");
		f.seek(10, SeekSet);
		long of = f.read() | (f.read() << 8) | (f.read() << 16) | (f.read() << 24);
		f.seek(4, SeekCur);
		long h, w;
		w = f.read() | (f.read() << 8) | (f.read() << 16) | (f.read() << 24);
		h = f.read() | (f.read() << 8) | (f.read() << 16) | (f.read() << 24);
		f.seek( of , SeekSet);
		for (int j = 0; j < h; j++)
		{
			int rsiz = w * 3;
			if (rsiz % 4 != 0) rsiz += 4 - (rsiz % 4);
			char row[rsiz];
			f.readBytes(row, rsiz);
			for (int i = 0; i < w; i++)
			{
				byte bB = row[i * 3];
				byte bG = row[i * 3 + 1];
				byte bR = row[i * 3 + 2];
				led_setpx(i, bR, bG, bB);
			}
			if (conf.leds > w)
			{
				for (int dp = w; dp < conf.leds; dp++)
				{
					led_setpx(dp, 0, 0, 0);
				}
			}
			led_show();
			delay(10);
		}
		f.close();
	}
}

void bmp_net()
{
	http_poll();
	udp_poll();
	check_off();
	//if (conf.enow) enow_poll();
	if (conf.lis_on) 
	{
		lis_fill();
		if (conf.lis_brgn || conf.lis_hit || conf.lis_spd) lis_poll();
	}
}

void bmp_wait(String path)
{
	int dl = ((conf.wait - 1) * 1000 + (conf.fwait * 10));
	unsigned long s = micros();
	while ((micros() - s) < dl /*&& (millis() - stm) < tm*/)
	{
		if (path != stat.currname || stat.go == false) break;
		bmp_net();
	}
}

void bmp_rainbow()
{
	static int hue = 0;
	static int romb = 0;
	HsbColor hsb(hue / 360.0f, 1, 1);
	hue += 3;
	hue = hue == 360 ? 0 : hue;
	led_clear(hsb);
	for (int i = 0; i < romb; i++)
	{
		led_setpx(i, 0, 0, 0);
		led_setpx(conf.leds - i - 1, 0, 0, 0);
	}
	romb++;
	if (romb > conf.leds / 3) romb = 0;
	led_show();
}

struct bmpheader bmp_header(String path)
{
	struct bmpheader ans = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "0"};
	
	if ((path.endsWith(exbmp) || path.endsWith(exbma)) && fileSystem->exists(path))
	{
		File file = fileSystem->open(path, "r");
		if (file.size() == 0) goto empty_exit;
		file.seek(0, SeekSet);
		file.read((uint8_t*)(&ans), 14);
		file.read((uint8_t*)(&ans.tp), 20);
		switch (ans.tp)
		{
			case 12:  strcpy(ans.bminfo, "core");break;
			case 40:  strcpy(ans.bminfo, "v3");  break;
			case 108: strcpy(ans.bminfo, "v4");  break;
			case 124: strcpy(ans.bminfo, "v5");  break;
			default:  strcpy(ans.bminfo, "unkn");
		}
		empty_exit:
		file.close();
	}
	return ans;
}

void bmp_wrheader(File f, uint32_t w, uint32_t h)
{
	f.write("BM", 2);
	uint32_t datas = (w * 2 + ((w * 2) % 4)) * h + 54;
	f.write((char*)&datas, 4); //size
	f.write("\x00\x00\x00\x00", 4); //reserved
	f.write("\x36\x00\x00\x00", 4); //ofset 54
	f.write("\x28\x00\x00\x00", 4); //v3
	f.write((char*)&w, 4); //w
	f.write((char*)&h, 4); //w
	f.write("\x01\x00", 2); //planes
	f.write("\x10\x00", 2); //bit count 16
	f.write("\x00\x00\x00\x00", 4); //compression none
	f.write("\x00\x00\x00\x00", 4); //size image ignored
	f.write("\xc4\x0e\x00\x00", 4); //x resolution
	f.write("\xc4\x0e\x00\x00", 4); //y resolution
	f.write("\x00\x00\x00\x00", 4); //color table ignored
	f.write("\x00\x00\x00\x00", 4); //color table cells qty ignored
}

void bmp_rotate(String path)
{
	struct bmpheader h;
	
	File f = fileSystem->open(path, "r");
	if (f.size() == 0){
		f.close();
		led_setpx(1, 255, 0, 0);
		led_setpx(2, 255, 0, 0);
		led_show();
		delay(200);
		return;
	}
	f.seek(0, SeekSet);
	f.read((uint8_t*)(&h), 14);
	f.read((uint8_t*)(&h.tp), 20);
	
	uint32_t rsiz = h.w * (h.bits / 8);
	if (rsiz % 4 != 0) rsiz += 4 - (rsiz % 4);
	uint32_t fsz = (h.w * 2 + ((h.w * 2) % 4)) * h.h + 54;
	FSInfo fs_info;
	fileSystem->info(fs_info);
	uint32_t spc = fs_info.totalBytes - fs_info.usedBytes;
	Serial.printf("spc\t%d\tneed\t%d\t", spc, fsz);
	if(spc < fsz + 8192 * 2){
		Serial.print(F("No free space to rotate "));
		Serial.println(path);
		f.close();
		led_setpx(1, 255, 0, 0);
		led_setpx(2, 0, 0, 0);
		led_show();
		delay(100);
		return;
	}
	String newname = path.substring(0, path.length() - 3);
	File newbmp = fileSystem->open(newname + "bma", "w");
	bool odd = false;
	if (h.h <= conf.leds)
	{
		bmp_wrheader(newbmp, h.h, h.w);
		
		for (int i = 0; i < h.w; i++)
		{
			uint16_t col = h.offset + i * 3;
			for (int j = h.h - 1; j >= 0; j--)
			{
				f.seek(col + j * (rsiz), SeekSet);
				uint16_t pix = (f.read() >> 3) | ((f.read() >> 3) << 5 ) | ((f.read() >> 3) << 10);
				newbmp.write((char*)&pix, 2);
			}
			for (int k = 0; k < (h.h * 2) % 4; k++) newbmp.write(0);
			odd = !odd;
			led_setpx(1, 0, 0, odd ? 255 : 0);
			led_setpx(2, 0, 0, odd ? 0 : 255);
			led_show();
		}

	}
	else
	{
		int divh = h.h / conf.leds;
		float divhf = h.h / (float)conf.leds;
		int neww = conf.leds * h.w / h.h;
		bmp_wrheader(newbmp, conf.leds, neww);
		for (int i = 0; i < neww; i++)
		{
			for (int j = conf.leds - 1; j >= 0; j--)
			{
				int r = 0, g = 0, b = 0, p = 0;
				for (int m = 0; m < divh; m++)
				{
					for (int n = divh - 1; n >=0; n--)
					{
						uint16_t col = h.offset + (floorf(i * divhf) + m) * 3;
						f.seek(col + (floorf(j * divhf) + n) * rsiz, SeekSet);
						b += f.read();
						g += f.read();
						r += f.read();
						p++;
					}
				}
				uint16_t c = (((r / p) >> 3) << 10) | (((g / p) >> 3) << 5) | ((b / p) >> 3);
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
	f.close();
	Serial.println();
	led_setpx(1, 0, 255, 0);
	led_setpx(2, 0, 0, 0);
	led_show();
	delay(100);
}
