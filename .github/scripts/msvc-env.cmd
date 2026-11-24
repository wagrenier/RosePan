@echo off
rem Put the newest installed x64 MSVC (cl, link, rc) on the caller's environment.
rem No setlocal: the point is what vcvars64 leaves behind.
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul
where cl >nul 2>&1 || (echo ::error::MSVC was not found & exit /b 1)
