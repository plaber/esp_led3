#include "sub_udp.h"
#include "conf.h"
#include "sub_eep.h"
#include "sub_led.h"
#include "sub_wifi.h"

WiFiUDP Udp;
char udpBuf[255]; //buffer to hold incoming packet

void udp_begin()
{
	Udp.begin(8888);
}

void udp_poll()
{
	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		int len = Udp.read(udpBuf, 255);
		if (len > 0)
		{
			udpBuf[len] = 0;
			String q = String(udpBuf);
			int dei = q.indexOf("=");
			if (dei != -1)
			{
				String sn = q.substring(0, dei);
				String sv = q.substring(dei + 1);
				String ans = get_answ(sn, sv);
				char ansBuf[ans.length() + 1];
				ans.toCharArray(ansBuf, ans.length() + 1);
				Udp.beginPacket(Udp.remoteIP(), 60201);
				Udp.write(ansBuf);
				Udp.endPacket();
				if (ans == F("restart"))
				{
					led_clear();
					delay(500);
					ESP.restart();
				}
			}
		}
	}
}

void udp_sendmac()
{
	IPAddress broadCast = WiFi.localIP();
	broadCast[3] = 255;
	uint8_t mc[6];
	WiFi.softAPmacAddress(mc);
	char mcb[29];
	sprintf(mcb, "macs=%02X%02X%02X%02X%02X%02X", mc[0], mc[1], mc[2], mc[3], mc[4], mc[5]);
	Udp.beginPacket(broadCast, 8888);
	Udp.write(mcb);
	Udp.endPacket();
}

void udp_sendip()
{
	IPAddress broadCast = WiFi.localIP();
	char ansBuf[21] = {0};
	sprintf(ansBuf, "myip %d.%d.%d.%d", broadCast[0], broadCast[1], broadCast[2], broadCast[3]);
	broadCast[3] = 255;
	Udp.beginPacket(broadCast, 60201);
	Udp.write(ansBuf);
	Udp.endPacket();
}

void udp_echo(char *buf)
{
	IPAddress broadCast = WiFi.localIP();
	broadCast[3] = 255;
	Udp.beginPacket(broadCast, 8888);
	Udp.write(buf);
	Udp.endPacket();
}

