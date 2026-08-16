#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <opencv2/opencv.hpp>
#include "../utils/MatUtils.h"

namespace Ui {
class ImageViewer;
}

enum class ViewCompareMode
{
    Single = 0,       // 单图模式
    SplitSlider = 1,  // 卷帘滑动对比
    SideBySide = 2,   // 左右并排对比
    TopBottom = 3     // 上下并排对比
};

/**
 * @brief 测线灰度与梯度剖面采样数据
 */
struct LineProfileData
{
    QPointF startPoint;
    QPointF endPoint;
    double lengthPx = 0.0;
    std::vector<float> samplePositions;
    std::vector<float> grayValues;
    std::vector<float> bValues;
    std::vector<float> gValues;
    std::vector<float> rValues;
    std::vector<float> gradientValues;
    int maxGradientIndex = -1;
    QPointF maxGradientPoint;
    float maxGradientValue = 0.0f;
    bool hasRgb = false;
};

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget* parent = nullptr);
    ~ImageViewer() override;

    /**
     * @brief 设置待展示的图像与原图
     */
    void SetImage(const cv::Mat& processedMat, const cv::Mat& originalMat = cv::Mat());

    /**
     * @brief 获取当前展示的核心图像
     */
    const cv::Mat& GetCurrentImage() const { return _currentProcessedMat; }

public slots:
    void ZoomIn();
    void ZoomOut();
    void ZoomFit();
    void ZoomOriginal();
    void SetLineProfileActive(bool active);

signals:
    void PixelHovered(const PixelProbeInfo& probeInfo);
    void ImageUpdated(const cv::Mat& mat);
    void LineProfileUpdated(const LineProfileData& profileData);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void OnCompareModeChanged(int index);
    void OnSplitSliderChanged(int value);

private:
    Ui::ImageViewer* ui;
    QGraphicsScene* _scene = nullptr;
    QGraphicsPixmapItem* _pixmapItem = nullptr;

    cv::Mat _currentProcessedMat;
    cv::Mat _currentOriginalMat;

    double _scaleFactor = 1.0;
    ViewCompareMode _compareMode = ViewCompareMode::Single;
    int _splitPercent = 50;

    // 测线剖面交互状态
    bool _isLineProfileMode = false;
    bool _isDrawingLine = false;
    int _dragHandle = 0; // 0: None, 1: Start, 2: End
    QPointF _profileStartPt;
    QPointF _profileEndPt;
    bool _hasValidProfileLine = false;

    // 测线矢量图元
    QGraphicsPathItem* _profileLineItem = nullptr;
    QGraphicsEllipseItem* _startHandleItem = nullptr;
    QGraphicsEllipseItem* _endHandleItem = nullptr;
    QGraphicsTextItem* _profileTextItem = nullptr;

    void RenderDisplay();
    void UpdateZoomLabel();
    void HandleMouseMove(const QPointF& scenePos);
    void UpdateProfileLineVisual();
    void CalculateAndEmitLineProfile();
};
