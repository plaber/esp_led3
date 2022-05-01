#ifndef SUB_LIS_H
#define SUB_LIS_H

#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

void lis_init();
void lis_fill();
void lis_poll();

#endif
