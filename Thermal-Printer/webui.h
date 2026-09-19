#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "globals.h"
#include "storage.h"
#include "helpers.h"
#include "auth.h"
#include "printer.h"
#include "settings_network.h"


// HTTP API documentation: see API.md in this folder.

class HtmlBuffer
{
public:
    HtmlBuffer& operator+=(const String& s)
    {
        buffer += s;
        flushIfNeeded();
        return *this;
    }

    HtmlBuffer& operator+=(const char* s)
    {
        buffer += s;
        flushIfNeeded();
        return *this;
    }

    void send()
    {
        if (!started)
        {
            start();
        }

        if (buffer.length() > 0)
        {
            server.sendContent(buffer);
            buffer.clear();
        }

        server.sendContent("");
    }

private:
    String buffer;
    bool started = false;
    static constexpr size_t CHUNK_SIZE = 1024;

    void start()
    {
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");
        started = true;
    }

    void flushIfNeeded()
    {
        if (buffer.length() >= CHUNK_SIZE)
        {
            if (!started)
            {
                start();
            }

            server.sendContent(buffer);
            buffer.clear();
        }
    }
};

String pageHeader(
    const String& title
)
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport"
content="width=device-width,initial-scale=1">
<title>)rawliteral";

    html += htmlEscape(title);

    html += R"rawliteral(</title>
<style>
*{box-sizing:border-box}
html{
min-height:100%;
}
body{
margin:0;
min-height:100vh;
display:flex;
flex-direction:column;
background:#0f172a;
color:#e5e7eb;
font-family:Arial,sans-serif
}
nav{
background:#111827;
padding:16px;
text-align:center;
position:sticky;
top:0
}
nav a{
color:white;
text-decoration:none;
font-weight:bold;
margin:8px 12px
}
.app-header{
background:#0b1220;
border-bottom:1px solid #263244;
padding:18px 20px 14px;
text-align:center;
}
.app-title{
font-size:24px;
font-weight:800;
letter-spacing:.5px;
}
.app-meta{
margin-top:5px;
color:#9ca3af;
font-size:13px;
}
.app-version{
display:inline-block;
margin-left:8px;
padding:2px 7px;
border:1px solid #374151;
border-radius:999px;
color:#d1d5db;
}
.app-footer{
width:100%;
margin-top:auto;
padding:16px 20px;
border-top:1px solid #263244;
background:#0b1220;
text-align:center;
color:#9ca3af;
font-size:13px;
line-height:1.7;
flex-shrink:0;
}
main{
width:100%;
max-width:900px;
margin:0 auto;
padding:20px;
flex:1 0 auto
}
.card{
background:#1f2937;
padding:22px;
border-radius:14px;
margin-bottom:18px
}
h1,h2,h3{margin-top:0}
input,select,textarea{
width:100%;
background:#111827;
color:white;
border:1px solid #4b5563;
border-radius:8px;
padding:12px;
margin:6px 0 14px
}
textarea{
min-height:350px;
font-family:monospace;
white-space:pre
}
button,.button{
display:inline-block;
padding:12px 18px;
border:0;
border-radius:8px;
font-weight:bold;
text-decoration:none;
cursor:pointer;
margin:4px
}
.green{background:#22c55e;color:#000}
.blue{background:#3b82f6;color:#fff}
.red{background:#ef4444;color:#fff}
.gray{background:#4b5563;color:#fff}
.counter{
font-size:54px;
font-weight:bold;
text-align:center;
margin:20px
}
.small{color:#9ca3af}
.good{color:#22c55e}
.bad{color:#ef4444}
.row{
display:flex;
gap:12px
}
.row>*{flex:1}
table{
width:100%;
border-collapse:collapse
}
td,th{
padding:10px;
border-bottom:1px solid #374151;
text-align:left
}
@media(max-width:650px){
.row{display:block}
nav a{
display:inline-block;
margin:6px
}
}

.settings-tabs{
display:flex;
gap:8px;
flex-wrap:wrap;
margin:0 0 18px 0;
}
.settings-tab{
display:inline-block;
padding:10px 16px;
border-radius:8px;
text-decoration:none;
font-weight:700;
background:#222;
color:#fff;
}
.settings-tab.active{
background:#00a651;
}
.settings-panel{
display:none;
}
.settings-panel.active{
display:block;
}

</style>
</head>
<body>

<header class="app-header">
<div class="app-title">TQ-PRINTER</div>
<div class="app-meta">Queue Number Printer <span class="app-version">v)rawliteral";

    html += FIRMWARE_VERSION;

    html += R"rawliteral(</span></div>
</header>

<nav>
<a href="/">Dashboard</a>
<a href="/templates">Templates</a>
<a href="/files">Images / Files</a>
<a href="/settings">Settings</a>
</nav>

<main>
)rawliteral";

    return html;
}

String pageFooter()
{
    String html = R"rawliteral(
</main>

<footer class="app-footer">
<div><strong>TQ-Printer</strong> &middot; v)rawliteral";

    html += FIRMWARE_VERSION;

    html += R"rawliteral(</div>
<div>ESP32-S3 &middot; ESC/POS</div>
<div><a href="https://github.com/HellBz/TQ-Printer">GitHub / Documentation</a></div>
</footer>

<script>
function confirmDelete(message)
{
    return confirm(message);
}
</script>
</body>
</html>
)rawliteral";

    return html;
}

void sendSettingsSavedPage(
    const String& title,
    const String& headline,
    const String& message,
    bool restart = false,
    uint16_t redirectDelayMs = 2000
)
{
    HtmlBuffer html;
    html += pageHeader(title);

    html +=
        "<div class='card'>"
        "<h1>" +
        htmlEscape(headline) +
        "</h1>"
        "<p>" +
        htmlEscape(message) +
        "</p>";

    if (restart)
    {
        html +=
            "<p class='small'>The device is restarting. Please wait a few seconds.</p>"
            "<p><a class='button blue' href='/settings'>Open settings after restart</a></p>";
    }
    else
    {
        html +=
            "<p class='small'>Redirecting to settings...</p>"
            "<meta http-equiv='refresh' content='" +
            String(
                redirectDelayMs / 1000
            ) +
            ";url=/settings'>"
            "<p><a class='button blue' href='/settings'>Back to settings</a></p>";
    }

    html += "</div>";

    html += pageFooter();

    html.send();
}

void sendJsonResponse(
    int code,
    const String& json
)
{
    server.send(
        code,
        "application/json",
        json
    );
}



void handleRoot()
{
    if (!requireAdmin()) return;
    HtmlBuffer html;
    html += pageHeader(
        "TQ-Printer"
    );

    html += storageSummaryHTML(true);

    html +=
        "<div class='card'>";

    html +=
        "<h1>Dashboard</h1>";

    html +=
        "<p>Active template: <b>" +
        htmlEscape(activeTemplate) +
        "</b></p>";

    html +=
        "<div class='small'>Current ticket</div>";

    html +=
        "<div class='counter'>#" +
        formatTicketNumber(
            ticketCounter
        ) +
        "</div>";

    html += R"rawliteral(
<button type="button" class="green"
onclick="printNext()">
PRINT NEXT TICKET
</button>

<div id="printStatus"
class="small"></div>
)rawliteral";

    html += "</div>";

    html += R"rawliteral(
<div class="card">
<h2>Print specific ticket number</h2>

<label>Ticket number</label>
<input id="specificNumber"
type="number"
min="0"
placeholder="42">

<button type="button" class="blue"
onclick="printSpecific()">
PRINT SPECIFIC NUMBER
</button>

<p class="small">
Printing a specific number does not change the normal counter.
</p>
</div>
)rawliteral";

    html += R"rawliteral(
<div class="card">
<h2>Counter</h2>

<label>Set current counter</label>
<input id="counterValue"
type="number"
min="0"
value=")rawliteral";

    html += String(ticketCounter);

    html += R"rawliteral(">

<button type="button" class="blue"
onclick="setCounter()">
SET COUNTER
</button>

<button type="button" class="red"
onclick="resetCounter()">
RESET COUNTER
</button>
</div>
)rawliteral";

    html +=
        "<div class='card'>"
        "<h2>Network</h2>";

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        html +=
            "<p class='good'>Connected to <b>" +
            htmlEscape(
                WiFi.SSID()
            ) +
            "</b></p>";

        html +=
            "<p>IP: <b>" +
            WiFi.localIP().toString() +
            "</b></p>";

        html +=
            "<p>Signal: <b>" +
            String(
                WiFi.RSSI()
            ) +
            " dBm</b></p>";

        html +=
            "<p>Local address: <b>http://" +
            htmlEscape(
                mdnsHostname
            ) +
            ".local</b></p>";
    }
    else
    {
        html +=
            "<p class='bad'>Not connected to an existing network.</p>";
    }

    if (fallbackAPActive)
    {
        html +=
            "<p>Fallback AP: <b>" +
            htmlEscape(apSSID) +
            "</b></p>";

        html +=
            "<p>AP IP: <b>" +
            WiFi.softAPIP().toString() +
            "</b></p>";
    }

    html +=
        "</div>";

    html += R"rawliteral(
<script>

function timestamp()
{
    const d = new Date();

    const pad =
        n => String(n).padStart(2,"0");

    return {
        date:
            d.getFullYear() +
            "-" +
            pad(d.getMonth()+1) +
            "-" +
            pad(d.getDate()),

        time:
            pad(d.getHours()) +
            ":" +
            pad(d.getMinutes()) +
            ":" +
            pad(d.getSeconds())
    };
}

async function postForm(url, params)
{
    return fetch(
        url,
        {
            method: "POST",
            headers:
            {
                "Content-Type": "application/x-www-form-urlencoded"
            },
            body: new URLSearchParams(params).toString()
        }
    );
}

async function printNext()
{
    const t = timestamp();

    const response =
        await postForm(
            "/api/print-next",
            {
                date: t.date,
                time: t.time
            }
        );

    document.getElementById(
        "printStatus"
    ).textContent =
        await response.text();

    setTimeout(
        () => location.reload(),
        500
    );
}

async function printSpecific()
{
    const number =
        document.getElementById(
            "specificNumber"
        ).value;

    if(number === "")
    {
        alert("Enter a ticket number.");
        return;
    }

    const t = timestamp();

    const response =
        await postForm(
            "/api/print-specific",
            {
                number: number,
                date: t.date,
                time: t.time
            }
        );

    alert(
        await response.text()
    );
}

async function setCounter()
{
    const value =
        document.getElementById(
            "counterValue"
        ).value;

    const response =
        await postForm(
            "/api/counter/set",
            { value: value }
        );

    alert(await response.text());

    location.reload();
}

async function resetCounter()
{
    if(
        !confirm(
            "Reset ticket counter to zero?"
        )
    )
    {
        return;
    }

    const response =
        await postForm(
            "/api/counter/reset",
            {}
        );

    alert(await response.text());

    location.reload();
}

</script>
)rawliteral";

    html += pageFooter();

    html.send();
}



String templateOptions()
{
    String options;

    File directory =
        storageFS().open(
            "/templates"
        );

    if (!directory)
    {
        return options;
    }

    File file =
        directory.openNextFile();

    while (file)
    {
        if (!file.isDirectory())
        {
            String name =
                String(
                    file.name()
                );

            if (
                name.startsWith(
                    "/templates/"
                )
            )
            {
                name =
                    name.substring(11);
            }

            if (
                name.endsWith(
                    ".tpl"
                )
            )
            {
                name.remove(
                    name.length() - 4
                );
            }

            options +=
                "<option value='" +
                htmlEscape(name) +
                "'";

            if (
                name ==
                activeTemplate
            )
            {
                options +=
                    " selected";
            }

            options +=
                ">" +
                htmlEscape(name) +
                "</option>";
        }

        file =
            directory.openNextFile();
    }

    directory.close();

    return options;
}

String templateListJson()
{
    String json = "[";

    File directory =
        storageFS().open(
            "/templates"
        );

    if (directory)
    {
        bool first = true;
        File file =
            directory.openNextFile();

        while (file)
        {
            if (!file.isDirectory())
            {
                String name =
                    String(file.name());

                if (
                    name.startsWith(
                        "/templates/"
                    )
                )
                {
                    name =
                        name.substring(11);
                }

                if (
                    name.endsWith(".tpl")
                )
                {
                    name.remove(
                        name.length() - 4
                    );
                }

                if (!first)
                {
                    json += ",";
                }
                first = false;

                json +=
                    "{\"name\":\"" +
                    jsonEscape(name) +
                    "\",\"active\":";

                json +=
                    name == activeTemplate
                        ? "true"
                        : "false";

                json +=
                    ",\"size\":" +
                    String(file.size()) +
                    "}";
            }

            file =
                directory.openNextFile();
        }

        directory.close();
    }

    json += "]";

    return json;
}

void handleTemplates()
{
    if (!requireAdmin()) return;
    String editName =
        server.arg("edit");

    if (
        editName.length() == 0
    )
    {
        editName =
            activeTemplate;
    }

    String content =
        loadTemplate(editName);

    HtmlBuffer html;
    html += pageHeader(
        "Templates"
    );

    html +=
        "<div class='card'>"
        "<h1>Templates</h1>"
        "<label>Active template</label>"
        "<form action='/template/activate' method='post'>"
        "<select name='name'>" +
        templateOptions() +
        "</select>"
        "<button class='green'>SET ACTIVE</button>"
        "</form>"
        "</div>";

    html +=
        "<div class='card'>"
        "<h2>Template Editor</h2>";

    html +=
        "<form action='/template/save' method='post'>";

    html +=
        "<label>Template name</label>"
        "<input name='name' value='" +
        htmlEscape(editName) +
        "'>";

    html +=
        "<label>Template</label>";

    html +=
        "<textarea name='content'>" +
        htmlEscape(content) +
        "</textarea>";

    html += R"rawliteral(
<button class="green">
SAVE TEMPLATE
</button>
</form>

<form action="/template/delete" method="post" onsubmit="return confirmDelete('Delete this template permanently?');">
<input type="hidden" name="name" value=")rawliteral";

    html +=
        "'" + htmlEscape(editName) + "'";

    html += R"rawliteral(>
<button type="submit" class="red">DELETE TEMPLATE</button>
</form>

<a class="button blue" href="/template/preview?name=)rawliteral";

    html +=
        htmlEscape(editName) +
        R"rawliteral(">PREVIEW TEMPLATE</a>

<h3>Template commands</h3>

<pre>
TEXT|center|2|1|Text
TEXT|left|1|0|Text
TEXT|right|1|0|Text

QR|5|TICKET:{{TICKET_ID}}

IMAGE|logo.bmp

FEED|2

CUT
</pre>

<h3>Variables</h3>

<pre>
{{TICKET_NUMBER}}
{{TICKET_ID}}
{{DATE}}
{{TIME}}
</pre>

</div>
)rawliteral";

    html += pageFooter();

    html.send();
}

bool getBMPDimensions(
    const String& path,
    int32_t& width,
    int32_t& height
);

String previewTemplateHTML(
    const String& content
)
{
    const String date = "2024-01-01";
    const String time = "12:00:00";
    constexpr uint32_t ticketNumber = 12345;

    String out =
        "<table>"
        "<tr><th>Element</th><th>Content / Parameters</th><th>Estimated height</th></tr>";

    uint16_t totalHeight = 0;
    int start = 0;

    while (
        start <
        static_cast<int>(content.length())
    )
    {
        int end =
            content.indexOf(
                '\n',
                start
            );

        if (end < 0)
        {
            end = content.length();
        }

        String line =
            content.substring(
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

        String element;
        String detail;
        uint16_t height = 0;

        if (line.startsWith("TEXT|"))
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
                element = "TEXT (invalid)";
                detail = htmlEscape(line);
            }
            else
            {
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
                    replaceTemplateVariables(
                        line.substring(p3 + 1),
                        ticketNumber,
                        date,
                        time
                    );

                element =
                    "TEXT size=" +
                    String(size) +
                    " bold=" +
                    (
                        bold
                            ? "yes"
                            : "no"
                    ) +
                    " align=" +
                    alignment;

                detail = htmlEscape(text);
                height = 24 * size;
            }
        }
        else if (line.startsWith("QR|"))
        {
            int separator =
                line.indexOf(
                    '|',
                    3
                );

            uint8_t size =
                line.substring(
                    3,
                    separator
                ).toInt();

            String data =
                replaceTemplateVariables(
                    line.substring(separator + 1),
                    ticketNumber,
                    date,
                    time
                );

            element =
                "QR size=" +
                String(size);

            detail = htmlEscape(data);
            height =
                static_cast<uint16_t>(
                    17 + 4 * size * 3
                );
        }
        else if (line.startsWith("IMAGE|"))
        {
            String filename =
                line.substring(6);

            filename.trim();

            element = "IMAGE";
            detail = htmlEscape(filename);

            int32_t imageWidth = 0;
            int32_t imageHeight = 0;

            if (
                getBMPDimensions(
                    imagePath(filename),
                    imageWidth,
                    imageHeight
                )
            )
            {
                height =
                    static_cast<uint16_t>(
                        imageHeight
                    );
            }
        }
        else if (line.startsWith("FEED|"))
        {
            uint8_t lines =
                line.substring(5).toInt();

            element = "FEED";
            detail = String(lines) + " line(s)";
            height = lines * 12;
        }
        else if (line == "CUT")
        {
            element = "CUT";
            detail = "Full cut";
            height = 30;
        }
        else
        {
            element = "UNKNOWN";
            detail = htmlEscape(line);
        }

        out +=
            "<tr><td>" +
            htmlEscape(element) +
            "</td><td>" +
            detail +
            "</td><td>" +
            String(height) +
            " px</td></tr>";

        totalHeight += height;
    }

    out +=
        "</table>"
        "<p><b>Total estimated height:</b> " +
        String(totalHeight) +
        " px</p>";

    return out;
}

void handleTemplatePreview()
{
    if (!requireAdmin()) return;

    String name =
        server.arg("name");

    if (name.length() == 0)
    {
        name = activeTemplate;
    }

    String content =
        loadTemplate(name);

    if (content.length() == 0)
    {
        server.send(
            404,
            "text/plain",
            "Template not found"
        );

        return;
    }

    HtmlBuffer html;
    html += pageHeader("Template Preview");
    html +=
        "<div class='card'>"
        "<h1>Preview: " +
        htmlEscape(name) +
        "</h1>"
        "<p class='small'>Sample values used: ticket number 12345, date 2024-01-01, time 12:00:00.</p>";

    html += previewTemplateHTML(content);

    html +=
        "<a class='button blue' href='/templates?edit=" +
        htmlEscape(name) +
        "'>BACK TO EDITOR</a>"
        "</div>";

    html += pageFooter();
    html.send();
}

bool getBMPDimensions(
    const String& path,
    int32_t& width,
    int32_t& height
)
{
    File file =
        storageFS().open(
            path,
            "r"
        );

    if (!file)
    {
        return false;
    }

    if (read16(file) != 0x4D42)
    {
        file.close();
        return false;
    }

    file.seek(18);

    width =
        static_cast<int32_t>(
            read32(file)
        );

    height =
        static_cast<int32_t>(
            read32(file)
        );

    if (height < 0)
    {
        height = -height;
    }

    file.close();

    return width > 0 && height > 0;
}



void handleFiles()
{
    if (!requireAdmin()) return;
    HtmlBuffer html;
    html += pageHeader(
        "Images / Files"
    );

    html += storageSummaryHTML(false);

    html += R"rawliteral(
<div class="card">

<h1>Images / Files</h1>

<h2>Upload BMP image</h2>

<p class="small">
For direct printing use uncompressed 24-bit BMP,
maximum width 576 pixels.
</p>

<form id="uploadForm"
enctype="multipart/form-data">

<input type="file"
id="fileInput"
name="file"
accept=".bmp">

<button type="button"
class="green"
id="uploadBtn"
onclick="uploadFile()">
UPLOAD IMAGE
</button>

</form>

<div id="uploadProgress"
style="width:100%;height:8px;background:#333;border-radius:4px;margin-top:12px;display:none;">
<div id="progressBar"
style="width:0%;height:100%;background:#4caf50;border-radius:4px;transition:width 0.2s;">
</div>
</div>

<div id="uploadStatus" class="small"></div>

<script>
function uploadFile()
{
    var fileInput = document.getElementById('fileInput');
    var file = fileInput.files[0];
    if (!file)
    {
        document.getElementById('uploadStatus').textContent = 'Please select a file.';
        return;
    }

    var formData = new FormData();
    formData.append('file', file);

    var xhr = new XMLHttpRequest();
    var progress = document.getElementById('uploadProgress');
    var bar = document.getElementById('progressBar');
    var status = document.getElementById('uploadStatus');
    var btn = document.getElementById('uploadBtn');

    progress.style.display = 'block';
    bar.style.width = '0%';
    status.textContent = 'Uploading...';
    btn.disabled = true;

    xhr.upload.onprogress = function(e)
    {
        if (e.lengthComputable)
        {
            bar.style.width = (e.loaded / e.total * 100) + '%';
        }
    };

    xhr.onload = function()
    {
        btn.disabled = false;
        try
        {
            var resp = JSON.parse(xhr.responseText);
            if (resp.ok)
            {
                status.textContent = resp.message || 'Upload complete.';
                setTimeout(function()
                {
                    window.location.reload();
                }, 1000);
            }
            else
            {
                status.textContent = 'Error: ' + (resp.error || 'Upload failed.');
            }
        }
        catch (err)
        {
            status.textContent = 'Upload finished.';
            setTimeout(function()
            {
                window.location.reload();
            }, 1000);
        }
    };

    xhr.onerror = function()
    {
        btn.disabled = false;
        status.textContent = 'Upload error.';
    };

    xhr.open('POST', '/upload');
    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
    xhr.send(formData);
}
</script>

</div>

<div class="card">

<h2>Stored images</h2>

<table>

<tr>
<th>Preview</th>
<th>File</th>
<th>Dimensions</th>
<th>Size</th>
<th></th>
</tr>
)rawliteral";

    File directory =
        storageFS().open(
            "/images"
        );

    if (directory)
    {
        File file =
            directory.openNextFile();

        while (file)
        {
            if (!file.isDirectory())
            {
                String filename =
                    String(
                        file.name()
                    );

                if (
                    filename.startsWith(
                        "/images/"
                    )
                )
                {
                    filename =
                        filename.substring(8);
                }

                html +=
                    "<tr>";

                html +=
                    "<td><a href='/file/view?name=" +
                    htmlEscape(filename) +
                    "' target='_blank'><img src='/file/view?name=" +
                    htmlEscape(filename) +
                    "' alt='Preview' style='max-width:180px;max-height:120px;background:white;border-radius:6px;padding:4px'></a></td>";

                html +=
                    "<td>" +
                    htmlEscape(filename) +
                    "</td>";

                int32_t imageWidth = 0;
                int32_t imageHeight = 0;

                bool dimensionsOK =
                    getBMPDimensions(
                        imagePath(filename),
                        imageWidth,
                        imageHeight
                    );

                html +=
                    "<td>";

                if (dimensionsOK)
                {
                    html +=
                        String(imageWidth) +
                        " × " +
                        String(imageHeight) +
                        " px";
                }
                else
                {
                    html +=
                        "Unknown";
                }

                html +=
                    "</td>";

                html +=
                    "<td>" +
                    String(
                        file.size()
                    ) +
                    " B</td>";

                html +=
                    "<td>"
                    "<form action='/file/delete' method='post' style='display:inline' onsubmit=\"return confirmDelete('Delete this image permanently?');\">"
                    "<input type='hidden' name='name' value='" +
                    htmlEscape(filename) +
                    "'>"
                    "<button type='submit' class='red'>DELETE</button>"
                    "</form>"
                    "</td>";

                html +=
                    "</tr>";
            }

            file =
                directory.openNextFile();
        }

        directory.close();
    }

    html +=
        "</table>"
        "</div>";

    html += pageFooter();

    html.send();
}



void handleSettings()
{
    if (!requireAdmin()) return;
    HtmlBuffer html;
    html += pageHeader(
        "Settings"
    );

    html += R"rawliteral(
<div class="settings-tabs">
<a href="#" class="settings-tab active" data-tab="general">GENERAL</a>
<a href="#" class="settings-tab" data-tab="wifi">WI-FI</a>
<a href="#" class="settings-tab" data-tab="mdns">mDNS</a>
<a href="#" class="settings-tab" data-tab="printer">PRINTER</a>
<a href="#" class="settings-tab" data-tab="trigger">TRIGGER</a>
<a href="#" class="settings-tab" data-tab="update">UPDATE</a>
</div>

<div id="settings-general" class="settings-panel active">
<div class="card">
<h2>General Settings</h2>
<p><b>Firmware:</b> )rawliteral";

    html += FIRMWARE_VERSION;

    html += R"rawliteral(</p>
<p><b>Build:</b> ESP32-S3 MY-Q805 TQ-Printer</p>
<p class="small">General system options, changelog and status information.</p>
</div>

<div class="card">
<h2>Changelog</h2>
<ul class="small">
<li><b>2.9.11</b> &mdash; OTA update can now be performed from a URL (e.g. GitHub release asset).</li>
<li><b>2.9.10</b> &mdash; CI: normal commits now use --only-compilation-database for a faster syntax/build-database check.</li>
<li><b>2.9.9</b> &mdash; Added GitHub / Documentation link in the page footer.</li>
<li><b>2.9.8</b> &mdash; Configurable print area roll width, HTML pages streamed in chunks, template preview page.</li>
<li><b>2.9.7</b> &mdash; State-changing endpoints (delete, counter, print, orientation test) now require POST; security hardening.</li>
<li><b>2.9.6</b> &mdash; Moved TQ-Printer branding into the fixed page header; dashboard title is now "Dashboard".</li>
<li><b>2.9.5</b> &mdash; Renamed product branding to TQ-Printer across Web-UI, auth realm and documentation.</li>
<li><b>2.9.4</b> &mdash; Added in-device changelog on the General Settings tab.</li>
<li><b>2.9.3</b> &mdash; Modularized firmware, REST API for templates, AJAX file upload, OTA firmware updates, settings saved confirmation pages.</li>
<li><b>2.9.2</b> &mdash; Initial Web UI/API cleanup, HTML/JSON escaping, upload and counter validation.</li>
</ul>
</div>
</div>

<div id="settings-wifi" class="settings-panel">
<div class="card">
<h2>Wi-Fi Connection</h2>
<form action="/settings/wifi" method="post">
<label>SSID</label>
<input name="ssid" value=")rawliteral";

    html += htmlEscape(wifiSSID);

    html += R"rawliteral(">
<label>Password</label>
<input type="password" name="password" value="">
<button class="green">SAVE WI-FI</button>
</form>
</div>

<div class="card">
<h2>Fallback Access Point</h2>
<form action="/settings/ap" method="post">
<label>Access Point name</label>
<input name="apssid" value=")rawliteral";

    html += htmlEscape(apSSID);

    html += R"rawliteral(">
<label>Access Point password</label>
<input type="password" name="appassword" value="">
<p class="small">Leave the password field empty to keep the currently stored password.</p>
<button class="green">SAVE ACCESS POINT</button>
</form>
</div>
</div>

<div id="settings-mdns" class="settings-panel">
<div class="card">
<h2>mDNS / Local Hostname</h2>
<form action="/settings/mdns" method="post">
<label>Local hostname</label>
<input name="hostname" maxlength="63" value=")rawliteral";

    html += htmlEscape(mdnsHostname);

    html += R"rawliteral(">
<p class="small">Local address: http://)rawliteral";

    html += htmlEscape(mdnsHostname);

    html += R"rawliteral(.local</p>
<p class="small">Use letters a-z, numbers 0-9 and hyphens. Do not enter .local.</p>
<button class="green">SAVE mDNS HOSTNAME</button>
</form>
</div>
</div>

<div id="settings-printer" class="settings-panel">
<div class="card">
<h2>Printer Settings</h2>
<form action="/settings/printer" method="post">

<label>Printer baud rate</label>
<input type="number"
       name="baud"
       list="printer-baudrates"
       min="300"
       max="5000000"
       step="1"
       value=")rawliteral";

    html += String(printerBaud);

    html += R"rawliteral(">

<datalist id="printer-baudrates">
<option value="9600">
<option value="19200">
<option value="38400">
<option value="57600">
<option value="115200">
<option value="230400">
<option value="460800">
<option value="921600">
<option value="1500000">
</datalist>

<p class="small">Known working value for the MY-Q805K: 1500000 baud. You can also enter a custom value.</p>

<label>Feed lines</label>
<input type="number" name="feed" min="0" max="20" value=")rawliteral";

    html += String(feedLines);

    html += R"rawliteral(">

<label>Default QR size</label>
<input type="number" name="qrsize" min="1" max="16" value=")rawliteral";

    html += String(defaultQRSize);

    html += R"rawliteral(">

<label>Print area width (dots)</label>
<input type="number" name="pageWidth" min="48" max="576" step="8" value=")rawliteral";

    html += String(pageWidth);

    html += R"rawliteral(">
<p class="small">576 dots = 80 mm roll, 384 dots = 58 mm roll. Must be a multiple of 8.</p>

<label>Cutter</label>
<select name="cutter">)rawliteral";

    html +=
        autoCut
            ? "<option value='1' selected>Enabled</option><option value='0'>Disabled</option>"
            : "<option value='1'>Enabled</option><option value='0' selected>Disabled</option>";

    html += R"rawliteral(
</select>

<label>Print direction</label>
<select name="orientation">
)rawliteral";

    html +=
        printOrientation == 0
            ? "<option value='0' selected>Normal</option>"
            : "<option value='0'>Normal</option>";

    html +=
        printOrientation == 1
            ? "<option value='1' selected>Reverse</option>"
            : "<option value='1'>Reverse</option>";

    html += R"rawliteral(
</select>

<p class="small">The selected direction is saved for web printing, GPIO14 and future triggers.</p>

<button class="green">SAVE PRINTER SETTINGS</button>
</form>
<form action="/printer/orientation-test" method="post">
<button type="submit" class="button blue">TEST DIRECTION</button>
</form>
</div>
</div>


<div id="settings-trigger" class="settings-panel">
<div class="card">
<h2>GPIO Trigger</h2>
<form action="/settings/trigger" method="post">

<label>GPIO button trigger</label>
<select name="gpioEnabled">
)rawliteral";

    html +=
        gpioTriggerEnabled
            ? "<option value='1' selected>Enabled</option><option value='0'>Disabled</option>"
            : "<option value='1'>Enabled</option><option value='0' selected>Disabled</option>";

    html += R"rawliteral(
</select>

<label>GPIO pin</label>
<input type="number" name="gpioPin" min="0" max="48" step="1" value=")rawliteral";

    html += String(triggerGPIO);

    html += R"rawliteral(">

<label>Trigger level</label>
<select name="activeLow">)rawliteral";

    html +=
        triggerActiveLow
            ? "<option value='1' selected>Active Low (button to GND)</option><option value='0'>Active High</option>"
            : "<option value='1'>Active Low (button to GND)</option><option value='0' selected>Active High</option>";

    html += R"rawliteral(
</select>

<label>Debounce (ms)</label>
<input type="number" name="debounce" min="0" max="5000" step="1" value=")rawliteral";

    html += String(triggerDebounceMs);

    html += R"rawliteral(">

<button class="green">SAVE TRIGGER SETTINGS</button>
</form>
</div>

<div class="card">
<h2>QR / Barcode Scanner</h2>
<form action="/settings/trigger" method="post">
<input type="hidden" name="scannerOnly" value="1">

<label>Scanner trigger</label>
<select name="scannerEnabled">)rawliteral";

    html +=
        scannerTriggerEnabled
            ? "<option value='1' selected>Enabled</option><option value='0'>Disabled</option>"
            : "<option value='1'>Enabled</option><option value='0' selected>Disabled</option>";

    html += R"rawliteral(
</select>

<label>Accepted scan type</label>
<select name="scannerMode">)rawliteral";

    html += scannerMode == 0 ? "<option value='0' selected>QR + Barcode</option>" : "<option value='0'>QR + Barcode</option>";
    html += scannerMode == 1 ? "<option value='1' selected>QR only</option>" : "<option value='1'>QR only</option>";
    html += scannerMode == 2 ? "<option value='2' selected>Barcode only</option>" : "<option value='2'>Barcode only</option>";

    html += R"rawliteral(
</select>

<label>Required pattern / prefix</label>
<input name="scanPattern" maxlength="96" placeholder="Example: EVENT-2026-" value=")rawliteral";

    html += htmlEscape(scannerPattern);

    html += R"rawliteral(">

<label>Code reuse</label>
<select name="uniqueOnly">)rawliteral";

    html +=
        scannerUniqueOnly
            ? "<option value='1' selected>Accept each code only once</option><option value='0'>Allow repeated scans</option>"
            : "<option value='1'>Accept each code only once</option><option value='0' selected>Allow repeated scans</option>";

    html += R"rawliteral(
</select>

<p class="small">Scanner hardware is not activated yet. These settings prepare validation for the next scanner implementation.</p>
<button class="green">SAVE SCANNER SETTINGS</button>
</form>
</div>
</div>

<div id="settings-update" class="settings-panel">
<div class="card">
<h2>Firmware Update (OTA)</h2>
<p class="small">
Upload a compiled firmware binary (.bin) over the air. The device will restart automatically.
</p>
<a class="button green" href="/ota">OPEN OTA UPDATE</a>
</div>
</div>

<script>
(function(){
    const tabs = document.querySelectorAll('.settings-tab');
    const panels = document.querySelectorAll('.settings-panel');

    function showTab(name)
    {
        tabs.forEach(function(tab){
            tab.classList.toggle(
                'active',
                tab.dataset.tab === name
            );
        });

        panels.forEach(function(panel){
            panel.classList.toggle(
                'active',
                panel.id === 'settings-' + name
            );
        });

        localStorage.setItem(
            'ticketPrinterSettingsTab',
            name
        );
    }

    tabs.forEach(function(tab){
        tab.addEventListener(
            'click',
            function(event){
                event.preventDefault();
                showTab(tab.dataset.tab);
            }
        );
    });

    // Always start on the General tab so the settings page is predictable.
    // Clicking a tab still updates localStorage, but the initial view resets.
    localStorage.removeItem('ticketPrinterSettingsTab');
})();
</script>
)rawliteral";

    html += pageFooter();

    html.send();
}



void handleWiFiScan()
{
    if (!requireAdmin()) return;
    int count =
        WiFi.scanNetworks();

    String json = "[";

    for (
        int i = 0;
        i < count;
        i++
    )
    {
        if (i > 0)
        {
            json += ",";
        }

        String ssid =
            jsonEscape(
                WiFi.SSID(i)
            );

        json +=
            "{\"ssid\":\"" +
            ssid +
            "\",\"rssi\":" +
            String(
                WiFi.RSSI(i)
            ) +
            ",\"secure\":" +
            String(
                WiFi.encryptionType(i) ==
                WIFI_AUTH_OPEN
                    ? "false"
                    : "true"
            ) +
            "}";
    }

    json += "]";

    WiFi.scanDelete();

    server.send(
        200,
        "application/json",
        json
    );
}



void handlePrintNext()
{
    if (!requireAdmin()) return;
    String date =
        server.arg("date");

    String time =
        server.arg("time");

    bool result =
        printNextTicket(
            date,
            time
        );

    server.send(
        result ? 200 : 500,
        "text/plain",
        result
            ? "Ticket printed."
            : "Print failed."
    );
}

void handlePrintSpecific()
{
    if (!requireAdmin()) return;
    if (
        !server.hasArg(
            "number"
        )
    )
    {
        server.send(
            400,
            "text/plain",
            "Missing ticket number."
        );

        return;
    }

    uint32_t number =
        strtoul(
            server.arg(
                "number"
            ).c_str(),
            nullptr,
            10
        );

    String date =
        server.arg("date");

    String time =
        server.arg("time");

    bool result =
        printTicketNumber(
            number,
            date,
            time
        );

    server.send(
        result ? 200 : 500,
        "text/plain",
        result
            ? "Specific ticket printed. Counter unchanged."
            : "Print failed."
    );
}



void handleSetCounter()
{
    if (!requireAdmin()) return;

    if (
        !server.hasArg("value")
    )
    {
        server.send(
            400,
            "text/plain",
            "Counter value missing."
        );

        return;
    }

    uint32_t value =
        strtoul(
            server.arg(
                "value"
            ).c_str(),
            nullptr,
            10
        );

    ticketCounter =
        value;

    preferences.putUInt(
        "counter",
        ticketCounter
    );

    server.send(
        200,
        "text/plain",
        "Counter updated."
    );
}

void handleResetCounter()
{
    if (!requireAdmin()) return;
    ticketCounter = 0;

    preferences.putUInt(
        "counter",
        0
    );

    server.send(
        200,
        "text/plain",
        "Counter reset."
    );
}



void handleTemplateSave()
{
    if (!requireAdmin()) return;
    String name =
        sanitizeTemplateName(
            server.arg("name")
        );

    String content =
        server.arg("content");

    if (
        name.length() == 0
    )
    {
        server.send(
            400,
            "text/plain",
            "Template name missing."
        );

        return;
    }

    if (
        !saveTemplate(
            name,
            content
        )
    )
    {
        server.send(
            500,
            "text/plain",
            "Could not save template."
        );

        return;
    }

    server.sendHeader(
        "Location",
        "/templates?edit=" + name
    );

    server.send(
        303
    );
}

void handleTemplateActivate()
{
    if (!requireAdmin()) return;
    String name =
        sanitizeTemplateName(
            server.arg("name")
        );

    if (
        !storageFS().exists(
            templatePath(name)
        )
    )
    {
        server.send(
            404,
            "text/plain",
            "Template not found."
        );

        return;
    }

    activeTemplate =
        name;

    preferences.putString(
        "template",
        activeTemplate
    );

    server.sendHeader(
        "Location",
        "/templates"
    );

    server.send(
        303
    );
}

void handleTemplateDelete()
{
    if (!requireAdmin()) return;
    String name =
        sanitizeTemplateName(
            server.arg(
                "name"
            )
        );

    if (name.length() == 0)
    {
        server.send(
            400,
            "text/plain",
            "Template name missing."
        );

        return;
    }

    String path =
        templatePath(
            name
        );

    if (!storageFS().exists(path))
    {
        server.send(
            404,
            "text/plain",
            "Template not found."
        );

        return;
    }

    if (!storageFS().remove(path))
    {
        server.send(
            500,
            "text/plain",
            "Could not delete template."
        );

        return;
    }

    if (name == activeTemplate)
    {
        if (
            storageFS().exists(
                templatePath(
                    "default"
                )
            )
        )
        {
            activeTemplate =
                "default";
        }
        else
        {
            activeTemplate =
                "";
        }

        preferences.putString(
            "template",
            activeTemplate
        );
    }

    server.sendHeader(
        "Location",
        "/templates"
    );

    server.send(
        303
    );
}

void handleApiTemplates()
{
    if (!requireAdmin())
    {
        sendJsonResponse(
            401,
            "{\"error\":\"Unauthorized\"}"
        );

        return;
    }

    sendJsonResponse(
        200,
        templateListJson()
    );
}

void handleApiTemplateLoad()
{
    if (!requireAdmin())
    {
        sendJsonResponse(
            401,
            "{\"error\":\"Unauthorized\"}"
        );

        return;
    }

    String name =
        sanitizeTemplateName(
            server.arg("name")
        );

    if (name.length() == 0)
    {
        sendJsonResponse(
            400,
            "{\"error\":\"Template name missing.\"}"
        );

        return;
    }

    String path = templatePath(name);

    if (!storageFS().exists(path))
    {
        sendJsonResponse(
            404,
            "{\"error\":\"Template not found.\"}"
        );

        return;
    }

    String content = readFile(path);

    String json =
        "{\"name\":\"" +
        jsonEscape(name) +
        "\",\"content\":\"" +
        jsonEscape(content) +
        "\"}";

    sendJsonResponse(200, json);
}

void handleApiTemplateAction()
{
    if (!requireAdmin())
    {
        sendJsonResponse(
            401,
            "{\"error\":\"Unauthorized\"}"
        );

        return;
    }

    String action = server.arg("action");
    String name =
        sanitizeTemplateName(
            server.arg("name")
        );

    if (
        action != "list" &&
        name.length() == 0
    )
    {
        sendJsonResponse(
            400,
            "{\"error\":\"Template name missing.\"}"
        );

        return;
    }

    if (action == "load")
    {
        handleApiTemplateLoad();
        return;
    }

    if (action == "save")
    {
        if (name.length() == 0)
        {
            sendJsonResponse(
                400,
                "{\"error\":\"Template name missing.\"}"
            );

            return;
        }

        String content = server.arg("content");

        if (!saveTemplate(name, content))
        {
            sendJsonResponse(
                500,
                "{\"error\":\"Could not save template.\"}"
            );

            return;
        }

        sendJsonResponse(
            200,
            "{\"ok\":true,\"message\":\"Template saved.\"}"
        );

        return;
    }

    if (action == "activate")
    {
        String path = templatePath(name);

        if (!storageFS().exists(path))
        {
            sendJsonResponse(
                404,
                "{\"error\":\"Template not found.\"}"
            );

            return;
        }

        activeTemplate = name;

        preferences.putString(
            "template",
            activeTemplate
        );

        sendJsonResponse(
            200,
            "{\"ok\":true,\"message\":\"Template activated.\"}"
        );

        return;
    }

    if (action == "delete")
    {
        String path = templatePath(name);

        if (!storageFS().exists(path))
        {
            sendJsonResponse(
                404,
                "{\"error\":\"Template not found.\"}"
            );

            return;
        }

        if (!storageFS().remove(path))
        {
            sendJsonResponse(
                500,
                "{\"error\":\"Could not delete template.\"}"
            );

            return;
        }

        if (name == activeTemplate)
        {
            if (
                storageFS().exists(
                    templatePath("default")
                )
            )
            {
                activeTemplate = "default";
            }
            else
            {
                activeTemplate = "";
            }

            preferences.putString(
                "template",
                activeTemplate
            );
        }

        sendJsonResponse(
            200,
            "{\"ok\":true,\"message\":\"Template deleted.\"}"
        );

        return;
    }

    sendJsonResponse(
        400,
        "{\"error\":\"Unknown action.\"}"
    );
}



void handleUploadData()
{
    if (!requireAdmin()) return;
    HTTPUpload& upload =
        server.upload();

    if (
        upload.status ==
        UPLOAD_FILE_START
    )
    {
        String filename =
            sanitizeFileName(
                upload.filename
            );

        if (filename.length() == 0)
        {
            uploadTargetPath = "";
            uploadOK = false;

            Serial.println(
                "Upload rejected: empty filename."
            );

            return;
        }

        uploadTargetPath =
            imagePath(
                filename
            );

        uploadFile =
            storageFS().open(
                uploadTargetPath,
                "w"
            );

        uploadOK =
            static_cast<bool>(
                uploadFile
            );

        Serial.print(
            "Upload start: "
        );

        Serial.println(
            uploadTargetPath
        );
    }

    else if (
        upload.status ==
        UPLOAD_FILE_WRITE
    )
    {
        if (uploadFile)
        {
            size_t written =
                uploadFile.write(
                    upload.buf,
                    upload.currentSize
                );

            if (
                written !=
                upload.currentSize
            )
            {
                uploadOK = false;
            }
        }
    }

    else if (
        upload.status ==
        UPLOAD_FILE_END
    )
    {
        if (uploadFile)
        {
            uploadFile.close();
        }

        Serial.print(
            "Upload completed: "
        );

        Serial.println(
            upload.totalSize
        );
    }

    else if (
        upload.status ==
        UPLOAD_FILE_ABORTED
    )
    {
        if (uploadFile)
        {
            uploadFile.close();
        }

        uploadOK = false;

        if (
            uploadTargetPath.length()
        )
        {
            storageFS().remove(
                uploadTargetPath
            );
        }
    }
}

void handleUploadComplete()
{
    if (!requireAdmin()) return;

    bool xhr =
        server.header(
            "X-Requested-With"
        ) == "XMLHttpRequest";

    if (uploadOK)
    {
        if (xhr)
        {
            sendJsonResponse(
                200,
                "{\"ok\":true,\"message\":\"Upload complete.\"}"
            );
        }
        else
        {
            server.sendHeader(
                "Location",
                "/files"
            );

            server.send(
                303
            );
        }
    }
    else
    {
        if (xhr)
        {
            sendJsonResponse(
                500,
                "{\"ok\":false,\"error\":\"Upload failed.\"}"
            );
        }
        else
        {
            server.send(
                500,
                "text/plain",
                "Upload failed."
            );
        }
    }
}

void handleViewFile()
{
    if (!requireAdmin()) return;
    String filename =
        sanitizeFileName(
            server.arg("name")
        );

    String path =
        imagePath(
            filename
        );

    if (
        filename.length() == 0 ||
        path.length() == 0
    )
    {
        server.send(
            400,
            "text/plain",
            "Image name missing."
        );

        return;
    }

    if (!storageFS().exists(path))
    {
        server.send(
            404,
            "text/plain",
            "Image not found."
        );

        return;
    }

    File file =
        storageFS().open(
            path,
            "r"
        );

    if (!file)
    {
        server.send(
            500,
            "text/plain",
            "Could not open image."
        );

        return;
    }

    server.streamFile(
        file,
        "image/bmp"
    );

    file.close();
}

void handleDeleteFile()
{
    if (!requireAdmin()) return;
    String filename =
        sanitizeFileName(
            server.arg("name")
        );

    String path =
        imagePath(
            filename
        );

    if (
        filename.length() == 0 ||
        !storageFS().exists(path)
    )
    {
        server.send(
            404,
            "text/plain",
            "Image not found."
        );

        return;
    }

    if (!storageFS().remove(path))
    {
        server.send(
            500,
            "text/plain",
            "Could not delete image."
        );

        return;
    }

    server.sendHeader(
        "Location",
        "/files"
    );

    server.send(
        303
    );
}

bool otaUpdateSuccess = false;

void handleOTA()
{
    if (!requireAdmin()) return;

    HtmlBuffer html;
    html += pageHeader("Firmware Update");

    html += R"rawliteral(
<div class="card">
<h1>Firmware Update (OTA)</h1>
<p class="small">
Upload a compiled firmware binary (.bin). The device will restart automatically.
</p>

<form id="otaForm" enctype="multipart/form-data">
<input type="file" id="otaFile" name="firmware" accept=".bin">
<button type="button" class="green" onclick="uploadFirmware()">
UPDATE FIRMWARE
</button>
</form>

<h2>Update from URL</h2>
<p class="small">
Enter a direct link to a compiled .bin file (e.g. a GitHub release asset).
The device will download and install it automatically.
</p>
<input type="text" id="otaUrl" placeholder="https://github.com/.../TQ-Printer_v2.9.10.bin" style="width:100%;margin-bottom:12px;">
<button type="button" class="blue" onclick="updateFromUrl()">
UPDATE FROM URL
</button>

<div id="otaUrlStatus" class="small"></div>

<div id="otaProgress"
style="width:100%;height:8px;background:#333;border-radius:4px;margin-top:12px;display:none;">
<div id="otaBar"
style="width:0%;height:100%;background:#4caf50;border-radius:4px;transition:width 0.2s;">
</div>
</div>

<div id="otaStatus" class="small"></div>

<script>
function uploadFirmware()
{
    var fileInput = document.getElementById('otaFile');
    var file = fileInput.files[0];
    if (!file)
    {
        document.getElementById('otaStatus').textContent = 'Please select a firmware binary.';
        return;
    }

    var formData = new FormData();
    formData.append('firmware', file);

    var xhr = new XMLHttpRequest();
    var progress = document.getElementById('otaProgress');
    var bar = document.getElementById('otaBar');
    var status = document.getElementById('otaStatus');

    progress.style.display = 'block';
    bar.style.width = '0%';
    status.textContent = 'Uploading firmware...';

    xhr.upload.onprogress = function(e)
    {
        if (e.lengthComputable)
        {
            bar.style.width = (e.loaded / e.total * 100) + '%';
        }
    };

    xhr.onload = function()
    {
        try
        {
            var resp = JSON.parse(xhr.responseText);
            status.textContent = resp.message || resp.error || 'Done.';
        }
        catch (err)
        {
            status.textContent = 'Update finished.';
        }
    };

    xhr.onerror = function()
    {
        status.textContent = 'Update error.';
    };

    xhr.open('POST', '/ota/upload');
    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
    xhr.send(formData);
}

function updateFromUrl()
{
    var urlInput = document.getElementById('otaUrl');
    var url = urlInput.value.trim();
    var status = document.getElementById('otaUrlStatus');

    if (!url)
    {
        status.textContent = 'Please enter a firmware URL.';
        return;
    }

    status.textContent = 'Downloading and installing firmware...';

    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/ota/url');
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');

    xhr.onload = function()
    {
        try
        {
            var resp = JSON.parse(xhr.responseText);
            status.textContent = resp.message || resp.error || 'Done.';
        }
        catch (err)
        {
            status.textContent = xhr.status === 200 ? 'Update finished.' : 'Update error.';
        }
    };

    xhr.onerror = function()
    {
        status.textContent = 'Update error.';
    };

    xhr.send('url=' + encodeURIComponent(url));
}
</script>

</div>
)rawliteral";

    html += pageFooter();

    html.send();
}

void handleOTAUploadData()
{
    if (!requireAdmin()) return;

    HTTPUpload& upload =
        server.upload();

    if (
        upload.status ==
        UPLOAD_FILE_START
    )
    {
        otaUpdateSuccess = true;

        if (
            !Update.begin(
                UPDATE_SIZE_UNKNOWN
            )
        )
        {
            Serial.print(
                "OTA start failed: "
            );

            Serial.println(
                Update.errorString()
            );

            otaUpdateSuccess = false;
        }
    }
    else if (
        upload.status ==
        UPLOAD_FILE_WRITE
    )
    {
        if (otaUpdateSuccess)
        {
            size_t written =
                Update.write(
                    upload.buf,
                    upload.currentSize
                );

            if (written != upload.currentSize)
            {
                otaUpdateSuccess = false;
            }
        }
    }
    else if (
        upload.status ==
        UPLOAD_FILE_END
    )
    {
        if (
            otaUpdateSuccess &&
            Update.end(true)
        )
        {
            Serial.println(
                "OTA update finished."
            );
        }
        else
        {
            otaUpdateSuccess = false;

            Serial.print(
                "OTA end failed: "
            );

            Serial.println(
                Update.errorString()
            );
        }
    }
    else if (
        upload.status ==
        UPLOAD_FILE_ABORTED
    )
    {
        otaUpdateSuccess = false;

        Update.abort();
    }
}

void handleOTAComplete()
{
    if (!requireAdmin()) return;

    bool xhr =
        server.header(
            "X-Requested-With"
        ) == "XMLHttpRequest";

    if (
        otaUpdateSuccess &&
        !Update.hasError()
    )
    {
        String message =
            "OTA update complete. "
            "The device is restarting.";

        if (xhr)
        {
            sendJsonResponse(
                200,
                "{\"ok\":true,\"message\":\"" +
                jsonEscape(message) +
                "\"}"
            );
        }
        else
        {
            server.send(
                200,
                "text/html",
                "<html><body><h2>" +
                htmlEscape(message) +
                "</h2></body></html>"
            );
        }

        delay(500);
        ESP.restart();
    }
    else
    {
        Update.abort();

        String error =
            "OTA update failed: " +
            String(
                Update.errorString()
            );

        if (xhr)
        {
            sendJsonResponse(
                500,
                "{\"ok\":false,\"error\":\"" +
                jsonEscape(error) +
                "\"}"
            );
        }
        else
        {
            server.send(
                500,
                "text/plain",
                error
            );
        }
    }
}

void handleOTAUrl()
{
    if (!requireAdmin()) return;

    String url =
        server.arg("url");

    url.trim();

    if (url.length() == 0)
    {
        sendJsonResponse(
            400,
            "{\"ok\":false,\"error\":\"Firmware URL is required\"}"
        );

        return;
    }

    const bool secure =
        url.startsWith("https://");

    WiFiClientSecure secureClient;
    WiFiClient plainClient;

    secureClient.setInsecure();

    HTTPClient http;

    if (secure)
    {
        http.begin(
            secureClient,
            url
        );
    }
    else
    {
        http.begin(
            plainClient,
            url
        );
    }

    http.setFollowRedirects(
        HTTPC_STRICT_FOLLOW_REDIRECTS
    );

    const int httpCode =
        http.GET();

    if (httpCode != 200)
    {
        http.end();

        sendJsonResponse(
            500,
            "{\"ok\":false,\"error\":\"Download failed, HTTP " +
            String(httpCode) +
            "\"}"
        );

        return;
    }

    int len = http.getSize();

    if (len <= 0)
    {
        http.end();

        sendJsonResponse(
            500,
            "{\"ok\":false,\"error\":\"Could not determine firmware size\"}"
        );

        return;
    }

    if (!Update.begin(len))
    {
        http.end();

        sendJsonResponse(
            500,
            "{\"ok\":false,\"error\":\"" +
            String(Update.errorString()) +
            "\"}"
        );

        return;
    }

    WiFiClient* stream =
        http.getStreamPtr();

    size_t written = 0;
    uint8_t buffer[1024];
    unsigned long lastActivity =
        millis();

    while (
        http.connected() &&
        (len > 0 || len == -1)
    )
    {
        size_t available =
            stream->available();

        if (available)
        {
            int bytesRead =
                stream->readBytes(
                    buffer,
                    min(
                        static_cast<size_t>(1024),
                        available
                    )
                );

            written +=
                Update.write(
                    buffer,
                    bytesRead
                );

            if (len > 0)
            {
                len -= bytesRead;
            }

            lastActivity = millis();
        }
        else
        {
            if (
                millis() - lastActivity >
                30000
            )
            {
                Update.abort();
                http.end();

                sendJsonResponse(
                    500,
                    "{\"ok\":false,\"error\":\"Download timeout\"}"
                );

                return;
            }

            delay(1);
        }
    }

    http.end();

    if (
        Update.end() &&
        !Update.hasError()
    )
    {
        sendJsonResponse(
            200,
            "{\"ok\":true,\"message\":\"OTA update complete. The device is restarting.\"}"
        );

        delay(500);
        ESP.restart();
    }
    else
    {
        Update.abort();

        sendJsonResponse(
            500,
            "{\"ok\":false,\"error\":\"" +
            String(Update.errorString()) +
            "\"}"
        );
    }
}

void handleSaveMDNS()
{
    if (!requireAdmin()) return;
    mdnsHostname =
        sanitizeHostname(
            server.arg(
                "hostname"
            )
        );

    preferences.putString(
        "mdns",
        mdnsHostname
    );

    sendSettingsSavedPage(
        "mDNS saved",
        "mDNS hostname saved",
        "New local address after restart: http://" +
            mdnsHostname +
            ".local",
        true
    );

    delay(1000);
    ESP.restart();
}

void handleOrientationTest()
{
    if (!requireAdmin()) return;
    printerInitialize();

    printerSelectStandardMode();

    printerAlign(1);
    printerBold(true);
    printerSize(2, 2);
    Printer.println("DIRECTION TEST");
    printerSize(1, 1);
    printerBold(false);
    Printer.println(
        printOrientation == 1
            ? "REVERSE"
            : "NORMAL"
    );
    Printer.println();
    Printer.println("FIRST LINE");
    Printer.println("123456789");
    Printer.println("LAST LINE");

    

    printerSelectStandardMode();

    if (autoCut)
    {
        printerCut();
    }
    else
    {
        printerFeed(4);
    }

    Printer.flush();

    server.sendHeader(
        "Location",
        "/settings"
    );

    server.send(
        303
    );
}



void handleSaveWiFi()
{
    if (!requireAdmin()) return;
    String newSSID =
        server.arg("ssid");

    String newPassword =
        server.arg(
            "password"
        );

    newSSID.trim();

    if (
        newSSID.length() == 0
    )
    {
        server.send(
            400,
            "text/plain",
            "SSID missing."
        );

        return;
    }

    wifiSSID =
        newSSID;

    if (
        newPassword.length() > 0
    )
    {
        wifiPassword =
            newPassword;

        preferences.putString(
            "wifiPass",
            wifiPassword
        );
    }

    preferences.putString(
        "wifiSSID",
        wifiSSID
    );

    sendSettingsSavedPage(
        "Wi-Fi saved",
        "Wi-Fi saved",
        "The ESP32 will restart and connect to the selected network.",
        true
    );

    delay(1000);

    ESP.restart();
}

void handleSaveAP()
{
    if (!requireAdmin()) return;
    String newSSID =
        server.arg("ssid");

    String newPassword =
        server.arg(
            "password"
        );

    newSSID.trim();

    if (
        newSSID.length() == 0
    )
    {
        server.send(
            400,
            "text/plain",
            "AP SSID missing."
        );

        return;
    }

    if (
        newPassword.length() < 8
    )
    {
        server.send(
            400,
            "text/plain",
            "AP password must contain at least 8 characters."
        );

        return;
    }

    apSSID =
        newSSID;

    apPassword =
        newPassword;

    preferences.putString(
        "apSSID",
        apSSID
    );

    preferences.putString(
        "apPass",
        apPassword
    );

    sendSettingsSavedPage(
        "AP saved",
        "Fallback AP saved",
        "The ESP32 will restart.",
        true
    );

    delay(1000);

    ESP.restart();
}

void handleSavePrinter()
{
    if (!requireAdmin()) return;
    feedLines =
        constrain(
            server.arg(
                "feed"
            ).toInt(),
            0,
            20
        );

    defaultQRSize =
        constrain(
            server.arg(
                "qrsize"
            ).toInt(),
            1,
            16
        );

    autoCut =
        server.arg(
            "cutter"
        ) == "1";

    uint32_t requestedBaud =
        static_cast<uint32_t>(
            server.arg(
                "baud"
            ).toInt()
        );

    if (
        requestedBaud >= 300 &&
        requestedBaud <= 5000000
    )
    {
        printerBaud =
            requestedBaud;
    }

    printOrientation =
        constrain(
            server.arg(
                "orientation"
            ).toInt(),
            0,
            1
        );

    preferences.putUChar(
        "feed",
        feedLines
    );

    preferences.putUChar(
        "qrSize",
        defaultQRSize
    );

    preferences.putBool(
        "autoCut",
        autoCut
    );

    preferences.putULong(
        "printerBaud",
        printerBaud
    );

    preferences.putUChar(
        "orientation",
        printOrientation
    );

    pageWidth =
        constrain(
            server.arg(
                "pageWidth"
            ).toInt(),
            48,
            576
        );

    pageWidth =
        pageWidth - (
            pageWidth % 8
        );

    preferences.putUShort(
        "pageWidth",
        pageWidth
    );

    Printer.end();
    delay(50);

    Printer.begin(
        printerBaud,
        SERIAL_8N1,
        PRINTER_RX_PIN,
        PRINTER_TX_PIN
    );

    sendSettingsSavedPage(
        "Printer settings saved",
        "Printer settings saved",
        "Printer configuration has been updated.",
        false
    );
}

bool triggerTicket()
{
    return printNextTicket(
        "",
        ""
    );
}

void configureTriggerGPIO()
{
    if (!gpioTriggerEnabled)
    {
        return;
    }

    pinMode(
        triggerGPIO,
        triggerActiveLow
            ? INPUT_PULLUP
            : INPUT_PULLDOWN
    );

    stableButtonState =
        digitalRead(
            triggerGPIO
        );

    lastButtonReading =
        stableButtonState;
}

void handleSaveTrigger()
{
    if (!requireAdmin()) return;
    if (server.hasArg("scannerOnly"))
    {
        scannerTriggerEnabled =
            server.arg("scannerEnabled") == "1";

        scannerMode =
            constrain(
                server.arg("scannerMode").toInt(),
                0,
                2
            );

        scannerPattern =
            server.arg("scanPattern");

        scannerPattern.trim();

        scannerUniqueOnly =
            server.arg("uniqueOnly") == "1";

        preferences.putBool(
            "scanEnabled",
            scannerTriggerEnabled
        );

        preferences.putUChar(
            "scanMode",
            scannerMode
        );

        preferences.putString(
            "scanPattern",
            scannerPattern
        );

        preferences.putBool(
            "scanUnique",
            scannerUniqueOnly
        );
    }
    else
    {
        gpioTriggerEnabled =
            server.arg("gpioEnabled") == "1";

        int requestedPin =
            server.arg("gpioPin").toInt();

        if (
            requestedPin >= 0 &&
            requestedPin <= 48
        )
        {
            triggerGPIO =
                static_cast<uint8_t>(
                    requestedPin
                );
        }

        triggerActiveLow =
            server.arg("activeLow") == "1";

        triggerDebounceMs =
            constrain(
                server.arg("debounce").toInt(),
                0,
                5000
            );

        preferences.putBool(
            "gpioEnabled",
            gpioTriggerEnabled
        );

        preferences.putUChar(
            "gpioPin",
            triggerGPIO
        );

        preferences.putBool(
            "gpioLow",
            triggerActiveLow
        );

        preferences.putULong(
            "gpioDebounce",
            triggerDebounceMs
        );

        configureTriggerGPIO();
    }

    sendSettingsSavedPage(
        "Trigger settings saved",
        "Trigger settings saved",
        "Trigger configuration has been updated.",
        false
    );
}



void handleButton()
{
    if (!gpioTriggerEnabled)
    {
        return;
    }

    bool reading =
        digitalRead(
            triggerGPIO
        );

    if (
        reading !=
        lastButtonReading
    )
    {
        lastButtonChange =
            millis();

        lastButtonReading =
            reading;
    }

    if (
        millis() -
            lastButtonChange >=
            triggerDebounceMs &&
        reading !=
            stableButtonState
    )
    {
        stableButtonState =
            reading;

        bool triggered =
            triggerActiveLow
                ? stableButtonState == LOW
                : stableButtonState == HIGH;

        if (triggered)
        {
            Serial.println(
                "GPIO ticket trigger activated."
            );

            triggerTicket();
        }
    }
}



void setupRoutes()
{
    server.on(
        "/",
        HTTP_GET,
        handleRoot
    );

    server.on(
        "/templates",
        HTTP_GET,
        handleTemplates
    );

    server.on(
        "/files",
        HTTP_GET,
        handleFiles
    );

    server.on(
        "/settings",
        HTTP_GET,
        handleSettings
    );

    server.on(
        "/api/print-next",
        HTTP_POST,
        handlePrintNext
    );

    server.on(
        "/api/print-specific",
        HTTP_POST,
        handlePrintSpecific
    );

    server.on(
        "/api/counter/set",
        HTTP_POST,
        handleSetCounter
    );

    server.on(
        "/api/counter/reset",
        HTTP_POST,
        handleResetCounter
    );

    server.on(
        "/api/wifi/scan",
        HTTP_GET,
        handleWiFiScan
    );

    server.on(
        "/template/save",
        HTTP_POST,
        handleTemplateSave
    );

    server.on(
        "/template/activate",
        HTTP_POST,
        handleTemplateActivate
    );

    server.on(
        "/template/delete",
        HTTP_POST,
        handleTemplateDelete
    );

    server.on(
        "/template/preview",
        HTTP_GET,
        handleTemplatePreview
    );

    server.on(
        "/api/templates",
        HTTP_GET,
        handleApiTemplates
    );

    server.on(
        "/api/template/load",
        HTTP_GET,
        handleApiTemplateLoad
    );

    server.on(
        "/api/template",
        HTTP_POST,
        handleApiTemplateAction
    );

    server.on(
        "/settings/wifi",
        HTTP_POST,
        handleSaveWiFi
    );

    server.on(
        "/settings/ap",
        HTTP_POST,
        handleSaveAP
    );

    server.on(
        "/settings/printer",
        HTTP_POST,
        handleSavePrinter
    );

    server.on(
        "/settings/trigger",
        HTTP_POST,
        handleSaveTrigger
    );

    server.on(
        "/settings/mdns",
        HTTP_POST,
        handleSaveMDNS
    );

    server.on(
        "/printer/orientation-test",
        HTTP_POST,
        handleOrientationTest
    );

    server.on(
        "/upload",
        HTTP_POST,
        handleUploadComplete,
        handleUploadData
    );

    server.on(
        "/file/view",
        HTTP_GET,
        handleViewFile
    );

    server.on(
        "/file/delete",
        HTTP_POST,
        handleDeleteFile
    );

    server.on(
        "/ota",
        HTTP_GET,
        handleOTA
    );

    server.on(
        "/ota/upload",
        HTTP_POST,
        handleOTAComplete,
        handleOTAUploadData
    );

    server.on(
        "/ota/url",
        HTTP_POST,
        handleOTAUrl
    );

    server.onNotFound(
        []()
        {
            server.send(
                404,
                "text/plain",
                "Not found."
            );
        }
    );
}


