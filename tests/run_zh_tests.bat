@echo off
setlocal
set ROOT=%~dp0..
set PYTHON=%ROOT%\_toolchain\python\cpython-3.13.17-windows-x86_64-none\python.exe
set NODE=%ROOT%\_toolchain\node\node.exe
call "%ROOT%\_toolchain\msvc\setup_x64.bat"
cl /nologo /std:c++17 /EHsc "%~dp0zh_cost_model.cpp" /Fo"%ROOT%\_toolchain\zh_cost_model.obj" /Fe"%ROOT%\_toolchain\zh_cost_model_test.exe" || exit /b 1
"%ROOT%\_toolchain\zh_cost_model_test.exe" || exit /b 1
cl /nologo /std:c++17 /EHsc "%~dp0zh_motion.cpp" /Fo"%ROOT%\_toolchain\zh_motion.obj" /Fe"%ROOT%\_toolchain\zh_motion_test.exe" || exit /b 1
"%ROOT%\_toolchain\zh_motion_test.exe" || exit /b 1
cl /nologo /std:c++17 /EHsc "%~dp0zh_network_policy.cpp" /Fo"%ROOT%\_toolchain\zh_network_policy.obj" /Fe"%ROOT%\_toolchain\zh_network_policy_test.exe" || exit /b 1
for %%M in (default cli config both threads) do "%ROOT%\_toolchain\zh_network_policy_test.exe" %%M || exit /b 1
"%PYTHON%" "%~dp0zh_encoding_test.py" || exit /b 1
"%PYTHON%" "%~dp0zh_assets_test.py" || exit /b 1
"%PYTHON%" "%~dp0zh_package_test.py" || exit /b 1
"%NODE%" "%~dp0zh_web_test.cjs" || exit /b 1
echo ===== ZH-Studio: all regression tests passed =====
exit /b 0