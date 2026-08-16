#include "HistogramWidget.h"
#include "ui_HistogramWidget.h"
#include "../utils/MatUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <iomanip>
#include <sstream>

// ============================================================================
// 1. 直方图原生绘制画布 (纯 paintEvent 自适应驱动，零尺寸反馈)
// ============================================================================
class HistogramCanvas : public QWidget
{
public:
    explicit HistogramCanvas(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(100, 60);
    }

    QSize sizeHint() const override { return QSize(260, 140); }

    void SetData(const std::vector<std::vector<float>>& data)
    {
        _histData = data;
        update();
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        int w = width();
        int h = height();
        painter.fillRect(rect(), QColor(26, 26, 28));

        if (_histData.empty())
        {
            painter.setPen(QColor(130, 130, 130));
            painter.drawText(rect(), Qt::AlignCenter, "无图像直方图数据");
            return;
        }

        // 绘制坐标背景线
        painter.setPen(QColor(48, 48, 52));
        painter.drawLine(0, h - 1, w, h - 1);
        painter.drawLine(0, h / 2, w, h / 2);

        int margin = 6;
        int drawW = w - margin * 2;
        int drawH = h - margin * 2;
        if (drawW <= 0 || drawH <= 0) return;

        if (_histData.size() == 1) // 灰度
        {
            painter.setPen(QPen(QColor(220, 220, 220), 1.5));
            const auto& grayH = _histData[0];
            QPolygonF poly;
            for (int i = 0; i < 256; ++i)
            {
                double x = margin + (static_cast<double>(i) / 255.0) * drawW;
                double y = (h - margin) - grayH[i] * drawH;
                poly << QPointF(x, y);
            }
            painter.drawPolyline(poly);
        }
        else if (_histData.size() >= 3) // BGR
        {
            QColor colors[3] = { QColor(64, 128, 255, 200), QColor(64, 255, 128, 200), QColor(255, 64, 64, 200) };
            for (size_t c = 0; c < 3; ++c)
            {
                painter.setPen(QPen(colors[c], 1.5));
                const auto& chH = _histData[c];
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
    }

private:
    std::vector<std::vector<float>> _histData;
};

// ============================================================================
// 2. 测线灰度剖面与梯度原生绘制画布 (纯 paintEvent 自适应驱动)
// ============================================================================
class ProfileCanvas : public QWidget
{
public:
    explicit ProfileCanvas(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(100, 80);
    }

    QSize sizeHint() const override { return QSize(260, 160); }

    void SetData(const LineProfileData& data)
    {
        _data = data;
        _hasData = (_data.lengthPx > 1.0 && !_data.grayValues.empty());
        update();
    }

protected:
    void paintEvent(QPaintEvent* /*event*/) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        int w = width();
        int h = height();
        painter.fillRect(rect(), QColor(26, 26, 28));

        if (!_hasData || _data.grayValues.empty())
        {
            painter.setPen(QColor(130, 130, 130));
            painter.drawText(rect(), Qt::AlignCenter, "请在视口开启【📏 测线剖面】并拖拽绘制测线");
            return;
        }

        int marginL = 34;
        int marginR = 10;
        int marginT = 14;
        int marginB = 18;

        int drawW = w - marginL - marginR;
        int totalH = h - marginT - marginB;
        if (drawW <= 0 || totalH <= 0) return;

        int upperH = static_cast<int>(totalH * 0.58);
        int lowerH = static_cast<int>(totalH * 0.35);
        int gap = totalH - upperH - lowerH;

        int upperY0 = marginT;
        int lowerY0 = marginT + upperH + gap;

        // 1. 网格与刻度
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

        // 2. 绘制折线
        size_t N = _data.grayValues.size();
        if (N >= 2)
        {
            if (_data.hasRgb && _data.rValues.size() == N)
            {
                QPolygonF polyR, polyG, polyB;
                for (size_t i = 0; i < N; ++i)
                {
                    double x = marginL + (static_cast<double>(i) / (N - 1)) * drawW;
                    double yR = (upperY0 + upperH) - (_data.rValues[i] / 255.0) * upperH;
                    double yG = (upperY0 + upperH) - (_data.gValues[i] / 255.0) * upperH;
                    double yB = (upperY0 + upperH) - (_data.bValues[i] / 255.0) * upperH;
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

            QPolygonF polyGray;
            for (size_t i = 0; i < N; ++i)
            {
                double x = marginL + (static_cast<double>(i) / (N - 1)) * drawW;
                double y = (upperY0 + upperH) - (_data.grayValues[i] / 255.0) * upperH;
                polyGray << QPointF(x, y);
            }
            painter.setPen(QPen(QColor(0, 255, 255), 1.8));
            painter.drawPolyline(polyGray);

            float maxGrad = std::max(1.0f, _data.maxGradientValue);
            QPolygonF polyGrad;
            for (size_t i = 0; i < N; ++i)
            {
                double x = marginL + (static_cast<double>(i) / (N - 1)) * drawW;
                double y = (lowerY0 + lowerH) - (_data.gradientValues[i] / maxGrad) * lowerH;
                polyGrad << QPointF(x, y);
            }
            painter.setPen(QPen(QColor(255, 180, 0), 1.5));
            painter.drawPolyline(polyGrad);

            if (_data.maxGradientIndex >= 0 && _data.maxGradientIndex < static_cast<int>(N))
            {
                double peakX = marginL + (static_cast<double>(_data.maxGradientIndex) / (N - 1)) * drawW;
                painter.setPen(QPen(QColor(255, 60, 60, 200), 1.2, Qt::DashLine));
                painter.drawLine(QPointF(peakX, upperY0), QPointF(peakX, lowerY0 + lowerH));

                painter.setBrush(QBrush(QColor(255, 60, 60)));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(QPointF(peakX, (upperY0 + upperH) - (_data.grayValues[_data.maxGradientIndex] / 255.0) * upperH), 3.5, 3.5);
                painter.drawEllipse(QPointF(peakX, (lowerY0 + lowerH) - (_data.gradientValues[_data.maxGradientIndex] / maxGrad) * lowerH), 3.5, 3.5);
            }
        }
    }

private:
    LineProfileData _data;
    bool _hasData = false;
};

// ============================================================================
// 3. HistogramWidget 主面板实现
// ============================================================================
HistogramWidget::HistogramWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::HistogramWidget)
{
    ui->setupUi(this);

    _histCanvas = new HistogramCanvas(this);
    ui->layoutHist->addWidget(_histCanvas);

    _profileCanvas = new ProfileCanvas(this);
    ui->layoutProfile->insertWidget(0, _profileCanvas);
}

HistogramWidget::~HistogramWidget()
{
    delete ui;
}

void HistogramWidget::UpdateHistogram(const cv::Mat& mat)
{
    if (mat.empty())
    {
        _histCanvas->SetData({});
        return;
    }

    std::vector<std::vector<float>> histData;
    if (MatUtils::CalculateHistogram(mat, histData))
    {
        _histCanvas->SetData(histData);
    }
}

void HistogramWidget::UpdateLineProfile(const LineProfileData& profileData)
{
    _profileCanvas->SetData(profileData);

    if (profileData.lengthPx > 1.0 && !profileData.grayValues.empty())
    {
        std::ostringstream ss;
        ss << "测线长度: " << std::fixed << std::setprecision(1) << profileData.lengthPx << " px | "
           << "起点: (" << static_cast<int>(profileData.startPoint.x()) << ", " << static_cast<int>(profileData.startPoint.y()) << ") -> "
           << "终点: (" << static_cast<int>(profileData.endPoint.x()) << ", " << static_cast<int>(profileData.endPoint.y()) << ")";
        if (profileData.maxGradientIndex >= 0)
        {
            ss << " | 边缘点: (" << static_cast<int>(profileData.maxGradientPoint.x()) << ", " << static_cast<int>(profileData.maxGradientPoint.y()) << ") 梯度: " << std::setprecision(1) << profileData.maxGradientValue;
        }
        ui->lblProfileInfo->setText(QString::fromStdString(ss.str()));
    }
    else
    {
        ui->lblProfileInfo->setText("测线状态: 就绪");
    }
}
