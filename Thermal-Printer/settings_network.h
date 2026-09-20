#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "globals.h"


static bool isConfigurablePin(
    uint8_t pin
)
{
    if (pin > 48)
    {
        return false;
    }

    switch (pin)
    {
        case 0:   // BOOT
        case 19:  // USB D+
        case 20:  // USB D-
        case 35:
        case 36:
        case 37:  // PSRAM
        case 38:  // SD_CMD
        case 39:  // SD_CLK
        case 40:  // SD_DATA
        case 43:  // Debug TX
        case 44:  // Debug RX
            return false;
        default:
            return true;
    }
}


void loadSettings()
{
    ticketCounter =
        preferences.getUInt(
            "counter",
            0
        );

    activeTemplate =
        preferences.getString(
            "template",
            "default"
        );

    wifiSSID =
        preferences.getString(
            "wifiSSID",
            ""
        );

    wifiPassword =
        preferences.getString(
            "wifiPass",
            ""
        );

    apSSID =
        preferences.getString(
            "apSSID",
            DEFAULT_AP_SSID
        );

    apPassword =
        preferences.getString(
            "apPass",
            DEFAULT_AP_PASSWORD
        );

    autoCut =
        preferences.getBool(
            "autoCut",
            true
        );

    printerBaud =
        preferences.getULong(
            "printerBaud",
            DEFAULT_PRINTER_BAUD
        );

    if (
        printerBaud < 300 ||
        printerBaud > 5000000
    )
    {
        printerBaud =
            DEFAULT_PRINTER_BAUD;
    }

    printOrientation =
        preferences.getUChar(
            "orientation",
            0
        );

    printOrientation =
        constrain(
            printOrientation,
            0,
            1
        );

    mdnsHostname =
        preferences.getString(
            "mdns",
            DEFAULT_MDNS_HOSTNAME
        );

    feedLines =
        preferences.getUChar(
            "feed",
            4
        );

    defaultQRSize =
        preferences.getUChar(
            "qrSize",
            5
        );

    pageWidth =
        preferences.getUShort(
            "pageWidth",
            DEFAULT_PAGE_WIDTH
        );

    pageWidth =
        constrain(
            pageWidth,
            static_cast<uint16_t>(48),
            static_cast<uint16_t>(576)
        );

    printerTxPin =
        preferences.getUChar(
            "printerTxPin",
            PRINTER_TX_PIN
        );

    printerRxPin =
        preferences.getUChar(
            "printerRxPin",
            PRINTER_RX_PIN
        );

    if (!isConfigurablePin(printerTxPin))
    {
        printerTxPin = PRINTER_TX_PIN;
    }

    if (!isConfigurablePin(printerRxPin))
    {
        printerRxPin = PRINTER_RX_PIN;
    }

    if (printerTxPin == printerRxPin)
    {
        printerTxPin = PRINTER_TX_PIN;
        printerRxPin = PRINTER_RX_PIN;
    }

    if (!isConfigurablePin(triggerGPIO))
    {
        triggerGPIO = DEFAULT_TRIGGER_GPIO;
    }
}



void startFallbackAP()
{
    WiFi.mode(WIFI_AP_STA);

    bool result =
        WiFi.softAP(
            apSSID.c_str(),
            apPassword.c_str()
        );

    fallbackAPActive =
        result;

    Serial.println();

    if (result)
    {
        Serial.println(
            "Fallback AP started."
        );

        Serial.print(
            "SSID: "
        );

        Serial.println(
            apSSID
        );

        Serial.print(
            "AP IP: "
        );

        Serial.println(
            WiFi.softAPIP()
        );
    }
    else
    {
        Serial.println(
            "Fallback AP failed."
        );
    }
}

bool connectConfiguredWiFi()
{
    wifiConnected = false;

    if (
        wifiSSID.length() == 0
    )
    {
        Serial.println(
            "No Wi-Fi network configured."
        );

        startFallbackAP();

        return false;
    }

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        wifiSSID.c_str(),
        wifiPassword.c_str()
    );

    Serial.print(
        "Connecting to Wi-Fi: "
    );

    Serial.println(
        wifiSSID
    );

    uint32_t started =
        millis();

    while (
        WiFi.status() != WL_CONNECTED &&
        millis() - started <
            WIFI_TIMEOUT_MS
    )
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        wifiConnected = true;

        Serial.println(
            "Wi-Fi connected."
        );

        Serial.print(
            "IP: "
        );

        Serial.println(
            WiFi.localIP()
        );

        Serial.print(
            "RSSI: "
        );

        Serial.print(
            WiFi.RSSI()
        );

        Serial.println(
            " dBm"
        );

        return true;
    }

    Serial.println(
        "Wi-Fi connection failed."
    );

    WiFi.disconnect();

    startFallbackAP();

    return false;
}


