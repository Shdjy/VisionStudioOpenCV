# VisionStudio OpenCV - 一键免安装绿色便携包打包脚本 (PowerShell)
[CmdletBinding()]
param(
    [string]$TargetExe = "VisionStudio_OpenCV.exe"
)

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -Path $ScriptDir

function Resolve-TargetExecutable
{
    param([string]$Path)

    $candidate = if ([System.IO.Path]::IsPathRooted($Path))
    {
        $Path
    }
    else
    {
        Join-Path $ScriptDir $Path
    }

    $item = Get-Item -LiteralPath $candidate -ErrorAction SilentlyContinue
    if (-not $item -or $item.PSIsContainer)
    {
        throw "未找到有效的主程序: $candidate"
    }
    return $item
}

function Assert-RequiredFile
{
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf))
    {
        throw "运行时文件缺失: $RelativePath"
    }
}

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   VisionStudio OpenCV - 免安装便携包一键打包工具" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan

try
{
    $targetItem = Resolve-TargetExecutable $TargetExe
    $targetExePath = $targetItem.FullName
    $targetExeName = $targetItem.Name

    Write-Host "[检查] 主程序: $targetExeName" -ForegroundColor Gray
    Assert-RequiredFile $ScriptDir "Qt6Core.dll"
    Assert-RequiredFile $ScriptDir "Qt6Gui.dll"
    Assert-RequiredFile $ScriptDir "Qt6Widgets.dll"
    Assert-RequiredFile $ScriptDir "libgcc_s_seh-1.dll"
    Assert-RequiredFile $ScriptDir "libstdc++-6.dll"
    Assert-RequiredFile $ScriptDir "libwinpthread-1.dll"
    Assert-RequiredFile $ScriptDir "platforms\qwindows.dll"

    $opencvDllFiles = @(Get-ChildItem -LiteralPath $ScriptDir -Filter "libopencv_*.dll" -File)
    if ($opencvDllFiles.Count -eq 0)
    {
        throw "未找到 OpenCV 运行库 DLL (libopencv_*.dll)"
    }
}
catch
{
    Write-Host "[失败] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$DistRoot = Join-Path $ScriptDir "_dist_portable"
$PackageName = "VisionStudio_OpenCV_Portable"
$DistDir = Join-Path $DistRoot $PackageName
$ZipOutputName = "${PackageName}_${Timestamp}.zip"
$ZipOutputPath = Join-Path $ScriptDir $ZipOutputName
$createdZip = $false

try
{
    Write-Host "`n[1/4] 清理旧临时目录..." -ForegroundColor Green
    if (Test-Path -LiteralPath $DistRoot)
    {
        Remove-Item -LiteralPath $DistRoot -Recurse -Force
    }
    New-Item -ItemType Directory -Path $DistDir -Force | Out-Null

    Write-Host "[2/4] 正在提取免安装运行时文件..." -ForegroundColor Green

    # 复制主可执行程序
    Write-Host "  -> 复制主程序: $targetExeName" -ForegroundColor Gray
    Copy-Item -LiteralPath $targetExePath -Destination (Join-Path $DistDir $targetExeName) -Force

    # 复制所有运行依赖 DLL
    $dllFiles = @(Get-ChildItem -LiteralPath $ScriptDir -Filter "*.dll" -File)
    foreach ($dll in $dllFiles)
    {
        Write-Host "  -> 复制依赖库: $($dll.Name)" -ForegroundColor Gray
        Copy-Item -LiteralPath $dll.FullName -Destination (Join-Path $DistDir $dll.Name) -Force
    }

    # 复制必要的 Qt 插件子文件夹
    $pluginDirs = @("platforms", "imageformats", "iconengines", "styles", "tls", "networkinformation", "generic")
    foreach ($pDir in $pluginDirs)
    {
        $srcPluginDir = Join-Path $ScriptDir $pDir
        if (Test-Path -LiteralPath $srcPluginDir -PathType Container)
        {
            Write-Host "  -> 复制插件目录: $pDir" -ForegroundColor Gray
            Copy-Item -LiteralPath $srcPluginDir -Destination (Join-Path $DistDir $pDir) -Recurse -Force
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
   - 60 个通用 OpenCV 算子（颜色空间、滤波降噪、自适应阈值、形态学、轮廓分析、霍夫检测、特征匹配、图像增强等）
   - 交互式选区掩码编辑器 (Alt+M)
   - 多通道直方图实时对比分析
   - 一键生成并导出标准 C++ (OpenCV) 与 Python (cv2) 工业级源码
   - 实时执行耗时分析与流水线参数交互调优

   注：QR/条码真实解码需要构建时包含 OpenCV objdetect 模块。

========================================================================
"@
    Set-Content -LiteralPath (Join-Path $DistDir "使用说明.txt") -Value $readmeContent -Encoding UTF8

    Write-Host "[3/4] 正在打包压缩为免安装 Zip 归档 (请稍候)..." -ForegroundColor Green
    if (Test-Path -LiteralPath $ZipOutputPath)
    {
        Remove-Item -LiteralPath $ZipOutputPath -Force
    }

    # 使用 PowerShell 压缩归档
    Compress-Archive -Path (Join-Path $DistDir "*") -DestinationPath $ZipOutputPath -CompressionLevel Optimal
    $createdZip = $true

    Write-Host "[4/4] 校验压缩包并清理临时目录..." -ForegroundColor Green
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($ZipOutputPath)
    try
    {
        $entryNames = @($archive.Entries | ForEach-Object { ($_.FullName -replace '\\', '/') })
        $requiredArchiveEntries = @(
            $targetExeName,
            "Qt6Core.dll",
            "Qt6Gui.dll",
            "Qt6Widgets.dll",
            "libgcc_s_seh-1.dll",
            "libstdc++-6.dll",
            "libwinpthread-1.dll",
            "platforms/qwindows.dll",
            "使用说明.txt"
        )
        foreach ($entry in $requiredArchiveEntries)
        {
            if ($entryNames -notcontains $entry)
            {
                throw "压缩包缺少必要文件: $entry"
            }
        }
    }
    finally
    {
        $archive.Dispose()
    }

    Remove-Item -LiteralPath $DistRoot -Recurse -Force

    $ZipItem = Get-Item -LiteralPath $ZipOutputPath
    $SizeMB = [math]::Round($ZipItem.Length / 1MB, 2)

    Write-Host "`n==========================================================" -ForegroundColor Green
    Write-Host " [成功] 免安装便携压缩包已生成！" -ForegroundColor Cyan
    Write-Host " 压缩包文件名: $ZipOutputName" -ForegroundColor Yellow
    Write-Host " 压缩包大小  : $SizeMB MB" -ForegroundColor Yellow
    Write-Host " 输出目标路径: $ZipOutputPath" -ForegroundColor Yellow
    Write-Host "==========================================================" -ForegroundColor Green
}
catch
{
    if ($createdZip -and (Test-Path -LiteralPath $ZipOutputPath))
    {
        Remove-Item -LiteralPath $ZipOutputPath -Force
    }
    Write-Host "[失败] $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
finally
{
    if (Test-Path -LiteralPath $DistRoot)
    {
        Remove-Item -LiteralPath $DistRoot -Recurse -Force
    }
}
