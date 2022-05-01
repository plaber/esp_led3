#include "sub_eep.h"
#include "conf.h"
#include "sub_bmp.h"
#include "sub_enow.h"
#include "sub_json.h"
#include "sub_led.h"
#include "sub_udp.h"
#include "sub_wifi.h"

extern String ssid[];
extern String pass[];
extern Dir rootFold;
extern FS* fileSystem;

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

void eep_init()
{
	EEPROM.begin(EEP_SIZE);
}

void reset_conf()
{
	conf.wait = 1; EEPROM.write(EEP_WAIT, conf.wait);
	conf.brgn = 8; EEPROM.write(EEP_BRGN, conf.brgn);
	conf.skpwf = false; EEPROM.write(EEP_SKPWF, 1);
	conf.skpwc = false; EEPROM.write(EEP_SKPWC, 1);
	conf.enow = false; EEPROM.write(EEP_ENOW, 1);
	conf.lis_on = false; EEPROM.write(EEP_LIS, 1);
	conf.lis_hit = false;
	conf.lis_brgn = false;
	conf.lis_spd = false;
	EEPROM.write(EEP_LISF, 0b00000000);
	stat.bpm = 4000;
	EEPROM.write(EEP_BPM   , highByte(stat.bpm));
	EEPROM.write(EEP_BPM + 1, lowByte(stat.bpm));
	EEPROM.commit();
}

void reset_factory()
{
	conf.mode = false;
	EEPROM.write(EEP_MODE, 0);
	conf.leds = 32;
	EEPROM.write(EEP_LEDS, 32);
	conf.vcc = 5.8;
	EEPROM.put(EEP_VCC, 5.8);
	conf.fwait = 190;
	EEPROM.write(EEP_FWAIT, 190);
	conf.cont = 0;
	EEPROM.write(EEP_CONT, 0);
	EEPROM.commit();
}

void print_conf()
{
	Serial.print(F("ver ")); Serial.println(conf.ver);
	Serial.print(F("wpref ")); Serial.println(conf.wpref);
	
	Serial.print(F("wait ")); Serial.println(conf.wait);
	Serial.print(F("brgn ")); Serial.println(conf.brgn);
	Serial.print(F("mode ")); Serial.println(conf.mode);
	Serial.print(F("whdr ")); Serial.println(stat.whdr);
	Serial.print(F("vcc  ")); Serial.println(conf.vcc);
	Serial.print(F("leds ")); Serial.println(conf.leds);
	Serial.print(F("skpwf ")); Serial.println(conf.skpwf);
	Serial.print(F("enow ")); Serial.println(conf.enow);
	Serial.print(F("fwait ")); Serial.println(conf.fwait);
	Serial.print(F("lis_on ")); Serial.println(conf.lis_on);
	Serial.print(F("lis_hit ")); Serial.println(conf.lis_hit);
	Serial.print(F("lis_brgn ")); Serial.println(conf.lis_brgn);
	Serial.print(F("lis_spd ")); Serial.println(conf.lis_spd);
	Serial.print(F("lis_oldpcb ")); Serial.println(conf.lis_oldpcb);
	Serial.print(F("cont ")); Serial.println(conf.cont);
	Serial.print(F("skpwc ")); Serial.println(conf.skpwc);
	Serial.print(F("bpm ")); Serial.println(stat.bpm);
	Serial.print(F("mcl ")); Serial.println(conf.macslen);
	Serial.println();
}

void eep_load()
{
	conf.wait = EEPROM.read(EEP_WAIT);
	conf.brgn = EEPROM.read(EEP_BRGN);
	if (conf.wait == 255 && conf.brgn == 255){
		reset_factory();
		reset_conf();
	}
	if (EEPROM.read(EEP_MODE) == 0) conf.mode = false; else conf.mode = true;
	if (EEPROM.read(EEP_WHDR) == 4) stat.whdr = 4; else stat.whdr = 3;
	conf.leds = EEPROM.read(EEP_LEDS);
	EEPROM.get(EEP_VCC, conf.vcc);
	conf.fwait = EEPROM.read(EEP_FWAIT);
	if (conf.fwait == 0) conf.fwait = 10;
	conf.cont = EEPROM.read(EEP_CONT);
	if (conf.cont > 128) conf.cont = 0;
	if (EEPROM.read(EEP_SKPWF) > 0) conf.skpwf = false; else conf.skpwf = true;
	if (EEPROM.read(EEP_SKPWC) > 0) conf.skpwc = false; else conf.skpwc = true;
	if (EEPROM.read(EEP_ENOW) > 0) conf.enow = false; else conf.enow = true;
	if (EEPROM.read(EEP_ENOW) == 2)
	{
		EEPROM.write(EEP_ENOW, 1);  //run once for debug
		EEPROM.commit();
		conf.skpwc =true;
		conf.enow = true;
	}
	if (EEPROM.read(EEP_LIS) > 0) conf.lis_on = false; else conf.lis_on = true;
	uint16_t bpm = word(EEPROM.read(EEP_BPM), EEPROM.read(EEP_BPM + 1));
	if (bpm > 100) stat.bpm = bpm; else stat.bpm = 4000;
	char lisConf = EEPROM.read(EEP_LISF);
	conf.lis_hit = bitRead(lisConf, 0);
	conf.lis_brgn = bitRead(lisConf, 1);
	conf.lis_spd = bitRead(lisConf, 2);
	conf.lis_oldpcb = bitRead(lisConf, 3);
}

int getvcc()
{
	return analogRead(A0) * conf.vcc;
}

int vcc2p(int gvcc)
{
	if (isnan(gvcc) || gvcc < 0 || gvcc > 4200) return 101;
	if (gvcc < 3300) return 0;
	const int svcc[12] = {0, 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4150, 5000};
	const int pvcc[12] = {0,    1,   12,   23,   34,   45,   56,   67,   78,   89,  100,  101};
	int d = 0;
	for (int i = 0; i < 11; i++)
		if (gvcc >= svcc[i] && gvcc < svcc[i + 1])
		{
			d = i;
			break;
		}
	float ans = pvcc[d] + (gvcc - svcc[d]) / 9.00;
	return ans;
}

void lis_save()
{
	EEPROM.write(EEP_LISF, (conf.lis_hit ? 1 : 0) + (conf.lis_brgn ? 2 : 0) + (conf.lis_spd ? 4 : 0) + + (conf.lis_oldpcb ? 8 : 0));
	EEPROM.commit();
}

String get_answ(String san, String sav)
{
	if (san == F("ver")) return (conf.ver + " " + conf.wpref + "_" + String(ESP.getChipId(), HEX));
	if (san == F("vcc"))
	{
		int gvcc = getvcc();
		int pr = vcc2p(gvcc);
		return (String(pr, DEC) + F("% (") + String(getvcc(), DEC) + F("mV) ") + WiFi.RSSI());
	}
	if (san == F("ip"))
	{
		IPAddress broadCast = WiFi.localIP();
		return (String(broadCast[0], DEC) + "." + String(broadCast[1], DEC) + "." + String(broadCast[2], DEC) + "." + String(broadCast[3], DEC));
	}
	if (san == F("drip"))
	{
		IPAddress broadCast = WiFi.localIP();
		led_drawip(broadCast[3]);
		return F("draw ip");
	}
	if (san == F("mac"))
	{
		return String(WiFi.macAddress());
	}
	if (san == F("maca"))
	{
		uint8_t mc[6];
		char mcb[24];
		WiFi.softAPmacAddress(mc);
		sprintf(mcb, "%02X%02X%02X%02X%02X%02X", mc[0], mc[1], mc[2], mc[3], mc[4], mc[5]);
		return String(mcb);
	}
	if (san == F("macs"))
	{
		if (sav == "1")
		{
			conf.macslen = 0;
			memset(conf.macs, 255, 16 * 6);
			json_save();
			return F("mac_ap removed");
		}
		else
		{
			uint8_t macb[6];
			mac_decode(sav, macb);
			char exs = 0;
			for (int i = 0; i < conf.macslen; i++)
			{
				if (strncmp((char *)macb, (char *)conf.macs[i], 6) == 0) {exs = 1; break;}
			}
			if (exs == 1)
			{
				Serial.print(F("mac exist "));
				for (int m = 0; m < 6; m++) Serial.printf("%02X", conf.macs[0][m]);
				Serial.println();
				return F("mac_ap");
			}
			if (conf.macslen < 16)
			{
				for (int i = 0; i < 6; i++) conf.macs[conf.macslen][i] = macb[i];
				conf.macslen += 1;
				json_save();
				Serial.print(F("mac_ap saved "));
				for (int m = 0; m < 6; m++) Serial.printf("%02X", macb[m]);
				Serial.println();
				return F("mac_ap");
			}
			else
			{
				Serial.println(F("mac list full\n"));
				return F("mac_ap");
			}
		}
	}
	if (san == F("macun"))
	{
		if (sav == F("echo"))
		{
			udp_sendmac();
			char uech[8] = "macun=1";
			udp_echo(uech);
			return F("mac sended all");
		}
		if (sav == F("unsn"))
		{
			conf.macslen = 0;
			memset(conf.macs, 255, 16 * 6);
			json_save();
			return F("mac_ap removed");
		}
		else
		{
			udp_sendmac();
			return F("mac sended");
		}
	}
	if (san == F("macor"))
	{
		int px = enow_getorder();
		led_clear();
		for (int i = 0; i < px; i++) led_setpx(i, 0, 255, 0);
		led_show();
		return F("mac order");
	}
	if (san == F("wfaps"))
	{
		int scn = WiFi.scanNetworks();
		String ans = "";
		if (scn > 0)
			for (int scni = 0; scni < scn; ++scni)
			{
				ans += F("<label><input type='radio' name='ssid' value='");
				ans += WiFi.SSID(scni) + "'>" + WiFi.SSID(scni);
				ans += F("</label> [");
				ans += String(WiFi.RSSI(scni), DEC) + "]<br>\n";
			}
			else
			{
				ans = F("no ap found");
			}
		return ans;
	}
	if (san == F("wpref"))
	{
		if (sav == "1")
		{
			conf.wpref = F("LedPOI");
			json_save();
			return F("prefix reseted");
		}
		else if (sav == "0")
		{
			return F("name=") + conf.wpref;
		}
		else
		{
			conf.wpref = sav;
			json_save();
			return F("prefix saved");
		}
	}
	if (san == F("wait") || san == F("spd"))
	{
		if (sav == "m")
		{
			conf.wait -= conf.wait > 1 ? 1 : 0;
		}
		if (sav == "p")
		{
			conf.wait += conf.wait < 200 ? 1 : 0;
		}
		int savi = sav.toInt();
		if (savi > 0 && savi < 200)
		{
			conf.wait = savi;
		}
		EEPROM.write(EEP_WAIT, conf.wait);
		return String(conf.wait);
	}
	if (san == F("brgn") || san == F("brg"))
	{
		String brgnwarn = "";
		if (sav == "m")
		{
			conf.brgn -= conf.brgn > 8 ? 4 : 0;
		}
		if (sav == "p")
		{
			conf.brgn += conf.brgn < 128 ? 4 : 0;
			if (conf.brgn == 128) brgnwarn = F(" max 128");
		}
		int savi = sav.toInt();
		if (savi >= 4 && savi <= 128)
		{
			conf.brgn = savi;
		}
		if (savi > 128) 
		{
			brgnwarn = F(" max 128 < ");
			brgnwarn += String(savi);
		}
		led_brgn();
		if (stat.go == false) led_show();
		EEPROM.write(EEP_BRGN, conf.brgn);
		return String(conf.brgn) + brgnwarn;
	}
	if (san == F("go"))
	{
		if (stat.go == false)
		{
			stat.go = true;
			if (stat.calcmax)
			{
				bmp_max();
				stat.calcmax = false;
				rootFold = fileSystem->openDir("/");
				bmp_next();
			}
			if (conf.wpref == F("fan"))
			{
				pinMode(2, OUTPUT);
				digitalWrite(2, HIGH);
			}
		}//			1					2								3								4								5									6								7									8				9
		return (F("runned ") + String(conf.wait, DEC) + " " + String(conf.brgn, DEC) + " " + String(stat.whdr, DEC) + 					" 0 "				 +String(stat.maxprog, DEC) + 					" 0 " + 				String(stat.maxbmp, DEC) + " 0");
	}
	if (san == F("info"))
	{
		return (		"w " + String(conf.wait, DEC) +" b "+ String(conf.brgn, DEC) +" m "+ String(stat.whdr, DEC) +" p "+ String(stat.currprog + 1, DEC) + "/" + String(stat.maxprog, DEC) + " b " + String(stat.currbmp, DEC) + "/" + String(stat.maxbmp, DEC));
	}
	if (san == F("stp"))
	{
		stat.go = false;
		led_clear();
		led_show();
		if (conf.wpref == F("fan")) 
		{
			digitalWrite(2, LOW);
			pinMode(2, INPUT);
		}
		return F("stop");
	}
	if (san == F("btmode"))
	{
		if (sav == F("one"))
		{
			stat.loop = false;
			return F("mode one");
		}
		if (sav == F("loop"))
		{
			stat.loop = true;
			return F("mode loop");
		}
	}
	if (san == F("mode"))
	{
		if (sav == "4")
		{
			stat.whdr = 4;
			EEPROM.write(EEP_WHDR, stat.whdr);
			return F("prog");
		}
		else
		{
			stat.whdr = 3;
			EEPROM.write(EEP_WHDR, stat.whdr);
			return F("files");
		}
	}
	if (san == F("modeone"))
	{
		if (sav == F("next"))
		{
			bmp_next();
			return stat.currname;
		}
		int pic = sav.toInt();
		int step = 0;
		while (stat.currbmp != pic && step < stat.maxbmp * 2)
		{
			bmp_next();
			step++;
		}
		return stat.currname;
	}
	if (san == F("modefile"))
	{
		int spidx = sav.lastIndexOf("&");
		String fpic;
		if (spidx == -1)
		{
			fpic = sav;
		}
		else
		{
			fpic = sav.substring(0, spidx);Serial.println(fpic);
		}
		if ((fpic.endsWith(exbmp) || fpic.endsWith(exgif) || fpic.endsWith(exjpg)) && fileSystem->exists(fpic))
		{
			
			if (spidx == -1)
			{
				do
				{
					bmp_next();
				}
				while (stat.currname != sav);
			}
			else
			{
				String dspic = sav.substring(spidx + 1);
				stat.currbmp = dspic.toInt();
				stat.currname = fpic;
				
			}
			stat.whdr = 3;
			stat.loop = false;
			return (F("mf ") + stat.currname);
		}
		else
		{
			return F("mf nofile");
		}
	}
	if (san == F("bpm"))
	{
		uint16_t savi = sav.toInt();
		if (savi > 100 && savi < 65535)
		{
			stat.bpm = savi;
			EEPROM.write(EEP_BPM   , highByte(stat.bpm));
			EEPROM.write(EEP_BPM + 1, lowByte(stat.bpm));
			return (F("bpm ") + String(stat.bpm, DEC));
		}
		else if (savi == 0)
		{
			return F("bpm ") + String(stat.bpm, DEC);
		}
		else
		{
			return F("bpm wrong value");
		}
	}
	if (san == F("free"))
	{
		FSInfo fs_info;
		fileSystem->info(fs_info);
		return String(fs_info.totalBytes - fs_info.usedBytes, DEC);
	}
	if (san == F("heap"))
	{
		long fh = ESP.getFreeHeap();
		char fhc[20];
		ltoa(fh, fhc, 10);
		return String(fhc);
	}
	if (san == F("format"))
	{
		if (stat.fcom == false)
		{
			fileSystem->format();
			json_save();
			stat.fcom = true;
			return F("format complete");
		}
		else
		{
			return F("format already complete");
		}
	}
	if (san == F("delbma"))
	{
		Dir rt = fileSystem->openDir("/");
		while (rt.next())
		{
			String f = rt.fileName();
			if (f.endsWith(exbma)) fileSystem->remove(f);
		}
		return F("done");
	}
	if (san == F("conf"))
	{
		if (fileSystem->exists("/config.txt"))
		{
			File cnffile = fileSystem->open("/config.txt", "r");
			return cnffile.readStringUntil('\n');
		}
		else
		{
			String ans;
			ans += F("prog:") + String(stat.progname);
			ans += F(" macslen:") + String(conf.macslen) + " ";
			char bufm[14];
			for (int i = 0; i < conf.macslen; i++)
			{
				sprintf(bufm, "%02X%02X%02X%02X%02X%02X,", conf.macs[i][0],conf.macs[i][1],conf.macs[i][2],conf.macs[i][3],conf.macs[i][4],conf.macs[i][5]);
				ans += String(bufm);
			}
			return ans;
		}
	}
	if (san == F("delconf"))
	{
		json_del();
		conf.macslen = 0;
		return F("conf deleted");
	}
	if (san == F("skwf"))
	{
		if (sav == "1")
		{
			conf.skpwf = true;
			EEPROM.write(EEP_SKPWF, 0);
			EEPROM.commit();
			return F("skip wait wifi on");
		}
		if (sav == "0")
		{
			conf.skpwf = false;
			EEPROM.write(EEP_SKPWF, 1);
			EEPROM.commit();
			return F("skip wait wifi off");
		}
	}
	if (san == F("enow"))
	{
		if (sav == "2")
		{
			EEPROM.write(EEP_ENOW, 2);
			EEPROM.commit();
			return F("next boot in enow");
		}
	}
	if (san == F("cont"))
	{
		int savi = sav.toInt();
		savi = savi > 128 ? 128 : savi;
		conf.cont = savi;
		led_calccont();
		EEPROM.write(EEP_CONT, conf.cont);
		EEPROM.commit();
		return F("cont set ok ") + sav;
	}
	if (san == F("uselis"))
	{
		if (sav == "1")
		{
			conf.lis_on = true;
			EEPROM.write(EEP_LIS, 0);
			EEPROM.commit();
			return F("lis On");
		}
		if (sav == "0")
		{
			conf.lis_on = false;
			EEPROM.write(EEP_LIS, 1);
			EEPROM.commit();
			return F("lis Off");
		}
		EEPROM.commit();
	}
	if (san == F("hitlis"))
	{
		if (sav == "1")
		{
			conf.lis_hit = true;
			lis_save();
			return F("hitLis On");
		}
		if (sav == "0")
		{
			conf.lis_hit = false;
			lis_save();
			return F("hitLis Off");
		}
	}
	if (san == F("brglis"))
	{
		if (sav == "1")
		{
			conf.lis_brgn = true;
			lis_save();
			return F("brgLis On");
		}
		if (sav == "0")
		{
			conf.lis_brgn = false;
			lis_save();
			return F("brgLis Off");
		}
	}
	if (san == F("spdlis"))
	{
		if (sav == "1")
		{
			conf.lis_spd = true;
			lis_save();
			return F("spdLis On");
		}
		if (sav == "0")
		{
			conf.lis_spd = false;
			lis_save();
			return F("spdLis Off");
		}
	}
	if (san == F("oldlis"))
	{
		if (sav == "1")
		{
			conf.lis_oldpcb = true;
			lis_save();
			return F("oldpcb Lis yes");
		}
		if (sav == "0")
		{
			conf.lis_oldpcb = false;
			lis_save();
			return F("oldpcb Lis no");
		}
	}
	if (san == F("cmt"))
	{
		json_save();
		return F("save ok");
	}
	if (san == F("rst"))
	{
		reset_conf();
		return F("reset ok");
	}
	if (san == F("restart"))
	{
		stat.go = false;
		return F("restart");
	}
	if (san == F("blink"))
	{
		led_blinkall();
		return F("blink white");
	}
	if (san == F("prog"))
	{
		return stat.progname;
	}
	if (san == F("prg"))
	{
		String prgwarn = "";
		if (sav == "m")
		{
			if (stat.currprog > 0)
			{
				stat.currprog--;
				stat.progname = String(stat.proglist[stat.currprog]);
			}
		}
		if (sav == "p")
		{
			if (stat.currprog < stat.maxprog - 1)
			{
				stat.currprog++;
				stat.progname = String(stat.proglist[stat.currprog]);
			}
		}
		int savi = sav.toInt();
		if (savi > 0 && savi <= stat.maxprog)
		{
			stat.currprog = savi - 1;
			stat.progname = String(stat.proglist[stat.currprog]);
		}
		return stat.progname;
	}
	return F("no ans ") + san + "=" + sav;
}

String get_answf(String san, String sav)
{
	int savi = sav.toInt();
	savi = savi < 0 ? 0 : savi;
	if (san == F("vcc"))
	{
		conf.vcc = (float) savi / analogRead(A0);
		EEPROM.put(EEP_VCC, conf.vcc);
		EEPROM.commit();
		return F("vcc set ok ") + sav;
	}
	savi = savi > 255 ? 255 : savi;
	if (san == F("leds"))
	{
		conf.leds = savi;
		EEPROM.write(EEP_LEDS, conf.leds);
		EEPROM.commit();
		led_reinit(conf.leds);
		return F("leds set ok ") + sav;
	}
	if (san == F("mode"))
	{
		conf.mode = savi ? true : false;
		EEPROM.write(EEP_MODE, savi);
		EEPROM.commit();
		return F("mode set ok ") + sav;
	}
	if (san == F("fwait"))
	{
		conf.fwait = savi;
		EEPROM.write(EEP_FWAIT, savi);
		EEPROM.commit();
		return F("fwait set ok ") + sav;
	}
	if (san == F("uptime"))
	{
		if(stat.uptime == 0)
		{
			stat.uptime = millis();
			stat.whdr = 3;
			stat.go = true;
		}
		return F("btest run ") + String(stat.uptime, DEC);
	}
	if (san == F("cont") || san == F("skwf") || san == F("uselis") || san == F("hitlis") || san == F("brglis") || san == F("spdlis") || san == F("oldlis"))
	{
		return get_answ(san, sav);
	}
	if (san == F("facres"))
	{
		reset_factory();
		return F("factory reset ok (refresh page)");
	}
	return F("no ans ") + san + "=" + sav;
}
