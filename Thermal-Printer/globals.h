#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#include "config.h"


HardwareSerial Printer(1);
WebServer server(80);
Preferences preferences;



uint32_t ticketCounter = 0;

String activeTemplate = "default";

String wifiSSID;
String wifiPassword;

String apSSID = DEFAULT_AP_SSID;
String apPassword = DEFAULT_AP_PASSWORD;

bool wifiConnected = false;
bool fallbackAPActive = false;

bool autoCut = true;
uint8_t printOrientation = 0;
uint8_t feedLines = 4;
uint8_t defaultQRSize = 5;
String mdnsHostname = DEFAULT_MDNS_HOSTNAME;

bool gpioTriggerEnabled = true;
uint8_t triggerGPIO = DEFAULT_TRIGGER_GPIO;
bool triggerActiveLow = true;
uint32_t triggerDebounceMs = DEFAULT_TRIGGER_DEBOUNCE_MS;

bool scannerTriggerEnabled = false;
uint8_t scannerMode = 0;
String scannerPattern;
bool scannerUniqueOnly = false;



bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;

uint32_t lastButtonChange = 0;





File uploadFile;
bool uploadOK = false;
String uploadTargetPath;


