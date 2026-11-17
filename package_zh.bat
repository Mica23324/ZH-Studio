@echo off
setlocal EnableExtensions DisableDelayedExpansion
REM Package into a NEW folder. Usage: package_zh.bat [ZH-Studio-ReleaseName]
set "ROOT=%~dp0"
set "PYTHON=%ROOT%_toolchain\python\cpython-3.13.17-windows-x86_64-none\python.exe"
if not exist "%PYTHON%" (
    echo Project-local Python is missing: %PYTHON%
    exit /b 1
)
if "%~1"=="" (
    "%PYTHON%" "%ROOT%scripts\zh_package.py"
) else (
    "%PYTHON%" "%ROOT%scripts\zh_package.py" "%~1"
)
exit /b %ERRORLEVEL%
