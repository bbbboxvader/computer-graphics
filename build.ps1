param(
    [ValidateSet('Debug','Release')][string]$Configuration='Debug',
    [switch]$Test
)
$ErrorActionPreference='Stop'
$projectDirectory=$PSScriptRoot
$candidates=@(
    "${env:ProgramFiles}/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe",
    "${env:ProgramFiles}/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
)
$msbuild=$candidates | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
if (-not $msbuild) {
    $command=Get-Command MSBuild.exe -ErrorAction SilentlyContinue
    if($command){$msbuild=$command.Source}
}
if (-not $msbuild) {throw 'Visual Studio C++ workload / MSBuild is required.'}
& $msbuild (Join-Path $projectDirectory 'Coordinates18_23.sln') /m:1 /nr:false /v:minimal "/p:Configuration=$Configuration" /p:Platform=x64 /p:PreferredToolArchitecture=x64 /nologo
if($LASTEXITCODE -ne 0){throw "Build failed: $LASTEXITCODE"}
if($Test) {
    foreach($number in 18..23) {
        $exe=Join-Path $projectDirectory "bin/x64/$Configuration/Practice$number/Practice$number.exe"
        & $exe --self-test
        if($LASTEXITCODE -ne 0){throw "Practice$number self-test failed"}
        & $exe --smoke-test --frames 120
        if($LASTEXITCODE -ne 0){throw "Practice$number OpenGL smoke-test failed"}
    }
}
