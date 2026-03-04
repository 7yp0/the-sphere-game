$buildDir = "build"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Name $buildDir }

Set-Location $buildDir

$buildType = "Debug"
if ($args[0] -eq "release") { $buildType = "Release" }

cmake -DCMAKE_BUILD_TYPE=$buildType ..
cmake --build . --config $buildType
