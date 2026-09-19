#pragma once

#include <Arduino.h>
#include <FS.h>

#include "storage.h"


String formatTicketNumber(uint32_t number)
{
    char buffer[16];

    snprintf(
        buffer,
        sizeof(buffer),
        "%06lu",
        static_cast<unsigned long>(number)
    );

    return String(buffer);
}

String htmlEscape(String value)
{
    value.replace("&", "&amp;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");
    value.replace("\"", "&quot;");
    value.replace("'", "&#39;");

    return value;
}

String jsonEscape(String value)
{
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\b", "\\b");
    value.replace("\f", "\\f");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    value.replace("\t", "\\t");

    return value;
}

String sanitizeFileName(String name)
{
    name.replace("\\", "_");
    name.replace("/", "_");
    name.replace("..", "_");
    name.replace(" ", "_");

    return name;
}

String sanitizeTemplateName(String name)
{
    name.trim();

    name.replace("\\", "_");
    name.replace("/", "_");
    name.replace("..", "_");
    name.replace(" ", "_");

    return name;
}

String sanitizeHostname(String hostname)
{
    hostname.trim();
    hostname.toLowerCase();

    String clean;

    for (
        size_t i = 0;
        i < hostname.length();
        i++
    )
    {
        char c = hostname[i];

        if (
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-'
        )
        {
            clean += c;
        }
    }

    while (clean.startsWith("-"))
    {
        clean.remove(0, 1);
    }

    while (clean.endsWith("-"))
    {
        clean.remove(clean.length() - 1);
    }

    if (clean.length() > 63)
    {
        clean.remove(63);
    }

    if (clean.length() == 0)
    {
        clean = DEFAULT_MDNS_HOSTNAME;
    }

    return clean;
}

String templatePath(const String& name)
{
    return "/templates/" + sanitizeTemplateName(name) + ".tpl";
}

String imagePath(const String& name)
{
    return "/images/" + sanitizeFileName(name);
}

String readFile(const String& path)
{
    File file = storageFS().open(path, FILE_READ);

    if (!file)
    {
        return "";
    }

    String content;

    while (file.available())
    {
        content += static_cast<char>(file.read());
    }

    file.close();

    return content;
}

bool writeFile(
    const String& path,
    const String& content
)
{
    if (storageFS().exists(path))
    {
        storageFS().remove(path);
    }

    File file = storageFS().open(path, FILE_WRITE);

    if (!file)
    {
        return false;
    }

    size_t written = file.print(content);

    file.close();

    return written == content.length();
}



