#pragma once

#include <QWidget>
#include <opencv2/opencv.hpp>
#include "ImageViewer.h"

namespace Ui {
class HistogramWidget;
}

class HistogramCanvas;
class ProfileCanvas;

class HistogramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    ~HistogramWidget() override;

public slots:
    void UpdateHistogram(const cv::Mat& mat);
    void UpdateLineProfile(const LineProfileData& profileData);

private:
    Ui::HistogramWidget* ui;
    HistogramCanvas* _histCanvas = nullptr;
    ProfileCanvas* _profileCanvas = nullptr;
};
