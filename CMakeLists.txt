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
set(VISIONSTUDIO_QT_ROOT "" CACHE PATH "Optional Qt installation root")
set(VISIONSTUDIO_OPENCV_ROOT "" CACHE PATH "Optional OpenCV installation root")

if(VISIONSTUDIO_QT_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${VISIONSTUDIO_QT_ROOT}")
elseif(MINGW AND EXISTS "D:/Qt/6.8.2/mingw_64")
    list(APPEND CMAKE_PREFIX_PATH "D:/Qt/6.8.2/mingw_64")
elseif(MSVC AND EXISTS "D:/Qt/6.8.2/msvc2019_64")
    list(APPEND CMAKE_PREFIX_PATH "D:/Qt/6.8.2/msvc2019_64")
endif()

if(VISIONSTUDIO_OPENCV_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${VISIONSTUDIO_OPENCV_ROOT}")
elseif(EXISTS "D:/opencv4.9/build_mingw")
    list(APPEND CMAKE_PREFIX_PATH "D:/opencv4.9/build_mingw")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui Network)

message(STATUS "Found Qt Version: ${QT_VERSION} (Qt${QT_VERSION_MAJOR})")

# ----------------------------------------------------------------------------
# 2. 查找 OpenCV (多 IDE 智能自动探测)
# ----------------------------------------------------------------------------
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
    src/core/operators/BranchOps.cpp
    src/core/operators/InteractiveMaskOp.cpp
    src/core/operators/AdvancedOps.cpp
    src/core/operators/IndustrialOps.cpp
    src/core/operators/GeneralOps.cpp
)

set(BLL_SOURCES
    src/bll/PipelineStep.cpp
    src/bll/PipelineManager.cpp
    src/bll/ProjectManager.cpp
    src/bll/PipelineValidator.cpp
)

set(DETECTION_SOURCES
    src/detection/InspectionRuleEngine.cpp
    src/detection/ObjectCounter.cpp
    src/detection/DefectDetector.cpp
    src/detection/GeometryMeasurement.cpp
    src/detection/DetectionExecutor.cpp
    src/detection/DetectionTaskManager.cpp
)

set(AI_SOURCES
    src/ai/OpenAiCompatibleProvider.cpp
    src/ai/AiConfigManager.cpp
    src/ai/OperatorKnowledgeBase.cpp
    src/ai/AiDetectionPlanner.cpp
    src/ai/AiResponseValidator.cpp
)

set(CODEGEN_SOURCES
    src/codegen/CppCodeGenerator.cpp
    src/codegen/PythonCodeGenerator.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/MatUtils.cpp
    src/utils/IconUtils.cpp
    src/utils/ImageIoUtils.cpp
    src/utils/AppSettings.cpp
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
    src/ui/detection/DetectionPanelWidget.cpp
    src/ui/detection/BatchTestDialog.cpp
    src/ui/ai/AiAssistantWidget.cpp
    src/ui/ai/AiSettingsDialog.cpp
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
    src/ui/detection/DetectionPanelWidget.ui
    src/ui/detection/BatchTestDialog.ui
    src/ui/ai/AiAssistantWidget.ui
    src/ui/ai/AiSettingsDialog.ui
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
    ${DETECTION_SOURCES}
    ${AI_SOURCES}
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
    src/detection
    src/ai
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
    Qt${QT_VERSION_MAJOR}::Network
    ${OpenCV_LIBS}
    dwmapi
)

# QR/Barcode 解码属于 OpenCV objdetect，可在精简 OpenCV 构建中缺失。
if(TARGET opencv_objdetect)
    target_link_libraries(${PROJECT_NAME} PRIVATE opencv_objdetect)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VISIONSTUDIO_HAS_OPENCV_OBJDETECT=1)
    message(STATUS "OpenCV objdetect enabled: QR/Barcode decoding is available")
else()
    message(WARNING "OpenCV objdetect is unavailable; QR/Barcode decoding will report a clear dependency error")
endif()

# 编译器选项适配
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /utf-8 /Zc:__cplusplus /permissive-)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Deploy OpenCV runtime libraries next to the executable for Qt Creator runs.
if(WIN32 AND MINGW)
    if(DEFINED OpenCV_INSTALL_PATH)
        set(OPENCV_RUNTIME_DIR "${OpenCV_INSTALL_PATH}/bin")
    elseif(DEFINED OpenCV_DIR)
        set(OPENCV_RUNTIME_DIR "${OpenCV_DIR}/bin")
    endif()

    if(OPENCV_RUNTIME_DIR AND EXISTS "${OPENCV_RUNTIME_DIR}")
        file(GLOB OPENCV_RUNTIME_DLLS CONFIGURE_DEPENDS
            "${OPENCV_RUNTIME_DIR}/*.dll"
        )

        if(OPENCV_RUNTIME_DLLS)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OPENCV_RUNTIME_DLLS}
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND_EXPAND_LISTS
                COMMENT "Deploying OpenCV runtime libraries"
            )
            message(STATUS "OpenCV runtime deployment enabled: ${OPENCV_RUNTIME_DIR}")
        else()
            message(WARNING "No OpenCV runtime libraries found in ${OPENCV_RUNTIME_DIR}")
        endif()
    endif()
endif()
# Deploy Qt libraries, MinGW runtime libraries, and platform plugins.
if(WIN32 AND MINGW)
    get_target_property(QT_QMAKE_PATH Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    if(QT_QMAKE_PATH)
        get_filename_component(QT_RUNTIME_BIN_DIR "${QT_QMAKE_PATH}" DIRECTORY)
        find_program(QT_WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt windeployqt.exe
            HINTS "${QT_RUNTIME_BIN_DIR}"
            NO_DEFAULT_PATH
        )
    endif()

    if(QT_WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${QT_WINDEPLOYQT_EXECUTABLE}"
                --no-translations
                --compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt runtime libraries"
            VERBATIM
        )
        message(STATUS "Qt runtime deployment enabled: ${QT_WINDEPLOYQT_EXECUTABLE}")
    else()
        message(WARNING "windeployqt was not found; direct execution may miss Qt runtime libraries")
    endif()
endif()

# Keep portable packaging scripts synchronized with every CMake configure.
# A POST_BUILD copy only runs when the executable is relinked, leaving stale
# scripts behind after a script-only change.
if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/package_portable.bat"
        "${CMAKE_CURRENT_BINARY_DIR}/package_portable.bat"
        COPYONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/package_portable.ps1"
        "${CMAKE_CURRENT_BINARY_DIR}/package_portable.ps1"
        COPYONLY
    )
endif()

message(STATUS "Configured ${PROJECT_NAME} successfully.")

include(CTest)
if(BUILD_TESTING)
    add_executable(VisionStudio_CoreTests
        tests/CoreTests.cpp
        ${CORE_SOURCES}
        ${BLL_SOURCES}
        ${DETECTION_SOURCES}
        ${AI_SOURCES}
        ${CODEGEN_SOURCES}
        src/utils/Logger.cpp
        src/utils/ImageIoUtils.cpp
        src/utils/MatUtils.cpp
        src/utils/AppSettings.cpp
    )
    target_include_directories(VisionStudio_CoreTests PRIVATE
        src
        src/core
        src/bll
        src/detection
        src/ai
        src/codegen
        src/utils
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(VisionStudio_CoreTests PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Network
        ${OpenCV_LIBS}
    )
    if(TARGET opencv_objdetect)
        target_link_libraries(VisionStudio_CoreTests PRIVATE opencv_objdetect)
        target_compile_definitions(VisionStudio_CoreTests PRIVATE VISIONSTUDIO_HAS_OPENCV_OBJDETECT=1)
    endif()
    if(MSVC)
        target_compile_options(VisionStudio_CoreTests PRIVATE /utf-8 /Zc:__cplusplus /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(VisionStudio_CoreTests PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()
    add_test(NAME VisionStudio_CoreTests COMMAND VisionStudio_CoreTests)
endif()
