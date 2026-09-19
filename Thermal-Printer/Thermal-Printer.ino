#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SD_MMC.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <time.h>

// Modular project headers
#include "config.h"
#include "globals.h"
#include "storage.h"
#include "helpers.h"
#include "auth.h"
#include "printer.h"
#include "settings_network.h"
#include "webui.h"


void setup()
{
    Serial.begin(
        DEBUG_BAUD
    );

    delay(1000);

    Serial.println();
    Serial.println(
        "########################################"
    );

    Serial.println(
        "#                                      #"
    );

    Serial.println(
        "#       TICKET PRINTER V2              #"
    );

    Serial.println(
        "#                                      #"
    );

    Serial.println(
        "########################################"
    );

    Serial.println();

    Serial.print(
        "Firmware: "
    );

    Serial.println(
        FIRMWARE_VERSION
    );

    // --------------------------------------------------------
    // Button
    // --------------------------------------------------------

    configureTriggerGPIO();

    // --------------------------------------------------------
    // Printer UART
    // --------------------------------------------------------

    Printer.begin(
        printerBaud,
        SERIAL_8N1,
        PRINTER_RX_PIN,
        PRINTER_TX_PIN
    );

    Serial.print(
        "Printer TX: GPIO"
    );

    Serial.println(
        PRINTER_TX_PIN
    );

    Serial.print(
        "Printer RX: GPIO"
    );

    Serial.println(
        PRINTER_RX_PIN
    );

    Serial.print(
        "Printer baud: "
    );

    Serial.println(
        printerBaud
    );

    // --------------------------------------------------------
    // Preferences
    // --------------------------------------------------------

    preferences.begin(
        "ticketV2",
        false
    );

    gpioTriggerEnabled =
        preferences.getBool(
            "gpioEnabled",
            true
        );

    triggerGPIO =
        preferences.getUChar(
            "gpioPin",
            DEFAULT_TRIGGER_GPIO
        );

    triggerActiveLow =
        preferences.getBool(
            "gpioLow",
            true
        );

    triggerDebounceMs =
        preferences.getULong(
            "gpioDebounce",
            DEFAULT_TRIGGER_DEBOUNCE_MS
        );

    scannerTriggerEnabled =
        preferences.getBool(
            "scanEnabled",
            false
        );

    scannerMode =
        preferences.getUChar(
            "scanMode",
            0
        );

    scannerPattern =
        preferences.getString(
            "scanPattern",
            ""
        );

    scannerUniqueOnly =
        preferences.getBool(
            "scanUnique",
            false
        );


    loadSettings();

    // --------------------------------------------------------
    // Storage
    // SD card has priority. LittleFS is the automatic fallback.
    // --------------------------------------------------------

    bool littleFSReady =
        LittleFS.begin(
            true
        );

    if (!littleFSReady)
    {
        Serial.println(
            "LittleFS mount failed."
        );
    }

    SD_MMC.setPins(
        SD_CLK_PIN,
        SD_CMD_PIN,
        SD_DATA_PIN
    );

    sdCardReady = SD_MMC.begin(
        "/sdcard",
        true
    );

    if (sdCardReady && SD_MMC.cardType() == CARD_NONE)
    {
        sdCardReady = false;
    }

    if (sdCardReady)
    {
        Serial.println(
            "Storage backend: SD_MMC (1-bit)"
        );
    }
    else
    {
        Serial.println(
            "SD card not available."
        );

        if (littleFSReady)
        {
            Serial.println(
                "Storage backend: LittleFS"
            );
        }
        else
        {
            Serial.println(
                "No usable storage backend."
            );
        }
    }

    if (sdCardReady || littleFSReady)
    {
        if (sdCardReady && littleFSReady)
        {
            migrateLittleFSToSD();
        }

        ensureStorageDirectories();
        createDefaultTemplate();
        loadAdminPassword();

        Serial.print(
            "Admin protection: "
        );

        Serial.println(
            adminProtectionEnabled
                ? "enabled"
                : "disabled"
        );

        if (sdCardReady)
        {
            Serial.print(
                "SD total: "
            );

            Serial.println(
                SD_MMC.totalBytes()
            );

            Serial.print(
                "SD used: "
            );

            Serial.println(
                SD_MMC.usedBytes()
            );
        }
    }

    // --------------------------------------------------------
    // Wi-Fi
    // --------------------------------------------------------

    connectConfiguredWiFi();

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        configTzTime(
            "CET-1CEST,M3.5.0,M10.5.0/3",
            "pool.ntp.org",
            "time.cloudflare.com"
        );

        if (
            MDNS.begin(
                mdnsHostname.c_str()
            )
        )
        {
            MDNS.addService(
                "http",
                "tcp",
                80
            );

            Serial.print(
                "mDNS: http://"
            );

            Serial.print(
                mdnsHostname.c_str()
            );

            Serial.println(
                ".local"
            );
        }
        else
        {
            Serial.println(
                "mDNS start failed."
            );
        }
    }

    // --------------------------------------------------------
    // HTTP server
    // --------------------------------------------------------

    setupRoutes();

    server.begin();

    Serial.println();
    Serial.println(
        "Web server started."
    );

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.print(
            "Network interface: http://"
        );

        Serial.println(
            WiFi.localIP()
        );
    }

    if (fallbackAPActive)
    {
        Serial.print(
            "Fallback interface: http://"
        );

        Serial.println(
            WiFi.softAPIP()
        );
    }

    Serial.print(
        "Active template: "
    );

    Serial.println(
        activeTemplate
    );

    Serial.print(
        "Current counter: "
    );

    Serial.println(
        ticketCounter
    );

    Serial.println();
    Serial.println(
        "READY"
    );
    Serial.println();
}



void loop()
{
    server.handleClient();

    handleButton();

    delay(1);
}