# VisionStudio OpenCV - 一键免安装绿色便携包打包脚本 (PowerShell)
[CmdletBinding()]
param(
    [string]$TargetExe = "VisionStudio_OpenCV.exe"
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -Path $ScriptDir

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   VisionStudio OpenCV - 免安装便携包一键打包工具" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. 检查主程序是否存在
if (-not (Test-Path -Path $TargetExe)) {
    Write-Host "[错误] 未在当前目录下找到 $TargetExe ！" -ForegroundColor Red
    Write-Host "当前执行路径: $ScriptDir" -ForegroundColor Gray
    Write-Host "请确保在包含可执行程序的构建输出目录中运行此脚本。" -ForegroundColor Yellow
    exit 1
}

$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$DistRoot = Join-Path $ScriptDir "_dist_portable"
$PackageName = "VisionStudio_OpenCV_Portable"
$DistDir = Join-Path $DistRoot $PackageName
$ZipOutputName = "${PackageName}_${Timestamp}.zip"
$ZipOutputPath = Join-Path $ScriptDir $ZipOutputName

Write-Host "`n[1/4] 清理旧临时目录..." -ForegroundColor Green
if (Test-Path $DistRoot) {
    Remove-Item -Path $DistRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

Write-Host "[2/4] 正在提取免安装运行时文件..." -ForegroundColor Green

# 复制主可执行程序
Write-Host "  -> 复制主程序: $TargetExe" -ForegroundColor Gray
Copy-Item -Path $TargetExe -Destination (Join-Path $DistDir $TargetExe) -Force

# 复制所有运行依赖 DLL
$dllFiles = Get-ChildItem -Path $ScriptDir -Filter "*.dll" -File
foreach ($dll in $dllFiles) {
    Write-Host "  -> 复制依赖库: $($dll.Name)" -ForegroundColor Gray
    Copy-Item -Path $dll.FullName -Destination (Join-Path $DistDir $dll.Name) -Force
}

# 复制必要的 Qt 插件子文件夹
$pluginDirs = @("platforms", "imageformats", "iconengines", "styles", "tls", "networkinformation", "generic")
foreach ($pDir in $pluginDirs) {
    $srcPluginDir = Join-Path $ScriptDir $pDir
    if (Test-Path $srcPluginDir) {
        Write-Host "  -> 复制插件目录: $pDir" -ForegroundColor Gray
        Copy-Item -Path $srcPluginDir -Destination (Join-Path $DistDir $pDir) -Recurse -Force
    }
}

# 创建免安装运行说明文档
$readmeContent = @"
========================================================================
 VisionStudio OpenCV - 现代化工业机器视觉仿真与算子开发工作台
========================================================================

【免安装绿色便携版使用指南】

1. 运行要求：
   - 操作系统：Windows 10 / Windows 11 (64位)
   - 本版本为免安装绿色便携版，已自包含所有 Qt6 与 OpenCV 运行时库，无需额外配置环境变量或安装 Qt。

2. 快速启动：
   - 双击直接运行 【VisionStudio_OpenCV.exe】 即可启动软件。

3. 包含功能：
   - 50+ 个丰富工业视觉核心算子（颜色空间、滤波降噪、自适应阈值、形态学、轮廓分析、霍夫检测、特征匹配、图像增强等）
   - 交互式选区掩码编辑器 (Alt+M)
   - 多通道直方图实时对比分析
   - 一键生成并导出标准 C++ (OpenCV) 与 Python (cv2) 工业级源码
   - 实时执行耗时分析与流水线参数交互调优

========================================================================
"@
Set-Content -Path (Join-Path $DistDir "使用说明.txt") -Value $readmeContent -Encoding UTF8

Write-Host "[3/4] 正在打包压缩为免安装 Zip 归档 (请稍候)..." -ForegroundColor Green
if (Test-Path $ZipOutputPath) {
    Remove-Item -Path $ZipOutputPath -Force
}

# 使用 PowerShell 压缩归档
Compress-Archive -Path "$DistDir\*" -DestinationPath $ZipOutputPath -CompressionLevel Optimal

Write-Host "[4/4] 清理临时构建目录..." -ForegroundColor Green
Remove-Item -Path $DistRoot -Recurse -Force

$ZipItem = Get-Item $ZipOutputPath
$SizeMB = [math]::Round($ZipItem.Length / 1MB, 2)

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host " [成功] 免安装便携压缩包已生成！" -ForegroundColor Cyan
Write-Host " 压缩包文件名: $ZipOutputName" -ForegroundColor Yellow
Write-Host " 压缩包大小  : $SizeMB MB" -ForegroundColor Yellow
Write-Host " 输出目标路径: $ZipOutputPath" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Green