#include "sub_lis.h"
#include "conf.h"
#include "sub_led.h"

#define OLIS3DH_CLK 16
#define OLIS3DH_MISO 12
#define OLIS3DH_MOSI 14
#define OLIS3DH_CS 13

#define LIS3DH_CLK 13
#define LIS3DH_MISO 14
#define LIS3DH_MOSI 12
#define LIS3DH_CS 16

#define  LIS_SIZE 1000

Adafruit_LIS3DH lis;
bool lis_found = false;
long *lisd;
long lisx = 0, lisy = 0, lisz = 0, lisxp = 0;

void lis_init()
{
	if (conf.lis_oldpcb)
		lis = Adafruit_LIS3DH(OLIS3DH_CS, OLIS3DH_MOSI, OLIS3DH_MISO, OLIS3DH_CLK);
	else
		lis = Adafruit_LIS3DH(LIS3DH_CS, LIS3DH_MOSI, LIS3DH_MISO, LIS3DH_CLK);
	Wire.begin(LIS3DH_MOSI, LIS3DH_MISO);
	if (lis.begin(0x18))
	{
		lisd = (long*)realloc(lisd, LIS_SIZE * sizeof(long));
		stat.lis_found = true;
		lis.setRange(LIS3DH_RANGE_16_G);
		//lis.setScale(lis3dh_scale_4_g);
	}
}

void lis_fill()
{
	lis.read();
	long ax = lis.x, ay = lis.y, az = lis.z;
	lisxp = lisx;
	lisx = ax;
	lisy = ay;
	lisz = az;
	long ab = abs(ax) + abs(ay) + abs(az);
	for (int a = 0; a < LIS_SIZE - 1; a++) lisd[a] = lisd[a + 1];
	lisd[LIS_SIZE - 1] = ab;
}

void lis_poll()
{
	long mLis = 0;
	for (int a = 0; a < LIS_SIZE - 1; a++)
	{
		if (lisd[a] > mLis) mLis = lisd[a];
	}
	if (mLis > 62000) mLis = 62000;
	
	if (mLis > 10000 && stat.go == false && conf.lis_hit)
	{
		stat.go = true;
	}
	
	if (conf.lis_brgn)
	{
		led_brgn(mLis / 250);
	}
	
	if (conf.lis_spd)
	{
		if (mLis > 36000) stat.lis_delay = 650;
		else if (mLis < 9000) stat.lis_delay = 2500;
		else stat.lis_delay = prop(mLis, 9000, 36000, 2500, 650);
	}
}
