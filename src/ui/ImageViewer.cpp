#include "ImageViewer.h"
#include "ui_ImageViewer.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPainter>
#include <cmath>

ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ImageViewer)
{
    ui->setupUi(this);

    _scene = new QGraphicsScene(this);
    _pixmapItem = new QGraphicsPixmapItem();
    _scene->addItem(_pixmapItem);
    ui->graphicsView->setScene(_scene);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->graphicsView->setMouseTracking(true);
    ui->graphicsView->viewport()->installEventFilter(this);

    ui->sliderSplitPos->setVisible(false);

    connect(ui->btnZoomIn, &QToolButton::clicked, this, &ImageViewer::ZoomIn);
    connect(ui->btnZoomOut, &QToolButton::clicked, this, &ImageViewer::ZoomOut);
    connect(ui->btnZoomFit, &QToolButton::clicked, this, &ImageViewer::ZoomFit);
    connect(ui->btnZoomOriginal, &QToolButton::clicked, this, &ImageViewer::ZoomOriginal);

    connect(ui->chkSplitView, &QCheckBox::toggled, this, &ImageViewer::OnSplitViewToggled);
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

    if (_currentProcessedMat.empty())
    {
        _pixmapItem->setPixmap(QPixmap());
        ui->lblImageInfo->setText("分辨率: 0x0 | 通道: 0");
        return;
    }

    ui->lblImageInfo->setText(QString("分辨率: %1x%2 | 通道: %3 | 类型: %4")
                                  .arg(_currentProcessedMat.cols)
                                  .arg(_currentProcessedMat.rows)
                                  .arg(_currentProcessedMat.channels())
                                  .arg(_currentProcessedMat.depth() == CV_8U ? "8-Bit" : "Float"));

    RenderDisplay();
    emit ImageUpdated(_currentProcessedMat);
}

void ImageViewer::RenderDisplay()
{
    if (_currentProcessedMat.empty())
    {
        return;
    }

    if (!_isSplitView || _currentOriginalMat.empty() ||
        _currentOriginalMat.size() != _currentProcessedMat.size())
    {
        QImage qImg = MatUtils::MatToQImage(_currentProcessedMat);
        _pixmapItem->setPixmap(QPixmap::fromImage(qImg));
    }
    else
    {
        // 拼接分屏图像：左侧原图，右侧处理图
        cv::Mat displayMat = _currentProcessedMat.clone();
        if (displayMat.channels() == 1 && _currentOriginalMat.channels() == 3)
        {
            cv::cvtColor(displayMat, displayMat, cv::COLOR_GRAY2BGR);
        }
        else if (displayMat.channels() == 3 && _currentOriginalMat.channels() == 1)
        {
            cv::cvtColor(_currentOriginalMat, _currentOriginalMat, cv::COLOR_GRAY2BGR);
        }

        int splitCol = (_splitPercent * displayMat.cols) / 100;
        if (splitCol > 0 && splitCol < displayMat.cols)
        {
            cv::Rect leftRect(0, 0, splitCol, displayMat.rows);
            if (_currentOriginalMat.channels() == displayMat.channels())
            {
                _currentOriginalMat(leftRect).copyTo(displayMat(leftRect));
            }
            // 绘制分屏指示虚线
            cv::line(displayMat, cv::Point(splitCol, 0), cv::Point(splitCol, displayMat.rows), cv::Scalar(0, 255, 255), 2);
        }

        QImage qImg = MatUtils::MatToQImage(displayMat);
        _pixmapItem->setPixmap(QPixmap::fromImage(qImg));
    }

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
    // 计算当前等效比例
    QTransform t = ui->graphicsView->transform();
    _scaleFactor = t.m11();
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
    int pct = static_cast<int>(_scaleFactor * 100.0);
    ui->lblZoomPercent->setText(QString::number(pct) + "%");
}

void ImageViewer::OnSplitViewToggled(bool checked)
{
    _isSplitView = checked;
    ui->sliderSplitPos->setVisible(checked);
    RenderDisplay();
}

void ImageViewer::OnSplitSliderChanged(int value)
{
    _splitPercent = value;
    RenderDisplay();
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

    if (x >= 0 && x < _currentProcessedMat.cols && y >= 0 && y < _currentProcessedMat.rows)
    {
        PixelProbeInfo info = MatUtils::GetPixelInfo(_currentProcessedMat, x, y);
        ui->lblPixelProbe->setText(QString::fromStdString("探针: " + info.FormattedText));
        emit PixelHovered(info);
    }
    else
    {
        ui->lblPixelProbe->setText("探针: 坐标超出有效范围");
    }
}
