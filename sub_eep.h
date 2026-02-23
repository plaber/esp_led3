#ifndef SUB_EEP_H
#define SUB_EEP_H

#include <EEPROM.h>
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

#define EEP_SIZE 320
#define EEP_WAIT 0
#define EEP_BRGN 1
#define EEP_DIR 2
#define EEP_WHDR 3
#define EEP_VCC  4 //4 bytes
#define EEP_BTN1 8 //NOT USING
#define EEP_LEDS 9
#define EEP_LOOP 10 //btn mode
#define EEP_SKPWF 11 //skip wifi
#define EEP_ENOW  12 //esp now on/off
#define EEP_FWAIT  13 //file delay micros
#define EEP_LIS  14 //use(0) or not(1-255)
#define EEP_LISF 15 //lis config
#define EEP_NET  16 //NOT USING net mode b g n
#define EEP_CONT 17 //contrast
#define EEP_SKPWC 18 //skip wifi connect
#define EEP_BPM 19 //2 bytes
#define EEP_MCL 21

#define EEP_SD1 32
#define EEP_PS1 64
#define EEP_SD2 96
#define EEP_PS2 128
#define EEP_WP 160
#define EEP_PG 192
#define EEP_MC 224

void eep_init();
void reset_conf();
void reset_factory();
void print_conf();
void eep_load();
int getvcc();
int vcc2p(int gvcc);
String get_answ(String san, String sav);

#endif
