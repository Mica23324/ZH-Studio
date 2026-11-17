@echo off
setlocal EnableExtensions DisableDelayedExpansion
REM ZH-Studio: all build tools are resolved from the project, never installed system-wide.
REM Usage: build_zh.bat [deps^|app^|all]
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "TC=%ROOT%\_toolchain"
set "STEP=%~1"
if "%STEP%"=="" set "STEP=all"
if /I "%STEP%"=="app" goto :VALIDATE
if /I "%STEP%"=="deps" goto :VALIDATE
if /I "%STEP%"=="all" goto :VALIDATE
echo Unknown build step: %STEP%
echo Usage: build_zh.bat [deps^|app^|all]
exit /b 2

:VALIDATE
for %%F in ("%TC%\msvc\setup_x64.bat" "%TC%\cmake\bin\cmake.exe" "%TC%\ninja\ninja.exe" "%TC%\perl\perl\bin\perl.exe") do (
    if not exist "%%~F" (
        echo Missing project-local build tool: %%~F
        exit /b 1
    )
)
call "%TC%\msvc\setup_x64.bat"
if errorlevel 1 exit /b 1
set "PATH=%TC%\node;%TC%\cmake\bin;%TC%\ninja;%TC%\perl\perl\bin;%PATH%"
set "WindowsSdkDir=%TC%\msvc\Windows Kits\10\"
set "CC=cl"
set "CXX=cl"
if not defined CMAKE_BUILD_PARALLEL_LEVEL set "CMAKE_BUILD_PARALLEL_LEVEL=%NUMBER_OF_PROCESSORS%"
set "DEPS_BUILD=%ROOT%\deps\build_zh"
set "DEPS_DEST=%DEPS_BUILD%\destdir"
set "APP_BUILD=%ROOT%\build_zh"
if /I "%STEP%"=="app" goto :APP

:DEPS
echo ===== ZH-Studio: building dependencies =====
"%TC%\cmake\bin\cmake.exe" -S "%ROOT%\deps" -B "%DEPS_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%TC%\ninja\ninja.exe" -DDESTDIR="%DEPS_DEST%" -DDEP_DEBUG=OFF
if errorlevel 1 exit /b 1
"%TC%\cmake\bin\cmake.exe" --build "%DEPS_BUILD%" --target deps
if errorlevel 1 exit /b 1
if /I "%STEP%"=="deps" goto :DONE

:APP
echo ===== ZH-Studio: building application =====
if not exist "%DEPS_DEST%\usr\local\include" (
    echo Dependencies are missing. Run build_zh.bat deps first.
    exit /b 1
)
"%TC%\cmake\bin\cmake.exe" -S "%ROOT%" -B "%APP_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%TC%\ninja\ninja.exe" -DCMAKE_PREFIX_PATH="%DEPS_DEST%\usr\local" -DBBL_RELEASE_TO_PUBLIC=1 -DBBL_INTERNAL_TESTING=0
if errorlevel 1 exit /b 1
"%TC%\cmake\bin\cmake.exe" --build "%APP_BUILD%"
if errorlevel 1 exit /b 1

:DONE
echo ===== ZH-Studio: build finished =====
exit /b 0
