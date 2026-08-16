#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include <QIcon>
#include <QPixmap>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/app_icon.png"));

#ifdef _WIN32
    HWND hwnd = (HWND)winId();
    if (hwnd)
    {
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        DwmSetWindowAttribute(hwnd, 19, &darkMode, sizeof(darkMode));
    }
#endif
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
