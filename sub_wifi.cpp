#include "sub_wifi.h"
#include "conf.h"
#include "sub_btn.h"
#include "sub_led.h"
#include "sub_udp.h"

extern int ssid_count;
extern String ssid[];
extern String pass[];
int wifid = -1; //wifi founded

/*
extern "C" {
#include <user_interface.h>
}
*/

void wifi_init()
{
	WiFi.mode(WIFI_AP_STA);
	uint8_t mc[6];
	WiFi.softAPmacAddress(mc);
	//if (mc[5] == 47){uint8_t mac[6] {0xb8, 0xd7, 0x63, 0x00, 0xfe, 0xef}; wifi_set_macaddr(SOFTAP_IF, mac); Serial.println("MAC changed");}
	WiFi.disconnect(true);
	WiFi.persistent(false);
	WiFi.softAP(conf.wpref + "_" + String(ESP.getChipId(), HEX), "12345678");
}

void wifi_initap()
{
	WiFi.mode(WIFI_AP);
	WiFi.softAP("LedDebug_" + String(ESP.getChipId(), HEX));
}

void wifi_conn()
{
	int scn = WiFi.scanNetworks();
	for (int scni = 0; scni < scn; ++scni)
	{
		Serial.println(WiFi.SSID(scni) + " [" + String(WiFi.RSSI(scni), DEC) + "] {ch " + WiFi.channel(scni) + "}");
	}
	for (int s3 = 0; s3 < ssid_count; s3++)
	{
		for (int scni = 0; scni < scn; ++scni)
		{
			if (ssid[s3].equals(WiFi.SSID(scni)))
			{
				wifid = s3;
				Serial.print(F("Founded AP: "));
				Serial.println(ssid[s3]);
				s3 = ssid_count;
				break;
			}
		}
		if (wifid != -1) break;
	}
	if (wifid != -1)
	{
		WiFi.begin(ssid[wifid], pass[wifid]);
		int trycon = 0;
		while (WiFi.status() != WL_CONNECTED && trycon < 20 && conf.skpwf == false)
		{
			Serial.write('.');
			check_off();
			led_blink(255, 0, 0);
			trycon++;
		}
		if (conf.skpwf == true)
		{
			stat.go = true;
		}
		else
		{
			IPAddress broadCast = WiFi.localIP();
			led_drawip(broadCast[3]);
			udp_sendip();
			Serial.println();
			Serial.print(F("IP address: "));
			Serial.println(broadCast.toString());
			char nbuf[7] = {0};
			sprintf(nbuf, "poi%d", broadCast[3]);
			NBNS.begin(nbuf);
		}
	}
	else
	{
		Serial.println(F("No saved AP founded"));
		stat.go = true;
	}
}

bool wifi_wps()
{
	Serial.println(F("WPS config start"));
	WiFi.mode(WIFI_STA);
	delay(200);
	WiFi.begin("foobar",""); // make a failed connection
	bool bln = false;
	while (WiFi.status() == WL_DISCONNECTED) {
		delay(500);
		Serial.print(".");
		if(bln)
		{
			led_setpx(29,   0, 0, 0);
			led_setpx(30, 255, 0, 0);
		}
		else
		{
			led_setpx(29, 255, 0, 0);
			led_setpx(30,   0, 0, 0);
		}
		led_show();
		bln = !bln;
	}
	bool wpsSuccess = WiFi.beginWPSConfig();
	if(wpsSuccess) {
		// Well this means not always success :-/ in case of a timeout we have an empty ssid
		String newSSID = WiFi.SSID();
		if(newSSID.length() > 0)
		{
			// WPSConfig has already connected in STA mode successfully to the new station. 
			Serial.println(F("WPS finished. Connected successfull to SSID"));
			Serial.println(newSSID);
			Serial.println(WiFi.psk());
			ssid[2] = newSSID;
			pass[2] = WiFi.psk();
		}
		else
		{
			wpsSuccess = false;
		}
	}
	return wpsSuccess; 
}

