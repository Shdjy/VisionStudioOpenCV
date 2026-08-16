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

    // 初始化测线矢量图元 (置于顶层)
    _profileLineItem = new QGraphicsPathItem();
    _profileLineItem->setZValue(100);
    _profileLineItem->setVisible(false);
    _scene->addItem(_profileLineItem);

    _startHandleItem = new QGraphicsEllipseItem(-5, -5, 10, 10);
    _startHandleItem->setPen(QPen(QColor(0, 255, 255), 2));
    _startHandleItem->setBrush(QBrush(QColor(0, 200, 255, 180)));
    _startHandleItem->setZValue(101);
    _startHandleItem->setVisible(false);
    _scene->addItem(_startHandleItem);

    _endHandleItem = new QGraphicsEllipseItem(-5, -5, 10, 10);
    _endHandleItem->setPen(QPen(QColor(255, 128, 0), 2));
    _endHandleItem->setBrush(QBrush(QColor(255, 100, 0, 180)));
    _endHandleItem->setZValue(101);
    _endHandleItem->setVisible(false);
    _scene->addItem(_endHandleItem);

    _profileTextItem = new QGraphicsTextItem();
    _profileTextItem->setDefaultTextColor(QColor(255, 255, 0));
    QFont font("Segoe UI", 9, QFont::Bold);
    _profileTextItem->setFont(font);
    _profileTextItem->setZValue(102);
    _profileTextItem->setVisible(false);
    _scene->addItem(_profileTextItem);

    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->graphicsView->viewport()->installEventFilter(this);
    ui->graphicsView->setMouseTracking(true);

    ui->sliderSplitPos->setVisible(false);

    connect(ui->btnZoomIn, &QToolButton::clicked, this, &ImageViewer::ZoomIn);
    connect(ui->btnZoomOut, &QToolButton::clicked, this, &ImageViewer::ZoomOut);
    connect(ui->btnZoomFit, &QToolButton::clicked, this, &ImageViewer::ZoomFit);
    connect(ui->btnZoomOriginal, &QToolButton::clicked, this, &ImageViewer::ZoomOriginal);
    connect(ui->btnLineProfile, &QToolButton::toggled, this, &ImageViewer::SetLineProfileActive);

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

    if (_isLineProfileMode && _hasValidProfileLine)
    {
        CalculateAndEmitLineProfile();
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
        else if (event->type() == QEvent::MouseButtonPress)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (_isLineProfileMode && mouseEvent->button() == Qt::LeftButton)
            {
                QPointF scenePos = ui->graphicsView->mapToScene(mouseEvent->pos());
                
                // 检查是否点中现有端点
                if (_hasValidProfileLine)
                {
                    double distStart = QLineF(scenePos, _profileStartPt).length();
                    double distEnd = QLineF(scenePos, _profileEndPt).length();
                    if (distStart <= 12.0)
                    {
                        _dragHandle = 1;
                        return true;
                    }
                    else if (distEnd <= 12.0)
                    {
                        _dragHandle = 2;
                        return true;
                    }
                }

                // 重新开始拉新测线
                _isDrawingLine = true;
                _dragHandle = 2;
                _profileStartPt = scenePos;
                _profileEndPt = scenePos;
                _hasValidProfileLine = true;
                UpdateProfileLineVisual();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            QPointF scenePos = ui->graphicsView->mapToScene(mouseEvent->pos());

            if (_isLineProfileMode)
            {
                if (_isDrawingLine || _dragHandle != 0)
                {
                    if (_dragHandle == 1)
                    {
                        _profileStartPt = scenePos;
                    }
                    else if (_dragHandle == 2)
                    {
                        _profileEndPt = scenePos;
                    }
                    UpdateProfileLineVisual();
                    CalculateAndEmitLineProfile();
                    return true;
                }
            }

            HandleMouseMove(scenePos);
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (_isLineProfileMode && mouseEvent->button() == Qt::LeftButton)
            {
                if (_isDrawingLine || _dragHandle != 0)
                {
                    _isDrawingLine = false;
                    _dragHandle = 0;
                    UpdateProfileLineVisual();
                    CalculateAndEmitLineProfile();
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ImageViewer::SetLineProfileActive(bool active)
{
    _isLineProfileMode = active;
    ui->btnLineProfile->setChecked(active);

    if (active)
    {
        ui->graphicsView->setDragMode(QGraphicsView::NoDrag);
        ui->graphicsView->viewport()->setCursor(Qt::CrossCursor);

        if (!_hasValidProfileLine && !_currentProcessedMat.empty())
        {
            int w = _currentProcessedMat.cols;
            int h = _currentProcessedMat.rows;
            _profileStartPt = QPointF(w * 0.2, h * 0.5);
            _profileEndPt = QPointF(w * 0.8, h * 0.5);
            _hasValidProfileLine = true;
        }

        UpdateProfileLineVisual();
        CalculateAndEmitLineProfile();
    }
    else
    {
        ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
        ui->graphicsView->viewport()->setCursor(Qt::ArrowCursor);
        _profileLineItem->setVisible(false);
        _startHandleItem->setVisible(false);
        _endHandleItem->setVisible(false);
        _profileTextItem->setVisible(false);
    }
}

void ImageViewer::UpdateProfileLineVisual()
{
    if (!_hasValidProfileLine || !_isLineProfileMode)
    {
        _profileLineItem->setVisible(false);
        _startHandleItem->setVisible(false);
        _endHandleItem->setVisible(false);
        _profileTextItem->setVisible(false);
        return;
    }

    _profileLineItem->setVisible(true);
    _startHandleItem->setVisible(true);
    _endHandleItem->setVisible(true);
    _profileTextItem->setVisible(true);

    QPainterPath path;
    path.moveTo(_profileStartPt);
    path.lineTo(_profileEndPt);
    _profileLineItem->setPath(path);

    QPen pen(QColor(0, 255, 255, 220), 2.0, Qt::DashLine, Qt::RoundCap);
    _profileLineItem->setPen(pen);

    _startHandleItem->setPos(_profileStartPt);
    _endHandleItem->setPos(_profileEndPt);

    double len = QLineF(_profileStartPt, _profileEndPt).length();
    QPointF midPt = (_profileStartPt + _profileEndPt) / 2.0;
    _profileTextItem->setPlainText(QString("📏 L: %1 px").arg(QString::number(len, 'f', 1)));
    _profileTextItem->setPos(midPt + QPointF(8, -20));
}

void ImageViewer::CalculateAndEmitLineProfile()
{
    if (_currentProcessedMat.empty() || !_hasValidProfileLine)
    {
        return;
    }

    LineProfileData data;
    data.startPoint = _profileStartPt;
    data.endPoint = _profileEndPt;

    double dx = _profileEndPt.x() - _profileStartPt.x();
    double dy = _profileEndPt.y() - _profileStartPt.y();
    data.lengthPx = std::sqrt(dx * dx + dy * dy);

    int numSamples = std::max(2, static_cast<int>(std::round(data.lengthPx)));
    data.samplePositions.resize(numSamples);
    data.grayValues.resize(numSamples);
    data.gradientValues.resize(numSamples, 0.0f);

    bool is3Channel = (_currentProcessedMat.channels() == 3);
    data.hasRgb = is3Channel;
    if (is3Channel)
    {
        data.bValues.resize(numSamples);
        data.gValues.resize(numSamples);
        data.rValues.resize(numSamples);
    }

    for (int i = 0; i < numSamples; ++i)
    {
        double t = (numSamples > 1) ? (static_cast<double>(i) / (numSamples - 1)) : 0.0;
        double curX = _profileStartPt.x() + t * dx;
        double curY = _profileStartPt.y() + t * dy;

        data.samplePositions[i] = static_cast<float>(t * data.lengthPx);

        int x0 = std::clamp(static_cast<int>(std::floor(curX)), 0, _currentProcessedMat.cols - 1);
        int x1 = std::clamp(x0 + 1, 0, _currentProcessedMat.cols - 1);
        int y0 = std::clamp(static_cast<int>(std::floor(curY)), 0, _currentProcessedMat.rows - 1);
        int y1 = std::clamp(y0 + 1, 0, _currentProcessedMat.rows - 1);

        double fx = curX - x0;
        double fy = curY - y0;

        if (is3Channel)
        {
            auto p00 = _currentProcessedMat.at<cv::Vec3b>(y0, x0);
            auto p01 = _currentProcessedMat.at<cv::Vec3b>(y0, x1);
            auto p10 = _currentProcessedMat.at<cv::Vec3b>(y1, x0);
            auto p11 = _currentProcessedMat.at<cv::Vec3b>(y1, x1);

            float b = static_cast<float>((1 - fx) * (1 - fy) * p00[0] + fx * (1 - fy) * p01[0] + (1 - fx) * fy * p10[0] + fx * fy * p11[0]);
            float g = static_cast<float>((1 - fx) * (1 - fy) * p00[1] + fx * (1 - fy) * p01[1] + (1 - fx) * fy * p10[1] + fx * fy * p11[1]);
            float r = static_cast<float>((1 - fx) * (1 - fy) * p00[2] + fx * (1 - fy) * p01[2] + (1 - fx) * fy * p10[2] + fx * fy * p11[2]);

            data.bValues[i] = b;
            data.gValues[i] = g;
            data.rValues[i] = r;
            data.grayValues[i] = 0.114f * b + 0.587f * g + 0.299f * r;
        }
        else
        {
            float v00 = _currentProcessedMat.at<uchar>(y0, x0);
            float v01 = _currentProcessedMat.at<uchar>(y0, x1);
            float v10 = _currentProcessedMat.at<uchar>(y1, x0);
            float v11 = _currentProcessedMat.at<uchar>(y1, x1);

            float val = static_cast<float>((1 - fx) * (1 - fy) * v00 + fx * (1 - fy) * v01 + (1 - fx) * fy * v10 + fx * fy * v11);
            data.grayValues[i] = val;
        }
    }

    float maxGrad = 0.0f;
    int maxGradIdx = -1;
    for (int i = 1; i < numSamples - 1; ++i)
    {
        float grad = std::abs(data.grayValues[i + 1] - data.grayValues[i - 1]) / 2.0f;
        data.gradientValues[i] = grad;
        if (grad > maxGrad)
        {
            maxGrad = grad;
            maxGradIdx = i;
        }
    }

    data.maxGradientValue = maxGrad;
    data.maxGradientIndex = maxGradIdx;
    if (maxGradIdx >= 0)
    {
        double t = static_cast<double>(maxGradIdx) / (numSamples - 1);
        data.maxGradientPoint = QPointF(_profileStartPt.x() + t * dx, _profileStartPt.y() + t * dy);
    }

    emit LineProfileUpdated(data);
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
