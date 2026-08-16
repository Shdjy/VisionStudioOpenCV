#include "MaskEditorDialog.h"
#include "ui_MaskEditorDialog.h"
#include "../utils/MatUtils.h"
#include <QMouseEvent>
#include <QIcon>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

MaskEditorDialog::MaskEditorDialog(const cv::Mat& currentImg, const cv::Mat& sourceImg, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::MaskEditorDialog)
    , _currentImg(currentImg.clone())
    , _sourceImg(sourceImg.empty() ? currentImg.clone() : sourceImg.clone())
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/app_icon.png"));

#ifdef _WIN32
    HWND hwnd = (HWND)winId();
    if (hwnd)
    {
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    }
#endif

    _scene = new QGraphicsScene(this);
    _pixmapItem = new QGraphicsPixmapItem();
    _scene->addItem(_pixmapItem);
    ui->graphicsView->setScene(_scene);
    ui->graphicsView->viewport()->installEventFilter(this);
    ui->graphicsView->setMouseTracking(true);

    // 默认初始 ROI
    int initW = std::max(50, _currentImg.cols / 2);
    int initH = std::max(50, _currentImg.rows / 2);
    int initX = std::max(0, (_currentImg.cols - initW) / 2);
    int initY = std::max(0, (_currentImg.rows - initH) / 2);
    _currentRoi = QRect(initX, initY, initW, initH);

    ui->spinX->setMaximum(std::max(100, _currentImg.cols));
    ui->spinY->setMaximum(std::max(100, _currentImg.rows));
    ui->spinW->setMaximum(std::max(100, _currentImg.cols));
    ui->spinH->setMaximum(std::max(100, _currentImg.rows));

    ui->spinX->setValue(initX);
    ui->spinY->setValue(initY);
    ui->spinW->setValue(initW);
    ui->spinH->setValue(initH);

    connect(ui->comboShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaskEditorDialog::OnParamChanged);
    connect(ui->comboTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaskEditorDialog::OnParamChanged);
    connect(ui->comboAction, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaskEditorDialog::OnParamChanged);
    connect(ui->sliderThresh, &QSlider::valueChanged, [this](int val) {
        ui->lblThresh->setText(QString("自动阈值分割门限: %1").arg(val));
        OnParamChanged();
    });

    connect(ui->spinX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int v) { _currentRoi.setX(v); OnParamChanged(); });
    connect(ui->spinY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int v) { _currentRoi.setY(v); OnParamChanged(); });
    connect(ui->spinW, QOverload<int>::of(&QSpinBox::valueChanged), [this](int v) { _currentRoi.setWidth(v); OnParamChanged(); });
    connect(ui->spinH, QOverload<int>::of(&QSpinBox::valueChanged), [this](int v) { _currentRoi.setHeight(v); OnParamChanged(); });

    connect(ui->btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->btnApply, &QPushButton::clicked, this, &MaskEditorDialog::OnApplyClicked);

    UpdateDisplayPreview();
}

MaskEditorDialog::~MaskEditorDialog()
{
    delete ui;
}

std::unique_ptr<InteractiveMaskOp> MaskEditorDialog::GetConfiguredOperator() const
{
    auto op = std::make_unique<InteractiveMaskOp>();
    op->SetParameter("shape", ui->comboShape->currentIndex());
    op->SetParameter("targetSource", ui->comboTarget->currentIndex());
    op->SetParameter("action", ui->comboAction->currentIndex());
    op->SetParameter("roiX", _currentRoi.x());
    op->SetParameter("roiY", _currentRoi.y());
    op->SetParameter("roiW", _currentRoi.width());
    op->SetParameter("roiH", _currentRoi.height());
    op->SetParameter("threshVal", ui->sliderThresh->value());
    return op;
}

void MaskEditorDialog::OnParamChanged()
{
    UpdateDisplayPreview();
}

void MaskEditorDialog::OnApplyClicked()
{
    accept();
}

void MaskEditorDialog::UpdateDisplayPreview()
{
    if (_currentImg.empty())
    {
        return;
    }

    auto op = GetConfiguredOperator();
    cv::Mat previewOut;
    std::string err;
    if (op->Process(_currentImg, previewOut, err))
    {
        QImage qimg = MatUtils::MatToQImage(previewOut);
        _pixmapItem->setPixmap(QPixmap::fromImage(qimg));
        _scene->setSceneRect(_pixmapItem->boundingRect());
    }
}

bool MaskEditorDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->graphicsView->viewport())
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                _isDrawing = true;
                QPointF sceneP = ui->graphicsView->mapToScene(mouseEvent->pos());
                _startPos = sceneP.toPoint();
                _currentRoi = QRect(_startPos, QSize(1, 1));
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove && _isDrawing)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF sceneP = ui->graphicsView->mapToScene(mouseEvent->pos());
            QPoint curP = sceneP.toPoint();

            int left = std::min(_startPos.x(), curP.x());
            int top = std::min(_startPos.y(), curP.y());
            int w = std::abs(_startPos.x() - curP.x());
            int h = std::abs(_startPos.y() - curP.y());

            left = std::max(0, std::min(left, _currentImg.cols - 10));
            top = std::max(0, std::min(top, _currentImg.rows - 10));
            w = std::max(10, std::min(w, _currentImg.cols - left));
            h = std::max(10, std::min(h, _currentImg.rows - top));

            _currentRoi = QRect(left, top, w, h);

            ui->spinX->blockSignals(true);
            ui->spinY->blockSignals(true);
            ui->spinW->blockSignals(true);
            ui->spinH->blockSignals(true);
            ui->spinX->setValue(left);
            ui->spinY->setValue(top);
            ui->spinW->setValue(w);
            ui->spinH->setValue(h);
            ui->spinX->blockSignals(false);
            ui->spinY->blockSignals(false);
            ui->spinW->blockSignals(false);
            ui->spinH->blockSignals(false);

            UpdateDisplayPreview();
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease && _isDrawing)
        {
            _isDrawing = false;
            UpdateDisplayPreview();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}
