$ErrorActionPreference = "Continue"
$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
$sln = "C:\Users\Cn47mP\Desktop\NrealSpatialDisplay\build\NrealSpatialDisplay.sln"
$out = "C:\Users\Cn47mP\Desktop\NrealSpatialDisplay\build\build.log"

Write-Host "Building NrealSpatialDisplay..."
& $msbuild $slin /p:Configuration=Release /p:Platform=x64 /v:minimal 2>&1 | Tee-Object -FilePath $out
$exitCode = $LASTEXITCODE
Write-Host "Exit code: $exitCode"
if (Test-Path $out) { Get-Content $out | Select-Object -Last 50 }
exit $exitCode