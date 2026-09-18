@echo off
setlocal
echo [AetherWave] Initializing Visual Studio Build Tools...
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" > nul

taskkill /F /IM AetherWave.exe 2>nul

if not exist "bin" mkdir bin
if not exist "config" mkdir config

echo [AetherWave] Compiling C++20 source files...
cl /std:c++20 /EHsc /O2 /W3 /DNOMINMAX /Iinclude ^
   src\main.cpp src\dsp.cpp src\audio_capture.cpp src\color_mixer.cpp src\renderer.cpp src\window.cpp src\config_manager.cpp ^
   /Fe:bin\AetherWave.exe ^
   /link /SUBSYSTEM:WINDOWS ^
   user32.lib gdi32.lib shell32.lib ole32.lib d2d1.lib

if %ERRORLEVEL% EQU 0 (
    echo [AetherWave] Build SUCCEEDED! Binary output: bin\AetherWave.exe
) else (
    echo [AetherWave] Build FAILED! Error code: %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
