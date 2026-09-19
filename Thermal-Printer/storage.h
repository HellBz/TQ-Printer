#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <LittleFS.h>

#include "config.h"


constexpr int SD_CLK_PIN  = 39;
constexpr int SD_CMD_PIN  = 38;
constexpr int SD_DATA_PIN = 40;

bool sdCardReady = false;
String adminPassword;
bool adminProtectionEnabled = false;


fs::FS& storageFS()
{
    return sdCardReady
        ? static_cast<fs::FS&>(SD_MMC)
        : static_cast<fs::FS&>(LittleFS);
}

const char* storageName()
{
    return sdCardReady
        ? "SD"
        : "LittleFS";
}

void ensureStorageDirectories()
{
    fs::FS& fs = storageFS();

    if (!fs.exists("/templates")) fs.mkdir("/templates");
    if (!fs.exists("/images")) fs.mkdir("/images");
    if (!fs.exists("/captures")) fs.mkdir("/captures");
    if (!fs.exists("/data")) fs.mkdir("/data");
}


bool copyFileBetweenFS(
    fs::FS& sourceFS,
    fs::FS& targetFS,
    const String& sourcePath,
    const String& targetPath
)
{
    File source = sourceFS.open(sourcePath, FILE_READ);

    if (!source || source.isDirectory())
    {
        if (source) source.close();
        return false;
    }

    if (targetFS.exists(targetPath))
    {
        targetFS.remove(targetPath);
    }

    File target = targetFS.open(targetPath, FILE_WRITE);

    if (!target)
    {
        source.close();
        return false;
    }

    uint8_t buffer[1024];
    size_t copied = 0;

    while (source.available())
    {
        size_t readCount =
            source.read(
                buffer,
                sizeof(buffer)
            );

        if (readCount == 0)
        {
            break;
        }

        size_t writeCount =
            target.write(
                buffer,
                readCount
            );

        if (writeCount != readCount)
        {
            target.close();
            source.close();
            targetFS.remove(targetPath);
            return false;
        }

        copied += writeCount;
    }

    size_t sourceSize = source.size();

    target.flush();
    target.close();
    source.close();

    File verify =
        targetFS.open(
            targetPath,
            FILE_READ
        );

    if (!verify)
    {
        return false;
    }

    size_t targetSize = verify.size();
    verify.close();

    return copied == sourceSize &&
           targetSize == sourceSize;
}

bool copyDirectoryBetweenFS(
    fs::FS& sourceFS,
    fs::FS& targetFS,
    const String& directory
)
{
    if (!sourceFS.exists(directory))
    {
        return true;
    }

    if (!targetFS.exists(directory))
    {
        if (!targetFS.mkdir(directory))
        {
            return false;
        }
    }

    File root =
        sourceFS.open(
            directory,
            FILE_READ
        );

    if (!root || !root.isDirectory())
    {
        if (root) root.close();
        return false;
    }

    bool success = true;
    File entry = root.openNextFile();

    while (entry)
    {
        String name = entry.name();

        if (!name.startsWith("/"))
        {
            name =
                directory +
                "/" +
                name;
        }

        bool isDirectory =
            entry.isDirectory();

        entry.close();

        if (isDirectory)
        {
            if (!copyDirectoryBetweenFS(
                    sourceFS,
                    targetFS,
                    name
                ))
            {
                success = false;
            }
        }
        else
        {
            if (!copyFileBetweenFS(
                    sourceFS,
                    targetFS,
                    name,
                    name
                ))
            {
                success = false;
            }
        }

        entry = root.openNextFile();
    }

    root.close();
    return success;
}

bool migrateLittleFSToSD()
{
    if (!sdCardReady)
    {
        return false;
    }

    if (SD_MMC.exists("/.ticketprinter"))
    {
        return true;
    }

    Serial.println(
        "New TicketPrinter SD detected."
    );

    Serial.println(
        "Migrating LittleFS data to SD..."
    );

    const char* directories[] =
    {
        "/templates",
        "/images",
        "/captures",
        "/data"
    };

    bool success = true;

    for (const char* directory : directories)
    {
        if (!copyDirectoryBetweenFS(
                LittleFS,
                SD_MMC,
                directory
            ))
        {
            success = false;
        }
    }

    if (LittleFS.exists("/admin.txt"))
    {
        if (!copyFileBetweenFS(
                LittleFS,
                SD_MMC,
                "/admin.txt",
                "/admin.txt"
            ))
        {
            success = false;
        }
    }

    if (!success)
    {
        Serial.println(
            "Migration failed. Marker not created."
        );

        return false;
    }

    File marker =
        SD_MMC.open(
            "/.ticketprinter",
            FILE_WRITE
        );

    if (!marker)
    {
        Serial.println(
            "Migration verified but marker creation failed."
        );

        return false;
    }

    marker.println(
        "TQ-Printer"
    );

    marker.println(
        "Storage initialized"
    );

    marker.close();

    Serial.println(
        "LittleFS to SD migration completed."
    );

    return true;
}

void getStorageStats(
    uint64_t& total,
    uint64_t& used,
    uint64_t& freeBytes
)
{
    if (sdCardReady)
    {
        total = SD_MMC.totalBytes();
        used = SD_MMC.usedBytes();
    }
    else
    {
        total = LittleFS.totalBytes();
        used = LittleFS.usedBytes();
    }

    freeBytes =
        total > used
            ? total - used
            : 0;
}

String formatStorageBytes(
    uint64_t bytes
)
{
    char buffer[32];

    if (bytes >= 1073741824ULL)
    {
        snprintf(
            buffer,
            sizeof(buffer),
            "%.2f GB",
            static_cast<double>(bytes) /
                1073741824.0
        );
    }
    else if (bytes >= 1048576ULL)
    {
        snprintf(
            buffer,
            sizeof(buffer),
            "%.2f MB",
            static_cast<double>(bytes) /
                1048576.0
        );
    }
    else if (bytes >= 1024ULL)
    {
        snprintf(
            buffer,
            sizeof(buffer),
            "%.2f KB",
            static_cast<double>(bytes) /
                1024.0
        );
    }
    else
    {
        snprintf(
            buffer,
            sizeof(buffer),
            "%llu B",
            static_cast<unsigned long long>(bytes)
        );
    }

    return String(buffer);
}

String storageSummaryHTML(
    bool compact
)
{
    uint64_t total = 0;
    uint64_t used = 0;
    uint64_t freeBytes = 0;

    getStorageStats(
        total,
        used,
        freeBytes
    );

    uint8_t usedPercent =
        total > 0
            ? static_cast<uint8_t>(
                (used * 100ULL) / total
              )
            : 0;

    String html;

    if (compact)
    {
        html +=
            "<div class='card'><strong>Storage:</strong> ";

        html += storageName();

        html +=
            " &middot; <strong>";

        html +=
            formatStorageBytes(
                freeBytes
            );

        html +=
            " free</strong></div>";

        return html;
    }

    html +=
        "<div class='card'><h2>Storage</h2>"
        "<p><strong>Active: ";

    html += storageName();

    html +=
        "</strong></p><p>Total: ";

    html +=
        formatStorageBytes(
            total
        );

    html +=
        " &middot; Used: ";

    html +=
        formatStorageBytes(
            used
        );

    html +=
        " &middot; <strong>Free: ";

    html +=
        formatStorageBytes(
            freeBytes
        );

    html +=
        "</strong></p>"
        "<div style='height:12px;background:#1f2937;border-radius:999px;overflow:hidden'>"
        "<div style='height:100%;background:#22c55e;width:";

    html += String(usedPercent);

    html +=
        "%'></div></div><p class='small'>";

    html += String(usedPercent);

    html +=
        "% used</p></div>";

    return html;
}



