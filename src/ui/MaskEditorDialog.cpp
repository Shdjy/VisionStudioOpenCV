#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MaskEditorDialog.h"
#include "ui_MaskEditorDialog.h"
#include "../utils/MatUtils.h"
#include <algorithm>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTimer>
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
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->graphicsView->viewport()->installEventFilter(this);
    ui->graphicsView->setMouseTracking(true);

    // 默认初始 ROI 位于图像中心
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

    // 视口导航按钮连接
    connect(ui->btnZoomIn, &QToolButton::clicked, this, &MaskEditorDialog::ZoomIn);
    connect(ui->btnZoomOut, &QToolButton::clicked, this, &MaskEditorDialog::ZoomOut);
    connect(ui->btnZoomFit, &QToolButton::clicked, this, &MaskEditorDialog::ZoomFit);
    connect(ui->btnZoomOriginal, &QToolButton::clicked, this, &MaskEditorDialog::ZoomOriginal);
    connect(ui->chkPreviewResult, &QCheckBox::toggled, this, &MaskEditorDialog::OnParamChanged);

    // 形状与参数联动连接
    connect(ui->comboShape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaskEditorDialog::OnShapeChanged);
    connect(ui->comboTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaskEditorDialog::OnParamChanged);
    connect(ui->comboAction, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MaskEditorDialog::OnParamChanged);

    connect(ui->spinPolySides, QOverload<int>::of(&QSpinBox::valueChanged), this, &MaskEditorDialog::OnParamChanged);
    connect(ui->sliderInnerRatio, &QSlider::valueChanged, [this](int val) {
        ui->lblInnerRatio->setText(QString("圆环内径比例: %1%").arg(val));
        OnParamChanged();
    });
    connect(ui->sliderAngle, &QSlider::valueChanged, [this](int val) {
        ui->lblAngle->setText(QString("选区旋转角度: %1°").arg(val));
        OnParamChanged();
    });
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

    UpdateShapeControlsVisibility();
    UpdateDisplayPreview();
}

MaskEditorDialog::~MaskEditorDialog()
{
    delete ui;
}

void MaskEditorDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // 窗口显示完成后立即自动执行一次充满自适应
    QTimer::singleShot(50, this, &MaskEditorDialog::ZoomFit);
}

void MaskEditorDialog::ZoomIn()
{
    _scaleFactor *= 1.25;
    ui->graphicsView->scale(1.25, 1.25);
    UpdateZoomLabel();
}

void MaskEditorDialog::ZoomOut()
{
    _scaleFactor *= 0.8;
    ui->graphicsView->scale(0.8, 0.8);
    UpdateZoomLabel();
}

void MaskEditorDialog::ZoomFit()
{
    if (_pixmapItem->pixmap().isNull())
    {
        return;
    }
    ui->graphicsView->fitInView(_pixmapItem, Qt::KeepAspectRatio);
    _scaleFactor = ui->graphicsView->transform().m11();
    UpdateZoomLabel();
}

void MaskEditorDialog::ZoomOriginal()
{
    ui->graphicsView->resetTransform();
    _scaleFactor = 1.0;
    UpdateZoomLabel();
}

void MaskEditorDialog::UpdateZoomLabel()
{
    int percent = static_cast<int>(_scaleFactor * 100);
    ui->lblZoomPercent->setText(QString("%1%").arg(percent));
}

void MaskEditorDialog::OnShapeChanged(int index)
{
    Q_UNUSED(index);
    UpdateShapeControlsVisibility();
    OnParamChanged();
}

void MaskEditorDialog::UpdateShapeControlsVisibility()
{
    int shape = ui->comboShape->currentIndex();
    // 0: Rect, 1: Circle, 2: Ring, 3: Poly, 4: RotatedRect, 5: ThresholdAuto
    ui->containerPolySides->setVisible(shape == 3);
    ui->containerInnerRatio->setVisible(shape == 2);
    ui->containerAngle->setVisible(shape == 1 || shape == 2 || shape == 3 || shape == 4);
    ui->containerThresh->setVisible(shape == 5);
    ui->grpCoords->setVisible(shape != 5);
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
    op->SetParameter("polySides", ui->spinPolySides->value());
    op->SetParameter("innerRatio", ui->sliderInnerRatio->value());
    op->SetParameter("roiAngle", ui->sliderAngle->value());
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

    cv::Mat baseImg = (ui->comboTarget->currentIndex() == 1 && !_sourceImg.empty()) ? _sourceImg : _currentImg;

    if (ui->chkPreviewResult->isChecked())
    {
        // 模式 A: 预览算子复合处理的最终产物
        auto op = GetConfiguredOperator();
        cv::Mat previewOut;
        std::string err;
        if (op->Process(baseImg, previewOut, err))
        {
            QImage qimg = MatUtils::MatToQImage(previewOut);
            _pixmapItem->setPixmap(QPixmap::fromImage(qimg));
            _scene->setSceneRect(_pixmapItem->boundingRect());
        }
    }
    else
    {
        // 模式 B: 选区高亮定位编辑模式 (完整保留底图细节 + 外部半透明暗化 + 鲜艳轮廓线与锚点)
        cv::Mat displayMat;
        if (baseImg.channels() == 1)
        {
            cv::cvtColor(baseImg, displayMat, cv::COLOR_GRAY2BGR);
        }
        else
        {
            displayMat = baseImg.clone();
        }

        int shape = ui->comboShape->currentIndex();
        int rx = _currentRoi.x();
        int ry = _currentRoi.y();
        int rw = _currentRoi.width();
        int rh = _currentRoi.height();
        int polySides = ui->spinPolySides->value();
        int innerRatio = ui->sliderInnerRatio->value();
        int angle = ui->sliderAngle->value();
        int threshVal = ui->sliderThresh->value();

        // 1. 生成单通道二值掩码 (ROI 内部为 255, 外部为 0)
        cv::Mat mask = cv::Mat::zeros(displayMat.size(), CV_8UC1);

        if (shape == 0) // 矩形
        {
            cv::Rect roi(rx, ry, rw, rh);
            roi = roi & cv::Rect(0, 0, displayMat.cols, displayMat.rows);
            if (roi.width > 0 && roi.height > 0)
            {
                mask(roi).setTo(255);
            }
        }
        else if (shape == 1) // 圆形/椭圆
        {
            cv::Point center(rx + rw / 2, ry + rh / 2);
            cv::Size axes(std::max(1, rw / 2), std::max(1, rh / 2));
            cv::ellipse(mask, center, axes, static_cast<double>(angle), 0, 360, cv::Scalar(255), -1);
        }
        else if (shape == 2) // 圆环
        {
            cv::Point center(rx + rw / 2, ry + rh / 2);
            cv::Size outerAxes(std::max(2, rw / 2), std::max(2, rh / 2));
            int innerW = std::max(1, (rw / 2) * innerRatio / 100);
            int innerH = std::max(1, (rh / 2) * innerRatio / 100);
            cv::ellipse(mask, center, outerAxes, static_cast<double>(angle), 0, 360, cv::Scalar(255), -1);
            cv::ellipse(mask, center, cv::Size(innerW, innerH), static_cast<double>(angle), 0, 360, cv::Scalar(0), -1);
        }
        else if (shape == 3) // 正多边形
        {
            int n = std::max(3, polySides);
            cv::Point2f center(rx + rw / 2.0f, ry + rh / 2.0f);
            float radiusX = std::max(1.0f, rw / 2.0f);
            float radiusY = std::max(1.0f, rh / 2.0f);
            double baseRad = angle * CV_PI / 180.0 - CV_PI / 2.0;

            std::vector<cv::Point> polyPts(n);
            for (int k = 0; k < n; ++k)
            {
                double theta = baseRad + (2.0 * CV_PI * k) / n;
                int px = static_cast<int>(std::round(center.x + radiusX * std::cos(theta)));
                int py = static_cast<int>(std::round(center.y + radiusY * std::sin(theta)));
                polyPts[k] = cv::Point(px, py);
            }
            std::vector<std::vector<cv::Point>> ptsList = { polyPts };
            cv::fillPoly(mask, ptsList, cv::Scalar(255));
        }
        else if (shape == 4) // 旋转矩形
        {
            cv::Point2f center(rx + rw / 2.0f, ry + rh / 2.0f);
            cv::RotatedRect rRect(center, cv::Size2f(static_cast<float>(rw), static_cast<float>(rh)), static_cast<float>(angle));
            cv::Point2f pts4[4];
            rRect.points(pts4);
            std::vector<cv::Point> ptsList(4);
            for (int k = 0; k < 4; ++k)
            {
                ptsList[k] = cv::Point(static_cast<int>(std::round(pts4[k].x)), static_cast<int>(std::round(pts4[k].y)));
            }
            std::vector<std::vector<cv::Point>> polys = { ptsList };
            cv::fillPoly(mask, polys, cv::Scalar(255));
        }
        else // 阈值自动
        {
            cv::Mat gray = baseImg;
            if (baseImg.channels() > 1) cv::cvtColor(baseImg, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, mask, threshVal, 255, cv::THRESH_BINARY);
        }

        // 2. 对 ROI 外部执行 35% 半透明暗化蒙版 (ROI 内部保持 100% 原始细节明亮)
        cv::Mat dimmedImg;
        displayMat.convertTo(dimmedImg, -1, 0.45, 0); // 降低暗区亮度
        dimmedImg.copyTo(displayMat, mask == 0);

        // 3. 绘制亮黄色/鲜绿色几何选区轮廓辅助线与顶点锚点
        const cv::Scalar yellowColor(0, 255, 255);
        const cv::Scalar greenColor(0, 255, 128);

        // 自适应计算缩放比例
        double fontScale = std::clamp(std::max(displayMat.cols, displayMat.rows) / 1000.0 * 0.75, 0.75, 2.5);
        int lineWidth = std::max(2, static_cast<int>(std::round(fontScale * 2.0)));
        int handleSz = std::max(8, static_cast<int>(std::round(fontScale * 10.0)));

        if (shape == 0)
        {
            cv::Rect roi(rx, ry, rw, rh);
            cv::rectangle(displayMat, roi, yellowColor, lineWidth, cv::LINE_AA);
            // 四角控制手柄
            cv::rectangle(displayMat, cv::Rect(rx - handleSz / 2, ry - handleSz / 2, handleSz, handleSz), greenColor, -1);
            cv::rectangle(displayMat, cv::Rect(rx + rw - handleSz / 2, ry - handleSz / 2, handleSz, handleSz), greenColor, -1);
            cv::rectangle(displayMat, cv::Rect(rx - handleSz / 2, ry + rh - handleSz / 2, handleSz, handleSz), greenColor, -1);
            cv::rectangle(displayMat, cv::Rect(rx + rw - handleSz / 2, ry + rh - handleSz / 2, handleSz, handleSz), greenColor, -1);
        }
        else if (shape == 1)
        {
            cv::Point center(rx + rw / 2, ry + rh / 2);
            cv::Size axes(std::max(1, rw / 2), std::max(1, rh / 2));
            cv::ellipse(displayMat, center, axes, static_cast<double>(angle), 0, 360, yellowColor, lineWidth, cv::LINE_AA);
            cv::drawMarker(displayMat, center, greenColor, cv::MARKER_CROSS, handleSz * 2, lineWidth);
        }
        else if (shape == 2)
        {
            cv::Point center(rx + rw / 2, ry + rh / 2);
            cv::Size outerAxes(std::max(2, rw / 2), std::max(2, rh / 2));
            int innerW = std::max(1, (rw / 2) * innerRatio / 100);
            int innerH = std::max(1, (rh / 2) * innerRatio / 100);
            cv::ellipse(displayMat, center, outerAxes, static_cast<double>(angle), 0, 360, yellowColor, lineWidth, cv::LINE_AA);
            cv::ellipse(displayMat, center, cv::Size(innerW, innerH), static_cast<double>(angle), 0, 360, greenColor, lineWidth, cv::LINE_AA);
            cv::drawMarker(displayMat, center, greenColor, cv::MARKER_CROSS, handleSz * 2, lineWidth);
        }
        else if (shape == 3)
        {
            int n = std::max(3, polySides);
            cv::Point2f center(rx + rw / 2.0f, ry + rh / 2.0f);
            float radiusX = std::max(1.0f, rw / 2.0f);
            float radiusY = std::max(1.0f, rh / 2.0f);
            double baseRad = angle * CV_PI / 180.0 - CV_PI / 2.0;

            std::vector<cv::Point> polyPts(n);
            for (int k = 0; k < n; ++k)
            {
                double theta = baseRad + (2.0 * CV_PI * k) / n;
                int px = static_cast<int>(std::round(center.x + radiusX * std::cos(theta)));
                int py = static_cast<int>(std::round(center.y + radiusY * std::sin(theta)));
                polyPts[k] = cv::Point(px, py);
                cv::circle(displayMat, polyPts[k], std::max(4, handleSz / 2), greenColor, -1);
            }
            std::vector<std::vector<cv::Point>> ptsList = { polyPts };
            cv::polylines(displayMat, ptsList, true, yellowColor, lineWidth, cv::LINE_AA);
            cv::drawMarker(displayMat, cv::Point(static_cast<int>(center.x), static_cast<int>(center.y)), greenColor, cv::MARKER_CROSS, handleSz * 2, lineWidth);
        }
        else if (shape == 4)
        {
            cv::Point2f center(rx + rw / 2.0f, ry + rh / 2.0f);
            cv::RotatedRect rRect(center, cv::Size2f(static_cast<float>(rw), static_cast<float>(rh)), static_cast<float>(angle));
            cv::Point2f pts4[4];
            rRect.points(pts4);
            std::vector<cv::Point> ptsList(4);
            for (int k = 0; k < 4; ++k)
            {
                ptsList[k] = cv::Point(static_cast<int>(std::round(pts4[k].x)), static_cast<int>(std::round(pts4[k].y)));
                cv::circle(displayMat, ptsList[k], std::max(4, handleSz / 2), greenColor, -1);
            }
            std::vector<std::vector<cv::Point>> polys = { ptsList };
            cv::polylines(displayMat, polys, true, yellowColor, lineWidth, cv::LINE_AA);
        }
        else
        {
            // 自动阈值模式：在前景连通域轮廓画绿色细边线
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            cv::polylines(displayMat, contours, true, greenColor, lineWidth, cv::LINE_AA);
        }

        // 绘制尺寸标注标签 (带有半透明深色高对比度背景底框，字号增大且加粗)
        if (shape != 5)
        {
            QString tipText = QString("ROI: %1x%2 @ (%3, %4)").arg(rw).arg(rh).arg(rx).arg(ry);
            std::string textStr = tipText.toStdString();
            int thickness = std::max(2, static_cast<int>(std::round(fontScale * 2.0)));
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(textStr, cv::FONT_HERSHEY_SIMPLEX, fontScale, thickness, &baseline);

            int textMargin = static_cast<int>(6 * fontScale);
            int boxX = rx;
            int boxY = ry - textSize.height - textMargin * 2;
            if (boxY < 0)
            {
                boxY = ry + 8; // 若贴顶超出上边界，则放置在 ROI 区域顶部内侧
            }

            cv::Rect bgRect(boxX, boxY, textSize.width + textMargin * 2, textSize.height + textMargin * 2);
            bgRect = bgRect & cv::Rect(0, 0, displayMat.cols, displayMat.rows);
            if (bgRect.width > 0 && bgRect.height > 0)
            {
                // 绘制深色背景衬底与黄色边框
                cv::rectangle(displayMat, bgRect, cv::Scalar(24, 24, 24), -1);
                cv::rectangle(displayMat, bgRect, yellowColor, 1, cv::LINE_AA);

                cv::Point textPos(bgRect.x + textMargin, bgRect.y + textMargin + textSize.height);
                cv::putText(displayMat, textStr, textPos, cv::FONT_HERSHEY_SIMPLEX, fontScale, yellowColor, thickness, cv::LINE_AA);
            }
        }

        QImage qimg = MatUtils::MatToQImage(displayMat);
        _pixmapItem->setPixmap(QPixmap::fromImage(qimg));
        _scene->setSceneRect(_pixmapItem->boundingRect());
    }
}

bool MaskEditorDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->graphicsView->viewport())
    {
        if (event->type() == QEvent::Wheel)
        {
            auto* wheelEvent = static_cast<QWheelEvent*>(event);
            if (wheelEvent->angleDelta().y() > 0)
            {
                ZoomIn();
            }
            else
            {
                ZoomOut();
            }
            return true;
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::RightButton || mouseEvent->button() == Qt::MiddleButton)
            {
                // 右键/中键按下进入画布平移拖拽模式
                _isPanning = true;
                _panStartPos = mouseEvent->pos();
                ui->graphicsView->viewport()->setCursor(Qt::ClosedHandCursor);
                return true;
            }
            else if (mouseEvent->button() == Qt::LeftButton)
            {
                // 左键按下开始绘制/重设选区
                _isDrawing = true;
                QPointF sceneP = ui->graphicsView->mapToScene(mouseEvent->pos());
                _startPos = sceneP.toPoint();
                _currentRoi = QRect(_startPos, QSize(10, 10));
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);

            if (_isPanning)
            {
                QPoint delta = mouseEvent->pos() - _panStartPos;
                _panStartPos = mouseEvent->pos();
                ui->graphicsView->horizontalScrollBar()->setValue(ui->graphicsView->horizontalScrollBar()->value() - delta.x());
                ui->graphicsView->verticalScrollBar()->setValue(ui->graphicsView->verticalScrollBar()->value() - delta.y());
                return true;
            }
            else if (_isDrawing)
            {
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
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::RightButton || mouseEvent->button() == Qt::MiddleButton)
            {
                _isPanning = false;
                ui->graphicsView->viewport()->setCursor(Qt::ArrowCursor);
                return true;
            }
            else if (mouseEvent->button() == Qt::LeftButton && _isDrawing)
            {
                _isDrawing = false;
                UpdateDisplayPreview();
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}
