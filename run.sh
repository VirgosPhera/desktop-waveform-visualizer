#!/bin/bash
# AetherWave Universal Linux & macOS Launcher
# Creator: Amir (Only-One-Kind)

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 1. Stop any existing instance
killall AetherWave 2>/dev/null
sleep 0.2

# 2. Check if binary exists; if not, auto-build on first run!
EXE=""
if [ -f "$DIR/build/AetherWave" ]; then
    EXE="$DIR/build/AetherWave"
elif [ -f "$DIR/bin/AetherWave" ]; then
    EXE="$DIR/bin/AetherWave"
elif [ -f "$DIR/build/AetherWave.app/Contents/MacOS/AetherWave" ]; then
    EXE="$DIR/build/AetherWave.app/Contents/MacOS/AetherWave"
else
    echo "[AetherWave] First time run detected! Compiling automatically..."
    CPU_CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    cmake -B "$DIR/build" -S "$DIR" && cmake --build "$DIR/build" -j"$CPU_CORES"
    
    if [ -f "$DIR/build/AetherWave" ]; then
        EXE="$DIR/build/AetherWave"
        # If macOS, create .app bundle automatically too
        if [ "$(uname)" = "Darwin" ] && [ -f "$DIR/bundle_macos.sh" ]; then
            bash "$DIR/bundle_macos.sh" >/dev/null 2>&1
        fi
        echo "[AetherWave] Build successful!"
    else
        echo "[AetherWave ERROR] Compilation failed. Please ensure cmake and build tools are installed."
        exit 1
    fi
fi

# 3. Launch in background detached
nohup "$EXE" >/dev/null 2>&1 &
PID=$!
echo "[AetherWave] Running in background! (PID: $PID)"
echo "[AetherWave] Run ./stop.sh anytime to stop."
