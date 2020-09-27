/*-----------------------------------------------------------------------------
Stéphane Lepoutère 10/2020

Objet contenant les valeurs en TR, avec les méthodes de
- mise à jour
- renvoie
-----------------------------------------------------------------------------*/
#ifndef VALEURS_H
#define VALEURS_H

#include <Arduino.h>
#include <Wire.h>

#include <FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/event_groups.h>

#include <SPIFFS.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "config.h"


class CValeurs
{
public:
    CValeurs(void);
    void setup(AsyncWebSocket *ws);
    void miseAJour(float temperature, float humidite, char *timestamp);
    void envoie(uint32_t id);

private:
    float temperature;
    float humidite;
    char timestamp[32];
    AsyncWebSocket *ws;
};  

#endif