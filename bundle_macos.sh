#!/bin/bash
# Builds a standalone macOS .app bundle
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP_DIR="$DIR/build/AetherWave.app"
CONTENTS="$APP_DIR/Contents"
MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"

mkdir -p "$MACOS" "$RESOURCES"

# Copy binary
if [ -f "$DIR/build/AetherWave" ]; then
    cp "$DIR/build/AetherWave" "$MACOS/AetherWave"
elif [ -f "$DIR/bin/AetherWave" ]; then
    cp "$DIR/bin/AetherWave" "$MACOS/AetherWave"
fi

# Create Info.plist
cat <<EOF > "$CONTENTS/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>AetherWave</string>
    <key>CFBundleIdentifier</key>
    <string>com.onlyonekind.aetherwave</string>
    <key>CFBundleName</key>
    <string>AetherWave</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.1.0</string>
    <key>LSUIElement</key>
    <true/>
</dict>
</plist>
EOF

echo "[AetherWave] macOS App Bundle created at: $APP_DIR"
echo "You can double-click it in Finder or run: open $APP_DIR"
