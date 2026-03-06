# Build script for The Sphere Game on Windows
# Usage: .\build.ps1 [debug|release]
# Requires: Visual Studio 2022+ (with C++ Desktop Development) and vcpkg

param(
    [string]$BuildType = "debug"
)

$ErrorActionPreference = "Stop"

Write-Host "Building The Sphere Game ($BuildType)..." -ForegroundColor Green

# Validate build type
if ($BuildType -notmatch "^(debug|release)$") {
    Write-Host "Invalid build type: $BuildType. Use 'debug' or 'release'." -ForegroundColor Red
    exit 1
}

$BuildTypeCapitalized = [char]::ToUpper($BuildType[0]) + $BuildType.Substring(1)

# Auto-detect vcpkg location
$VcpkgPath = $null
$VcpkgCandidates = @(
    "C:\vcpkg",
    "$env:USERPROFILE\vcpkg",
    "$env:USERPROFILE\Development\vcpkg",
    ".\vcpkg"
)

foreach ($candidate in $VcpkgCandidates) {
    if (Test-Path "$candidate\scripts\buildsystems\vcpkg.cmake") {
        $VcpkgPath = $candidate
        Write-Host "Found vcpkg at: $VcpkgPath" -ForegroundColor Yellow
        break
    }
}

if (!$VcpkgPath) {
    Write-Host "ERROR: vcpkg not found. Install it with:" -ForegroundColor Red
    Write-Host "  git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg" -ForegroundColor Gray
    Write-Host "  cd C:\vcpkg && .\bootstrap-vcpkg.bat" -ForegroundColor Gray
    exit 1
}

# Check if CMake is available
$CmakePath = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (!$CmakePath) {
    Write-Host "ERROR: CMake not found in PATH. Install it and add to PATH, or download from:" -ForegroundColor Red
    Write-Host "  https://cmake.org/download/" -ForegroundColor Gray
    exit 1
}
Write-Host "Using CMake: $CmakePath" -ForegroundColor Yellow

# Create build directory
if (!(Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Run CMake
Write-Host "`n[1/2] Running CMake..." -ForegroundColor Cyan
Push-Location "build" -ErrorAction Stop

# Convert path for CMake
$VcpkgToolchain = "$VcpkgPath/scripts/buildsystems/vcpkg.cmake" -replace '\\', '/'

# Configure with Visual Studio generator (try newest first)
cmake -G "Visual Studio 18 2026" -A x64 -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" -DCMAKE_BUILD_TYPE=$BuildTypeCapitalized ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "VS 2026 generator not found, trying VS 2022..." -ForegroundColor Yellow
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="$VcpkgToolchain" -DCMAKE_BUILD_TYPE=$BuildTypeCapitalized ..
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "No Visual Studio generator found. Make sure Visual Studio 2022 or later is installed with C++ tools." -ForegroundColor Red
        Pop-Location
        exit 1
    }
}

# Build
Write-Host "`n[2/2] Building..." -ForegroundColor Cyan
cmake --build . --config $BuildTypeCapitalized --parallel 4

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed." -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location

# Find executable
$ExePath = "build\$BuildTypeCapitalized\the_sphere_game.exe"

Write-Host "`nBuild complete!" -ForegroundColor Green
if (Test-Path $ExePath) {
    Write-Host "Executable: $ExePath" -ForegroundColor Green
    Write-Host "`nRun with: .\$ExePath" -ForegroundColor Yellow
} else {
    Write-Host "WARNING: Executable not found at $ExePath" -ForegroundColor Yellow
}
