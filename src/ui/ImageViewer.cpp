#include "ImageViewer.h"
#include "ui_ImageViewer.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTimer>

ImageViewer::ImageViewer(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ImageViewer)
{
    ui->setupUi(this);

    // 1. 设置主布局 Stretch，确保 viewSplitter 充满所有可用区域
    ui->verticalLayout->setStretch(0, 0); // 顶部工具栏高度固定
    ui->verticalLayout->setStretch(1, 1); // 视口 Splitter 占满全部剩余空间
    ui->verticalLayout->setStretch(2, 0); // 底部状态栏高度固定

    ui->viewSplitter->setChildrenCollapsible(false);
    ui->viewSplitter->setStretchFactor(0, 1);
    ui->viewSplitter->setStretchFactor(1, 1);

    // 2. 初始化处理图视口场景 (主视口)
    _scene = new QGraphicsScene(this);
    _pixmapItem = new QGraphicsPixmapItem();
    _scene->addItem(_pixmapItem);
    ui->graphicsView->setScene(_scene);

    // 3. 初始化原图独立视口场景
    _sceneSource = new QGraphicsScene(this);
    _pixmapItemSource = new QGraphicsPixmapItem();
    _sceneSource->addItem(_pixmapItemSource);
    ui->viewSource->setScene(_sceneSource);

    // 初始化测线矢量图元 (置于主处理图场景顶层)
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

    // 视口抗锯齿与事件追踪
    ui->graphicsView->setRenderHint(QPainter::Antialiasing, true);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->graphicsView->viewport()->installEventFilter(this);
    ui->graphicsView->setMouseTracking(true);

    ui->viewSource->setRenderHint(QPainter::Antialiasing, true);
    ui->viewSource->setRenderHint(QPainter::SmoothPixmapTransform, true);
    ui->viewSource->viewport()->installEventFilter(this);
    ui->viewSource->setMouseTracking(true);

    _lastActiveView = ui->graphicsView;

    // 初始界面状态 (单图模式)
    ui->containerSource->setVisible(false);
    ui->lblTitleProcessed->setVisible(false);
    ui->sliderSplitPos->setVisible(false);
    ui->btnSyncViews->setVisible(false);

    // 默认开启同步联动
    _isSyncViews = true;
    ui->btnSyncViews->setChecked(true);

    connect(ui->btnZoomIn, &QToolButton::clicked, this, &ImageViewer::ZoomIn);
    connect(ui->btnZoomOut, &QToolButton::clicked, this, &ImageViewer::ZoomOut);
    connect(ui->btnZoomFit, &QToolButton::clicked, this, &ImageViewer::ZoomFit);
    connect(ui->btnZoomOriginal, &QToolButton::clicked, this, &ImageViewer::ZoomOriginal);
    connect(ui->btnLineProfile, &QToolButton::toggled, this, &ImageViewer::SetLineProfileActive);

    connect(ui->comboCompareMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageViewer::OnCompareModeChanged);
    connect(ui->sliderSplitPos, &QSlider::valueChanged, this, &ImageViewer::OnSplitSliderChanged);
    connect(ui->btnSyncViews, &QToolButton::toggled, this, &ImageViewer::OnSyncViewsToggled);

    // 监听滚动条联动同步
    auto syncScrollbars = [this](QGraphicsView* from, QGraphicsView* to) {
        if (!_isSyncViews || _isSyncing || !from || !to) return;
        _isSyncing = true;
        to->horizontalScrollBar()->setValue(from->horizontalScrollBar()->value());
        to->verticalScrollBar()->setValue(from->verticalScrollBar()->value());
        _isSyncing = false;
    };

    connect(ui->graphicsView->horizontalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
        syncScrollbars(ui->graphicsView, ui->viewSource);
    });
    connect(ui->graphicsView->verticalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
        syncScrollbars(ui->graphicsView, ui->viewSource);
    });
    connect(ui->viewSource->horizontalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
        syncScrollbars(ui->viewSource, ui->graphicsView);
    });
    connect(ui->viewSource->verticalScrollBar(), &QScrollBar::valueChanged, this, [=]() {
        syncScrollbars(ui->viewSource, ui->graphicsView);
    });
}

ImageViewer::~ImageViewer()
{
    delete ui;
}

void ImageViewer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(50, this, [this]() {
        if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
        {
            int total = (_compareMode == ViewCompareMode::SideBySide) ? ui->viewSplitter->width() : ui->viewSplitter->height();
            if (total > 0)
            {
                ui->viewSplitter->setSizes({ total / 2, total / 2 });
            }
        }
        ZoomFit();
    });
}

void ImageViewer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        int total = (_compareMode == ViewCompareMode::SideBySide) ? ui->viewSplitter->width() : ui->viewSplitter->height();
        if (total > 100)
        {
            ui->viewSplitter->setSizes({ total / 2, total / 2 });
        }
    }
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
    ui->btnSyncViews->setVisible(_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom);

    // 对比模式下默认开启同步联动
    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        _isSyncViews = true;
        ui->btnSyncViews->setChecked(true);
    }

    RenderDisplay();

    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        // 默认左右/上下各占 50%
        int total = (_compareMode == ViewCompareMode::SideBySide) ? ui->viewSplitter->width() : ui->viewSplitter->height();
        if (total <= 0)
        {
            total = (_compareMode == ViewCompareMode::SideBySide) ? 800 : 500;
        }
        ui->viewSplitter->setSizes({ total / 2, total / 2 });

        QTimer::singleShot(50, this, [this]() {
            int curTotal = (_compareMode == ViewCompareMode::SideBySide) ? ui->viewSplitter->width() : ui->viewSplitter->height();
            if (curTotal > 0)
            {
                ui->viewSplitter->setSizes({ curTotal / 2, curTotal / 2 });
            }
            ZoomFit();
            if (_isSyncViews)
            {
                SyncViewportState(ui->graphicsView, ui->viewSource);
            }
        });
    }
}

void ImageViewer::OnSplitSliderChanged(int value)
{
    _splitPercent = value;
    if (_compareMode == ViewCompareMode::SplitSlider)
    {
        RenderDisplay();
    }
}

void ImageViewer::OnSyncViewsToggled(bool checked)
{
    _isSyncViews = checked;
    if (_isSyncViews && (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom))
    {
        SyncViewportState(ui->graphicsView, ui->viewSource);
    }
}

void ImageViewer::RenderDisplay()
{
    if (_currentProcessedMat.empty())
    {
        _pixmapItem->setPixmap(QPixmap());
        if (_pixmapItemSource) _pixmapItemSource->setPixmap(QPixmap());
        return;
    }

    if (_compareMode == ViewCompareMode::SplitSlider && !_currentOriginalMat.empty() &&
        _currentOriginalMat.size() == _currentProcessedMat.size())
    {
        // 1. 卷帘分屏模式 (单视口混合)
        ui->containerSource->setVisible(false);
        ui->containerProcessed->setVisible(true);
        ui->lblTitleProcessed->setVisible(false);

        cv::Mat origBGR, procBGR;
        if (_currentOriginalMat.channels() == 1) cv::cvtColor(_currentOriginalMat, origBGR, cv::COLOR_GRAY2BGR);
        else origBGR = _currentOriginalMat;

        if (_currentProcessedMat.channels() == 1) cv::cvtColor(_currentProcessedMat, procBGR, cv::COLOR_GRAY2BGR);
        else procBGR = _currentProcessedMat;

        cv::Mat displayMat = procBGR.clone();
        int splitCol = (procBGR.cols * _splitPercent) / 100;
        if (splitCol > 0 && splitCol < procBGR.cols)
        {
            cv::Rect leftRoi(0, 0, splitCol, procBGR.rows);
            origBGR(leftRoi).copyTo(displayMat(leftRoi));
            cv::line(displayMat, cv::Point(splitCol, 0), cv::Point(splitCol, procBGR.rows), cv::Scalar(0, 255, 255), 2);
        }

        QImage qimg = MatUtils::MatToQImage(displayMat);
        _pixmapItem->setPixmap(QPixmap::fromImage(qimg));
        _scene->setSceneRect(_pixmapItem->boundingRect());
    }
    else if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        // 2 & 3. 左右/上下 独立双视口模式
        ui->viewSplitter->setOrientation((_compareMode == ViewCompareMode::SideBySide) ? Qt::Horizontal : Qt::Vertical);
        ui->containerSource->setVisible(true);
        ui->containerProcessed->setVisible(true);
        ui->lblTitleSource->setVisible(true);
        ui->lblTitleProcessed->setVisible(true);

        // 渲染左侧原图
        if (!_currentOriginalMat.empty())
        {
            QImage origQImg = MatUtils::MatToQImage(_currentOriginalMat);
            _pixmapItemSource->setPixmap(QPixmap::fromImage(origQImg));
            _sceneSource->setSceneRect(_pixmapItemSource->boundingRect());
        }

        // 渲染右侧处理图
        QImage procQImg = MatUtils::MatToQImage(_currentProcessedMat);
        _pixmapItem->setPixmap(QPixmap::fromImage(procQImg));
        _scene->setSceneRect(_pixmapItem->boundingRect());
    }
    else
    {
        // 4. 单图标准显示模式
        ui->containerSource->setVisible(false);
        ui->containerProcessed->setVisible(true);
        ui->lblTitleProcessed->setVisible(false);

        QImage qimg = MatUtils::MatToQImage(_currentProcessedMat);
        _pixmapItem->setPixmap(QPixmap::fromImage(qimg));
        _scene->setSceneRect(_pixmapItem->boundingRect());
    }
}

void ImageViewer::ZoomInView(QGraphicsView* view, double& scale)
{
    if (!view) return;
    scale *= 1.25;
    view->scale(1.25, 1.25);
    UpdateZoomLabel();
}

void ImageViewer::ZoomOutView(QGraphicsView* view, double& scale)
{
    if (!view) return;
    scale *= 0.8;
    view->scale(0.8, 0.8);
    UpdateZoomLabel();
}

void ImageViewer::ZoomFitView(QGraphicsView* view, QGraphicsPixmapItem* item, double& scale)
{
    if (!view || !item || item->pixmap().isNull()) return;
    view->setSceneRect(item->boundingRect());
    view->fitInView(item, Qt::KeepAspectRatio);
    scale = view->transform().m11();
    UpdateZoomLabel();
}

void ImageViewer::ZoomOriginalView(QGraphicsView* view, double& scale)
{
    if (!view) return;
    view->resetTransform();
    scale = 1.0;
    UpdateZoomLabel();
}

void ImageViewer::SyncViewportState(QGraphicsView* sourceView, QGraphicsView* targetView)
{
    if (!sourceView || !targetView || _isSyncing) return;
    _isSyncing = true;
    targetView->setTransform(sourceView->transform());
    targetView->horizontalScrollBar()->setValue(sourceView->horizontalScrollBar()->value());
    targetView->verticalScrollBar()->setValue(sourceView->verticalScrollBar()->value());
    _scaleFactorSource = sourceView->transform().m11();
    _scaleFactor = _scaleFactorSource;
    UpdateZoomLabel();
    _isSyncing = false;
}

void ImageViewer::ZoomIn()
{
    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        if (_isSyncViews)
        {
            ZoomInView(ui->graphicsView, _scaleFactor);
            SyncViewportState(ui->graphicsView, ui->viewSource);
        }
        else if (_lastActiveView == ui->viewSource)
        {
            ZoomInView(ui->viewSource, _scaleFactorSource);
        }
        else
        {
            ZoomInView(ui->graphicsView, _scaleFactor);
        }
    }
    else
    {
        ZoomInView(ui->graphicsView, _scaleFactor);
    }
}

void ImageViewer::ZoomOut()
{
    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        if (_isSyncViews)
        {
            ZoomOutView(ui->graphicsView, _scaleFactor);
            SyncViewportState(ui->graphicsView, ui->viewSource);
        }
        else if (_lastActiveView == ui->viewSource)
        {
            ZoomOutView(ui->viewSource, _scaleFactorSource);
        }
        else
        {
            ZoomOutView(ui->graphicsView, _scaleFactor);
        }
    }
    else
    {
        ZoomOutView(ui->graphicsView, _scaleFactor);
    }
}

void ImageViewer::ZoomFit()
{
    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        ZoomFitView(ui->viewSource, _pixmapItemSource, _scaleFactorSource);
        ZoomFitView(ui->graphicsView, _pixmapItem, _scaleFactor);
        if (_isSyncViews)
        {
            SyncViewportState(ui->graphicsView, ui->viewSource);
        }
    }
    else
    {
        ZoomFitView(ui->graphicsView, _pixmapItem, _scaleFactor);
    }
}

void ImageViewer::ZoomOriginal()
{
    if (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
    {
        ZoomOriginalView(ui->viewSource, _scaleFactorSource);
        ZoomOriginalView(ui->graphicsView, _scaleFactor);
    }
    else
    {
        ZoomOriginalView(ui->graphicsView, _scaleFactor);
    }
}

void ImageViewer::UpdateZoomLabel()
{
    double curScale = (_lastActiveView == ui->viewSource) ? _scaleFactorSource : _scaleFactor;
    int percent = static_cast<int>(curScale * 100);
    ui->lblZoomPercent->setText(QString("%1%").arg(percent));
}

bool ImageViewer::eventFilter(QObject* watched, QEvent* event)
{
    QGraphicsView* targetView = nullptr;
    QGraphicsView* otherView = nullptr;
    double* targetScale = nullptr;

    if (watched == ui->graphicsView->viewport())
    {
        targetView = ui->graphicsView;
        otherView = ui->viewSource;
        targetScale = &_scaleFactor;
        _lastActiveView = ui->graphicsView;
    }
    else if (watched == ui->viewSource->viewport())
    {
        targetView = ui->viewSource;
        otherView = ui->graphicsView;
        targetScale = &_scaleFactorSource;
        _lastActiveView = ui->viewSource;
    }

    if (targetView)
    {
        if (event->type() == QEvent::Wheel)
        {
            auto* wheelEvent = static_cast<QWheelEvent*>(event);
            if (wheelEvent->angleDelta().y() > 0)
            {
                ZoomInView(targetView, *targetScale);
            }
            else
            {
                ZoomOutView(targetView, *targetScale);
            }

            if (_isSyncViews && otherView && (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom))
            {
                SyncViewportState(targetView, otherView);
            }
            return true;
        }
        else if (event->type() == QEvent::MouseButtonPress)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (targetView == ui->graphicsView && _isLineProfileMode && mouseEvent->button() == Qt::LeftButton)
            {
                QPointF scenePos = targetView->mapToScene(mouseEvent->pos());
                
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
            QPointF scenePos = targetView->mapToScene(mouseEvent->pos());

            if (targetView == ui->graphicsView && _isLineProfileMode)
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

            HandleMouseMove(targetView, scenePos);
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (targetView == ui->graphicsView && _isLineProfileMode && mouseEvent->button() == Qt::LeftButton)
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

void ImageViewer::HandleMouseMove(QGraphicsView* view, const QPointF& scenePos)
{
    if (!view) return;

    int x = static_cast<int>(scenePos.x());
    int y = static_cast<int>(scenePos.y());

    if (view == ui->viewSource)
    {
        if (!_currentOriginalMat.empty() && x >= 0 && x < _currentOriginalMat.cols && y >= 0 && y < _currentOriginalMat.rows)
        {
            PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentOriginalMat, x, y);
            ui->lblPixelProbe->setText(QString("探针 [原图]: %1").arg(QString::fromStdString(probe.FormattedText)));
        }
        else
        {
            ui->lblPixelProbe->setText("探针 [原图]: 超出图像边界");
        }
    }
    else
    {
        if (!_currentProcessedMat.empty() && x >= 0 && x < _currentProcessedMat.cols && y >= 0 && y < _currentProcessedMat.rows)
        {
            PixelProbeInfo probe = MatUtils::GetPixelInfo(_currentProcessedMat, x, y);
            emit PixelHovered(probe);
            QString prefix = (_compareMode == ViewCompareMode::SideBySide || _compareMode == ViewCompareMode::TopBottom)
                                 ? "探针 [处理图]: "
                                 : "探针: ";
            ui->lblPixelProbe->setText(prefix + QString::fromStdString(probe.FormattedText));
        }
        else
        {
            ui->lblPixelProbe->setText("探针: 超出图像边界");
        }
    }
}
