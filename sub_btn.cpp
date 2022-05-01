#include "sub_btn.h"
#include "conf.h"
#include "sub_eep.h"
#include "sub_http.h"
#include "sub_json.h"
#include "sub_led.h"
#include "sub_wifi.h"

extern FS* fileSystem;

int onoff = 0, offp = 0;
unsigned long offm = 0;

void check_off()
{
	if (digitalRead(4) == HIGH) {
		if (onoff == 0)
		{
			onoff = 1;
			Serial.println(F("released"));
		}
		if (offp > 0 && offm != millis())
		{
			offp -= 1;
			offm = millis();
		}
	}
	if (digitalRead(4) == LOW && onoff == 1)
	{
		if (offm != millis())
		{
			offp += offm == 0 ? 3 : (millis() - offm);
			offm = millis();
		}
		Serial.printf("OFF %d / %d\n", offp, 1000);
		if (offp > 1000)
		{
			digitalWrite(5, HIGH);
			pinMode(5, INPUT);
			led_brgn(4);
			led_show();
		}
	}
}

void check_up()
{
	int showUp = 0;
	led_setpx(0, 128, 128, 128);
	led_show();
	while (digitalRead(4) == LOW && showUp < 20)
	{
		delay(50);
		showUp++;
	}
	if (digitalRead(4) == LOW)
	{
		led_brgn(12);
		int pixBut = -1;
		while (pixBut < 31 && digitalRead(4) == LOW)
		{
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
