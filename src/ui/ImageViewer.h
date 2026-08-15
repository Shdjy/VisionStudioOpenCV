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

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget* parent = nullptr);
    ~ImageViewer() override;

    /**
     * @brief 显示处理结果图像
     */
    void SetImage(const cv::Mat& processedMat, const cv::Mat& originalMat = cv::Mat());

    /**
     * @brief 获取当前显示的图像
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
    void OnSplitViewToggled(bool checked);
    void OnSplitSliderChanged(int value);

private:
    Ui::ImageViewer* ui;
    QGraphicsScene* _scene = nullptr;
    QGraphicsPixmapItem* _pixmapItem = nullptr;

    cv::Mat _currentProcessedMat;
    cv::Mat _currentOriginalMat;

    double _scaleFactor = 1.0;
    bool _isSplitView = false;
    int _splitPercent = 50;

    void RenderDisplay();
    void UpdateZoomLabel();
    void HandleMouseMove(const QPointF& scenePos);
};
