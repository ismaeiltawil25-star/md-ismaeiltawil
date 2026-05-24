# Windows Deployment

## Prerequisites

- Qt 6.7.0 (MSVC 2019 64-bit) installed at `C:\Qt\6.7.0\msvc2019_64`
- Visual Studio 2022 build tools
- CMake

## Build

```powershell
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"
cmake --build build --config Release
```

## Package

```powershell
.\deployment\windows\package-windows.ps1
```

This creates a portable folder at `dist/windows/md-backpack-reader/` containing
the executable and all required Qt DLLs.

## Run

Launch the deployed executable directly from Explorer or PowerShell — no
manual PATH modification needed:

```powershell
.\dist\windows\md-backpack-reader\md-backpack-reader.exe
```
