$ErrorActionPreference = "Stop"

$QtDir    = "C:\Qt\6.7.0\msvc2019_64"
$BuildDir = "C:\Users\ismae\Desktop\md\build"
$DistDir  = "C:\Users\ismae\Desktop\md\dist\windows\md-backpack-reader"
$VSRoot   = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC"

Write-Output "=== Cleaning dist folder ==="
if (Test-Path -LiteralPath $DistDir) {
    Remove-Item -LiteralPath $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

Write-Output "=== Copying release executable ==="
Copy-Item -LiteralPath "$BuildDir\Release\md-backpack-reader.exe" -Destination $DistDir

Write-Output "=== Running windeployqt ==="
$windeployqt = "$QtDir\bin\windeployqt.exe"
& $windeployqt --release "$DistDir\md-backpack-reader.exe"
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

Write-Output "=== Deploying MSVC runtime DLLs ==="
$latestMsvc = Get-ChildItem -LiteralPath $VSRoot -Directory |
    Where-Object { $_.Name -match '^\d+\.' } |
    Sort-Object { [version]$_.Name } -Descending |
    Select-Object -First 1
if (-not $latestMsvc) {
    throw "Cannot find versioned MSVC redist directory under $VSRoot"
}
$crtDir = Join-Path $latestMsvc.FullName "x64\Microsoft.VC143.CRT"
if (-not (Test-Path -LiteralPath $crtDir)) {
    throw "Cannot find CRT directory: $crtDir"
}

$runtimeDlls = @("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")
foreach ($dll in $runtimeDlls) {
    $src = Join-Path $crtDir $dll
    if (-not (Test-Path -LiteralPath $src)) {
        throw "Missing runtime DLL: $src"
    }
    Copy-Item -LiteralPath $src -Destination $DistDir
    Write-Output "  Deployed: $dll"
}

Write-Output "=== Verifying required runtime DLLs ==="
foreach ($dll in $runtimeDlls) {
    $dst = Join-Path $DistDir $dll
    if (-not (Test-Path -LiteralPath $dst)) {
        throw "Verification failed: $dll not found in dist"
    }
    Write-Output "  Verified: $dll"
}

Write-Output "=== Deployment complete ==="
Write-Output "Output: $DistDir"
Get-ChildItem -LiteralPath $DistDir | Select-Object Name, Length
