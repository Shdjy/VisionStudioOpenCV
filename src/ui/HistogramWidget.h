#pragma once

#include <QWidget>
#include <opencv2/opencv.hpp>
#include "ImageViewer.h"

namespace Ui {
class HistogramWidget;
}

class HistogramWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    ~HistogramWidget() override;

public slots:
    void UpdateHistogram(const cv::Mat& mat);
    void UpdateLineProfile(const LineProfileData& profileData);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::HistogramWidget* ui;
    cv::Mat _currentMat;
    LineProfileData _profileData;
    bool _hasProfileData = false;

    void RenderHistogram();
    void RenderLineProfile();
};
