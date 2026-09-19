@echo off
setlocal enabledelayedexpansion

REM Flash Thermal-Printer.ino to an ESP32-S3 using the Python helper.
REM Usage:
REM   flash.bat
REM   flash.bat COM3
REM   flash.bat --port COM3

set PYTHON_CMD=py -3
%PYTHON_CMD% --version >nul 2>&1
if errorlevel 1 (
    set PYTHON_CMD=python
    %PYTHON_CMD% --version >nul 2>&1
    if errorlevel 1 (
        echo Python 3 was not found. Please install Python from https://www.python.org/.
        pause
        exit /b 1
    )
)

if "%~1"=="" (
    %PYTHON_CMD% "%~dp0flash.py"
) else (
    %PYTHON_CMD% "%~dp0flash.py" %*
)

if errorlevel 1 (
    echo.
    echo Upload failed.
    pause
    exit /b 1
)

echo.
echo Upload finished.
pause
