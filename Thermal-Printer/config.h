#pragma once

#include <Arduino.h>


constexpr char FIRMWARE_VERSION[] = "2.9.17";



// Pinout-Referenz: ESP32-S3_Ticket_Printer_Pinout.txt
// Belegt/reserviert: GPIO17/18 (Drucker-UART), GPIO14 (Trigger),
// GPIO38/39/40 (SD_MMC 1-bit), GPIO35/36/37 (PSRAM).
// USB: GPIO19/20, BOOT: GPIO0, Debug: GPIO43/44 vermeiden.
constexpr uint8_t PRINTER_TX_PIN = 17;
constexpr uint8_t PRINTER_RX_PIN = 18;
constexpr uint8_t DEFAULT_TRIGGER_GPIO = 14;
constexpr uint32_t DEFAULT_TRIGGER_DEBOUNCE_MS = 50;

constexpr uint32_t DEFAULT_PRINTER_BAUD = 1500000;
uint32_t printerBaud = DEFAULT_PRINTER_BAUD;
constexpr uint32_t DEBUG_BAUD = 115200;

constexpr uint16_t DEFAULT_PAGE_WIDTH = 576;
uint16_t pageWidth = DEFAULT_PAGE_WIDTH;



constexpr char DEFAULT_AP_SSID[] = "TicketPrinter";
constexpr char DEFAULT_AP_PASSWORD[] = "12345678";
constexpr char DEFAULT_MDNS_HOSTNAME[] = "ticketprinter";

constexpr uint32_t WIFI_TIMEOUT_MS = 15000;


