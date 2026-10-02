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

## 3. Code signing

Windows **Smart App Control** and **SmartScreen** block or warn about new
unsigned executables. A signature from a trusted code-signing certificate is
what makes Windows trust the app. A self-signed certificate is **not** enough:
Windows rejects it like an unsigned file.

### Signing a package

```powershell
# Certificate in a .pfx file (password prompted, or $env:MDBR_SIGN_PFX_PASSWORD on CI)
.\deployment\windows\package-windows.ps1 -SignPfx C:\keys\codesign.pfx

# Certificate in the Windows store or on a USB token / smart card
.\deployment\windows\package-windows.ps1 -SignThumbprint <certificate thumbprint>
```

The script:
- checks the certificate first (private key, Code Signing usage, not expired),
  before any packaging work;
- signs **only binaries without a valid signature**: our `.exe` and, for
  MinGW, the GCC runtime DLLs. Qt's DLLs, signed by The Qt Company, and
  Microsoft's runtime DLLs are left untouched;
- uses SHA-256 with an RFC 3161 timestamp (`-TimestampServer`, DigiCert by
  default), so signatures stay valid after the certificate expires;
- verifies every signature and stops if any one is not trusted;
- never installs anything into a certificate store (a `.pfx` is read into memory only).

`-AllowUntrustedSignature` accepts a self-signed test certificate, for testing
the signing step only. Never use it for releases.

### Getting a certificate

All options require verifying your identity with the certificate authority.
Compare current prices and country availability before choosing:

| Option | Notes |
|---|---|
| Microsoft Trusted Signing (Azure) | Monthly subscription; signing happens in the cloud. Check which countries are supported for individual developers |
| Certum "Open Source Code Signing" | Lower-cost certificate for open-source developers |
| SignPath Foundation | Free signing for qualifying open-source projects, done in CI |
| Commercial CAs (DigiCert, Sectigo, SSL.com...) | OV/EV certificates. Private keys must live on a hardware token or cloud HSM, so use `-SignThumbprint` |

After signing, Windows may still need time to build reputation for a new
publisher. The signature is the prerequisite, not an instant guarantee.

## 4. Run

```powershell
.\dist\windows\md-backpack-reader\md-backpack-reader.exe
.\dist\windows\md-backpack-reader\md-backpack-reader.exe .\tests\data\sample.md
```

No `PATH` changes are needed. MSVC packages include the Visual C++ runtime
DLLs; MinGW packages include `libgcc`, `libstdc++` and `libwinpthread`.
