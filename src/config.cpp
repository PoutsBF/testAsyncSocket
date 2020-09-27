// Fonctions d'ordre générale du programme

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
#include <NtpClient.h>

#include <RTClib.h>
#include "DHTesp.h"
#include "valeurs.h"

#include "config.h"

char daysOfTheWeek[7][9] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 2 * 3600);
// MUTEX pour les accès aux variables d'heure ---
SemaphoreHandle_t mutex_VariablesHeure;

const uint8_t DHTPIN = 21;            // Digital pin connected to the DHT sensor
DHTesp dht;

CValeurs valeurs;

#ifdef DEBUG
const TickType_t xdelai_mesure = (1 * 60 * 1000) / portTICK_PERIOD_MS;        // 1mn x 60s x 1000ms
#else
const TickType_t xdelai_mesure = (15 * 60 * 1000);        // 15mn x 60s x 1000ms
#endif
const TickType_t mesures_delai = (5 * 1000) / portTICK_PERIOD_MS;
const uint8_t mesures_nb = 3;

TaskHandle_t id_tache_ntc;
void tache_ntc(void *pvParameters);

TaskHandle_t id_onTimer;
void onTimer(void *pvParameters);

RTC_DS3231 rtc;


extern AsyncWebSocket ws;

void setup_DHT(void)
{
    Serial.print("\nConfiguration du capteur DHT... ");
    dht.setup(DHTPIN, DHTesp::DHT11);

    switch (dht.getStatus())
    {
        case DHTesp::ERROR_NONE:
        {
            Serial.println("DHT ok !");
        }
        break;
        case DHTesp::ERROR_CHECKSUM:
        {
            Serial.println("DHT : erreur sur le checksum");
        }
        break;
        case DHTesp::ERROR_TIMEOUT:
        {
            Serial.println("DHT : erreur de timeout");
        }
        break;
    }

    valeurs.setup(&ws);

    xTaskCreatePinnedToCore(
        onTimer,              /* Task function. */
        "timer Task",         /* name of task. */
        2048,                 /* Stack size of task */
        NULL,                 /* parameter of the task */
        configMAX_PRIORITIES, /* priority of the task */
        &id_onTimer,          /* Task handle to keep track of created task */
        1);                   /* sur CPU1 */
}

void setup_RTC()
{
    Serial.println("---- config RTC ----");
    if(rtc.begin())
    {
        Serial.println("RTC ok, démarrage...");
    }
    else
    {
        Serial.println("RTC NOK, fonctionnement anormal !");
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC alimentation perdue, remise à l'heure !");
        // When time needs to be set on a new device, or after a power loss, the
        // following line sets the RTC to the date & time this sketch was compiled
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        rtc.begin();
        // This line sets the RTC with an explicit date & time, for example to set
        // January 21, 2014 at 3am you would call:
        // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    DateTime now = rtc.now();

    Serial.println(now.timestamp());

    Serial.print("Temperature RTC: ");
    Serial.print(rtc.getTemperature());
    Serial.println(" °C");

    // Configuration et lancement des tâches ------
    mutex_VariablesHeure = xSemaphoreCreateMutex();

    // configuration ntc --
    Serial.println("Configuration de la connexion ntc");
    timeClient.begin();

    xTaskCreatePinnedToCore(
        tache_ntc,     /* Task function. */
        "ntc Task",    /* name of task. */
        2048,          /* Stack size of task */
        NULL,          /* parameter of the task */
        2,             /* priority of the task */
        &id_tache_ntc, /* Task handle to keep track of created task */
        1);            /* sur CPU1 */


}

// ****************************************************************************
// tache_ntc : tache vérifiant que le rtc est à l'heure ***********************
void tache_ntc(void *pvParameters)
{
    // Block for 24h = 24 * 60 * 60 * 1000
    const TickType_t xDelay = (24 * 60 * 60 * 1000) / portTICK_PERIOD_MS;

    for (;;)
    {
                // Update l'heure.
        if(timeClient.forceUpdate())
        {
            DateTime now = rtc.now();

            Serial.print("ntp - ");

            if (now.unixtime() != timeClient.getEpochTime())
            {
                Serial.printf (" dérive heure : %s ", timeClient.getFormattedTime());
                now = timeClient.getEpochTime();
                xSemaphoreTake(mutex_VariablesHeure, portMAX_DELAY);
                    rtc.adjust(now);
                xSemaphoreGive(mutex_VariablesHeure);
            }
            else
            {
                Serial.print(" heure ok ! - ");
            }
            
        }

        Serial.println("fin");
        vTaskDelay(xDelay);
    }
}

// ****************************************************************************
// onTimer : appelé pour faire les x mesures à la fin des périodes ************
void onTimer(void *pvParameters)
{
    static portMUX_TYPE my_mutex;

    vPortCPUInitializeMutex(&my_mutex);
    
    for (;;)
    {
        float mesure_total_t = 0.0;
        float mesure_total_h = 0.0;
        uint8_t mesures_t = 1 << mesures_nb;
        uint8_t mesures_h = 1 << mesures_nb;

        for (uint8_t i = 0; i < (1 << mesures_nb); i++)
        {
            portENTER_CRITICAL(&my_mutex);
            TempAndHumidity newValues = dht.getTempAndHumidity();
            portEXIT_CRITICAL(&my_mutex);
            // Check if any reads failed and exit early (to try again).
            if (dht.getStatus() != 0)
            {
                Serial.printf("lecture %d, DHT11 error status: %s\n", i, dht.getStatusString());
                mesures_t--;
                mesures_h--;
            }
            else
            {
                mesure_total_t += newValues.temperature;
                mesure_total_h += newValues.humidity;
            }
            vTaskDelay(mesures_delai);
        }

        if (mesures_t)
            mesure_total_t /= mesures_t;
        if (mesures_h)
            mesure_total_h /= mesures_h;

        //    portENTER_CRITICAL(&mux);
        DateTime now = rtc.now();
        //    portEXIT_CRITICAL(&mux);

        char strbuffer[64];
        snprintf(strbuffer, 64, "%04d-%02d-%02dT%02d:%02d:%02d,%.2f,%.2f\n",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second(),
                 mesure_total_t,
                 mesure_total_h);

        File f = SPIFFS.open("/temperature.csv", "a+");
        if (!f)
        {
            Serial.println("erreur ouverture fichier!");
        }
        else
        {
            f.print(strbuffer);
            f.close();
        }

        snprintf(strbuffer, 64, "%04d-%02d-%02dT%02d:%02d:%02d",
                 now.year(), now.month(), now.day(),
                 now.hour(), now.minute(), now.second());
DBG        Serial.println(strbuffer);

        valeurs.miseAJour(mesure_total_t, mesure_total_h, strbuffer);
// if (ws.count() != 0)
// {
//     ws.textAll(strbuffer);
//     //            globalClient->text(output);
//         }

        vTaskDelay(xdelai_mesure);
    }
}
