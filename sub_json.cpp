#include "sub_json.h"
#include "sub_eep.h"
#include "conf.h"

extern String ssid[];
extern String pass[];
extern FS* fileSystem;

String str_encode(String in)
{
	int len = in.length();
	char ans[(len*2) + 1];
	int i = 0; 
	for (int j = 0; j < len; j++)
	{
		sprintf((char*)(ans + i), "%02X", in.charAt(j));
		i += 2;
	}
	ans[i] = 0;
	return String(ans);
}

String str_decode(String in)
{
	int len = in.length();
	char ans[(len/2) + 1];
	char buf[] = "00";
	for (int j = 0; j < len; j += 2)
	{
		buf[0] = in.charAt(j);
		buf[1] = in.charAt(j + 1);
		ans[j / 2] = strtol(buf, NULL, 16);
	}
	ans[len / 2] = 0;
	return String(ans);
}

void mac_decode(String in, uint8_t *ans)
{
	int len = in.length();
	char buf[] = "00";
	for (int j = 0; j < len; j += 2)
	{
		if (j == 12) break;
		buf[0] = in.charAt(j);
		buf[1] = in.charAt(j + 1);
		ans[j / 2] = strtol(buf, NULL, 16);
	}
}

void json_save()
{
	char ebuf[32];
	if (ssid[1] != F("spiffs") && pass[1] != F("spiffs"))
	{
		memset(ebuf, 0, 32); ssid[1].toCharArray(ebuf, 32); EEPROM.put(EEP_SD1, ebuf);
		memset(ebuf, 0, 32); pass[1].toCharArray(ebuf, 32); EEPROM.put(EEP_PS1, ebuf);
	}
	else
	{
		EEPROM.put(EEP_SD1, 0);
		EEPROM.put(EEP_PS1, 0);
	}
	if (ssid[2] != F("spiffs") && pass[2] != F("spiffs"))
	{
		memset(ebuf, 0, 32); ssid[2].toCharArray(ebuf, 32); EEPROM.put(EEP_SD2, ebuf);
		memset(ebuf, 0, 32); pass[2].toCharArray(ebuf, 32); EEPROM.put(EEP_PS2, ebuf);
	}
	else
	{
		EEPROM.put(EEP_SD2, 0);
		EEPROM.put(EEP_PS2, 0);
	}
	if (conf.wpref != F("LedPOI"))
	{
		memset(ebuf, 0, 32); conf.wpref.toCharArray(ebuf, 32); EEPROM.put(EEP_WP, ebuf);
	}
	else
	{
		EEPROM.put(EEP_WP, 0);
	}
	EEPROM.put(EEP_MCL, conf.macslen);
	for (int j = 0; j < 16; j++) EEPROM.put(EEP_MC + j * 6, conf.macs[j]);
	if (stat.maxprog > 0)
	{
		memset(ebuf, 0, 32); stat.progname.toCharArray(ebuf, 32); EEPROM.put(EEP_PG, ebuf);
	}
	else
	{
		EEPROM.put(EEP_PG, 0);
	}
	EEPROM.commit();
}

void json_loadold()
{
	File cnffile = fileSystem->open("/config.txt", "r");
	String confs = cnffile.readStringUntil('\n');
	JSONVar myObject = JSON.parse(confs);
	if (myObject.hasOwnProperty("sd1") && myObject.hasOwnProperty("ps1"))
	{
		ssid[1] = String((const char * ) myObject["sd1"]);
		pass[1] = str_decode(String((const char * ) myObject["ps1"]));
	}
	if (myObject.hasOwnProperty("sd2") && myObject.hasOwnProperty("ps2"))
	{
		ssid[2] = String((const char * ) myObject["sd2"]);
		pass[2] = str_decode(String((const char * ) myObject["ps2"]));
	}
	if (myObject.hasOwnProperty("pr"))
	{
		conf.wpref = String((const char * ) myObject["pr"]);
	}
	if (myObject.hasOwnProperty("mc"))
	{
		JSONVar myArray = myObject["mc"];
		conf.macslen = myArray.length();
		for (int i = 0; i <conf.macslen; i++)
		{
			mac_decode(String((const char * ) myArray[i]),  conf.macs[i]);
		}
	}
	if (myObject.hasOwnProperty("prog")) stat.progname = String((const char * ) myObject["prog"]);
	cnffile.close();
}

void json_load()
{
	char ebuf[32];
	if (fileSystem->exists(F("/config.txt")))
	{
		json_loadold();
		json_save();
		fileSystem->remove(F("/config.txt"));
	}
	else
	{
		EEPROM.get(EEP_SD1, ebuf); if (ebuf[0] != 0 && ebuf[0] != 255) ssid[1] = String(ebuf);
		EEPROM.get(EEP_PS1, ebuf); if (ebuf[0] != 0 && ebuf[0] != 255) pass[1] = String(ebuf);
		EEPROM.get(EEP_SD2, ebuf); if (ebuf[0] != 0 && ebuf[0] != 255) ssid[2] = String(ebuf);
		EEPROM.get(EEP_PS2, ebuf); if (ebuf[0] != 0 && ebuf[0] != 255) pass[2] = String(ebuf);
		EEPROM.get(EEP_WP,  ebuf); if (ebuf[0] != 0 && ebuf[0] != 255) conf.wpref = String(ebuf);
		EEPROM.get(EEP_PG,  ebuf); if (ebuf[0] != 0 && ebuf[0] != 255) stat.progname = String(ebuf);
		ebuf[0] = EEPROM.read(EEP_MCL); if (ebuf[0] != 255) conf.macslen  = ebuf[0];
		EEPROM.get(EEP_MC, conf.macs);
	}
}

void json_del()
{
	EEPROM.put(EEP_SD1, 0);
	EEPROM.put(EEP_PS1, 0);
	EEPROM.put(EEP_SD2, 0);
	EEPROM.put(EEP_PS2, 0);
	EEPROM.put(EEP_WP, 0);
	EEPROM.put(EEP_PG, 0);
	EEPROM.write(EEP_MC, 0);
}
