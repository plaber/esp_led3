#ifndef CONF_H
#define CONF_H

#define ATOMIC_FS_UPDATE

struct config
{
	String ver;
	String wpref;
	uint8_t macs[16][6];
	uint16_t macson;
	uint8_t macslen;
	uint8_t wait;
	uint8_t brgn;
	bool dir;
	uint8_t leds;
	float vcc;
	uint8_t fwait;
	uint8_t cont;
	bool skpwf;
	bool skpwc;
	bool enow;
	bool lis_on;
	bool lis_hit;
	bool lis_brgn;
	bool lis_spd;
	bool lis_oldpcb;
};

struct status
{
	bool go;
	int maxbmp;
	int currbmp;
	String currname;
	bool loop;
	int whdr;
	uint16_t bpm;
	bool fcom;
	bool lis_found;
	int lis_delay;
	unsigned long uptime;
	uint8_t maxprog;
	uint8_t currprog;
	String progname;
	char proglist[15][32];
	bool calcmax;
	uint8_t enrgsv;
};

extern struct config conf;
extern struct status stat;
extern char exbmp[5];
extern char exbma[5];
extern char exgif[5];
extern char exjpg[5];
extern char extxt[5];

#endif
