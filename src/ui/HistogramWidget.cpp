#include "HistogramWidget.h"
#include "ui_HistogramWidget.h"
#include "../utils/MatUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QResizeEvent>
#include <iomanip>
#include <sstream>

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

void HistogramWidget::UpdateLineProfile(const LineProfileData& profileData)
{
    _profileData = profileData;
    _hasProfileData = (_profileData.lengthPx > 1.0 && !_profileData.grayValues.empty());

    if (_hasProfileData)
    {
        ui->tabWidget->setCurrentIndex(1);
    }
    RenderLineProfile();
}

void HistogramWidget::resizeEvent(QResizeEvent* /*event*/)
{
    RenderHistogram();
    RenderLineProfile();
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

    int margin = 8;
    int drawW = w - margin * 2;
    int drawH = h - margin * 2;

    if (histData.size() == 1) // 灰度
    {
        painter.setPen(QPen(QColor(220, 220, 220), 1.5));
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

void HistogramWidget::RenderLineProfile()
{
    if (!_hasProfileData || _profileData.grayValues.empty())
    {
        ui->lblProfileDraw->setText("请在视口开启【📏 测线剖面】并拖拽绘制测线");
        ui->lblProfileInfo->setText("测线状态: 就绪");
        return;
    }

    int w = std::max(200, ui->lblProfileDraw->width());
    int h = std::max(120, ui->lblProfileDraw->height());

    QPixmap pix(w, h);
    pix.fill(QColor(26, 26, 28));

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int marginL = 36;
    int marginR = 12;
    int marginT = 16;
    int marginB = 24;

    int drawW = w - marginL - marginR;
    int totalH = h - marginT - marginB;

    // 上半部分为灰度剖面 (60% 高度)，下半部分为梯度曲线 (35% 高度)
    int upperH = static_cast<int>(totalH * 0.58);
    int lowerH = static_cast<int>(totalH * 0.35);
    int gap = totalH - upperH - lowerH;

    int upperY0 = marginT;
    int lowerY0 = marginT + upperH + gap;

    // 1. 绘制网格与刻度
    painter.setPen(QColor(48, 48, 52));
    painter.drawRect(marginL, upperY0, drawW, upperH);
    painter.drawRect(marginL, lowerY0, drawW, lowerH);

    painter.drawLine(marginL, upperY0 + upperH / 2, marginL + drawW, upperY0 + upperH / 2);
    painter.drawLine(marginL, lowerY0 + lowerH / 2, marginL + drawW, lowerY0 + lowerH / 2);

    painter.setFont(QFont("Segoe UI", 7));
    painter.setPen(QColor(160, 160, 160));
    painter.drawText(2, upperY0 + 10, "255");
    painter.drawText(2, upperY0 + upperH, "0");
    painter.drawText(marginL + 4, upperY0 + 12, "亮度 (Intensity)");

    painter.drawText(2, lowerY0 + 10, "Grad");
    painter.drawText(marginL + 4, lowerY0 + 12, "梯度 |dI/ds|");

    // 2. 绘制剖面曲线
    size_t N = _profileData.grayValues.size();
    if (N >= 2)
    {
        // 绘制 RGB 通道 (如有)
        if (_profileData.hasRgb && _profileData.rValues.size() == N)
        {
            QPolygonF polyR, polyG, polyB;
            for (size_t i = 0; i < N; ++i)
            {
                double x = marginL + (static_cast<double>(i) / (N - 1)) * drawW;
                double yR = (upperY0 + upperH) - (_profileData.rValues[i] / 255.0) * upperH;
                double yG = (upperY0 + upperH) - (_profileData.gValues[i] / 255.0) * upperH;
                double yB = (upperY0 + upperH) - (_profileData.bValues[i] / 255.0) * upperH;
                polyR << QPointF(x, yR);
                polyG << QPointF(x, yG);
                polyB << QPointF(x, yB);
            }
            painter.setPen(QPen(QColor(255, 60, 60, 140), 1.2));
            painter.drawPolyline(polyR);
            painter.setPen(QPen(QColor(60, 255, 60, 140), 1.2));
            painter.drawPolyline(polyG);
            painter.setPen(QPen(QColor(60, 140, 255, 140), 1.2));
            painter.drawPolyline(polyB);
        }

        // 绘制主灰度亮度折线
        QPolygonF polyGray;
        for (size_t i = 0; i < N; ++i)
        {
            double x = marginL + (static_cast<double>(i) / (N - 1)) * drawW;
            double y = (upperY0 + upperH) - (_profileData.grayValues[i] / 255.0) * upperH;
            polyGray << QPointF(x, y);
        }
        painter.setPen(QPen(QColor(0, 255, 255), 1.8));
        painter.drawPolyline(polyGray);

        // 3. 绘制梯度曲线
        float maxGrad = std::max(1.0f, _profileData.maxGradientValue);
        QPolygonF polyGrad;
        for (size_t i = 0; i < N; ++i)
        {
            double x = marginL + (static_cast<double>(i) / (N - 1)) * drawW;
            double y = (lowerY0 + lowerH) - (_profileData.gradientValues[i] / maxGrad) * lowerH;
            polyGrad << QPointF(x, y);
        }
        painter.setPen(QPen(QColor(255, 180, 0), 1.5));
        painter.drawPolyline(polyGrad);

        // 4. 标注最大梯度边缘点 (Edge Point)
        if (_profileData.maxGradientIndex >= 0 && _profileData.maxGradientIndex < static_cast<int>(N))
        {
            double peakX = marginL + (static_cast<double>(_profileData.maxGradientIndex) / (N - 1)) * drawW;
            painter.setPen(QPen(QColor(255, 60, 60, 200), 1.2, Qt::DashLine));
            painter.drawLine(QPointF(peakX, upperY0), QPointF(peakX, lowerY0 + lowerH));

            painter.setBrush(QBrush(QColor(255, 60, 60)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(peakX, (upperY0 + upperH) - (_profileData.grayValues[_profileData.maxGradientIndex] / 255.0) * upperH), 3.5, 3.5);
            painter.drawEllipse(QPointF(peakX, (lowerY0 + lowerH) - (_profileData.gradientValues[_profileData.maxGradientIndex] / maxGrad) * lowerH), 3.5, 3.5);
        }
    }

    painter.end();
    ui->lblProfileDraw->setPixmap(pix);

    // 5. 更新底部状态信息
    std::ostringstream ss;
    ss << "测线长度: " << std::fixed << std::setprecision(1) << _profileData.lengthPx << " px | "
       << "起点: (" << _profileData.startPoint.x() << ", " << _profileData.startPoint.y() << ") -> "
       << "终点: (" << _profileData.endPoint.x() << ", " << _profileData.endPoint.y() << ")";
    if (_profileData.maxGradientIndex >= 0)
    {
        ss << " | 边缘点: (" << _profileData.maxGradientPoint.x() << ", " << _profileData.maxGradientPoint.y() << ") 梯度: " << _profileData.maxGradientValue;
    }
    ui->lblProfileInfo->setText(QString::fromStdString(ss.str()));
}
