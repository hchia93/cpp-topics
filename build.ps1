# 用法:.\build.ps1 <path-to-cpp-file> [--no-run]
#
# 自动激活 MSVC 环境,编译指定 .cpp 到 build\,默认运行一次。
# 同一个 PowerShell 会话里第二次调用不会重复激活。

param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$Source,

    [switch]$NoRun
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path $Source)) {
    Write-Error "Source not found: $Source"
}

# 第一次调用时激活 MSVC 环境:跑 vcvars64.bat 把它注入的环境变量
# 复制回当前 PowerShell 会话。同会话再次调用会跳过这一步。
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    $vcvars = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path $vcvars)) {
        Write-Error "vcvars64.bat not found at: $vcvars"
    }
    $envDump = & cmd.exe /c "`"$vcvars`" >nul 2>&1 && set"
    foreach ($line in $envDump) {
        if ($line -match '^([^=]+)=(.*)$') {
            Set-Item -Path "env:$($Matches[1])" -Value $Matches[2]
        }
    }
}

$repoRoot = $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build'
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$srcFull = (Resolve-Path $Source).Path
$exeName = [System.IO.Path]::GetFileNameWithoutExtension($srcFull) + '.exe'
$exePath = Join-Path $buildDir $exeName

Push-Location $buildDir
try {
    & cl /nologo /std:c++20 /EHsc /O2 /Fe:$exeName $srcFull
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

if (-not $NoRun) {
    Write-Output ''
    Write-Output "--- run: $exeName ---"
    & $exePath
}
