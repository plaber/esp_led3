#include "sub_enow.h"
#include "conf.h"
#include "sub_eep.h"
#include "sub_led.h"

#define WIFI_CHANNEL 3

static uint16_t macmask;

void enow_begin()
{
	bool conENow = WifiEspNowBroadcast.begin("ESPNOW", WIFI_CHANNEL);
	if (!conENow) Serial.println(F("WifiEspNowBroadcast.begin() failed"));
	else WifiEspNowBroadcast.onReceive(enow_receive, nullptr);
	macmask = 1;
	for (int i = 0; i <= conf.macslen; i++) macmask |= 1 << i;
	int me = enow_getorder();
	conf.macson |= 1 << (me - 1);
}

void enow_end()
{
	WifiEspNowBroadcast.onReceive(nullptr, nullptr);
}

void enow_poll()
{
	WifiEspNowBroadcast.loop();
}

void enow_receive(const uint8_t mac[6], const uint8_t * buf, size_t count, void * cbarg)
{
	bool nonb = true;
	for (int i = 0; i < conf.macslen; i++)
	{
		if (mac[0] == conf.macs[i][0] && mac[1] == conf.macs[i][1] && mac[2] == conf.macs[i][2] &&
			mac[3] == conf.macs[i][3] && mac[4] == conf.macs[i][4] && mac[5] == conf.macs[i][5])
		{
			nonb = false;
			break;
		}
	}
	if (nonb) return;
	Serial.print(F("Message from "));
	for (int m = 0; m < 6; m++) Serial.printf("%02X", mac[m]);
	Serial.println();
	char buft[count + 1];
	for (int i = 0; i < count; ++i) buft[i] = buf[i]; //Serial.print(static_cast<char>(buf[i]));
	buft[count] = 0;
	String q = String(buft);
	Serial.println(q);
	if (q.startsWith("enow"))
	{
		int dei = q.indexOf("=");
		String sn = q.substring(5, dei);
		String sv = q.substring(dei + 1);
		Serial.println("sn " + sn + " sv " + sv);
		String ans = get_answ(sn, sv);
		char bans[ans.length() + 1];
		ans.toCharArray(bans, ans.length() + 1);
		bans[ans.length()] = 0;
		Serial.print(F("bans is: "));
		Serial.println(String(bans));
		WifiEspNow.send(mac, reinterpret_cast <const uint8_t * > (bans), sizeof bans);
		if (ans == F("restart"))
		{
			led_clear();
			delay(500);
			ESP.restart();
		}
	}
	if (q.startsWith("go"))
	{
		/*
		if (conf.macslen == 1)
		{
			WifiEspNow.send(mac, reinterpret_cast <const uint8_t * > ("enow_go=1"), 9);
		}
		else
		{
		*/
			char ans[8];
			sprintf(ans, "iamon%02d", enow_getorder());
			WifiEspNow.send(mac, reinterpret_cast <const uint8_t * > (ans), 7);
		//}
	}
	if (q.startsWith("iamon"))
	{
		int cl = q.substring(5).toInt();
		conf.macson |= 1 << (cl - 1);
		Serial.print(conf.macson, BIN); Serial.print(" "); Serial.println(macmask, BIN);
		if (conf.macson == macmask)
		{
			WifiEspNowBroadcast.send(reinterpret_cast < const uint8_t * > ("enow_go=1"), 9);
			WifiEspNowBroadcast.send(reinterpret_cast < const uint8_t * > ("enow_go=1"), 9);
			stat.go = true;
		}
	}
}

void enow_send(char txt[])
{
	char msg[60];
	int len = snprintf(msg, sizeof(msg), "%s", txt);
	WifiEspNowBroadcast.send(reinterpret_cast < const uint8_t * > (msg), len);
	Serial.println(F("Sending message"));
	Serial.println(msg);
	Serial.print(F("Recipients:"));
	const int MAX_PEERS = 20;
	WifiEspNowPeerInfo peers[MAX_PEERS];
	int nPeers = std::min(WifiEspNow.listPeers(peers, MAX_PEERS), MAX_PEERS);
	for (int i = 0; i < nPeers; ++i)
	{
		for (int m = 0; m < 6; m++) Serial.printf("%02X", peers[i].mac[m]);
		Serial.write(' ');
	}
	Serial.println();
}

int enow_getorder()
{
	uint8_t mc[6];
	WiFi.softAPmacAddress(mc);
	int px = 1;
	for (int i = 0; i < conf.macslen; i++)
	{
		if (strncmp((char *)conf.macs[i], (char *)mc, 6) > 0) px++;
	}
	return px;
}

void enow_wakeup2()
{
	if (conf.macslen > 0)
	{
		if (!WifiEspNow.hasPeer(conf.macs[0]))
		{
			Serial.print(F("adding peer "));
			WifiEspNow.addPeer(conf.macs[0], 3);
		}
		uint8_t mc[6];
		WiFi.softAPmacAddress(mc);
		Serial.print(F("compare ")); Serial.println(strncmp((char *)mc, (char *)conf.macs[0], 6));
		if (strncmp((char *)mc, (char *)conf.macs[0], 6) > 0)
		{
			WifiEspNow.send(conf.macs[0], reinterpret_cast<const uint8_t*>("go"), 2);
		}
	}
	else
	{
		stat.go = true;
	}
}

void enow_wakeup()
{
	if (conf.macslen > 0)
	{
		for (int i = 0; i < conf.macslen; i++)
		{
			if (!WifiEspNow.hasPeer(conf.macs[i]))
			{
				Serial.print(F("adding peer "));
				for (int m = 0; m < 6; m++) Serial.printf("%02X",  conf.macs[i][m]);
				Serial.println();
				WifiEspNow.addPeer(conf.macs[i], 3);
			}
		}
		int ord = enow_getorder();
		if (ord == conf.macslen + 1)
		{
			WifiEspNowBroadcast.send(reinterpret_cast<const uint8_t*>("go"), 2);
		}
	}
	else
	{
		stat.go = true;
	}
}


