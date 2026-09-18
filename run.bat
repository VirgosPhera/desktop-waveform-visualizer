@echo off
taskkill /F /IM AetherWave.exe 2>nul
timeout /t 1 /nobreak >nul
start "" "%~dp0bin\AetherWave.exe"
echo [AetherWave] Running in background! Press Ctrl+Shift+V to toggle visibility.
