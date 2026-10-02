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

.EXAMPLE
    .\deployment\windows\package-windows.ps1 -BuildDir build-mingw
    Packages a MinGW build folder.
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
    [switch]$SkipLaunchTest
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
