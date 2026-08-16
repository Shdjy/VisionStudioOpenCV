#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
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

signals:
    void PixelHovered(const PixelProbeInfo& probeInfo);
    void ImageUpdated(const cv::Mat& mat);

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

    void RenderDisplay();
    void UpdateZoomLabel();
    void HandleMouseMove(const QPointF& scenePos);
};
