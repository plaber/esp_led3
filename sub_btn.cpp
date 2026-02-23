#include "sub_btn.h"
#include "conf.h"
#include "sub_bmp.h"
#include "sub_eep.h"
#include "sub_http.h"
#include "sub_json.h"
#include "sub_led.h"
#include "sub_wifi.h"

extern FS* fileSystem;

int onoff = 0, offp = 0;

GButton butt1(4);

void check_off()
{
	butt1.tick();
	if (butt1.isStep() && onoff == 1 && offp < 3) { offp++; Serial.printf("OFF %d\n", offp); }
	if (butt1.isRelease())
	{
		offp = 0;
		if (onoff == 0)
		{
			onoff = 1;
			Serial.println(F("released"));
		}
	}
	if (onoff == 1)
	{
		if (offp == 1 || offp == 2)
		{
			stat.go = false;
			RgbColor dwhite(50, 50, 50);
			led_clear();
			led_setpx(0, dwhite);
			led_show();
		}
		if (offp == 3)
		{
			digitalWrite(5, HIGH);
			pinMode(5, INPUT);
			RgbColor dred(50, 0, 0);
			led_clear();
			led_setpx(0, dred);
			led_show();
		}
	}
	if (butt1.isSingle())
	{
		if (stat.go == false)
		{
			Serial.println(get_answ("go","1"));
		}
		else if (stat.whdr == 3 && stat.loop == false)
		{
			bmp_next();
		}
		else if (stat.whdr == 4)
		{
			if (stat.currprog < stat.maxprog - 1)
			{
				stat.currprog++;
			}
			else
			{
				stat.currprog = 0;
			}
			stat.progname = String(stat.proglist[stat.currprog]);
			stat.currname = "a";
			Serial.println(stat.progname);
		}
	}
	if (butt1.isDouble())
	{
		if (stat.whdr == 3 && stat.loop == false)
		{
			int prevn = 0;
			if (stat.currbmp > 1) prevn = stat.currbmp - 1; else prevn = stat.maxbmp;
			while (stat.currbmp != prevn) bmp_next();
		}
		else if (stat.whdr == 4)
		{
			if (stat.currprog > 0)
			{
				stat.currprog--;
			}
			else
			{
				stat.currprog = stat.maxprog - 1;
			}
			stat.progname = String(stat.proglist[stat.currprog]);
			stat.currname = "a";
			Serial.println(stat.progname);
		}
	}
	RgbColor green(0, 255, 0);
	if (butt1.isTriple())
	{
		if (stat.whdr == 3 && stat.loop == true)
		{
			stat.loop = false;
			Serial.println("pics one");
			led_clear();
			led_setpx(0, green);led_setpx(1, green);
			led_show();
			delay(300);
		}
		else if (stat.whdr == 3 && stat.loop == false)
		{
			led_clear();
			if (stat.maxprog > 0)
			{
				stat.whdr = 4;
				Serial.println("prog");
				led_setpx(0, green);led_setpx(1, green);led_setpx(2, green);
			}
			else
			{
				stat.whdr = 3;
				stat.loop = true;
				Serial.println("pics all");
				led_setpx(0, green);
			}
			led_show();
			delay(300);
		}
		else if (stat.whdr == 4)
		{
			stat.whdr = 3;
			stat.loop = true;
			Serial.println("pics all");
			led_clear();
			led_setpx(0, green);
			led_show();
			delay(300);
		}
		EEPROM.write(EEP_WHDR, stat.whdr);
		EEPROM.write(EEP_LOOP, stat.loop);
		char ebuf[32];
		memset(ebuf, 0, 32); stat.progname.toCharArray(ebuf, 32); EEPROM.put(EEP_PG, ebuf);
		EEPROM.commit();
	}
}

void check_up()
{
	int showUp = 0;
	led_setpx(0, 128, 128, 128);
	led_show();
	butt1.tick();
	while (butt1.state() && showUp < 20)
	{
		delay(50);
		showUp++;
		butt1.tick();
	}
	if (butt1.state())
	{
		butt1.tick();
		led_brgn(12);
		int pixBut = -1;
		while (pixBut < 31 && butt1.state())
		{
			butt1.tick();
			pixBut++;
			if (pixBut == 4)
			{
				led_setpx(10, 0, 255, 0);
				led_setpx(16, 255, 0, 0);
			}
			if (pixBut != 10 && pixBut != 16 && pixBut < 22) led_setpx(pixBut, 128, 128, 128);
			if (pixBut == 31) led_setpx(31, 0, 0, 255);
			led_show();
			delay(100);
		}
		if (pixBut > 10 && pixBut <= 16)
		{
			Serial.println(F("But on"));
			led_setpx(5, 0, 255, 0);
			led_show();
			delay(500);
			conf.skpwc = false;
			EEPROM.write(EEP_SKPWC, 1);
			conf.enow = false;
			EEPROM.write(EEP_ENOW, 1);
			EEPROM.commit();
		}
		if (pixBut > 16 && pixBut <= 22)
		{
			Serial.println(F("But off"));
			led_setpx(5, 255, 0, 0);
			led_show();
			delay(500);
			conf.skpwc = true;
			EEPROM.write(EEP_SKPWC, 0);
			conf.enow = true;
			EEPROM.write(EEP_ENOW, 0);
			EEPROM.commit();
		}
		if (pixBut == 31)
		{
			led_clear();
			led_setpx(31, 0, 0, 255);
			led_show();
			int dbgloop = 0;
			while(digitalRead(4) == LOW && onoff == 0)
			{
				if (dbgloop == 300)
				{
					fileSystem->begin();
					json_load();
					if(wifi_wps())
					{
						json_save();
						led_setpx(29, 0, 255, 0); led_setpx(30, 0, 255, 0); led_setpx(31, 0, 255, 0);
					}
					else
					{
						led_setpx(29, 255, 0, 0); led_setpx(30, 255, 0, 0); led_setpx(31, 255, 0, 0);
					}
					led_show();
				}
				dbgloop++;
				delay(10);
			}
			wifi_initap();
			http_beginap();
			while(1)
			{
				http_poll();
				check_off();
				delay(10);
			}
		}
		led_brgn();
	}
}
