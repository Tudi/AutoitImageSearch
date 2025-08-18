@echo off
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo Running take_screenshots.py with admin rights...
cd /d "%~dp0"
python take_screenshots.py
pause
