# The Sphere Game

A cross-platform C++20 game engine with OpenGL rendering support. Currently targeting Windows and macOS.

## Project Overview

- **Language**: C++20
- **Rendering**: OpenGL 3.0+
- **Build System**: CMake 3.20+
- **Platforms**: Windows, macOS (Linux planned)

## Prerequisites

### Windows

#### 1. Visual Studio 2022 or Later
- Download: https://visualstudio.microsoft.com/downloads/
- Install with "Desktop development with C++" workload
- Required components:
  - MSVC (C++ compiler)
  - Windows SDK (10.0+)

#### 2. CMake
- Download: https://cmake.org/download/
- Or install via scoop/chocolatey:
  ```powershell
  scoop install cmake
  ```
- Ensure it's in PATH or installed to `C:\Program Files\CMake`

#### 3. vcpkg (Dependency Manager)
```powershell
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
```

### macOS

#### 1. Xcode Command Line Tools
```bash
xcode-select --install
```

#### 2. Homebrew
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 3. Dependencies via Homebrew
```bash
brew install cmake zlib
```

## Build Instructions

### Windows

Navigate to the project directory and run:

```powershell
# Debug build
.\build.ps1 debug

# Release build
.\build.ps1 release
```

The build script automatically:
- Detects Visual Studio (2026, 2022, or older)
- Locates vcpkg installation
- Configures CMake with appropriate toolchain
- Builds the project with parallel compilation
- Copies assets and shaders

**Output**: `build/Debug/the_sphere_game.exe` or `build/Release/the_sphere_game.exe`

### macOS

Navigate to the project directory and run:

```bash
# Debug build
./build.sh debug

# Release build
./build.sh release
```

The build script:
- Uses CMake with Unix Makefiles generator
- Links against system frameworks (Cocoa, OpenGL, IOKit, CoreVideo)
- Copies assets and shaders automatically

**Output**: `build/Debug/the_sphere_game` or `build/Release/the_sphere_game`

## Troubleshooting

### Windows

#### CMake not found
- Install CMake from https://cmake.org/download/
- Add to PATH: `C:\Program Files\CMake\bin`

#### vcpkg not found
The script searches these locations:
- `C:\vcpkg`
- `%USERPROFILE%\vcpkg`
- `%USERPROFILE%\Development\vcpkg`

If vcpkg is elsewhere, clone it to one of these locations.

#### Visual Studio not found
Ensure Visual Studio 2022 or later is installed with C++ tools:
```powershell
# Check for Visual Studio installations
Get-CimInstance MSFT_VSInstance -Namespace root/cimv2/vs | Select-Object InstallLocation, Version
```

#### Build failures with GL errors
Regenerate the build files by deleting `build/` directory and rebuilding:
```powershell
Remove-Item -Recurse build/
.\build.ps1 debug
```

### macOS

#### Xcode tools not installed
```bash
xcode-select --install
```

#### Homebrew cmake/zlib not found
```bash
brew install cmake zlib
```

#### Permission denied on build.sh
```bash
chmod +x build.sh
./build.sh debug
```

## Project Structure

```
the-sphere-game/
├── src/
│   ├── core/              # Engine core (animation, engine, timing)
│   ├── game/              # Game logic (player, main, game)
│   ├── renderer/          # OpenGL rendering system
│   ├── scene/             # Scene management
│   ├── debug/             # Debug utilities
│   ├── platform/          # Platform-specific code
│   │   ├── mac/           # macOS window management (Cocoa/Objective-C++)
│   │   └── windows/       # Windows window management (Win32 API)
│   ├── platform.h         # Platform abstraction layer
│   ├── config.h           # Configuration constants
│   ├── types.h            # Type definitions
│   └── main.cpp           # Entry point
├── assets/                # Game assets (sprites, fonts, scenes)
├── build.ps1              # Windows build script (PowerShell)
├── build.sh               # macOS/Linux build script (Bash)
├── CMakeLists.txt         # CMake configuration
└── README.md              # This file
```

## Dependencies

### Windows (via vcpkg)
- **ZLIB**: PNG image decompression
- **GLEW**: OpenGL extension loading

Install with:
```powershell
cd C:\vcpkg
.\vcpkg install zlib:x64-windows glew:x64-windows
```

### macOS (system/Homebrew)
- **Cocoa Framework**: Window management
- **OpenGL Framework**: Rendering
- **zlib**: PNG image decompression (system library)
- **IOKit Framework**: Hardware input
- **CoreVideo Framework**: Video timing

## Development

### Code Style
- C++20 standard
- Cross-platform code in `src/` (no platform-specific includes)
- Platform-specific code uses `#ifdef` guards or separate files in `src/platform/`

### Adding Dependencies

#### Windows (vcpkg)
```bash
cd C:\vcpkg
.\vcpkg install <package>:x64-windows
```

Update `CMakeLists.txt`:
```cmake
find_package(<Package> CONFIG REQUIRED)
target_link_libraries(the_sphere_game PRIVATE <Package>::<Package>)
```

#### macOS (Homebrew)
```bash
brew install <package>
```

Update `CMakeLists.txt` to find and link the package.

## Building for Release

### Windows
```powershell
.\build.ps1 release
```
Output: `build/Release/the_sphere_game.exe`

### macOS
```bash
./build.sh release
```
Output: `build/Release/the_sphere_game`

## Contributing

When adding features or fixes:
1. Keep platform-agnostic code in `src/`
2. Use platform abstraction headers (`platform.h`, `opengl_compat.h`)
3. Add platform-specific code only in `src/platform/<platform>/`
4. Test builds on both Windows and macOS before committing
