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

#include "valeurs.h"

CValeurs::CValeurs(void)
{
    temperature = 0.0;
    humidite = 0.0;
    timestamp[0] = 0;
}

void CValeurs::setup(AsyncWebSocket *pws)
{
    ws = pws;
}
void CValeurs::miseAJour(float _temperature, float _humidite, char *_timestamp)
{
    // Sauver les parties
    temperature = _temperature;
    humidite = _humidite;
    strlcpy(timestamp, _timestamp, sizeof(timestamp));
    envoie(0);
}

void CValeurs::envoie(uint32_t id)
{
    // Envoyer les données sur le websocket
    char strbuffer[64];
    snprintf(strbuffer, 64, "{\"stamp\":\"%s\",\"temp\":%.2f,\"hydr\":%.2f}",
             timestamp,
             temperature,
             humidite);
    DBG Serial.println(strbuffer);
    if (id == 0)
    {
        if (ws->count() != 0)
        {
            ws->textAll(strbuffer);
        }
    }
    else
    {
        if ((id - 1) <= ws->count())
        {
            ws->text(id, strbuffer);
        }
    }
}
