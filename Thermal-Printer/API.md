# Thermal-Printer REST-API

Alle API-Endpunkte liegen unter dem Web-UI des ESP32-S3 TQ-Printer.
Falls ein Admin-Passwort konfiguriert ist (`/admin.txt`), ist HTTP Basic Auth
mit Benutzername `admin` erforderlich.

## Templates

### Alle Templates auflisten

```text
GET /api/templates
```

**Antwort (200):**

```json
[
  {"name": "default", "active": true, "size": 256},
  {"name": "event",   "active": false, "size": 180}
]
```

### Einzelnes Template laden

```text
GET /api/template/load?name=<name>
```

**Antwort (200):**

```json
{"name": "default", "content": "TEXT|center|2|1|TICKET\n..."}
```

### Template speichern

```text
POST /api/template?action=save&name=<name>
Body: content=<template-inhalt>
```

**Antwort (200):**

```json
{"ok": true, "message": "Template saved."}
```

### Template aktivieren

```text
POST /api/template?action=activate&name=<name>
```

**Antwort (200):**

```json
{"ok": true, "message": "Template activated."}
```

### Template löschen

```text
POST /api/template?action=delete&name=<name>
```

**Antwort (200):**

```json
{"ok": true, "message": "Template deleted."}
```

## Drucken

### Nächstes Ticket drucken

```text
GET /api/print-next
```

### Bestimmte Ticketnummer drucken

```text
GET /api/print-specific?number=<nummer>
```

## Counter

### Counter abfragen

Der aktuelle Counter wird auf der Startseite angezeigt.
Ein eigener Endpunkt ist aktuell nicht implementiert.

### Counter setzen

```text
GET /api/counter/set?value=<nummer>
```

### Counter zurücksetzen

```text
GET /api/counter/reset
```

## WLAN

### WLAN-Scan

```text
GET /api/wifi/scan
```

**Antwort (200):**

```json
[
  {"ssid": "Netzwerk", "rssi": -42, "secure": true},
  ...
]
```

## Firmware-Update (OTA)

```text
GET /ota
```

Zeigt eine Web-Oberfläche zum Hochladen eines kompilierten `.bin`-Images an.
Alternativ kann die Datei auch per `POST /ota/upload` hochgeladen werden.

## Fehlerantworten

Fehler werden mit einem passenden HTTP-Statuscode und einer JSON-Antwort
zurückgegeben:

```json
{"error": "Beschreibung des Fehlers"}
```
