#include "sub_gif.h"
#include "conf.h"
#include "sub_bmp.h"
#include "sub_led.h"

extern FS* fileSystem;

static int getBit(uint8_t *data, int pos)
{
	int posByte = pos / 8;
	int posBit = pos % 8;
	uint8_t valByte = data[posByte];
	int valInt = valByte >> ((posBit)) & 0x0001;
	return valInt;
}

const int pow2[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

static uint8_t colors[256*3];
static uint16_t colorslen;
static uint16_t *dict, dictlen, wrbuf[128], wrbuflen;
static bool dicton = true;
static uint8_t block[256];
static uint16_t bt, bi, lzb, lz, last, imglen, imgw;
static File giftemp;
static uint32_t mil;

void gif_dict(bool frz)
{
	if(frz)
	{
		dict = (uint16_t*)realloc(dict, 4096 * 2 * sizeof(uint16_t));
		dicton = false;
	}
	else
	{
		dict = (uint16_t*)realloc(dict, 1);
		dicton = true;
	}
}

static void freeall()
{
	if (dicton) dict = (uint16_t*)realloc(dict, 1);
	colorslen = dictlen = bt = bi = lzb = lz = last = imglen = 0;
}

static bool indict(int el)
{
	return el < (colorslen + 2 + (dictlen / 2));
}

static uint16_t first(uint16_t el)
{
	if (el < colorslen)
	{
		return el;
	}
	else
	{
		if (!indict(el))
		{
			Serial.printf("%d ", el);
			Serial.print(F("not in dict------------------\n"));
			return 0;
		}
		uint16_t deep = el;
		while((deep = *(dict + (deep - colorslen - 2) * 2)) > colorslen);
		return deep;
	}
}

static void img_push(uint16_t el)
{
	//yield();
	imglen++;
	uint16_t pix = 
		((*(colors + el * 3 + 0) >> 3) << 10) |
		((*(colors + el * 3 + 1) >> 3) << 5 ) |
		 (*(colors + el * 3 + 2) >> 3);
	//giftemp.write((char*)&pix, 2);
	wrbuf[wrbuflen++] = pix;
	if (wrbuflen == 128)
	{
		giftemp.write((char*)&wrbuf, 256);
		wrbuflen = 0;
	}
}

static void dict_push(uint16_t el1, uint16_t el2)
{
	if (dictlen == 4096 * 2) return;
	dict[dictlen++] = el1;
	dict[dictlen++] = el2;
}

static void add(uint16_t c){
	uint16_t stack[256];
	int i = 0;
	stack[i++] = c;

	while (i > 0)
	{
		uint16_t el = stack[--i];
		if (el < colorslen)
		{
			img_push(el);
		}
		else
		{
			uint16_t c1 = *(dict + (el - colorslen - 2) * 2);
			uint16_t c2 = *(dict + (el - colorslen - 2) * 2 + 1);
			stack[i++] = c2;
			stack[i++] = c1;
		}
	}
}

static void decode(uint16_t idx, bool start)
{
	//if (idx == colorslen || idx == colorslen + 1) return;
	//yield();
	//Serial.printf("dec %04d dict %04d\n" , idx, dictlen / 2 + colorslen + 2);
	if (start) //decode first pixel
	{
		add(idx);
		last = idx;
		return;
	}
	if (indict(idx))
	{
		dict_push(last, first(idx));
		add(idx);
	}
	else
	{
		dict_push(last, first(last));
		add(colorslen + 2 + (dictlen / 2) -1);
	}
	last = idx;
}

static void extract(uint8_t len)
{
	//Serial.printf("extr %d ", len);
	if (len == 255) Serial.write('.'); else Serial.print(len);
	static bool reinit = true;
	for(int i = 0; i < len * 8; i++)
	{
		int bit = getBit(block, i);
		bt |= (bit << bi);
		bi++;
		if (bi == lz)
		{
			if (bt == colorslen) //clear code
			{
				dictlen = 0;
				lz = lzb;
				bi = 0;
				bt = 0;
				last = 0;
				reinit = true;
				continue;
			}
			if (bt == colorslen + 1) //stop code
			{
				bi = 0;
				bt = 0;
				break;
			}
			decode(bt, reinit);
			bi = 0;
			bt = 0;
			reinit = false;
			if((colorslen + 2 + (dictlen / 2)) == pow2[lz] && lz < 12) lz++;
		}
	}
	uint32_t dif = millis() - mil;
	Serial.println(dif);
}

struct gifheader gif_header(String path, bool rundec)
{
	struct gifheader ans;
	static bool brk_w = false;
	if (brk_w && rundec) return ans;
	if (path.endsWith(exgif) && fileSystem->exists(path))
	{
		File file = fileSystem->open(path, "r");
		if (file.size() == 0){
			file.close();
			led_setpx(1, 255, 0, 0);
			led_setpx(2, 255, 0, 0);
			led_show();
			delay(200);
			return ans;
		}
		file.seek(0, SeekSet);
		file.read((uint8_t*)(&ans), 13);
		if (ans.flag & 0b10000000)
		{
			ans.cdp = ans.flag & 0b111;
		} else {
			ans.cdp = 0;
			if (rundec) led_setpx(1, 255, 0, 0); led_show(); delay(100);
		}
		freeall();
		if (ans.cdp)
		{
			int ctsize = pow2[ans.cdp + 1] * 3;
			if (rundec)
			{
				uint32_t fsz = (ans.w * 2 + ((ans.w * 2) % 4)) * ans.h + 54;
				FSInfo fs_info;
				fileSystem->info(fs_info);
				uint32_t spc = fs_info.totalBytes - fs_info.usedBytes;
				Serial.printf("spc\t%d\tneed\t%d\n", spc, fsz);
				if(spc < fsz * 2 + 8192 * 2){
					Serial.print(F("No free space to convert "));
					Serial.println(path);
					file.close();
					led_setpx(1, 255, 0, 0);
					led_setpx(2, 0, 0, 0);
					led_show();
					delay(100);
					return ans;
				}
				file.read((uint8_t*)(colors), ctsize);
				colorslen = ctsize / 3;
				imgw = ans.w;
				//same 1
				ans.fb = file.read();
				while (ans.fb != 0x2C) //non image block
				{
					file.read(); //scip code
					uint8_t sz = 0;
					while((sz = file.read()) != 0) file.seek(sz, SeekCur); //jump block
					ans.fb = file.read();
				}
				file.read((uint8_t*)(&ans.row), 11);
				ans.fsz = 0;
				//end same
				dictlen = 0;
				if (dicton) dict = (uint16_t*)realloc(dict, 4096 * 2 * sizeof(uint16_t));
				if (dict == NULL) Serial.println(F("*dict NULL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
				String newname = path.substring(0, path.length() - 3);
				giftemp = fileSystem->open("convert.tmp", "w");
				lzb = ans.lzw + 1;
				lz = ans.lzw + 1;
				mil = millis();
				bool odd = false;
				do
				{
					file.read((uint8_t*)(block), ans.sz);
					extract(ans.sz);
					if (digitalRead(4) == LOW){
						brk_w = true;
						file.close();
						giftemp.close();
						led_setpx(1, 255, 128, 0);
						led_setpx(2, 255, 128, 0);
						led_show();
						delay(500);
						break;
					}
					ans.fsz += ans.sz;
					odd = !odd;
					led_setpx(1, 0, 0, odd ? 255 : 0);
					led_setpx(2, 0, 0, odd ? 0 : 255);
					led_show();
				} while ((ans.sz = file.read()) != 0);
				if (brk_w) return ans;
				if (wrbuflen)
				{
					giftemp.write((char*)&wrbuf, wrbuflen * 2);
					wrbuflen = 0;
				}
				giftemp.close();
				file.close();
				Serial.println(F("reopen giftemp"));
				giftemp = fileSystem->open("convert.tmp", "r");
				File newbmp = fileSystem->open(newname + "bma", "w");
				if (ans.h <= conf.leds)
				{
					bmp_wrheader(newbmp, ans.h, ans.w);
					for (int i = 0; i < ans.w; i++)
					{
						giftemp.seek(i * 2, SeekSet);
						for (int j = 0; j < ans.h; j++)
						{
							newbmp.write(giftemp.read());
							newbmp.write(giftemp.read());
							giftemp.seek(ans.w * 2 - 2, SeekCur);
						}
						for (int k = 0; k < (ans.h * 2) % 4; k++) newbmp.write(0);
						odd = !odd;
						led_setpx(1, 0, 0, odd ? 255 : 0);
						led_setpx(2, 0, 0, odd ? 0 : 255);
						led_show();
					}
				}
				else
				{
					int divh = ans.h / conf.leds;
					float divhf = ans.h / (float)conf.leds;
					int neww = conf.leds * ans.w / ans.h;
					bmp_wrheader(newbmp, conf.leds, neww);
					for (int i = 0; i < neww; i++)
					{
						for (int j = 0; j < conf.leds; j++)
						{
							int r = 0, g = 0, b = 0, p = 0;
							for (int m = 0; m < divh; m++)
							{
								for (int n = 0; n < divh; n++)
								{
									giftemp.seek( (floorf(i * divhf) + m) * 2  + (floorf(j * divhf) + n) * 2 * ans.w, SeekSet);
									uint16_t px;
									giftemp.read((uint8_t*)(&px), 2);
									r += (px >> 10);
									g += ((px >> 5) & 0b11111);
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
				giftemp.close();
				//fileSystem->remove("convert.tmp");
				Serial.printf("imglen %d, real %d\n", imglen, ans.w * ans.h);
				led_setpx(1, 0, 255, 0);
				led_setpx(2, 0, 0, 0);
				led_show();
				delay(100);
			}
			else
			{
				file.seek(ctsize, SeekCur);  //jump color table
				//same 1
				ans.fb = file.read();
				while (ans.fb != 0x2C) //non image block
				{
					file.read(); //scip code
					uint8_t sz = 0;
					while((sz = file.read()) != 0) file.seek(sz, SeekCur); //jump block
					ans.fb = file.read();
				}
				file.read((uint8_t*)(&ans.row), 11);
				ans.fsz = 0;
				//end same
				do
				{
					file.seek(ans.sz, SeekCur);
					ans.fsz += ans.sz;
					//Serial.println("ans.sz: " + String(ans.sz, DEC));
				} while ((ans.sz = file.read()) != 0);
			}
		}
		else
		{
			ans.fb = 0;
		}
		exitread:
		if (rundec)
		{
			Serial.print(F("                                  dictlen: "));
			Serial.println(String(dictlen, DEC));
		}
		file.close();
		freeall();
	}
	return ans;
}

