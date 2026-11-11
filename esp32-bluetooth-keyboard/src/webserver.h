#ifndef MEDIA_WEBSERVER_H
#define MEDIA_WEBSERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <LittleFS.h>

void initWebServer(uint16_t port = 80);
void startWebServer();
void stopWebServer();
void startWifiAndServer();

#endif
