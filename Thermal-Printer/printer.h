#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include "globals.h"
#include "helpers.h"
#include "storage.h"


void printerInitialize()
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x40
    };

    Printer.write(cmd, sizeof(cmd));
    Printer.flush();

    delay(100);
}

void printerAlign(uint8_t alignment)
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x61,
        alignment
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerBold(bool enabled)
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x45,
        static_cast<uint8_t>(
            enabled ? 1 : 0
        )
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerSize(
    uint8_t width,
    uint8_t height
)
{
    width = constrain(width, 1, 8);
    height = constrain(height, 1, 8);

    uint8_t value =
        ((width - 1) << 4) |
        (height - 1);

    const uint8_t cmd[] =
    {
        0x1D,
        0x21,
        value
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerSelectStandardMode()
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x53
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerSelectPageMode()
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x4C
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerSetPageDirection(uint8_t direction)
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x54,
        static_cast<uint8_t>(direction & 0x03)
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerSetPageArea(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height
)
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x57,
        static_cast<uint8_t>(x & 0xFF),
        static_cast<uint8_t>((x >> 8) & 0xFF),
        static_cast<uint8_t>(y & 0xFF),
        static_cast<uint8_t>((y >> 8) & 0xFF),
        static_cast<uint8_t>(width & 0xFF),
        static_cast<uint8_t>((width >> 8) & 0xFF),
        static_cast<uint8_t>(height & 0xFF),
        static_cast<uint8_t>((height >> 8) & 0xFF)
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerPrintPageAndReturn()
{
    Printer.write(
        static_cast<uint8_t>(0x0C)
    );
}

void printerPrintPageKeepMode()
{
    const uint8_t cmd[] =
    {
        0x1B,
        0x0C
    };

    Printer.write(cmd, sizeof(cmd));
}

void printerBeginDirectionTest(uint8_t direction)
{
    printerInitialize();
    printerSelectPageMode();
    printerSetPageDirection(direction);

    /*
     * This area is intentionally generous for compatibility testing.
     * Width and height are page-mode logical dimensions, not a verified
     * physical printable-width specification for this printer.
     */
    printerSetPageArea(
        0,
        0,
        576,
        1200
    );
}

void printerEndDirectionTest()
{
    printerPrintPageAndReturn();
    printerSelectStandardMode();
}

void printerFeed(uint8_t lines)
{
    for (uint8_t i = 0; i < lines; i++)
    {
        Printer.write('\n');
    }

    Printer.flush();
}

void printerCut()
{
    printerFeed(feedLines);

    delay(150);

    const uint8_t cmd[] =
    {
        0x1D,
        0x56,
        0x42,
        0x00
    };

    Printer.write(cmd, sizeof(cmd));
    Printer.flush();

    delay(500);
}



void printerQRCode(
    const String& data,
    uint8_t moduleSize
)
{
    moduleSize = constrain(
        moduleSize,
        1,
        16
    );

    const uint8_t model[] =
    {
        0x1D, 0x28, 0x6B,
        0x04, 0x00,
        0x31, 0x41,
        0x32, 0x00
    };

    Printer.write(
        model,
        sizeof(model)
    );

    uint8_t sizeCommand[] =
    {
        0x1D, 0x28, 0x6B,
        0x03, 0x00,
        0x31, 0x43,
        moduleSize
    };

    Printer.write(
        sizeCommand,
        sizeof(sizeCommand)
    );

    const uint8_t ecc[] =
    {
        0x1D, 0x28, 0x6B,
        0x03, 0x00,
        0x31, 0x45,
        0x31
    };

    Printer.write(
        ecc,
        sizeof(ecc)
    );

    uint16_t dataLength =
        data.length();

    uint16_t storeLength =
        dataLength + 3;

    Printer.write(0x1D);
    Printer.write(0x28);
    Printer.write(0x6B);

    Printer.write(
        storeLength & 0xFF
    );

    Printer.write(
        (storeLength >> 8) & 0xFF
    );

    Printer.write(0x31);
    Printer.write(0x50);
    Printer.write(0x30);

    Printer.write(
        reinterpret_cast<const uint8_t*>(
            data.c_str()
        ),
        dataLength
    );

    Printer.flush();

    delay(50);

    const uint8_t printCommand[] =
    {
        0x1D, 0x28, 0x6B,
        0x03, 0x00,
        0x31, 0x51,
        0x30
    };

    Printer.write(
        printCommand,
        sizeof(printCommand)
    );

    Printer.flush();

    delay(400);
}



uint16_t read16(File& file)
{
    uint16_t value;

    value = file.read();
    value |=
        static_cast<uint16_t>(
            file.read()
        ) << 8;

    return value;
}

uint32_t read32(File& file)
{
    uint32_t value;

    value = file.read();
    value |=
        static_cast<uint32_t>(
            file.read()
        ) << 8;

    value |=
        static_cast<uint32_t>(
            file.read()
        ) << 16;

    value |=
        static_cast<uint32_t>(
            file.read()
        ) << 24;

    return value;
}

bool printerBMP(const String& filename)
{
    String path = filename;

    if (!path.startsWith("/"))
    {
        path = "/images/" + path;
    }

    File bmp = storageFS().open(path, FILE_READ);

    if (!bmp)
    {
        Serial.print("Image not found: ");
        Serial.println(path);

        return false;
    }

    if (
        read16(bmp) != 0x4D42
    )
    {
        Serial.println("Invalid BMP.");

        bmp.close();

        return false;
    }

    read32(bmp);
    read32(bmp);

    uint32_t pixelOffset =
        read32(bmp);

    uint32_t headerSize =
        read32(bmp);

    if (headerSize < 40)
    {
        Serial.println(
            "Unsupported BMP header."
        );

        bmp.close();

        return false;
    }

    int32_t width =
        static_cast<int32_t>(
            read32(bmp)
        );

    int32_t height =
        static_cast<int32_t>(
            read32(bmp)
        );

    uint16_t planes =
        read16(bmp);

    uint16_t depth =
        read16(bmp);

    uint32_t compression =
        read32(bmp);

    if (
        planes != 1 ||
        depth != 24 ||
        compression != 0 ||
        width <= 0 ||
        height == 0
    )
    {
        Serial.println(
            "BMP must be uncompressed 24-bit."
        );

        bmp.close();

        return false;
    }

    bool bottomUp =
        height > 0;

    if (height < 0)
    {
        height = -height;
    }

    if (width > 576)
    {
        Serial.println(
            "BMP is wider than 576 pixels."
        );

        bmp.close();

        return false;
    }

    uint32_t rowSize =
        (width * 3 + 3) & ~3;

    uint16_t bytesPerRow =
        (width + 7) / 8;

    uint8_t* raster =
        static_cast<uint8_t*>(
            malloc(bytesPerRow)
        );

    if (!raster)
    {
        bmp.close();
        return false;
    }

    printerAlign(1);

    for (
        int32_t outputRow = 0;
        outputRow < height;
        outputRow++
    )
    {
        memset(
            raster,
            0,
            bytesPerRow
        );

        int32_t sourceRow;

        if (bottomUp)
        {
            sourceRow =
                height -
                1 -
                outputRow;
        }
        else
        {
            sourceRow =
                outputRow;
        }

        bmp.seek(
            pixelOffset +
            sourceRow * rowSize
        );

        for (
            int32_t x = 0;
            x < width;
            x++
        )
        {
            uint8_t blue =
                bmp.read();

            uint8_t green =
                bmp.read();

            uint8_t red =
                bmp.read();

            uint16_t luminance =
                (
                    static_cast<uint16_t>(red) * 30 +
                    static_cast<uint16_t>(green) * 59 +
                    static_cast<uint16_t>(blue) * 11
                ) / 100;

            if (luminance < 128)
            {
                raster[x / 8] |=
                    0x80 >>
                    (x & 7);
            }
        }

        Printer.write(0x1D);
        Printer.write(0x76);
        Printer.write(0x30);
        Printer.write(0x00);

        Printer.write(
            bytesPerRow & 0xFF
        );

        Printer.write(
            (bytesPerRow >> 8) & 0xFF
        );

        Printer.write(0x01);
        Printer.write(0x00);

        Printer.write(
            raster,
            bytesPerRow
        );
    }

    Printer.flush();

    free(raster);

    bmp.close();

    return true;
}



String replaceTemplateVariables(
    String value,
    uint32_t ticketNumber,
    const String& date,
    const String& time
)
{
    String number =
        formatTicketNumber(
            ticketNumber
        );

    String compactDate =
        date;

    compactDate.replace(
        "-",
        ""
    );

    String ticketID;

    if (compactDate.length() > 0)
    {
        ticketID =
            compactDate +
            "-" +
            number;
    }
    else
    {
        ticketID =
            "TICKET-" +
            number;
    }

    value.replace(
        "{{TICKET_NUMBER}}",
        number
    );

    value.replace(
        "{{TICKET_ID}}",
        ticketID
    );

    value.replace(
        "{{DATE}}",
        date
    );

    value.replace(
        "{{TIME}}",
        time
    );

    return value;
}

bool getTicketDateTime(
    String& date,
    String& time
)
{
    struct tm info;

    if (!getLocalTime(&info, 1000))
    {
        date = "";
        time = "";
        return false;
    }

    char dateBuffer[11];
    char timeBuffer[9];

    strftime(
        dateBuffer,
        sizeof(dateBuffer),
        "%d.%m.%Y",
        &info
    );

    strftime(
        timeBuffer,
        sizeof(timeBuffer),
        "%H:%M:%S",
        &info
    );

    date = dateBuffer;
    time = timeBuffer;

    return true;
}


bool getBMPDimensions(
    const String& filename,
    uint16_t& width,
    uint16_t& height
)
{
    width = 0;
    height = 0;

    String path = filename;

    if (!path.startsWith("/"))
    {
        path = "/images/" + path;
    }

    File bmp = storageFS().open(path, FILE_READ);

    if (!bmp)
    {
        return false;
    }

    if (read16(bmp) != 0x4D42)
    {
        bmp.close();
        return false;
    }

    read32(bmp);
    read32(bmp);
    read32(bmp);

    uint32_t headerSize = read32(bmp);

    if (headerSize < 40)
    {
        bmp.close();
        return false;
    }

    int32_t bmpWidth =
        static_cast<int32_t>(
            read32(bmp)
        );

    int32_t bmpHeight =
        static_cast<int32_t>(
            read32(bmp)
        );

    bmp.close();

    if (
        bmpWidth <= 0 ||
        bmpHeight == 0
    )
    {
        return false;
    }

    if (bmpHeight < 0)
    {
        bmpHeight = -bmpHeight;
    }

    width =
        static_cast<uint16_t>(
            min<int32_t>(
                bmpWidth,
                65535
            )
        );

    height =
        static_cast<uint16_t>(
            min<int32_t>(
                bmpHeight,
                65535
            )
        );

    return true;
}



constexpr uint16_t MYQ_PAGE_MAX_HEIGHT = 1200;

void myqWrite16(uint16_t value)
{
    Printer.write(value & 0xFF);
    Printer.write((value >> 8) & 0xFF);
}

void myqPageBegin(uint16_t height)
{
    height = constrain(
        height,
        1,
        MYQ_PAGE_MAX_HEIGHT
    );

    Printer.write(0x1A);
    Printer.write(0x5B);
    Printer.write(0x01);

    myqWrite16(0);
    myqWrite16(0);
    myqWrite16(pageWidth);
    myqWrite16(height);

    Printer.write(0x00);
}

void myqPageEnd()
{
    Printer.write(0x1A);
    Printer.write(0x5D);
    Printer.write(0x00);
}

void myqPagePrint()
{
    Printer.write(0x1A);
    Printer.write(0x4F);
    Printer.write(0x00);
}

uint16_t myqTextHeight(uint8_t size)
{
    if (size <= 1) return 24;
    if (size == 2) return 48;
    if (size == 3) return 64;
    return 80;
}

uint16_t myqEstimateTextWidth(
    const String& text,
    uint16_t fontHeight
)
{
    uint16_t charWidth =
        max<uint16_t>(
            8,
            fontHeight / 2
        );

    uint32_t width =
        static_cast<uint32_t>(
            text.length()
        ) *
        charWidth;

    return min<uint32_t>(
        width,
        pageWidth
    );
}

void myqPageText180(
    uint16_t x,
    uint16_t y,
    uint16_t fontHeight,
    bool bold,
    const String& text
)
{
    Printer.write(0x1A);
    Printer.write(0x54);
    Printer.write(0x01);

    myqWrite16(x);
    myqWrite16(y);
    myqWrite16(fontHeight);

    uint16_t fontType = 0x0020;

    if (bold)
    {
        fontType |= 0x0001;
    }

    myqWrite16(fontType);

    Printer.write(
        reinterpret_cast<const uint8_t*>(
            text.c_str()
        ),
        text.length()
    );

    Printer.write(0x00);
}

uint8_t myqQRMinimumVersion(size_t byteLength)
{
    static const uint16_t capacityM[20] =
    {
        14, 26, 42, 62, 84,
        106, 122, 152, 180, 213,
        251, 287, 331, 362, 412,
        450, 504, 560, 624, 666
    };

    for (uint8_t version = 1; version <= 20; version++)
    {
        if (byteLength <= capacityM[version - 1])
        {
            return version;
        }
    }

    return 20;
}

void myqResolveQRLayout(
    uint8_t requestedSize,
    const String& data,
    uint8_t& version,
    uint8_t& unitWidth,
    uint16_t& pixelSize
)
{
    requestedSize =
        constrain(
            requestedSize,
            1,
            16
        );

    uint8_t minimumVersion =
        myqQRMinimumVersion(
            data.length()
        );

    uint16_t minimumModules =
        17 +
        4 * minimumVersion;

    uint16_t targetPixels =
        minimumModules *
        requestedSize;

    unitWidth =
        min<uint8_t>(
            requestedSize,
            4
        );

    version = minimumVersion;

    if (requestedSize > 4)
    {
        uint16_t bestDifference = 0xFFFF;

        for (
            uint8_t candidate = minimumVersion;
            candidate <= 20;
            candidate++
        )
        {
            uint16_t candidatePixels =
                (
                    17 +
                    4 * candidate
                ) *
                unitWidth;

            uint16_t difference =
                candidatePixels > targetPixels
                    ? candidatePixels - targetPixels
                    : targetPixels - candidatePixels;

            if (difference < bestDifference)
            {
                bestDifference = difference;
                version = candidate;
            }
        }
    }

    pixelSize =
        (
            17 +
            4 * version
        ) *
        unitWidth;
}

void myqPageQR180(
    uint16_t x,
    uint16_t y,
    uint8_t version,
    uint8_t unitWidth,
    const String& data
)
{
    version =
        constrain(
            version,
            1,
            20
        );

    unitWidth =
        constrain(
            unitWidth,
            1,
            4
        );

    Printer.write(0x1A);
    Printer.write(0x31);
    Printer.write(0x00);

    Printer.write(version);
    Printer.write(0x02);

    myqWrite16(x);
    myqWrite16(y);

    Printer.write(unitWidth);
    Printer.write(0x02);

    Printer.write(
        reinterpret_cast<const uint8_t*>(
            data.c_str()
        ),
        data.length()
    );

    Printer.write(0x00);
}

bool myqLoadBMPMono(
    const String& filename,
    uint8_t*& bitmap,
    uint16_t& width,
    uint16_t& height
)
{
    bitmap = nullptr;
    width = 0;
    height = 0;

    String path = filename;

    if (!path.startsWith("/"))
    {
        path = "/images/" + path;
    }

    File bmp =
        storageFS().open(
            path,
            FILE_READ
        );

    if (!bmp)
    {
        return false;
    }

    if (read16(bmp) != 0x4D42)
    {
        bmp.close();
        return false;
    }

    read32(bmp);
    read32(bmp);

    uint32_t pixelOffset =
        read32(bmp);

    uint32_t headerSize =
        read32(bmp);

    if (headerSize < 40)
    {
        bmp.close();
        return false;
    }

    int32_t bmpWidth =
        static_cast<int32_t>(
            read32(bmp)
        );

    int32_t bmpHeight =
        static_cast<int32_t>(
            read32(bmp)
        );

    uint16_t planes = read16(bmp);
    uint16_t depth = read16(bmp);
    uint32_t compression = read32(bmp);

    if (
        planes != 1 ||
        depth != 24 ||
        compression != 0 ||
        bmpWidth <= 0 ||
        bmpHeight == 0
    )
    {
        bmp.close();
        return false;
    }

    bool bottomUp =
        bmpHeight > 0;

    if (bmpHeight < 0)
    {
        bmpHeight = -bmpHeight;
    }

    width =
        min<int32_t>(
            bmpWidth,
            pageWidth
        );

    height =
        min<int32_t>(
            bmpHeight,
            MYQ_PAGE_MAX_HEIGHT
        );

    uint16_t bytesPerRow =
        (width + 7) / 8;

    size_t dataSize =
        static_cast<size_t>(
            bytesPerRow
        ) *
        height;

    bitmap =
        static_cast<uint8_t*>(
            heap_caps_calloc(
                dataSize,
                1,
                MALLOC_CAP_SPIRAM |
                MALLOC_CAP_8BIT
            )
        );

    if (!bitmap)
    {
        bitmap =
            static_cast<uint8_t*>(
                calloc(
                    dataSize,
                    1
                )
            );
    }

    if (!bitmap)
    {
        bmp.close();
        return false;
    }

    uint32_t sourceRowSize =
        (bmpWidth * 3 + 3) & ~3;

    for (
        uint16_t outputY = 0;
        outputY < height;
        outputY++
    )
    {
        int32_t sourceY =
            bottomUp
                ? bmpHeight - 1 - outputY
                : outputY;

        bmp.seek(
            pixelOffset +
            sourceY * sourceRowSize
        );

        for (
            uint16_t x = 0;
            x < width;
            x++
        )
        {
            int blue = bmp.read();
            int green = bmp.read();
            int red = bmp.read();

            if (
                blue < 0 ||
                green < 0 ||
                red < 0
            )
            {
                free(bitmap);
                bitmap = nullptr;
                bmp.close();
                return false;
            }

            uint16_t luminance =
                (
                    static_cast<uint16_t>(red) * 30 +
                    static_cast<uint16_t>(green) * 59 +
                    static_cast<uint16_t>(blue) * 11
                ) / 100;

            if (luminance < 128)
            {
                bitmap[
                    static_cast<size_t>(
                        outputY
                    ) *
                    bytesPerRow +
                    (x >> 3)
                ] |=
                    0x80 >>
                    (x & 7);
            }
        }
    }

    bmp.close();
    return true;
}

void myqPageBitmap180(
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint8_t* data
)
{
    Printer.write(0x1A);
    Printer.write(0x21);
    Printer.write(0x01);

    myqWrite16(x);
    myqWrite16(y);
    myqWrite16(width);
    myqWrite16(height);

    myqWrite16(0x0004);

    size_t dataSize =
        static_cast<size_t>(
            (width + 7) / 8
        ) *
        height;

    Printer.write(
        data,
        dataSize
    );
}

uint16_t myqEstimateReverseHeight(
    const String& templateContent,
    uint32_t ticketNumber,
    const String& date,
    const String& time
)
{
    uint16_t height = 16;
    int start = 0;

    while (start < templateContent.length())
    {
        int end =
            templateContent.indexOf(
                '\n',
                start
            );

        if (end < 0)
        {
            end =
                templateContent.length();
        }

        String line =
            templateContent.substring(
                start,
                end
            );

        start = end + 1;
        line.trim();

        if (
            line.length() == 0 ||
            line.startsWith("#")
        )
        {
            continue;
        }

        if (line.startsWith("TEXT|"))
        {
            int p1 = line.indexOf('|', 5);
            int p2 = line.indexOf('|', p1 + 1);

            if (p1 >= 0 && p2 >= 0)
            {
                uint8_t size =
                    line.substring(
                        p1 + 1,
                        p2
                    ).toInt();

                height +=
                    myqTextHeight(size) +
                    8;
            }
        }
        else if (line.startsWith("QR|"))
        {
            int separator =
                line.indexOf('|', 3);

            if (separator < 0)
            {
                continue;
            }

            uint8_t requestedSize =
                constrain(
                    line.substring(
                        3,
                        separator
                    ).toInt(),
                    1,
                    16
                );

            String data =
                replaceTemplateVariables(
                    line.substring(
                        separator + 1
                    ),
                    ticketNumber,
                    date,
                    time
                );

            uint8_t qrVersion = 1;
            uint8_t qrUnitWidth = 1;
            uint16_t qrPixelSize = 21;

            myqResolveQRLayout(
                requestedSize,
                data,
                qrVersion,
                qrUnitWidth,
                qrPixelSize
            );

            height +=
                qrPixelSize +
                16;
        }
        else if (line.startsWith("IMAGE|"))
        {
            String filename =
                line.substring(6);

            filename.trim();

            uint16_t imageWidth = 0;
            uint16_t imageHeight = 0;

            if (
                getBMPDimensions(
                    filename,
                    imageWidth,
                    imageHeight
                )
            )
            {
                height +=
                    min<uint16_t>(
                        imageHeight,
                        400
                    ) +
                    8;
            }
        }
        else if (line.startsWith("FEED|"))
        {
            uint8_t lines =
                line.substring(5).toInt();

            height +=
                lines * 24;
        }

        if (height >= MYQ_PAGE_MAX_HEIGHT)
        {
            return MYQ_PAGE_MAX_HEIGHT;
        }
    }

    return constrain(
        height,
        64,
        MYQ_PAGE_MAX_HEIGHT
    );
}

void executeTemplateReverseMYQ(
    const String& templateContent,
    uint32_t ticketNumber,
    const String& date,
    const String& time
)
{
    printerInitialize();

    uint16_t pageHeight =
        myqEstimateReverseHeight(
            templateContent,
            ticketNumber,
            date,
            time
        );

    myqPageBegin(pageHeight);

    uint16_t logicalY = 8;
    bool cutRequested = false;
    int start = 0;

    while (start < templateContent.length())
    {
        int end =
            templateContent.indexOf(
                '\n',
                start
            );

        if (end < 0)
        {
            end =
                templateContent.length();
        }

        String line =
            templateContent.substring(
                start,
                end
            );

        start = end + 1;
        line.trim();

        if (
            line.length() == 0 ||
            line.startsWith("#")
        )
        {
            continue;
        }

        if (line.startsWith("TEXT|"))
        {
            int p1 = line.indexOf('|', 5);
            int p2 = line.indexOf('|', p1 + 1);
            int p3 = line.indexOf('|', p2 + 1);

            if (
                p1 < 0 ||
                p2 < 0 ||
                p3 < 0
            )
            {
                continue;
            }

            String alignment =
                line.substring(5, p1);

            uint8_t size =
                line.substring(
                    p1 + 1,
                    p2
                ).toInt();

            bool bold =
                line.substring(
                    p2 + 1,
                    p3
                ).toInt() != 0;

            String text =
                replaceTemplateVariables(
                    line.substring(p3 + 1),
                    ticketNumber,
                    date,
                    time
                );

            uint16_t fontHeight =
                myqTextHeight(size);

            uint16_t textWidth =
                myqEstimateTextWidth(
                    text,
                    fontHeight
                );

            uint16_t normalX = 0;

            if (alignment == "center")
            {
                normalX =
                    (
                        pageWidth -
                        textWidth
                    ) / 2;
            }
            else if (alignment == "right")
            {
                normalX =
                    pageWidth -
                    textWidth;
            }

            uint16_t x =
                pageWidth -
                normalX -
                1;

            uint16_t y =
                pageHeight -
                logicalY -
                1;

            myqPageText180(
                x,
                y,
                fontHeight,
                bold,
                text
            );

            logicalY +=
                fontHeight +
                8;
        }
        else if (line.startsWith("QR|"))
        {
            int separator =
                line.indexOf('|', 3);

            if (separator < 0)
            {
                continue;
            }

            uint8_t requestedSize =
                constrain(
                    line.substring(
                        3,
                        separator
                    ).toInt(),
                    1,
                    16
                );

            String data =
                replaceTemplateVariables(
                    line.substring(
                        separator + 1
                    ),
                    ticketNumber,
                    date,
                    time
                );

            uint8_t qrVersion = 1;
            uint8_t qrUnitWidth = 1;
            uint16_t qrPixelSize = 21;

            myqResolveQRLayout(
                requestedSize,
                data,
                qrVersion,
                qrUnitWidth,
                qrPixelSize
            );

            uint16_t normalX =
                (
                    pageWidth -
                    qrPixelSize
                ) / 2;

            uint16_t x =
                pageWidth -
                normalX -
                1;

            uint16_t y =
                pageHeight -
                logicalY -
                1;

            myqPageQR180(
                x,
                y,
                qrVersion,
                qrUnitWidth,
                data
            );

            logicalY +=
                qrPixelSize +
                16;
        }
        else if (line.startsWith("IMAGE|"))
        {
            String filename =
                line.substring(6);

            filename.trim();

            uint8_t* bitmap = nullptr;
            uint16_t width = 0;
            uint16_t height = 0;

            if (
                myqLoadBMPMono(
                    filename,
                    bitmap,
                    width,
                    height
                )
            )
            {
                uint16_t normalX =
                    (
                        pageWidth -
                        width
                    ) / 2;

                uint16_t x =
                    pageWidth -
                    normalX -
                    1;

                uint16_t y =
                    pageHeight -
                    logicalY -
                    1;

                myqPageBitmap180(
                    x,
                    y,
                    width,
                    height,
                    bitmap
                );

                logicalY +=
                    height +
                    8;

                free(bitmap);
            }
        }
        else if (line.startsWith("FEED|"))
        {
            uint8_t lines =
                line.substring(5).toInt();

            logicalY +=
                lines * 24;
        }
        else if (line == "CUT")
        {
            cutRequested = true;
        }
    }

    myqPageEnd();
    myqPagePrint();
    Printer.flush();

    if (
        cutRequested &&
        autoCut
    )
    {
        printerCut();
    }
}



void executeTemplate(
    const String& templateContent,
    uint32_t ticketNumber,
    const String& date,
    const String& time
)
{
    if (printOrientation == 1)
    {
        executeTemplateReverseMYQ(
            templateContent,
            ticketNumber,
            date,
            time
        );

        return;
    }


    printerInitialize();

    printerSelectStandardMode();

    int start = 0;

    while (
        start <
        templateContent.length()
    )
    {
        int end =
            templateContent.indexOf(
                '\n',
                start
            );

        if (end < 0)
        {
            end =
                templateContent.length();
        }

        String line =
            templateContent.substring(
                start,
                end
            );

        start = end + 1;

        line.trim();

        if (
            line.length() == 0 ||
            line.startsWith("#")
        )
        {
            continue;
        }

        if (
            line.startsWith("TEXT|")
        )
        {
            int p1 =
                line.indexOf(
                    '|',
                    5
                );

            int p2 =
                line.indexOf(
                    '|',
                    p1 + 1
                );

            int p3 =
                line.indexOf(
                    '|',
                    p2 + 1
                );

            if (
                p1 < 0 ||
                p2 < 0 ||
                p3 < 0
            )
            {
                continue;
            }

            String alignment =
                line.substring(
                    5,
                    p1
                );

            uint8_t size =
                line.substring(
                    p1 + 1,
                    p2
                ).toInt();

            bool bold =
                line.substring(
                    p2 + 1,
                    p3
                ).toInt() != 0;

            String text =
                line.substring(
                    p3 + 1
                );

            text =
                replaceTemplateVariables(
                    text,
                    ticketNumber,
                    date,
                    time
                );

            if (
                alignment == "center"
            )
            {
                printerAlign(1);
            }
            else if (
                alignment == "right"
            )
            {
                printerAlign(2);
            }
            else
            {
                printerAlign(0);
            }

            printerSize(
                size,
                size
            );

            printerBold(bold);

            Printer.print(text);
            Printer.print('\n');

            printerSize(1, 1);
            printerBold(false);
        }

        else if (
            line.startsWith("QR|")
        )
        {
            int separator =
                line.indexOf(
                    '|',
                    3
                );

            if (separator < 0)
            {
                continue;
            }

            uint8_t size =
                line.substring(
                    3,
                    separator
                ).toInt();

            String data =
                line.substring(
                    separator + 1
                );

            data =
                replaceTemplateVariables(
                    data,
                    ticketNumber,
                    date,
                    time
                );

            printerAlign(1);

            printerQRCode(
                data,
                size
            );

            printerFeed(1);
        }

        else if (
            line.startsWith("IMAGE|")
        )
        {
            String filename =
                line.substring(6);

            filename.trim();

            printerBMP(filename);

            printerFeed(1);
        }

        else if (
            line.startsWith("FEED|")
        )
        {
            uint8_t lines =
                line.substring(5).toInt();

            printerFeed(lines);
        }

        else if (
            line == "CUT"
        )
        {
            if (autoCut)
            {
                printerCut();
            }
        }
    }

    printerSelectStandardMode();
    Printer.flush();
}



String loadTemplate(
    const String& name
)
{
    return readFile(
        templatePath(name)
    );
}

bool saveTemplate(
    const String& name,
    const String& content
)
{
    return writeFile(
        templatePath(name),
        content
    );
}

void createDefaultTemplate()
{
    if (
        storageFS().exists(
            "/templates/default.tpl"
        )
    )
    {
        return;
    }

    const char* defaultTemplate =
        "TEXT|center|2|1|TICKET\n"
        "TEXT|center|1|0|================================\n"
        "FEED|1\n"
        "TEXT|center|3|1|#{{TICKET_NUMBER}}\n"
        "FEED|1\n"
        "TEXT|center|1|0|{{DATE}} {{TIME}}\n"
        "FEED|1\n"
        "QR|5|TICKET:{{TICKET_ID}}\n"
        "FEED|1\n"
        "TEXT|center|1|0|ID: {{TICKET_ID}}\n"
        "TEXT|center|1|0|Please keep this ticket.\n"
        "CUT\n";

    saveTemplate(
        "default",
        defaultTemplate
    );
}



bool printTicketNumber(
    uint32_t number,
    const String& date,
    const String& time
)
{
    String content =
        loadTemplate(
            activeTemplate
        );

    if (
        content.length() == 0
    )
    {
        Serial.println(
            "Active template not found."
        );

        return false;
    }

    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "PRINTING TICKET"
    );

    Serial.println(
        "================================"
    );

    Serial.print(
        "Template: "
    );

    Serial.println(
        activeTemplate
    );

    Serial.print(
        "Ticket number: "
    );

    Serial.println(number);

    executeTemplate(
        content,
        number,
        date,
        time
    );

    Serial.println(
        "Ticket completed."
    );

    return true;
}

bool printNextTicket(
    const String& date,
    const String& time
)
{
    String ticketDate = date;
    String ticketTime = time;

    if (
        ticketDate.length() == 0 ||
        ticketTime.length() == 0
    )
    {
        getTicketDateTime(
            ticketDate,
            ticketTime
        );
    }

    uint32_t next =
        ticketCounter + 1;

    if (
        !printTicketNumber(
            next,
            ticketDate,
            ticketTime
        )
    )
    {
        return false;
    }

    ticketCounter = next;

    preferences.putUInt(
        "counter",
        ticketCounter
    );

    return true;
}


