#include "HistogramWidget.h"
#include "ui_HistogramWidget.h"
#include "../utils/MatUtils.h"
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>

HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HistogramWidget)
{
    ui->setupUi(this);
}

HistogramWidget::~HistogramWidget()
{
    delete ui;
}

void HistogramWidget::UpdateHistogram(const cv::Mat& mat)
{
    _currentMat = mat;
    RenderHistogram();
}

void HistogramWidget::resizeEvent(QResizeEvent* /*event*/)
{
    RenderHistogram();
}

void HistogramWidget::RenderHistogram()
{
    if (_currentMat.empty())
    {
        ui->lblHistogramDraw->setText("无图像数据");
        return;
    }

    std::vector<std::vector<float>> histData;
    if (!MatUtils::CalculateHistogram(_currentMat, histData) || histData.empty())
    {
        ui->lblHistogramDraw->setText("直方图计算失败");
        return;
    }

    int w = std::max(200, ui->lblHistogramDraw->width());
    int h = std::max(100, ui->lblHistogramDraw->height());

    QPixmap pix(w, h);
    pix.fill(QColor(30, 30, 30));

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 绘制坐标背景线
    painter.setPen(QColor(60, 60, 60));
    painter.drawLine(0, h - 1, w, h - 1);
    painter.drawLine(0, h / 2, w, h / 2);

    int margin = 5;
    int drawW = w - margin * 2;
    int drawH = h - margin * 2;

    if (histData.size() == 1) // 灰度
    {
        painter.setPen(QPen(QColor(200, 200, 200), 1.5));
        const auto& grayH = histData[0];
        QPolygonF poly;
        for (int i = 0; i < 256; ++i)
        {
            double x = margin + (static_cast<double>(i) / 255.0) * drawW;
            double y = (h - margin) - grayH[i] * drawH;
            poly << QPointF(x, y);
        }
        painter.drawPolyline(poly);
    }
    else if (histData.size() >= 3) // BGR
    {
        QColor colors[3] = { QColor(64, 128, 255, 200), QColor(64, 255, 128, 200), QColor(255, 64, 64, 200) };
        for (size_t c = 0; c < 3; ++c)
        {
            painter.setPen(QPen(colors[c], 1.5));
            const auto& chH = histData[c];
            QPolygonF poly;
            for (int i = 0; i < 256; ++i)
            {
                double x = margin + (static_cast<double>(i) / 255.0) * drawW;
                double y = (h - margin) - chH[i] * drawH;
                poly << QPointF(x, y);
            }
            painter.drawPolyline(poly);
        }
    }

    painter.end();
    ui->lblHistogramDraw->setPixmap(pix);
}
