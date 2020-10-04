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
void CValeurs::miseAJour(float _temperature, float _humidite, uint32_t _pression, uint32_t _gas_resist, char *_timestamp)
{
    // Sauver les parties
    temperature = _temperature;
    humidite = _humidite;
    pression = _pression;
    gas_resist = _gas_resist;
    strlcpy(timestamp, _timestamp, sizeof(timestamp));
    envoie(0);
}

void CValeurs::envoie(uint32_t id)
{
    // Envoyer les données sur le websocket
    char strbuffer[128];
    snprintf(strbuffer, 128, "{\"stamp\":\"%s\",\"temp\":%.2f,\"hydr\":%.2f,\"pression\":%u,\"gas_r\":%u}",
             timestamp,
             temperature,
             humidite,
             pression,
             gas_resist);
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
        ws->text(id, strbuffer);
    }
}
