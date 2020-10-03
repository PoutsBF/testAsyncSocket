#include <Arduino.h>
#include <Wire.h>

#include <FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>

//#define DEBUG

//#include <RTClib.h>
#ifndef CONFIG_H
#define CONFIG_H

#ifdef DEBUG
#define DBG     
#else
#define DBG     // --- DEBUG 
#endif

extern char daysOfTheWeek[7][9];

const uint8_t led = 13;

void setup_DHT(void);
void setup_RTC(void);

/* -- Macro Definitions */

#endif