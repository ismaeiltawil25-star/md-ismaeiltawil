# Windows Deployment

The packaging script turns a CMake Release build into a portable folder that
runs on a clean Windows machine without Qt or a compiler installed.

## 1. Build (Release)

MSVC (the usual toolchain):

```powershell
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"
cmake --build build --config Release
```

MinGW (alternative):

```powershell
cmake -B build-mingw -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\mingw_64"
cmake --build build-mingw
```

## 2. Package

```powershell
.\deployment\windows\package-windows.ps1                 # packages .\build
.\deployment\windows\package-windows.ps1 -BuildDir build-mingw
```

You don't need to edit any paths. The script detects everything from the build folder:

| Setting | Source |
|---|---|
| Qt location | `Qt6_DIR` in `CMakeCache.txt` |
| Toolchain (msvc / mingw) | Qt kit folder name |
| Executable | `build\Release\` (Visual Studio) or `build\` (Ninja) |
| MSVC runtime DLLs | Latest Visual Studio found by `vswhere` |

Optional parameters: `-DistDir`, `-QtDir`, `-Toolchain msvc|mingw`, `-SkipLaunchTest`.

The script then:
1. stages the package in `<DistDir>.staging`;
2. runs `windeployqt` without translations, software OpenGL or the D3D shader
   compiler, which a Widgets app doesn't need (the package is about 30 MB
   instead of 55 MB);
3. checks that the required DLLs are present;
4. launches the app once with a minimal `PATH`, so a missing DLL fails the
   script instead of a user's first launch;
5. moves the old package to `<DistDir>.previous` and installs the new one.

**Roll back:** delete `dist\windows\md-backpack-reader` and rename
`md-backpack-reader.previous` back to that name.

**Smart App Control:** if Windows blocks the freshly built executable during
the launch test, re-run with `-SkipLaunchTest` and start the app manually.

## 3. Run

```powershell
.\dist\windows\md-backpack-reader\md-backpack-reader.exe
.\dist\windows\md-backpack-reader\md-backpack-reader.exe .\tests\data\sample.md
```

No `PATH` changes are needed. MSVC packages include the Visual C++ runtime
DLLs; MinGW packages include `libgcc`, `libstdc++` and `libwinpthread`.
