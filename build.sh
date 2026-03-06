#!/bin/bash
# Build script für The Sphere Game - creates proper macOS .app bundle with signing

set -e  # Exit on error

BUILD_TYPE="Debug"
if [ "$1" == "release" ]; then
    BUILD_TYPE="Release"
fi

echo "Building The Sphere Game ($BUILD_TYPE)..."

# Build with CMake in build subdirectory
mkdir -p build/$BUILD_TYPE
cd build/$BUILD_TYPE
if [ "$BUILD_TYPE" == "Release" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release ../..
else
    cmake -DCMAKE_BUILD_TYPE=Debug ../..
fi
make -j$(sysctl -n hw.ncpu)
cd ../..

# Create .app bundle structure
APP_NAME="The Sphere Game"
BUNDLE_PATH="build/$BUILD_TYPE/The Sphere Game.app"
CONTENTS_PATH="$BUNDLE_PATH/Contents"
MACOS_PATH="$CONTENTS_PATH/MacOS"
RESOURCES_PATH="$CONTENTS_PATH/Resources"

echo "Creating .app bundle..."
mkdir -p "$MACOS_PATH"
mkdir -p "$RESOURCES_PATH"

# Copy executable
cp "build/$BUILD_TYPE/the_sphere_game" "$MACOS_PATH/the_sphere_game"

# Copy assets and shaders to Resources
cp -r build/$BUILD_TYPE/assets "$RESOURCES_PATH/"
cp -r build/$BUILD_TYPE/shaders "$RESOURCES_PATH/"

# Create Info.plist
cat > "$CONTENTS_PATH/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>the_sphere_game</string>
    <key>CFBundleIdentifier</key>
    <string>com.michaelluckgen.spheregame</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>The Sphere Game</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright 2026. All rights reserved.</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>
EOF

echo "✓ Signing app bundle..."
codesign -s - -f "$BUNDLE_PATH"

echo "✓ Build complete!"
echo "✓ App bundle created: build/$BUILD_TYPE/The Sphere Game.app"
echo ""
echo "Run with:"
echo "  open \"build/$BUILD_TYPE/The Sphere Game.app\""
