$ErrorActionPreference = "Stop"

$QtDir    = "C:\Qt\6.7.0\msvc2019_64"
$BuildDir = "C:\Users\ismae\Desktop\md\build"
$DistDir  = "C:\Users\ismae\Desktop\md\dist\windows\md-backpack-reader"

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

Write-Output "=== Deployment complete ==="
Write-Output "Output: $DistDir"
Get-ChildItem -LiteralPath $DistDir | Select-Object Name, Length
