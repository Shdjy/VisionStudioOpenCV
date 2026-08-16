#include "ImageViewer.h"
#include "ui_ImageViewer.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>

ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ImageViewer)
{
    ui->setupUi(this);

    _scene = new QGraphicsScene(this);
    _pixmapItem = new QGraphicsPixmapItem();
    _scene->addItem(_pixmapItem);
    ui->graphicsView->setScene(_scene);

    ui->graphicsView->setRenderHint(QPainter::Antialiasing, false);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->graphicsView->viewport()->installEventFilter(this);
    ui->graphicsView->setMouseTracking(true);

    ui->sliderSplitPos->setVisible(false);

    connect(ui->btnZoomIn, &QToolButton::clicked, this, &ImageViewer::ZoomIn);
    connect(ui->btnZoomOut, &QToolButton::clicked, this, &ImageViewer::ZoomOut);
    connect(ui->btnZoomFit, &QToolButton::clicked, this, &ImageViewer::ZoomFit);
    connect(ui->btnZoomOriginal, &QToolButton::clicked, this, &ImageViewer::ZoomOriginal);

    connect(ui->comboCompareMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageViewer::OnCompareModeChanged);
    connect(ui->sliderSplitPos, &QSlider::valueChanged, this, &ImageViewer::OnSplitSliderChanged);
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

void ImageViewer::SetImage(const cv::Mat& processedMat, const cv::Mat& originalMat)
{
    _currentProcessedMat = processedMat;
    if (!originalMat.empty())
    {
        _currentOriginalMat = originalMat;
    }
    else if (_currentOriginalMat.empty() && !processedMat.empty())
    {
        _currentOriginalMat = processedMat.clone();
    }

    RenderDisplay();
    emit ImageUpdated(_currentProcessedMat);

    if (!_currentProcessedMat.empty())
    {
        QString info = QString("分辨率: %1x%2 | 通道: %3 | 类型: %4")
            .arg(_currentProcessedMat.cols)
            .arg(_currentProcessedMat.rows)
            .arg(_currentProcessedMat.channels())
            .arg(MatUtils::GetMatTypeString(_currentProcessedMat));
        ui->lblImageInfo->setText(info);
    }
}

void ImageViewer::OnCompareModeChanged(int index)
{
    _compareMode = static_cast<ViewCompareMode>(index);
    ui->sliderSplitPos->setVisible(_compareMode == ViewCompareMode::SplitSlider);
    RenderDisplay();
}

void ImageViewer::OnSplitSliderChanged(int value)
{
    _splitPercent = value;
    if (_compareMode == ViewCompareMode::SplitSlider)
    {
        RenderDisplay();
    }
}

void ImageViewer::RenderDisplay()
{
    if (_currentProcessedMat.empty())
    {
        _pixmapItem->setPixmap(QPixmap());
        return;
    }

    cv::Mat displayMat;

    if (_compareMode == ViewCompareMode::SplitSlider && !_currentOriginalMat.empty() &&
        _currentOriginalMat.size() == _currentProcessedMat.size())
    {
        // 1. 卷帘分屏模式
        cv::Mat origBGR, procBGR;
        if (_currentOriginalMat.channels() == 1) cv::cvtColor(_currentOriginalMat, origBGR, cv::COLOR_GRAY2BGR);
        else origBGR = _currentOriginalMat;

        if (_currentProcessedMat.channels() == 1) cv::cvtColor(_currentProcessedMat, procBGR, cv::COLOR_GRAY2BGR);
        else procBGR = _currentProcessedMat;

        displayMat = procBGR.clone();
        int splitCol = (procBGR.cols * _splitPercent) / 100;
        if (splitCol > 0 && splitCol < procBGR.cols)
        {
            cv::Rect leftRoi(0, 0, splitCol, procBGR.rows);
            origBGR(leftRoi).copyTo(displayMat(leftRoi));
            cv::line(displayMat, cv::Point(splitCol, 0), cv::Point(splitCol, procBGR.rows), cv::Scalar(0, 255, 255), 2);
        }
    }
    else if (_compareMode == ViewCompareMode::SideBySide && !_currentOriginalMat.empty())
    {
        // 2. 左右并排对比模式
        cv::Mat origBGR, procBGR;
        if (_currentOriginalMat.channels() == 1) cv::cvtColor(_currentOriginalMat, origBGR, cv::COLOR_GRAY2BGR);
        else origBGR = _currentOriginalMat;

        if (_currentProcessedMat.channels() == 1) cv::cvtColor(_currentProcessedMat, procBGR, cv::COLOR_GRAY2BGR);
        else procBGR = _currentProcessedMat;

        int headerH = 26;
        int gap = 16;
        int maxH = std::max(origBGR.rows, procBGR.rows);
        int totalW = origBGR.cols + procBGR.cols + gap;
        int totalH = maxH + headerH;

        displayMat = cv::Mat(totalH, totalW, CV_8UC3, cv::Scalar(32, 32, 35));

        // 绘制标题
        cv::putText(displayMat, "Source Image", cv::Point(10, 18), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 128), 1, cv::LINE_AA);
        cv::putText(displayMat, "Processed Output", cv::Point(origBGR.cols + gap + 10, 18), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 180, 50), 1, cv::LINE_AA);

        // 放置原图与处理图
        cv::Rect r1(0, headerH, origBGR.cols, origBGR.rows);
        origBGR.copyTo(displayMat(r1));

        cv::Rect r2(origBGR.cols + gap, headerH, procBGR.cols, procBGR.rows);
        procBGR.copyTo(displayMat(r2));

        // 分割线
        int sepX = origBGR.cols + gap / 2;
        cv::line(displayMat, cv::Point(sepX, 0), cv::Point(sepX, totalH), cv::Scalar(60, 60, 65), 2);
    }
    else if (_compareMode == ViewCompareMode::TopBottom && !_currentOriginalMat.empty())
    {
        // 3. 上下并排对比模式
        cv::Mat origBGR, procBGR;
        if (_currentOriginalMat.channels() == 1) cv::cvtColor(_currentOriginalMat, origBGR, cv::COLOR_GRAY2BGR);
        else origBGR = _currentOriginalMat;

        if (_currentProcessedMat.channels() == 1) cv::cvtColor(_currentProcessedMat, procBGR, cv::COLOR_GRAY2BGR);
        else procBGR = _currentProcessedMat;

        int headerH = 24;
        int gap = 16;
        int maxW = std::max(origBGR.cols, procBGR.cols);
        int totalH = origBGR.rows + procBGR.rows + headerH * 2 + gap;

        displayMat = cv::Mat(totalH, maxW, CV_8UC3, cv::Scalar(32, 32, 35));

        // 绘制上方原图
        cv::putText(displayMat, "Source Image", cv::Point(10, 16), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 128), 1, cv::LINE_AA);
        cv::Rect r1(0, headerH, origBGR.cols, origBGR.rows);
        origBGR.copyTo(displayMat(r1));

        // 绘制下方处理图
        int procTop = headerH + origBGR.rows + gap;
        cv::putText(displayMat, "Processed Output", cv::Point(10, procTop + 16), cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(255, 180, 50), 1, cv::LINE_AA);
        cv::Rect r2(0, procTop + headerH, procBGR.cols, procBGR.rows);
        procBGR.copyTo(displayMat(r2));

        // 分割线
        int sepY = headerH + origBGR.rows + gap / 2;
        cv::line(displayMat, cv::Point(0, sepY), cv::Point(maxW, sepY), cv::Scalar(60, 60, 65), 2);
    }
    else
    {
        // 4. 单图标准显示模式
        displayMat = _currentProcessedMat;
    }

    QImage qimg = MatUtils::MatToQImage(displayMat);
    _pixmapItem->setPixmap(QPixmap::fromImage(qimg));
    _scene->setSceneRect(_pixmapItem->boundingRect());
}

void ImageViewer::ZoomIn()
{
    _scaleFactor *= 1.25;
    ui->graphicsView->scale(1.25, 1.25);
    UpdateZoomLabel();
}

void ImageViewer::ZoomOut()
{
    _scaleFactor *= 0.8;
    ui->graphicsView->scale(0.8, 0.8);
    UpdateZoomLabel();
}

void ImageViewer::ZoomFit()
{
    if (_pixmapItem->pixmap().isNull())
    {
        return;
    }

    ui->graphicsView->fitInView(_pixmapItem, Qt::KeepAspectRatio);
    _scaleFactor = ui->graphicsView->transform().m11();
    UpdateZoomLabel();
}

void ImageViewer::ZoomOriginal()
{
    ui->graphicsView->resetTransform();
    _scaleFactor = 1.0;
    UpdateZoomLabel();
}

void ImageViewer::UpdateZoomLabel()
{
    int percent = static_cast<int>(_scaleFactor * 100);
    ui->lblZoomPercent->setText(QString("%1%").arg(percent));
}

bool ImageViewer::eventFilter(QObject* watched, QEvent* event)
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
        else if (event->type() == QEvent::MouseMove)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = ui->graphicsView->mapToScene(mouseEvent->pos());
            HandleMouseMove(scenePos);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ImageViewer::HandleMouseMove(const QPointF& scenePos)
{
    if (_currentProcessedMat.empty())
    {
        return;
    }

    int x = static_cast<int>(scenePos.x());
    int y = static_cast<int>(scenePos.y());

    if (_compareMode == ViewCompareMode::Single || _compareMode == ViewCompareMode::SplitSlider)
    {
        if (x >= 0 && x < _currentProcessedMat.cols && y >= 0 && y < _currentProcessedMat.rows)
        {
            PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentProcessedMat, x, y);
            emit PixelHovered(probe);
            ui->lblPixelProbe->setText(QString("探针: %1").arg(QString::fromStdString(probe.FormattedText)));
        }
        else
        {
            ui->lblPixelProbe->setText("探针: 超出图像边界");
        }
    }
    else if (_compareMode == ViewCompareMode::SideBySide)
    {
        int headerH = 26;
        int gap = 16;
        int imgY = y - headerH;

        if (imgY >= 0 && imgY < _currentProcessedMat.rows)
        {
            if (x >= 0 && x < _currentOriginalMat.cols && !_currentOriginalMat.empty())
            {
                PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentOriginalMat, x, imgY);
                ui->lblPixelProbe->setText(QString("探针 [原图]: %1").arg(QString::fromStdString(probe.FormattedText)));
            }
            else if (x >= _currentOriginalMat.cols + gap && x < _currentOriginalMat.cols + gap + _currentProcessedMat.cols)
            {
                int procX = x - (_currentOriginalMat.cols + gap);
                PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentProcessedMat, procX, imgY);
                ui->lblPixelProbe->setText(QString("探针 [处理图]: %1").arg(QString::fromStdString(probe.FormattedText)));
            }
            else
            {
                ui->lblPixelProbe->setText("探针: 位于分割区域");
            }
        }
    }
    else if (_compareMode == ViewCompareMode::TopBottom)
    {
        int headerH = 24;
        int gap = 16;

        if (x >= 0 && x < _currentProcessedMat.cols)
        {
            if (y >= headerH && y < headerH + _currentOriginalMat.rows && !_currentOriginalMat.empty())
            {
                int origY = y - headerH;
                PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentOriginalMat, x, origY);
                ui->lblPixelProbe->setText(QString("探针 [原图]: %1").arg(QString::fromStdString(probe.FormattedText)));
            }
            else
            {
                int procTop = headerH + _currentOriginalMat.rows + gap + headerH;
                if (y >= procTop && y < procTop + _currentProcessedMat.rows)
                {
                    int procY = y - procTop;
                    PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentProcessedMat, x, procY);
                    ui->lblPixelProbe->setText(QString("探针 [处理图]: %1").arg(QString::fromStdString(probe.FormattedText)));
                }
            }
        }
    }
}
