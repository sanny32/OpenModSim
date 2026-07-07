@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "REPO_ROOT=%%~fI"

set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
if not exist "%VSDEVCMD%" (
    echo Visual Studio Developer Command Prompt not found:
    echo   %VSDEVCMD%
    exit /b 1
)

set "ORIGINAL_PATH=%PATH%"
call "%VSDEVCMD%" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
set "PATH=%PATH%;%ORIGINAL_PATH%"

if not defined QT_PREFIX (
    if defined QTDIR (
        if exist "%QTDIR%\bin" set "QT_PREFIX=%QTDIR%"
    )
)

if not defined QT_PREFIX (
    if defined CMAKE_PREFIX_PATH (
        for %%P in ("%CMAKE_PREFIX_PATH:;=" "%") do (
            if not defined QT_PREFIX (
                if exist "%%~fP\bin" set "QT_PREFIX=%%~fP"
                if exist "%%~fP\Qt6Config.cmake" (
                    for %%Q in ("%%~fP\..\..\..") do (
                        if exist "%%~fQ\bin" set "QT_PREFIX=%%~fQ"
                    )
                )
            )
        )
    )
)

if not defined QT_PREFIX (
    for /f "usebackq delims=" %%Q in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem 'C:\Qt' -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' -and [version]$_.Name -ge [version]'6.6.0' } | Sort-Object { [version]$_.Name } -Descending | ForEach-Object { Get-ChildItem $_.FullName -Directory -Filter 'msvc*' -ErrorAction SilentlyContinue | Where-Object { (Test-Path (Join-Path $_.FullName 'bin\qmake.exe')) -or (Test-Path (Join-Path $_.FullName 'bin\Qt6Core.dll')) } | Sort-Object Name -Descending | Select-Object -ExpandProperty FullName } | Select-Object -First 1"`) do (
        if not defined QT_PREFIX set "QT_PREFIX=%%Q"
    )
)

if not defined QT_PREFIX (
    echo Qt was not found. Set QT_PREFIX, QTDIR, CMAKE_PREFIX_PATH, or install Qt under C:\Qt.
    exit /b 1
)

if not exist "%QT_PREFIX%\bin" (
    echo Qt prefix does not contain bin directory:
    echo   %QT_PREFIX%
    exit /b 1
)

set "QT_VERSION_STATUS="
for /f "usebackq delims=" %%V in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "$prefix=$env:QT_PREFIX; $version=$null; $qmake=Join-Path $prefix 'bin\qmake.exe'; if (Test-Path $qmake) { $version=(& $qmake -query QT_VERSION 2>$null) }; if (-not $version) { $match=[regex]::Match($prefix, '(?i)(?:^|[\\/])(\d+\.\d+\.\d+)(?:[\\/]|$)'); if ($match.Success) { $version=$match.Groups[1].Value } }; if (-not $version) { 'UNKNOWN' } elseif ([version]$version -lt [version]'6.6.0') { 'OLD:' + $version } else { 'OK:' + $version }"`) do (
    set "QT_VERSION_STATUS=%%V"
)
if "%QT_VERSION_STATUS%"=="UNKNOWN" (
    echo Unable to determine Qt version for:
    echo   %QT_PREFIX%
    exit /b 1
)
if "%QT_VERSION_STATUS:~0,4%"=="OLD:" (
    echo Qt 6.6 or newer is required. Found Qt %QT_VERSION_STATUS:~4%:
    echo   %QT_PREFIX%
    exit /b 1
)
if not "%QT_VERSION_STATUS:~0,3%"=="OK:" (
    echo Unable to determine Qt version for:
    echo   %QT_PREFIX%
    exit /b 1
)
set "QT_VERSION=%QT_VERSION_STATUS:~3%"
echo Using Qt %QT_VERSION%: %QT_PREFIX%
set "PATH=%QT_PREFIX%\bin;%PATH%"

if defined CMAKE_BIN (
    if not exist "%CMAKE_BIN%\cmake.exe" (
        echo CMAKE_BIN does not contain cmake.exe:
        echo   %CMAKE_BIN%
        exit /b 1
    )
    set "PATH=%CMAKE_BIN%;%PATH%"
) else (
    for /f "usebackq delims=" %%C in (`powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-ChildItem 'C:\Qt\Tools' -Directory -Filter 'CMake_*' -ErrorAction SilentlyContinue | Where-Object { Test-Path (Join-Path $_.FullName 'bin\cmake.exe') } | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'bin' } | Select-Object -First 1"`) do (
        set "PATH=%%C;%PATH%"
        echo Using CMake: %%C\cmake.exe
    )
)

where OpenCppCoverage >nul 2>nul
if errorlevel 1 (
    for %%O in (
        "%ProgramFiles%\OpenCppCoverage"
        "%ProgramFiles(x86)%\OpenCppCoverage"
        "%LOCALAPPDATA%\Programs\OpenCppCoverage"
    ) do (
        if exist "%%~fO\OpenCppCoverage.exe" (
            set "PATH=%%~fO;%PATH%"
            echo Using OpenCppCoverage: %%~fO\OpenCppCoverage.exe
        )
    )
)

if defined PYTHON_EXE (
    if not exist "%PYTHON_EXE%" (
        echo PYTHON_EXE does not exist:
        echo   %PYTHON_EXE%
        exit /b 1
    )
    set PYTHON_CMD="%PYTHON_EXE%"
) else (
    where python3 >nul 2>nul
    if not errorlevel 1 (
        set "PYTHON_CMD=python3"
    ) else (
        where python >nul 2>nul
        if not errorlevel 1 (
            set "PYTHON_CMD=python"
        ) else (
            py -3 --version >nul 2>nul
            if not errorlevel 1 (
                set "PYTHON_CMD=py -3"
            ) else (
                echo Python was not found. Install Python or set PYTHON_EXE to python.exe.
                echo Example:
                echo   set "PYTHON_EXE=C:\Path\To\python.exe"
                echo   tools\coverage_msvc.cmd
                exit /b 1
            )
        )
    )
)

set "BUILD_DIR=%REPO_ROOT%\build-coverage-vs-cl"
set "CMAKE_ARGS="
set CMAKE_ARGS=-- "-DCMAKE_PREFIX_PATH=%QT_PREFIX%"

pushd "%REPO_ROOT%"
%PYTHON_CMD% "%SCRIPT_DIR%coverage.py" --build-dir "%BUILD_DIR%" --config RelWithDebInfo %* %CMAKE_ARGS%
set "RESULT=%ERRORLEVEL%"
popd

exit /b %RESULT%
