# Thermo-Printer – Projektnotizen

## Hardware
- Mikrocontroller: ESP32-S3 (Board-Variante N16R8: 16 MB Flash, 8 MB PSRAM)
- Thermodrucker: MY-Q805K (ESC/POS-kompatibel, native Page-Mode-Befehle)
- UART: TX = GPIO 17, RX = GPIO 18 (1500000 Baud Standard)
- GPIO-Trigger: GPIO 14 (Standard, aktiv Low, entprellt)
- SD-Karte: 1-Bit SD_MMC – CLK=GPIO39, CMD=GPIO38, DATA=GPIO40
- PSRAM: GPIO35, GPIO36, GPIO37 (reserviert)
- USB: GPIO19 (D+), GPIO20 (D-) – vermeiden, wenn nativer USB benötigt wird
- BOOT: GPIO0 – vermeiden
- Debug/UART0: GPIO43 (TX), GPIO44 (RX)
- Onboard-LED: GPIO2, WS2812: GPIO48
- Kamera-Pins (falls später verwendet): GPIO4–GPIO18
- Detaillierte Pinout-Referenz: <ref_file file="D:\DEV\Thermo-Printer\ESP32-S3_Ticket_Printer_Pinout.txt" />

## Firmware
- Hauptdatei (Einstieg): `Thermal-Printer\Thermal-Printer.ino`
- Framework: Arduino-ESP32
- Modularisierte Header liegen direkt neben der `.ino`-Datei im Ordner `Thermal-Printer`:
  - `config.h` – Firmware-Version, Pins, Default-Werte
  - `globals.h` – Globale Objekte (`Printer`, `server`, `preferences`) und Laufzeitvariablen
  - `storage.h` – SD/LittleFS-Backend, Kopieren, Migration, Speicherstatistik
  - `helpers.h` – Hilfsfunktionen (Escape, Sanitize, Datei-IO)
  - `auth.h` – Admin-Passwort laden und Basic-Auth-Prüfung
  - `printer.h` – ESC/POS-Befehle, QR-Code, BMP-Bild, Template-Engine, Ticket-Druck
  - `settings_network.h` – Einstellungen laden und Wi-Fi-Verbindung
  - `webui.h` – HTML-Templates, HTTP-Handler, Routen
- Benötigte Bibliotheken (über Board-Manager / Arduino Library Manager):
  - `WiFi` (Core)
  - `WebServer` (Core)
  - `Preferences` (Core)
  - `SD_MMC` (Core)
  - `LittleFS` (Core)
  - `ESPmDNS` (Core)

## Build & Upload
- Im Arduino IDE: Board `ESP32S3 Dev Module` (oder passendes S3-Board), Partition Scheme mit SPIFFS/LittleFS (z. B. "Default 4MB with spiffs").
- Alternativ PlatformIO: `platform = espressif32`, `board = esp32-s3-devkitc-1`, `framework = arduino`.
- Upload über USB/UART oder OTA (`/ota` im Web-UI, Arduino-Binary `.bin` hochladen).
- Automatisch bauen + flashen:
  - `flash.py` (Python 3) erkennt das lokale `arduino-cli.exe`, installiert den ESP32-Core bei Bedarf und lädt hoch.
  - `flash.bat` (Windows) ruft `flash.py` auf.
  - Beispiel: `python flash.py --port COM3`

## Speicher
- Bevorzugt SD_MMC; wenn keine SD-Karte erkannt wird, fallback auf LittleFS.
- Verzeichnisse: `/templates`, `/images`, `/captures`, `/data`.
- Administrator-Passwort: `/admin.txt` (erste Zeile, falls vorhanden).

## Web-Oberfläche
- Startseite: `http://ticketprinter.local` (mDNS) oder AP-IP.
- Standard-AP: SSID `TicketPrinter`, Passwort `12345678`.
- Endpunkte (Auswahl):
  - `GET  /`
  - `GET  /templates`
  - `GET  /files`
  - `GET  /settings`
  - `GET  /api/print-next`
  - `GET  /api/print-specific?number=...`
  - `GET  /api/counter/set?value=...`
  - `GET  /api/counter/reset`
  - `GET  /api/wifi/scan`
  - `POST /template/save`
  - `POST /template/activate`
  - `GET  /template/delete?name=...`
  - `GET  /api/templates` (JSON-Liste)
  - `GET  /api/template/load?name=...` (JSON mit Inhalt)
  - `POST /api/template?action=save&name=...&content=...`
  - `POST /api/template?action=activate&name=...`
  - `POST /api/template?action=delete&name=...`
  - `GET  /ota` (OTA-Update Seite)
  - `POST /ota/upload` (Firmware `.bin` hochladen)
  - `POST /settings/wifi`
  - `POST /settings/ap`
  - `POST /settings/printer`
  - `POST /settings/trigger`
  - `POST /settings/mdns`
  - `POST /upload`
  - `GET  /file/view?name=...`
  - `GET  /file/delete?name=...`

## Template-Syntax
```
TEXT|center|2|1|Hallo Welt
QR|5|TICKET:{{TICKET_ID}}
IMAGE|logo.bmp
FEED|2
CUT
```
Verfügbare Variablen: `{{TICKET_NUMBER}}`, `{{TICKET_ID}}`, `{{DATE}}`, `{{TIME}}`.

## REST-API

Die wichtigsten Endpunkte für externe Steuerung und Automatisierung:

| Methode | URL | Parameter | Beschreibung |
|---|---|---|---|
| `GET` | `/api/templates` | – | Liste aller Templates als JSON |
| `GET` | `/api/template/load` | `name` | Inhalt eines Templates als JSON |
| `POST` | `/api/template` | `action=save&name=...` + Body `content=...` | Template speichern |
| `POST` | `/api/template` | `action=activate&name=...` | Template aktivieren |
| `POST` | `/api/template` | `action=delete&name=...` | Template löschen |
| `GET` | `/api/print-next` | – | Nächstes Ticket drucken |
| `GET` | `/api/print-specific` | `number=...` | Bestimmte Ticketnummer drucken |
| `GET` | `/api/counter/set` | `value=...` | Zähler setzen |
| `GET` | `/api/counter/reset` | – | Zähler auf 0 setzen |
| `GET` | `/api/wifi/scan` | – | WLAN-Netze als JSON |

Authentifizierung erfolgt per HTTP Basic Auth (`admin` / Inhalt von `/admin.txt`), falls vorhanden.

## Versionskontrolle & Releases
- Lokales Git-Repository wurde initialisiert (siehe `git log`).
- `.gitignore` ignoriert Build-Output, `arduino-cli.exe`, Backups und IDE/OS-Dateien.
- GitHub Actions Workflow liegt unter `.github/workflows/release.yml`:
  - Jeder Push auf `main`/`master` kompiliert die Firmware.
  - Jeder Tag `v*` erzeugt automatisch ein GitHub-Release und hängt `Thermal-Printer.ino.bin` an.
- README.md enthält die Anleitung für Repository, Build und Release.

## Hinweise für Änderungen
- HTML-Ausgaben werden in `String` zusammengebaut; bei größeren Seiten kann Heap-Fragmentierung entstehen.
- Admin-Authentifizierung erfolgt per HTTP Basic Auth (`admin` / Inhalt von `/admin.txt`).
- Zustandsändernde Endpunkte für Templates (`/api/template?action=delete/activate/save`) sind als `POST` implementiert; `/template/delete`, `/file/delete` und `/api/counter/*` sind aus Kompatibilitätsgründen noch `GET`.
