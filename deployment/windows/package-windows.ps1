<#
.SYNOPSIS
    Packages MD Backpack Reader as a portable Windows folder.

.DESCRIPTION
    Copies the built executable into the dist folder and adds every DLL it
    needs (Qt via windeployqt, plus the compiler runtime) so it runs on a
    clean Windows machine without Qt or Visual Studio installed.

    Every setting is detected from the CMake build folder:
      * Qt location   <- Qt6_DIR in CMakeCache.txt
      * Toolchain     <- the Qt kit name (msvc2019_64 / mingw_64)
      * Executable    <- build\Release\ (Visual Studio) or build\ (Ninja/Make)
    Pass parameters only to override the detection.

    Safety: the old package is moved to "<DistDir>.previous" (replacing an
    older backup) before the new one is written, so one bad build can always
    be rolled back. The new package is smoke-tested (launched once) unless
    -SkipLaunchTest is given.

.EXAMPLE
    .\deployment\windows\package-windows.ps1
    Packages ./build into ./dist/windows/md-backpack-reader.

    Code signing (optional): with -SignPfx or -SignThumbprint, every .exe/.dll
    that does not already carry a valid signature (our executable, compiler
    runtime DLLs) is Authenticode-signed with SHA-256 and timestamped. Files
    already signed by their vendor (Qt, Microsoft) are left untouched. A
    signature from a trusted code-signing certificate is what lets Windows
    Smart App Control and SmartScreen trust the app.

.EXAMPLE
    .\deployment\windows\package-windows.ps1 -BuildDir build-mingw
    Packages a MinGW build folder.

.EXAMPLE
    .\deployment\windows\package-windows.ps1 -SignPfx C:\keys\codesign.pfx
    Packages and signs with a certificate file (the password is asked for,
    or read from the MDBR_SIGN_PFX_PASSWORD environment variable on CI).

.EXAMPLE
    .\deployment\windows\package-windows.ps1 -SignThumbprint 1A2B3C...
    Signs with a certificate already installed in the Windows certificate store
    (e.g. on a USB token / smart card, as most CAs now require).
#>
[CmdletBinding()]
param(
    # CMake build folder (relative to the repository root, or absolute).
    [string]$BuildDir = "build",

    # Output folder for the portable package.
    [string]$DistDir = "dist\windows\md-backpack-reader",

    # Qt kit root, e.g. C:\Qt\6.7.0\msvc2019_64. Detected from the build when empty.
    [string]$QtDir = "",

    # msvc | mingw. Detected from the Qt kit name when empty.
    [ValidateSet("", "msvc", "mingw")]
    [string]$Toolchain = "",

    # Skip launching the packaged app as a final check (e.g. on headless CI).
    [switch]$SkipLaunchTest,

    # --- Code signing (optional) ---
    # A .pfx file containing a code-signing certificate and its private key.
    [string]$SignPfx = "",

    # Password of the .pfx. If omitted: MDBR_SIGN_PFX_PASSWORD, else a prompt.
    [SecureString]$SignPfxPassword,

    # Thumbprint of a code-signing certificate in Cert:\CurrentUser\My or Cert:\LocalMachine\My.
    [string]$SignThumbprint = "",

    # RFC 3161 timestamp server, so signatures stay valid after the certificate
    # expires. Pass "" to skip timestamping (offline tests only).
    [string]$TimestampServer = "http://timestamp.digicert.com",

    # Accept signatures from an untrusted certificate (testing the signing
    # step with a self-signed certificate). Never use for releases.
    [switch]$AllowUntrustedSignature
)

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ExeName  = "md-backpack-reader.exe"

function Resolve-RepoPath([string]$Path) {
    if ([System.IO.Path]::IsPathRooted($Path)) { return $Path }
    return Join-Path $RepoRoot $Path
}

function Write-Step([string]$Message) {
    Write-Output ""
    Write-Output "=== $Message ==="
}

$BuildDir = Resolve-RepoPath $BuildDir
$DistDir  = Resolve-RepoPath $DistDir

# Loads the signing certificate (or returns $null when not signing). Done first,
# so a wrong password or an unusable certificate fails before any packaging.
# The .pfx is read into memory only; nothing is installed in a certificate store.
function Get-SigningCertificate {
    if ($SignPfx -and $SignThumbprint) { throw "Use either -SignPfx or -SignThumbprint, not both." }

    $cert = $null
    if ($SignPfx) {
        $pfxPath = Resolve-RepoPath $SignPfx
        if (-not (Test-Path -LiteralPath $pfxPath)) { throw "Certificate file not found: $pfxPath" }
        $password = $SignPfxPassword
        if (-not $password -and $env:MDBR_SIGN_PFX_PASSWORD) {
            $password = ConvertTo-SecureString $env:MDBR_SIGN_PFX_PASSWORD -AsPlainText -Force
        }
        if (-not $password) { $password = Read-Host "Password for $pfxPath" -AsSecureString }
        try {
            $cert = [System.Security.Cryptography.X509Certificates.X509Certificate2]::new(
                $pfxPath, $password,
                [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::EphemeralKeySet)
        } catch {
            throw "Cannot open $pfxPath (wrong password or not a .pfx file): $($_.Exception.Message)"
        }
    } elseif ($SignThumbprint) {
        $thumb = $SignThumbprint -replace '\s', ''
        $cert = Get-ChildItem -Path "Cert:\CurrentUser\My\$thumb", "Cert:\LocalMachine\My\$thumb" -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (-not $cert) { throw "No certificate with thumbprint $thumb in CurrentUser\My or LocalMachine\My." }
    } else {
        return $null
    }

    if (-not $cert.HasPrivateKey) { throw "The certificate has no private key, so it cannot sign." }
    $codeSigningOid = "1.3.6.1.5.5.7.3.3"
    $ekus = $cert.Extensions | Where-Object { $_ -is [System.Security.Cryptography.X509Certificates.X509EnhancedKeyUsageExtension] }
    if (-not ($ekus.EnhancedKeyUsages | Where-Object { $_.Value -eq $codeSigningOid })) {
        throw "The certificate is not a code-signing certificate (missing Code Signing usage)."
    }
    if ($cert.NotAfter -lt (Get-Date)) { throw "The certificate expired on $($cert.NotAfter)." }
    return $cert
}

$SigningCert = Get-SigningCertificate

# On any failure, remove the half-built staging folder; the installed package
# is only replaced in the last step, so it is never left broken.
$StageDir = $null
trap {
    if ($StageDir -and (Test-Path -LiteralPath $StageDir)) {
        Remove-Item -LiteralPath $StageDir -Recurse -Force -ErrorAction SilentlyContinue
    }
    break
}

# --- 1. Detect Qt kit and toolchain from the build -------------------------
Write-Step "Detecting build configuration"

$cache = Join-Path $BuildDir "CMakeCache.txt"
if (-not (Test-Path -LiteralPath $cache)) {
    throw "No CMakeCache.txt in '$BuildDir'. Configure and build first (see docs/DEPLOYMENT_WINDOWS.md)."
}

if (-not $QtDir) {
    $qtLine = Select-String -LiteralPath $cache -Pattern '^Qt6_DIR:PATH=(.+)$' | Select-Object -First 1
    if (-not $qtLine) { throw "Qt6_DIR not found in $cache. Pass -QtDir explicitly." }
    # Qt6_DIR is <kit>\lib\cmake\Qt6
    $QtDir = (Resolve-Path (Join-Path $qtLine.Matches[0].Groups[1].Value "..\..\..")).Path
}
if (-not (Test-Path -LiteralPath (Join-Path $QtDir "bin\windeployqt.exe"))) {
    throw "windeployqt.exe not found under '$QtDir\bin'."
}

if (-not $Toolchain) {
    $Toolchain = if ((Split-Path $QtDir -Leaf) -match 'mingw') { "mingw" } else { "msvc" }
}

$exeCandidates = @(
    (Join-Path $BuildDir "Release\$ExeName"),  # Visual Studio generator
    (Join-Path $BuildDir $ExeName)              # Ninja / Makefiles
)
$BuiltExe = $exeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $BuiltExe) {
    throw "Release executable not found. Looked in:`n  $($exeCandidates -join "`n  ")"
}

Write-Output "  Qt kit     : $QtDir"
Write-Output "  Toolchain  : $Toolchain"
Write-Output "  Executable : $BuiltExe ($((Get-Item $BuiltExe).LastWriteTime))"
Write-Output "  Output     : $DistDir"

# --- 2. Stage the new package in a temporary folder --------------------------
Write-Step "Staging package"

$StageDir = "$DistDir.staging"
if (Test-Path -LiteralPath $StageDir) { Remove-Item -LiteralPath $StageDir -Recurse -Force }
New-Item -ItemType Directory -Path $StageDir -Force | Out-Null
Copy-Item -LiteralPath $BuiltExe -Destination $StageDir

# Only what a Qt Widgets reader needs: no translations (UI is English),
# no software OpenGL (~20 MB) and no D3D shader compiler (Qt Quick only).
$deployArgs = @("--release", "--no-translations", "--no-opengl-sw", "--no-system-d3d-compiler")
if ($Toolchain -eq "mingw") {
    $deployArgs += "--compiler-runtime"   # libgcc / libstdc++ / libwinpthread
}

# windeployqt locates the MinGW runtime through PATH.
$env:PATH = "$QtDir\bin;$env:PATH"
if ($Toolchain -eq "mingw") {
    $mingwBin = Get-ChildItem -LiteralPath (Join-Path $QtDir "..\..\Tools") -Directory -Filter "mingw*" -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($mingwBin) { $env:PATH = "$($mingwBin.FullName)\bin;$env:PATH" }
}

& (Join-Path $QtDir "bin\windeployqt.exe") @deployArgs (Join-Path $StageDir $ExeName)
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed with exit code $LASTEXITCODE" }

# --- 3. MSVC runtime (windeployqt does not copy it by default) ---------------
if ($Toolchain -eq "msvc") {
    Write-Step "Deploying MSVC runtime DLLs"

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) { throw "vswhere.exe not found; is Visual Studio installed?" }
    $vsPath = & $vswhere -latest -products * -property installationPath
    if (-not $vsPath) { throw "No Visual Studio installation found." }

    $redistRoot = Join-Path $vsPath "VC\Redist\MSVC"
    $crtDir = Get-ChildItem -LiteralPath $redistRoot -Directory |
        Where-Object { $_.Name -match '^\d+\.' } |
        Sort-Object { [version]$_.Name } -Descending |
        ForEach-Object { Get-ChildItem -LiteralPath (Join-Path $_.FullName "x64") -Directory -Filter "Microsoft.VC*.CRT" -ErrorAction SilentlyContinue } |
        Select-Object -First 1
    if (-not $crtDir) { throw "Cannot find Microsoft.VC*.CRT under $redistRoot" }

    foreach ($dll in "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll") {
        Copy-Item -LiteralPath (Join-Path $crtDir.FullName $dll) -Destination $StageDir
        Write-Output "  Deployed: $dll"
    }
}

# --- 3b. Code signing (optional) -----------------------------------------------
if ($SigningCert) {
    Write-Step "Signing binaries"
    Write-Output "  Certificate: $($SigningCert.Subject) (expires $($SigningCert.NotAfter.ToString('yyyy-MM-dd')))"

    # Sign only what is not already validly signed (Qt and Microsoft DLLs are).
    $binaries = Get-ChildItem -LiteralPath $StageDir -Recurse -File -Include *.exe, *.dll
    foreach ($file in $binaries) {
        if ((Get-AuthenticodeSignature -LiteralPath $file.FullName).Status -eq "Valid") { continue }

        $signArgs = @{
            LiteralPath   = $file.FullName
            Certificate   = $SigningCert
            HashAlgorithm = "SHA256"
        }
        if ($TimestampServer) { $signArgs.TimestampServer = $TimestampServer }
        $null = Set-AuthenticodeSignature @signArgs

        # Verify: a trusted certificate must give "Valid"; a test certificate is
        # accepted only with -AllowUntrustedSignature, and only if it is ours.
        $check = Get-AuthenticodeSignature -LiteralPath $file.FullName
        $isOurs = $check.SignerCertificate -and $check.SignerCertificate.Thumbprint -eq $SigningCert.Thumbprint
        if ($check.Status -eq "Valid" -and $isOurs) {
            Write-Output "  Signed: $($file.Name)"
        } elseif ($AllowUntrustedSignature -and $isOurs) {
            Write-Output "  Signed (untrusted test certificate, $($check.Status)): $($file.Name)"
        } else {
            throw "Signing failed for $($file.Name): $($check.Status) - $($check.StatusMessage)"
        }
        if ($TimestampServer -and -not $check.TimeStamperCertificate) {
            throw "Signing of $($file.Name) has no timestamp; the signature would stop being valid when the certificate expires."
        }
    }
}

# --- 4. Verify the staged package --------------------------------------------
Write-Step "Verifying package"

$required = @($ExeName, "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "platforms\qwindows.dll")
if ($Toolchain -eq "mingw") {
    $required += "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"
} else {
    $required += "msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll"
}
foreach ($file in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $StageDir $file))) {
        throw "Verification failed: '$file' is missing from the package."
    }
    Write-Output "  OK: $file"
}

if (-not $SkipLaunchTest) {
    # Launch with a minimal PATH so the test fails if a DLL is missing from the package.
    $savedPath = $env:PATH
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    try {
        $proc = Start-Process -FilePath (Join-Path $StageDir $ExeName) -PassThru
        Start-Sleep -Seconds 4
        $alive = -not $proc.HasExited
        if ($alive) { Stop-Process -Id $proc.Id -Force }
    } catch {
        throw "Launch test failed: $($_.Exception.Message)`n(If Windows Smart App Control blocked it, re-run with -SkipLaunchTest.)"
    } finally {
        $env:PATH = $savedPath
    }
    if (-not $alive) { throw "Launch test failed: the app exited immediately (exit code $($proc.ExitCode))." }
    Write-Output "  OK: app launched with a clean PATH"
}

# --- 5. Swap in the new package, keeping the previous one ---------------------
Write-Step "Installing package"

$PreviousDir = "$DistDir.previous"
if (Test-Path -LiteralPath $DistDir) {
    if (Test-Path -LiteralPath $PreviousDir) { Remove-Item -LiteralPath $PreviousDir -Recurse -Force }
    Move-Item -LiteralPath $DistDir -Destination $PreviousDir
    Write-Output "  Previous package kept at: $PreviousDir"
}
Move-Item -LiteralPath $StageDir -Destination $DistDir

$sizeMb = [math]::Round(((Get-ChildItem -LiteralPath $DistDir -Recurse -File | Measure-Object Length -Sum).Sum / 1MB), 1)
Write-Output ""
Write-Output "Done: $DistDir ($sizeMb MB, $Toolchain)"
