#include "main.h"
#include "conf.h"
#include "sub_bmp.h"
#include "sub_btn.h"
#include "sub_gif.h"
#include "sub_eep.h"
#include "sub_enow.h"
#include "sub_http.h"
#include "sub_jpg.h"
#include "sub_json.h"
#include "sub_led.h"
#include "sub_lis.h"
#include "sub_udp.h"
#include "sub_wifi.h"

int ssid_count = 6;
String ssid[] = {"ledControl", "spiffs", "spiffs", "scp-121",  "salat"      };
String pass[] = {"12345678",   "spiffs", "spiffs", "bioshock", "salat191919"};
FS* fileSystem = &LittleFS;
//FS* fileSystem = &SPIFFS;
char exbmp[5] = ".bmp";
char exbma[5] = ".bma";
char exgif[5] = ".gif";
char exjpg[5] = ".jpg";
char extxt[5] = ".txt";

struct config conf = {
	"v0.36",
	"LedPOI",
	{}, //macs
	0, //macson
	0, //macslen
	1, //wait
	8, //brgn
	false, //mode f-normal t-invert
	32, //pixels count
	5, //vcc
	190, //fwait
	0, //contrast
	false, //skip wifi conn wait
	false, //skip wifi conn begin
	false, //enow
	false, //use lis
	false, //lis hit
	false, //lis brgn
	false, //lis spd
	false  //lis old pcb
};

struct status stat = {
	false, //go
	0, //maxbmp
	0, //curr bmp
	"no_bmp", //curr bmp name
	true, //loop files
	3, //who draw
	4000, //bpm bit per minute
	false, //format complete
	false, //lis found
	0,
	0, //uptime battery test
	0, //maxprog
	0, //curr prog
	"no_prog", //curr prog name
	0, //proglist
	false //calcmax
};

Dir rootFold;

void setup()
{
	pinMode(4, INPUT_PULLUP); //init mosfet button
	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
	
	Serial.begin(115200);
	Serial.println(F("Hello!"));

	eep_init();
	eep_load();
	print_conf();

	led_init();
	led_clear();
	
	check_up();
	led_clear();
	
	fileSystem->begin();

	FSInfo fs_info;
	fileSystem->info(fs_info);
	Serial.printf("free %d bytes\n", fs_info.totalBytes - fs_info.usedBytes);

	json_load();

	if (conf.wpref == F("fan"))
	{
		analogWriteFreq(25000);
		pinMode(2, OUTPUT);
	}

	gif_dict(true);
	Dir rt = fileSystem->openDir("/");
	String prevfile = "empty";
	bool fnd = false;
	Serial.println(millis());
	while (rt.next())
	{
		String f = rt.fileName();
		bool ex = f.substring(0, f.length() - 3) == prevfile.substring(0, prevfile.length() - 3);
		//bool ex = fileSystem->exists(f.substring(0, f.length() - 3) + "bma";
		if (f.endsWith(exgif))
		{
			if (!ex) {Serial.print(F("convert ")); Serial.println(f); gif_header(f, true);}
			stat.maxbmp++;
		}
		if (f.endsWith(exbmp))
		{
			if (!ex) {Serial.print(F("convert ")); Serial.println(f); bmp_rotate(f);}
			stat.maxbmp++;
		}
		if (f.endsWith(exjpg))
		{
			if (!ex) {Serial.print(F("convert ")); Serial.println(f); jpg_header(f, true);}
			stat.maxbmp++;
		}
		if (f.startsWith("prog") && f.endsWith(extxt))
		{
			if (f == stat.progname)
			{
				stat.currprog = stat.maxprog;
				fnd = true;
			}
			f.toCharArray(stat.proglist[stat.maxprog], 32);
			stat.maxprog++;
		}
		prevfile = f;
	}
	if (fnd == false && stat.maxprog > 0)
	{
		stat.progname = String(stat.proglist[0]);
	}
	Serial.printf("maxbmp %d prog %d\n", stat.maxbmp, stat.maxprog);
	Serial.println(millis());
	const String convtmp = "convert.tmp";
	if (fileSystem->exists(convtmp)) fileSystem->remove(convtmp);
	gif_dict(false);
	
	rootFold = fileSystem->openDir("/");
	bmp_next();
	
	http_begin();
	wifi_init();
	
	led_clear();

	led_drawvcc();

	if (conf.lis_on) lis_init();
	
	if (conf.enow)
	{
		int px = enow_getorder();
		for (int i = 0; i < px; i++) led_setpx(i, 0, 255, 0);
		led_show();
		enow_begin();
		delay(200);
		while (stat.go == false)
		{
			enow_poll();
			check_off();
			delay(10);
			/*
			if (conf.macslen == 1)
			{
				enow_wakeup2();
			}
			else
			{
			*/
				enow_wakeup();
			//}
		}
		enow_end();
	}
	else
	{
		if (conf.skpwc == false) wifi_conn();
		udp_begin();
	}
	Serial.println(F("begin loop"));
}

void loop()
{
	if (stat.go)
	{
		if (stat.whdr == 3)
		{
			if (stat.loop)
			{
				bmp_draw(stat.currname, stat.bpm);
				bmp_next();
			}
			else
			{
				bmp_draw(stat.currname, -1);
			}
		}
		if (stat.whdr == 4)
		{
			if (fileSystem->exists(stat.progname))
			{
				String prgopen = stat.progname;
				File prgfile = fileSystem->open(prgopen, "r");
				String pic;
				while (prgfile.available())
				{
					if (stat.whdr != 4 || stat.go == false || prgopen != stat.progname) break;
					pic = prgfile.readStringUntil('\n');
					//Serial.println("debug pic [" + pic + "] " + String(pic.length(), DEC) + "] fpos " + String(prgfile.position(), DEC) + " flen " + String(prgfile.size()));
					pic.trim();
					if (pic == "stop") break;
					int spidx = pic.indexOf(" ");
					if (spidx == -1 || spidx == pic.length() - 1)
					{
						Serial.println(F("prog err: no second arg"));
						bmp_net();
						if (prgfile.position() >= prgfile.size()){
							prgfile.close();
							Serial.println(F("file closed"));
						}
					}
					else
					{
						String fpic = pic.substring(0, spidx);
						String dspic = pic.substring(spidx + 1);
						dspic.trim();
						int dpic = dspic.toInt();
						stat.currname = fpic;
						bmp_draw(fpic, dpic);
					}
				}
				prgfile.close();
				bmp_net();
				if (pic == "stop")
				{
					Serial.println(F("stop"));
					stat.go = false;
				}
				led_clear();
				led_show();
			}
			else
			{
				bmp_rainbow();
				bmp_wait(stat.currname);
				Serial.println(F("prog err: no prog"));
			}
		}
	}
	else
	{
		bmp_net();
		//http_poll();
		ftp_poll();
	}
}
