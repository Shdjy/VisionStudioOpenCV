#pragma once

#include <QWidget>
#include <opencv2/opencv.hpp>

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

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::HistogramWidget* ui;
    cv::Mat _currentMat;

    void RenderHistogram();
};
