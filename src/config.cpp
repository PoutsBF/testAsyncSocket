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
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include "valeurs.h"

#include "config.h"

char daysOfTheWeek[7][9] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 2 * 3600);
// MUTEX pour les accès aux variables d'heure ---
SemaphoreHandle_t mutex_VariablesHeure;

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme;        // I2C

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
    Serial.print("\n---- Configuration du capteur BME680 ---- \n");

    //  Démarrage du capteur
    if (!bme.begin())
    {
        Serial.println(F("Could not find a valid BME680 sensor, check wiring!"));
    }

    //    configuration du capteur (sur échantillonage, filtres)
    bme.setTemperatureOversampling(BME680_OS_8X);   
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);        // 320*C for 150 ms

    valeurs.setup(&ws);

    xTaskCreatePinnedToCore(
        onTimer,              /* Task function. */
        "timer Task",         /* name of task. */
        2048,                 /* Stack size of task */
        NULL,                 /* parameter of the task */
        configMAX_PRIORITIES, /* priority of the task */
        &id_onTimer,          /* Task handle to keep track of created task */
        1);                   /* sur CPU1 */

    Serial.print("\n---- fin de la configuration du capteur BME680 ---- \n");
}

void setup_RTC()
{
    Serial.println("\n---- config RTC ----");
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
        Serial.println("\n---- fin configuration NTC ----");
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
//                portENTER_CRITICAL(&my_mutex);
        unsigned long endTime = bme.beginReading();
//                portEXIT_CRITICAL(&my_mutex);
        if (endTime != 0)
        {
DBG            Serial.print(F("Reading started at "));
DBG            Serial.print(millis());
DBG            Serial.print(F(" and will finish at "));
DBG            Serial.println(endTime);

            if (bme.endReading())
            {
                float mesure_t = 0.0;
                float mesure_h = 0.0;
                uint32_t mesure_p = 0.0;
                uint32_t mesure_g = 0.0;

                // Mise en sommeil en attendant la fin de la lecture
                if (endTime > millis())
                    vTaskDelay(endTime - millis());        // cas classique de différence de temps
                else
                    vTaskDelay(millis() - endTime);        // Débordement du 'millis' : on double le débordement...

                mesure_t = bme.temperature;
                mesure_h = bme.humidity;
                mesure_p = bme.pressure/100;
                mesure_g = bme.gas_resistance;

                //    portENTER_CRITICAL(&mux);
                DateTime now = rtc.now();
                //    portEXIT_CRITICAL(&mux);

                char strbuffer[128];
                snprintf(strbuffer, 128, "%04d-%02d-%02dT%02d:%02d:%02d,%.2f,%.2f,%d,%d\r\n",
                         now.year(), now.month(), now.day(),
                         now.hour(), now.minute(), now.second(),
                         mesure_t,
                         mesure_h,
                         mesure_p,
                         mesure_g);

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
DBG             Serial.println(strbuffer);

                valeurs.miseAJour(mesure_t, mesure_h, mesure_p, mesure_g, strbuffer);
            }
            else
            {
                Serial.println(F("Failed to complete reading :("));
            }
        }
        else
        {
            Serial.println(F("Failed to begin reading :("));
        }
        vTaskDelay(xdelai_mesure);
    }
}
