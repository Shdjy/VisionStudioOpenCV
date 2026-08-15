#include <QApplication>
#include <QFile>
#include <QDir>
#include "core/OpRegistry.h"
#include "ui/MainWindow.h"
#include "utils/Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // 修复 Windows 控制台中文输出乱码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("VisionStudio OpenCV");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("VisionStudio");

    // 1. 初始化并注册全部 OpenCV 核心算子
    OpRegistry::Instance().RegisterAllBuiltinOperators();
    Logger::Info("System", "已成功注册全部 10 大分类 40+ 核心机器视觉算子");

    // 2. 加载工业级深色样式表
    QFile styleFile(":/styles/dark_style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text))
    {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
    }
    else
    {
        // 尝试从本地相对路径加载
        QFile localStyle("src/resources/styles/dark_style.qss");
        if (localStyle.open(QFile::ReadOnly | QFile::Text))
        {
            QString styleSheet = QLatin1String(localStyle.readAll());
            app.setStyleSheet(styleSheet);
            localStyle.close();
        }
    }

    // 3. 启动主窗口 (默认最大化显示)
    MainWindow mainWindow;
    mainWindow.showMaximized();

    return app.exec();
}
