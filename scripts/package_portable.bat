@echo off
chcp 65001 >nul
title VisionStudio OpenCV - 一键免安装打包工具
set SCRIPT_DIR=%~dp0
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%package_portable.ps1"
if %ERRORLEVEL% neq 0 (
    echo.
    echo 打包过程遇到错误，按任意键退出...
    pause >nul
) else (
    echo.
    echo 打包完成！
    timeout /t 3 >nul
)
