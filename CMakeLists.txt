cmake_minimum_required(VERSION 3.16)

project(VisionStudio_OpenCV VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 禁用 Windows.h 的 min/max 宏，避免与 std::min / std::max 冲突
add_compile_definitions(NOMINMAX)

# 自动处理 Qt 元对象编译器 (moc)、UI 编译器 (uic) 和资源编译器 (rcc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ----------------------------------------------------------------------------
# 1. 查找 Qt (自动优先匹配 Qt6，若无则自动回退至 Qt5)
# ----------------------------------------------------------------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui)

message(STATUS "Found Qt Version: ${QT_VERSION} (Qt${QT_VERSION_MAJOR})")

# ----------------------------------------------------------------------------
# 2. 查找 OpenCV (多 IDE 智能自动探测)
# ----------------------------------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH
    "D:/Qt/6.8.2/mingw_64"
    "D:/opencv4.9/build_mingw"
    "D:/opencv4.9/opencv/build/x64/vc16/lib"
    "D:/opencv4.9/opencv/build/x64/vc15/lib"
    "D:/opencv4.9/opencv/build"
    "D:/opencv4.9/opencv"
    "D:/OpenCV3.4.1/build_vs2017_NoCuda_Win64/install"
    "D:/OpenCV3.4.1/build"
    "D:/C++library/opencv/build"
    "C:/opencv/build"
)

find_package(OpenCV QUIET)

if(NOT OpenCV_FOUND)
    message(STATUS "OpenCV package config not directly matched, attempting manual path detection...")
    find_path(OpenCV_INCLUDE_DIRS NAMES opencv2/opencv.hpp
        PATHS
        "D:/opencv4.9/build_mingw"
        "D:/opencv4.9/opencv/sources/include"
        "D:/opencv4.9/opencv/build/include"
        "D:/OpenCV3.4.1/build_vs2017_NoCuda_Win64/install/include"
        "D:/C++library/opencv/build/include"
        "C:/opencv/build/include"
    )

    find_library(OpenCV_LIBS NAMES opencv_core490 opencv_world490 opencv_world490.dll opencv_world490d opencv_world341 opencv_world341d opencv_world
        PATHS
        "D:/opencv4.9/build_mingw/lib"
        "D:/opencv4.9/build_mingw/bin"
        "D:/opencv4.9/opencv/build/x64/vc16/lib"
        "D:/opencv4.9/opencv/build/x64/vc15/lib"
        "D:/OpenCV3.4.1/build_vs2017_NoCuda_Win64/install/x64/vc15/lib"
        "D:/C++library/opencv/build/x64/vc14/lib"
    )

    if(OpenCV_INCLUDE_DIRS AND OpenCV_LIBS)
        set(OpenCV_FOUND TRUE)
        message(STATUS "Fallback OpenCV Include: ${OpenCV_INCLUDE_DIRS}")
        message(STATUS "Fallback OpenCV Lib: ${OpenCV_LIBS}")
    else()
        message(FATAL_ERROR "Could not find OpenCV. Please specify OpenCV_DIR in CMake or environment.")
    endif()
endif()

message(STATUS "OpenCV Include Dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV Libraries: ${OpenCV_LIBS}")

# ----------------------------------------------------------------------------
# 3. 收集所有源代码与 UI 界面文件
# ----------------------------------------------------------------------------
set(CORE_SOURCES
    src/core/IOpNode.cpp
    src/core/OpRegistry.cpp
    src/core/operators/ColorOps.cpp
    src/core/operators/FilterOps.cpp
    src/core/operators/ThresholdOps.cpp
    src/core/operators/MorphologyOps.cpp
    src/core/operators/EdgeOps.cpp
    src/core/operators/TransformOps.cpp
    src/core/operators/EnhanceOps.cpp
    src/core/operators/ContourOps.cpp
    src/core/operators/HoughOps.cpp
    src/core/operators/FeatureOps.cpp
    src/core/operators/ArithmeticOps.cpp
    src/core/operators/InteractiveMaskOp.cpp
    src/core/operators/AdvancedOps.cpp
)

set(BLL_SOURCES
    src/bll/PipelineStep.cpp
    src/bll/PipelineManager.cpp
    src/bll/ProjectManager.cpp
)

set(CODEGEN_SOURCES
    src/codegen/CppCodeGenerator.cpp
    src/codegen/PythonCodeGenerator.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/MatUtils.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/ImageViewer.cpp
    src/ui/OperatorTreeWidget.cpp
    src/ui/PipelineListWidget.cpp
    src/ui/PropertyPanel.cpp
    src/ui/CodePreviewWidget.cpp
    src/ui/LogPanelWidget.cpp
    src/ui/HistogramWidget.cpp
    src/ui/AboutDialog.cpp
    src/ui/MaskEditorDialog.cpp
)

set(UI_FORMS
    src/ui/MainWindow.ui
    src/ui/ImageViewer.ui
    src/ui/OperatorTreeWidget.ui
    src/ui/PipelineListWidget.ui
    src/ui/PropertyPanel.ui
    src/ui/CodePreviewWidget.ui
    src/ui/LogPanelWidget.ui
    src/ui/HistogramWidget.ui
    src/ui/AboutDialog.ui
    src/ui/MaskEditorDialog.ui
)

# 原生 Windows 资源文件 (Qt Creator / Visual Studio 自动跨平台编译)
if(MINGW)
    set(APP_RC_OBJ "${CMAKE_CURRENT_BINARY_DIR}/app_icon_res.o")
    add_custom_command(
        OUTPUT ${APP_RC_OBJ}
        COMMAND ${CMAKE_RC_COMPILER} -i "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.rc" -o ${APP_RC_OBJ} -O coff
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/app.rc" "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/app_icon.ico"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/resources"
        COMMENT "Compiling Windows Resource app.rc for MinGW"
    )
    set(RESOURCES
        src/resources/resources.qrc
        ${APP_RC_OBJ}
    )
else()
    set(RESOURCES
        src/resources/resources.qrc
        src/resources/app.rc
    )
endif()

# ----------------------------------------------------------------------------
# 4. 创建纯净 GUI 可执行目标 (WIN32 移除附加黑框控制台)
# ----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    ${CORE_SOURCES}
    ${BLL_SOURCES}
    ${CODEGEN_SOURCES}
    ${UTILS_SOURCES}
    ${UI_SOURCES}
    ${UI_FORMS}
    ${RESOURCES}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
    src/bll
    src/codegen
    src/utils
    src/ui
    ${OpenCV_INCLUDE_DIRS}
)

# 链接库 (包含 dwmapi 支持沉浸式深色标题栏)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    ${OpenCV_LIBS}
    dwmapi
)

# 编译器选项适配
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /utf-8 /Zc:__cplusplus /permissive-)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

message(STATUS "Configured ${PROJECT_NAME} successfully.")
