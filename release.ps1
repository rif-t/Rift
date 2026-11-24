$ErrorActionPreference = 'Stop'

$root = $PSScriptRoot
$buildDir = Join-Path $root 'build-release'
$releaseDir = Join-Path $root 'release'
$stageDir = Join-Path $releaseDir 'package'

cmake -S $root -B $buildDir -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }

cmake --build $buildDir --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }

$executable = @(
    (Join-Path $buildDir 'Release/rift.exe'),
    (Join-Path $buildDir 'rift.exe')
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $executable) { throw 'Could not find the built rift.exe.' }

New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
Remove-Item -Recurse -Force $stageDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
Copy-Item $executable (Join-Path $stageDir 'rift.exe')
Copy-Item -Recurse (Join-Path $root 'docs') $stageDir
Copy-Item -Recurse (Join-Path $root 'examples') $stageDir
Copy-Item (Join-Path $root 'README.md'), (Join-Path $root 'LICENSE') $stageDir

$archive = Join-Path $releaseDir 'Rift.zip'
$temporaryArchive = Join-Path $releaseDir 'Rift.tmp.zip'
Compress-Archive -Path (Join-Path $stageDir '*') -DestinationPath $temporaryArchive
Move-Item -Force $temporaryArchive $archive
Remove-Item -Recurse -Force $stageDir
Write-Host "Created $archive"
