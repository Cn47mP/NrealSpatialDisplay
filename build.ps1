$msbuild = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
$sln = "C:\Users\Cn47mP\Desktop\NrealSpatialDisplay\build\NrealSpatialDisplay.sln"
& $msbuild $sln /p:Configuration=Release /p:Platform=x64