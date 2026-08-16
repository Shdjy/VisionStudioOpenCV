# VisionStudio OpenCV - 构建与部署指南 (Build & Deployment Guide)

本文档提供 **VisionStudio OpenCV** 在 Windows 平台上的完整编译、测试与便携打包部署说明。

---

## 📑 目录

1. [开发环境与系统要求](#1-开发环境与系统要求)
2. [方式一：CMake 命令行构建 (推荐 MinGW / Ninja)](#2-方式一cmake-命令行构建-推荐-mingw--ninja)
3. [方式二：Qt Creator IDE 构建](#3-方式二qt-creator-ide-构建)
4. [方式三：Visual Studio 2022 构建 (MSVC x64)](#4-方式三visual-studio-2022-构建-msvc-x64)
5. [运行单元测试 (CTest)](#5-运行单元测试-ctest)
6. [生成免安装便携发布包 (Portable Package)](#6-生成免安装便携发布包-portable-package)
7. [常见问题排查与 FAQ](#7-常见问题排查与-faq)

---

## 1. 开发环境与系统要求

| 组件 | 推荐版本 | 说明 |
| :--- | :--- | :--- |
| **操作系统** | Windows 10 / 11 (64-bit) | 针对 Windows 深度适配沉浸式暗色标题栏 |
| **C++ 标准** | C++17 | 依赖 `<variant>`, `<optional>`, `<filesystem>` 等现代特性 |
| **CMake** | >= 3.16 (推荐 3.20+) | 支持 CMakePresets.json 标准预设 |
| **Qt 框架** | Qt 6.5+ / Qt 6.8+ (或 Qt 5.15+) | 依赖 `Widgets`, `Core`, `Gui` 模块 |
| **OpenCV** | OpenCV 4.5+ ~ 4.10+ (或 OpenCV 3.4.x) | 需包含 `core`, `imgproc`, `imgcodecs`, `highgui`, `features2d` (推荐含 `objdetect`) |
| **构建工具** | Ninja / MinGW GCC 11+ 或 MSVC 2019/2022 | 推荐使用 Ninja + GCC/Clang/MSVC |

---

## 2. 方式一：CMake 命令行构建 (推荐 MinGW / Ninja)

在 PowerShell 或 Windows Terminal 中执行以下命令：

```powershell
# 1. 进入项目根目录
cd "d:\code for github\VisionStudio OpenCV"

# 2. 创建并进入构建目录
mkdir build -ErrorAction SilentlyContinue
cd build

# 3. 运行 CMake 配置 (指定 Qt 和 OpenCV 路径)
# 提示：若您的 Qt 或 OpenCV 路径与默认不同，请通过 -DCMAKE_PREFIX_PATH 指定
cmake .. -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="D:/Qt/6.8.2/mingw_64;D:/opencv4.9/build_mingw"

# 4. 编译可执行程序与测试程序
cmake --build . --config Release -j 8
```

> 💡 **提示**：若构建配置时设置了 MinGW，CMake 将在编译完成后自动调用 `windeployqt` 并复制 OpenCV 运行时 `.dll` 到目标目录，生成即可直接点击运行。

---

## 3. 方式二：Qt Creator IDE 构建

1. 启动 **Qt Creator**。
2. 点击 **「打开文件或项目」** (Open File or Project)，选择工程根目录下的 `CMakeLists.txt`。
3. 选择您已安装的 Qt Kit（如 `Desktop Qt 6.8.2 MinGW 64-bit` 或 `Desktop Qt 6.x MSVC2019/2022 64-bit`）。
4. 在 **「项目」->「构建设置」** 中，确认 CMake 变量 `CMAKE_PREFIX_PATH` 包含了 OpenCV 的安装路径（如 `D:/opencv4.9/build_mingw`）。
5. 点击左下角 **「构建」** (Ctrl+B) 或 **「运行」** (Ctrl+R) 即可启动 VisionStudio OpenCV。

---

## 4. 方式三：Visual Studio 2022 构建 (MSVC x64)

1. 打开 **Visual Studio 2022**。
2. 选择 **「打开本地文件夹」** (Open a Local Folder)，选择项目根目录。
3. Visual Studio 会自动检测到根目录的 `CMakePresets.json` 与 `CMakeLists.txt`。
4. 在顶部配置下拉框中选择预设 `windows-vs2022`。
5. 在 `CMakeSettings.json` 或 `CMakePresets.json` 中配置您的 MSVC 版本 Qt 与 OpenCV 路径：
   ```json
   "cacheVariables": {
       "CMAKE_BUILD_TYPE": "Release",
       "CMAKE_PREFIX_PATH": "C:/Qt/6.8.2/msvc2019_64;D:/opencv4.9/opencv/build"
   }
   ```
6. 点击 **「生成全部」** (Ctrl+Shift+B) 或直接选择 `VisionStudio_OpenCV.exe` 进行调试。

---

## 5. 运行单元测试 (CTest)

VisionStudio 内置了针对算子注册表、参数校验、图像统计、模板匹配、RANSAC 拟合、UTF-8 路径 IO、工程序列化事务回滚的完备自动化测试集：

```powershell
# 在构建目录下运行测试
ctest --output-on-failure
```

预期输出示例：
```
Test project D:/code for github/VisionStudio OpenCV/build
    Start 1: VisionStudio_CoreTests
1/1 Test #1: VisionStudio_CoreTests ...........   Passed    0.04 sec

100% tests passed, 0 tests failed out of 1
```

---

## 6. 生成免安装便携发布包 (Portable Package)

项目提供了全自动一键便携化打包脚本，可自动收集主程序、Qt 核心 DLL、Qt 平台插件（`platforms/qwindows.dll` 等）、MinGW/MSVC 运行时以及 OpenCV 依赖库，打包为独立的 `.zip` 绿色发布包。

### 运行打包脚本：

```powershell
# 方式 A：运行 PowerShell 脚本
powershell -ExecutionPolicy Bypass -File .\scripts\package_portable.ps1 -TargetExe ".\build\qt6-mingw\Release\VisionStudio_OpenCV.exe"

# 方式 B：双击或在终端运行批处理文件
.\scripts\package_portable.bat
```

打包完成后，将在根目录或构建目录生成 `VisionStudio_OpenCV_Portable_YYYYMMDD_HHMMSS.zip`，解压到任意未安装 Qt 或 OpenCV 的纯净 Windows 10/11 电脑上即可双击直接运行！

---

## 7. 常见问题排查与 FAQ

### Q1: CMake 报告 `Could not find OpenCV`
- **解决办法**：请在 CMake 命令中加入 `-DCMAKE_PREFIX_PATH="<您的OpenCV安装路径>"`，或在环境变量中设置 `OpenCV_DIR="<路径>/build"`。

### Q2: 启动程序时报错缺少 `qwindows.dll` 或无法初始化平台插件
- **解决办法**：确保可执行程序所在目录下存在 `platforms/qwindows.dll`。若使用 MinGW 构建，CMake 脚本已包含 `windeployqt` 自动化部署逻辑；或直接使用 `scripts/package_portable.ps1` 自动提取。

### Q3: 中文路径或文件名无法正确读取
- **解决办法**：VisionStudio OpenCV 内部统一使用 `ImageIoUtils` 工具类，基于 UTF-8 / UTF-16 宽字符流进行编解码，完全支持包含中文、空格或特殊字符的文件路径。

### Q4: 条形码/二维码识别提示依赖缺失
- **解决办法**：二维码和条码解码依赖 OpenCV 的 `objdetect` 模块。若您使用的是极简裁剪版 OpenCV，请确保编译 OpenCV 时开启了 `opencv_objdetect` 模块。
