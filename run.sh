#!/bin/bash
# Stop any existing instance
killall AetherWave 2>/dev/null
sleep 0.2

# Find executable
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -f "$DIR/build/AetherWave" ]; then
    EXE="$DIR/build/AetherWave"
elif [ -f "$DIR/bin/AetherWave" ]; then
    EXE="$DIR/bin/AetherWave"
else
    EXE="AetherWave"
fi

# Run in background detached
nohup "$EXE" >/dev/null 2>&1 &
echo "[AetherWave] Started in background! Process PID: $!"
