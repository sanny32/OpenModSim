param(
    [ValidateSet('x64', 'x86')][string]$Architecture = 'x64',
    [string]$QtToolsRoot = 'C:\Qt'
)

$ErrorActionPreference = 'Stop'
$opensslRoot = Join-Path $QtToolsRoot "Tools\OpenSSLv3\Win_$Architecture"
if (-not (Test-Path (Join-Path $opensslRoot 'include\openssl\opensslv.h'))) {
    if ($Architecture -eq 'x64') {
        python -m aqt install-tool windows desktop tools_opensslv3_x64 --outputdir $QtToolsRoot | Out-Host
        if ($LASTEXITCODE -ne 0) { throw 'Cannot install the Qt OpenSSL 3 package.' }
    } else {
        $sourceRoot = Join-Path $QtToolsRoot 'Tools\OpenSSLv3\src'
        if (-not (Test-Path (Join-Path $sourceRoot 'Configure'))) {
            python -m aqt install-tool windows desktop tools_opensslv3_src --outputdir $QtToolsRoot | Out-Host
            if ($LASTEXITCODE -ne 0) { throw 'Cannot install the OpenSSL 3 sources.' }
        }
        if (-not (Get-Command perl -ErrorAction SilentlyContinue)) {
            throw 'Building OpenSSL 3 for x86 requires Perl in PATH (for example Strawberry Perl).'
        }
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not $vsPath) { throw 'Cannot find the Visual Studio C++ build tools.' }
        $vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars32.bat'
        $buildRoot = Join-Path $sourceRoot 'build-modsim-x86'
        New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
        Push-Location $buildRoot
        try {
            cmd /c "call `"$vcvars`" && perl ..\Configure VC-WIN32 no-asm shared no-tests --prefix=`"$opensslRoot`" && nmake && nmake install_sw" | Out-Host
            if ($LASTEXITCODE -ne 0) { throw 'Cannot build OpenSSL 3 for x86.' }
        } finally {
            Pop-Location
        }
    }
}
if (-not (Test-Path (Join-Path $opensslRoot 'include\openssl\opensslv.h')) -or
    -not (Get-ChildItem (Join-Path $opensslRoot 'bin') -Filter 'libcrypto-*.dll' -ErrorAction SilentlyContinue)) {
    throw "OpenSSL 3 installation is incomplete: $opensslRoot"
}
Write-Output $opensslRoot
