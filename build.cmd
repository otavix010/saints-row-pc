@echo off
rem Xbox 360 port: configures (first time) and builds the release with the portable LLVM and the ReXGlue SDK.
rem The same in every port (x360-ports\comum, copied by sincronizar.py).
rem Usage: build.cmd [release|debug]
setlocal
set "CFG=%~1"
if "%CFG%"=="" set "CFG=release"
set "ROOT=%~dp0"
set "LLVM=%ROOT%..\llvm\clang+llvm-23.1.3-x86_64-pc-windows-msvc\bin"
rem the SDK built with AMD FidelityFX (sdk-fidelityfx.cmd) when it is there, else the official nightly
set "SDK=%ROOT%..\rexglue-sdk-ffx\win-amd64"
if not exist "%SDK%\bin\rexglue.exe" set "SDK=%ROOT%..\rexglue-sdk\win-amd64"
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath`) do set "VS=%%i"
call "%VS%\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 >nul || exit /b 1
set "PATH=%LLVM%;%PATH%"
cd /d "%ROOT%"
if not exist "out\build\win-amd64-%CFG%\build.ninja" (
    cmake --preset win-amd64-%CFG% -DCMAKE_C_COMPILER="%LLVM%\clang.exe" -DCMAKE_CXX_COMPILER="%LLVM%\clang++.exe" -DCMAKE_PREFIX_PATH="%SDK%" || exit /b 1
)
cmake --build out\build\win-amd64-%CFG% --parallel || exit /b 1
echo pronto: out\build\win-amd64-%CFG%
