# PowerShell build script for SCWiki C++

Write-Host "SCWiki C++ Build Script" -ForegroundColor Cyan
Write-Host "======================" -ForegroundColor Cyan

# Check if we're in the right directory
if (-not (Test-Path .\CMakeLists.txt)) {
    Write-Error "Please run this script from the cpp_stubs directory"
    exit 1
}

# Create build directory if it doesn't exist
if (-not (Test-Path .\build)) {
    New-Item -ItemType Directory -Path .\build | Out-Null
    Write-Host "Created build directory" -ForegroundColor Green
}

# Change to build directory
Set-Location .\build

# Check for CMake
try {
    cmake --version | Out-Null
    Write-Host "CMake found" -ForegroundColor Green
} catch {
    Write-Error "CMake is not installed or not in PATH"
    Write-Host "Download from: https://cmake.org/download/" -ForegroundColor Yellow
    exit 1
}

# Configure with Visual Studio generator
Write-Host "Configuring project..." -ForegroundColor Yellow
cmake .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    exit 1
}

Write-Host "Building project..." -ForegroundColor Yellow
cmake --build . --config Debug

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}

Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executable: .\Debug\scwiki.exe" -ForegroundColor Green

# Copy test data if it exists
if (Test-Path ..\data\wiki.json) {
    Copy-Item ..\data\wiki.json .\Debug\ -Force
    Write-Host "Copied test data to build directory" -ForegroundColor Green
}

Write-Host "" -ForegroundColor Cyan
Write-Host "To run:" -ForegroundColor Cyan
Write-Host "  .\Debug\scwiki.exe" -ForegroundColor Yellow
