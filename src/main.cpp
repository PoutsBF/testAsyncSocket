#include <ArduinoOTA.h>

#include <TimeLib.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include <freertos/FreeRTOSConfig.h>

#include <FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <freertos/event_groups.h>


#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#include <RTClib.h>

#include "valeurs.h"
#include "config.h"
#include "WifiConfig.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    extern CValeurs valeurs;

    if (type == WS_EVT_CONNECT)
    {
DBG     char texte[64];
DBG     snprintf(texte, 64, "{\"Hello Client\": \"%u\"}", client->id());
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
DBG     client->printf(texte);
        valeurs.envoie(client->id());
DBG     client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
    }
    else if (type == WS_EVT_ERROR)
    {
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
    else if (type == WS_EVT_PONG)
    {
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        String msg = "";
        if (info->final && info->index == 0 && info->len == len)
        {
            //the whole message is in a single frame and we got all of it's data
            Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < info->len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }
            Serial.printf("%s\n", msg.c_str());

            if (info->opcode == WS_TEXT)
DBG                client->text("{\"message\":\"I got your text message\"}");
            else
DBG                client->binary("{\"message\":\"I got your binary message\"}");
        }
        else
        {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0)
            {
                if (info->num == 0)
                    Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
            }

            Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }
            Serial.printf("%s\n", msg.c_str());

            if ((info->index + len) == info->len)
            {
                Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                if (info->final)
                {
                    Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    if (info->message_opcode == WS_TEXT)
DBG                     client->text("{\"message\":\"I got your text message\"}");
                    else
DBG                     client->binary("{\"message\":\"I got your binary message\"}");
                }
            }
        }
    }
}

const char *ssid = YOUR_WIFI_SSID;
const char *password = YOUR_WIFI_PASSWD;
const char *hostName = "esp-async";
const char *http_username = "admin";
const char *http_password = "admin";

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println("------- Stéphane Lepoutère 2020 -- Pemperanda -- début de la config'");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(hostName);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.printf("STA: Failed!\n");
        WiFi.disconnect(false);
        delay(1000);
        WiFi.begin(ssid, password);
    }
    Serial.print("adresse IP : ");
    Serial.println(WiFi.localIP());

    //Send OTA events to the browser
    ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
    ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        char p[32];
        sprintf(p, "Progress: %u%%\n", (progress / (total / 100)));
        events.send(p, "ota");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        if (error == OTA_AUTH_ERROR)
            events.send("Auth Failed", "ota");
        else if (error == OTA_BEGIN_ERROR)
            events.send("Begin Failed", "ota");
        else if (error == OTA_CONNECT_ERROR)
            events.send("Connect Failed", "ota");
        else if (error == OTA_RECEIVE_ERROR)
            events.send("Recieve Failed", "ota");
        else if (error == OTA_END_ERROR)
            events.send("End Failed", "ota");
    });
    ArduinoOTA.setHostname(hostName);
    ArduinoOTA.begin();

    MDNS.addService("http", "tcp", 80);

    //  Configuration du gestionnaire de fichiers SPIFFS -----------
    if (!SPIFFS.begin())
    {
        Serial.println("Erreur initialisation SPIFFS");
    }

    /* Détection des fichiers présents sur l'Esp32 */
    File root = SPIFFS.open("/");    /* Ouverture de la racine */
    File file = root.openNextFile(); /* Ouverture du 1er fichier */
    while (file)                     /* Boucle de test de présence des fichiers - Si plus de fichiers la boucle s'arrête*/

    {
        Serial.print("File: ");
        Serial.println(file.name());
        file.close();
        file = root.openNextFile(); /* Lecture du fichier suivant */
    }

// Configuration des évènements web server
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    events.onConnect([](AsyncEventSourceClient *client) {
        client->send("hello!", NULL, millis(), 1000);
    });
    server.addHandler(&events);

    server.addHandler(new SPIFFSEditor(SPIFFS, http_username, http_password));

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

    server.onNotFound([](AsyncWebServerRequest *request) {
        Serial.printf("NOT_FOUND: ");
        if (request->method() == HTTP_GET)
            Serial.printf("GET");
        else if (request->method() == HTTP_POST)
            Serial.printf("POST");
        else if (request->method() == HTTP_DELETE)
            Serial.printf("DELETE");
        else if (request->method() == HTTP_PUT)
            Serial.printf("PUT");
        else if (request->method() == HTTP_PATCH)
            Serial.printf("PATCH");
        else if (request->method() == HTTP_HEAD)
            Serial.printf("HEAD");
        else if (request->method() == HTTP_OPTIONS)
            Serial.printf("OPTIONS");
        else
            Serial.printf("UNKNOWN");
        Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

        if (request->contentLength())
        {
            Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
            Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
        }

        int headers = request->headers();
        int i;
        for (i = 0; i < headers; i++)
        {
            AsyncWebHeader *h = request->getHeader(i);
            Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
        }

        int params = request->params();
        for (i = 0; i < params; i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isFile())
            {
                Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
            }
            else if (p->isPost())
            {
                Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
            else
            {
                Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
        }

        request->send(404);
    });
    server.onFileUpload([](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index)
            Serial.printf("UploadStart: %s\n", filename.c_str());
        Serial.printf("%s", (const char *)data);
        if (final)
            Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index + len);
    });
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (!index)
            Serial.printf("BodyStart: %u\n", total);
        Serial.printf("%s", (const char *)data);
        if (index + len == total)
            Serial.printf("BodyEnd: %u\n", total);
    });
    server.begin();

    setup_RTC();
    setup_DHT();
    Serial.println("---- Fin du setup ----");
}

void print_shit(void);
void loop()
{
    ArduinoOTA.handle();
    ws.cleanupClients();
    print_shit();
}

// -- https://www.esp32.com/viewtopic.php?t=3674
// void print_shit(void)
// {
//     char *str = (char *)malloc(sizeof(char) * 2000);
//     memset(str, 0, 2000);
//     char *pcWriteBuffer = str;

//     TaskStatus_t *pxTaskStatusArray;

//     volatile UBaseType_t uxArraySize, x;
//     unsigned long ulStatsAsPercentage;
//     uint32_t ulTotalRunTime;

//     /* Make sure the write buffer does not contain a string. */
//     *pcWriteBuffer = 0x00;

//     /* Take a snapshot of the number of tasks in case it changes while this
//        function is executing. */
//     uxArraySize = uxTaskGetNumberOfTasks();

//     /* Allocate a TaskStatus_t structure for each task.  An array could be
//        allocated statically at compile time. */
//     Serial.printf("sizeof TaskStatus_t: %d\n", sizeof(TaskStatus_t));
//     pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
//     memset(pxTaskStatusArray, 0, uxArraySize * sizeof(TaskStatus_t));

//     if (pxTaskStatusArray != NULL)
//     {
//         /* Generate raw status information about each task. */
//         Serial.printf("Array size before: %d\n", uxArraySize);
//         uxArraySize = uxTaskGetSystemState(pxTaskStatusArray,
//                                            uxArraySize,
//                                            &ulTotalRunTime);
//         Serial.printf("Array size after: %d\n", uxArraySize);
//         Serial.printf("Total runtime: %d\n", ulTotalRunTime);

//         /* For percentage calculations. */
//         ulTotalRunTime /= 100UL;

//         /* Avoid divide by zero errors. */
//         if (ulTotalRunTime > 0)
//         {
//             /* For each populated position in the pxTaskStatusArray array,
//              format the raw data as human readable ASCII data. */
//             Serial.printf("  name        runtime      pct     core         prio\n");
//             for (int x = 0; x < uxArraySize; x++)
//             {
//                 TaskStatus_t *fuckit;
//                 void *tmp = &pxTaskStatusArray[x];
//                 void *hmm = tmp + (4 * x);
//                 fuckit = (TaskStatus_t *)hmm;

//                 // Serial.printf("Name: %.5s, ulRunTimeCounter: %d\n", fuckit->pcTaskName , fuckit->ulRunTimeCounter);
//                 /* What percentage of the total run time has the task used?
//                 This will always be rounded down to the nearest integer.
//                 ulTotalRunTimeDiv100 has already been divided by 100. */
//                 ulStatsAsPercentage =
//                     fuckit->ulRunTimeCounter / ulTotalRunTime;

//                 if (ulStatsAsPercentage > 0UL)
//                 {
//                     sprintf(pcWriteBuffer, "%30s %10lu %10lu%% %5d %5d\n",
//                             fuckit->pcTaskName,
//                             fuckit->ulRunTimeCounter,
//                             ulStatsAsPercentage,
//                             *((int *)(&fuckit->usStackHighWaterMark) + 1),
//                             fuckit->uxBasePriority);
//                 }
//                 else
//                 {
//                     /* If the percentage is zero here then the task has
//                    consumed less than 1% of the total run time. */
//                     sprintf(pcWriteBuffer, "%30s %10lu %10s  %5d %5d\n",
//                             fuckit->pcTaskName,
//                             fuckit->ulRunTimeCounter,
//                             "<1%",
//                             *((uint32_t *)(&fuckit->usStackHighWaterMark) + 1),
//                             fuckit->uxBasePriority);
//                 }

//                 pcWriteBuffer += strlen((char *)pcWriteBuffer);
//                 // Serial.printf("len: %d, idx: %d\n", pcWriteBuffer - str, x);
//             }
//             Serial.print(str);
//         }

//         /* The array is no longer needed, free the memory it consumes. */
//         //   vPortFree( pxTaskStatusArray );
//         //   free(str);
//     }
// }
