param(
    [ValidateSet('x64', 'x86')][string]$Architecture = 'x64',
    [ValidateSet('1.1', '3')][string]$Version = '3',
    [string]$QtToolsRoot = 'C:\Qt'
)

$ErrorActionPreference = 'Stop'

function Build-OpenSsl {
    param([string]$SourceRoot, [string]$InstallRoot, [string]$TargetArchitecture)

    if (-not (Get-Command perl -ErrorAction SilentlyContinue)) {
        throw 'Building OpenSSL requires Perl in PATH (for example Strawberry Perl).'
    }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Cannot find the Visual Studio C++ build tools.' }
    $vcvarsName = if ($TargetArchitecture -eq 'x86') { 'vcvars32.bat' } else { 'vcvars64.bat' }
    $configureTarget = if ($TargetArchitecture -eq 'x86') { 'VC-WIN32' } else { 'VC-WIN64A' }
    $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\$vcvarsName"
    $buildRoot = Join-Path $SourceRoot "build-modsim-$TargetArchitecture"
    New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
    Push-Location $buildRoot
    try {
        cmd /c "call `"$vcvars`" && perl ..\Configure $configureTarget no-asm shared no-tests --prefix=`"$InstallRoot`" && nmake && nmake install_sw" | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Cannot build OpenSSL $Version for $TargetArchitecture." }
    } finally {
        Pop-Location
    }
}

$packageName = if ($Version -eq '1.1') { 'OpenSSL11' } else { 'OpenSSLv3' }
$opensslRoot = Join-Path $QtToolsRoot "Tools\$packageName\Win_$Architecture"
if (-not (Test-Path (Join-Path $opensslRoot 'include\openssl\opensslv.h'))) {
    if ($Version -eq '1.1') {
        $sourceRoot = & python (Join-Path $PSScriptRoot 'fetch_openssl11.py') (Join-Path $QtToolsRoot 'Tools\OpenSSL11\src')
        if ($LASTEXITCODE -ne 0) { throw 'Cannot download the OpenSSL 1.1 sources.' }
        Build-OpenSsl -SourceRoot $sourceRoot -InstallRoot $opensslRoot -TargetArchitecture $Architecture
    } elseif ($Architecture -eq 'x64') {
        python -m aqt install-tool windows desktop tools_opensslv3_x64 --outputdir $QtToolsRoot | Out-Host
        if ($LASTEXITCODE -ne 0) { throw 'Cannot install the Qt OpenSSL 3 package.' }
    } else {
        $sourceRoot = Join-Path $QtToolsRoot 'Tools\OpenSSLv3\src'
        if (-not (Test-Path (Join-Path $sourceRoot 'Configure'))) {
            python -m aqt install-tool windows desktop tools_opensslv3_src --outputdir $QtToolsRoot | Out-Host
            if ($LASTEXITCODE -ne 0) { throw 'Cannot install the OpenSSL 3 sources.' }
        }
        Build-OpenSsl -SourceRoot $sourceRoot -InstallRoot $opensslRoot -TargetArchitecture $Architecture
    }
}
if (-not (Test-Path (Join-Path $opensslRoot 'include\openssl\opensslv.h')) -or
    -not (Get-ChildItem (Join-Path $opensslRoot 'bin') -Filter 'libcrypto-*.dll' -ErrorAction SilentlyContinue) -or
    -not (Get-ChildItem (Join-Path $opensslRoot 'bin') -Filter 'libssl-*.dll' -ErrorAction SilentlyContinue)) {
    throw "OpenSSL $Version installation is incomplete: $opensslRoot"
}
Write-Output $opensslRoot
