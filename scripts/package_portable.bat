@echo off
setlocal
title VisionStudio OpenCV Portable Packaging
set SCRIPT_DIR=%~dp0
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%package_portable.ps1"
set EXIT_CODE=%ERRORLEVEL%
if not "%EXIT_CODE%"=="0" (
    echo.
    echo Packaging failed.
) else (
    echo.
    echo Packaging completed.
)
powershell.exe -NoProfile -Command "Start-Sleep -Seconds 3"
endlocal & exit /b %EXIT_CODE%
