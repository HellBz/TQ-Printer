#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "globals.h"
#include "storage.h"
#include "helpers.h"


bool loadAdminPassword()
{
    adminPassword = "";
    adminProtectionEnabled = false;

    if (!storageFS().exists("/admin.txt"))
    {
        return false;
    }

    File file = storageFS().open("/admin.txt", FILE_READ);

    if (!file)
    {
        return false;
    }

    adminPassword = file.readStringUntil('\n');
    file.close();

    adminPassword.trim();

    if (adminPassword.length() == 0)
    {
        return false;
    }

    adminProtectionEnabled = true;
    return true;
}

bool requireAdmin()
{
    if (!adminProtectionEnabled)
    {
        return true;
    }

    if (server.authenticate("admin", adminPassword.c_str()))
    {
        return true;
    }

    server.requestAuthentication(
        BASIC_AUTH,
        "TQ-Printer",
        "Administrator password required"
    );

    return false;
}


