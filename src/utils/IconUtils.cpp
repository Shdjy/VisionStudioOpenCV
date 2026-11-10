#include "IconUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

QIcon IconUtils::CreateExportIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 绘制 U 型托盘 (Tray)
    QPen trayPen(color, size * 0.085, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(trayPen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath trayPath;
    qreal xLeft = size * 0.18;
    qreal xRight = size * 0.82;
    qreal yTop = size * 0.52;
    qreal yBottom = size * 0.84;

    trayPath.moveTo(xLeft, yTop);
    trayPath.lineTo(xLeft, yBottom);
    trayPath.lineTo(xRight, yBottom);
    trayPath.lineTo(xRight, yTop);
    painter.drawPath(trayPath);

    // 2. 绘制向上升起的实心粗箭头 (Up Arrow)
    QPainterPath arrowPath;
    qreal arrowTipY = size * 0.12;
    qreal arrowWingY = size * 0.38;
    qreal arrowStemBottomY = size * 0.66;
    qreal arrowCenterX = size * 0.50;
    qreal arrowLeftWingX = size * 0.24;
    qreal arrowRightWingX = size * 0.76;
    qreal stemHalfWidth = size * 0.09;

    arrowPath.moveTo(arrowCenterX, arrowTipY);
    arrowPath.lineTo(arrowLeftWingX, arrowWingY);
    arrowPath.lineTo(arrowCenterX - stemHalfWidth, arrowWingY);
    arrowPath.lineTo(arrowCenterX - stemHalfWidth, arrowStemBottomY);
    arrowPath.lineTo(arrowCenterX + stemHalfWidth, arrowStemBottomY);
    arrowPath.lineTo(arrowCenterX + stemHalfWidth, arrowWingY);
    arrowPath.lineTo(arrowRightWingX, arrowWingY);
    arrowPath.closeSubpath();

    painter.setPen(QPen(color, size * 0.02, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(color));
    painter.drawPath(arrowPath);

    return QIcon(pixmap);
}

QIcon IconUtils::CreateImportIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 绘制 U 型托盘 (Tray)
    QPen trayPen(color, size * 0.085, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(trayPen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath trayPath;
    qreal xLeft = size * 0.18;
    qreal xRight = size * 0.82;
    qreal yTop = size * 0.52;
    qreal yBottom = size * 0.84;

    trayPath.moveTo(xLeft, yTop);
    trayPath.lineTo(xLeft, yBottom);
    trayPath.lineTo(xRight, yBottom);
    trayPath.lineTo(xRight, yTop);
    painter.drawPath(trayPath);

    // 2. 绘制向下降落的实心粗箭头 (Down Arrow)
    QPainterPath arrowPath;
    qreal arrowStemTopY = size * 0.14;
    qreal arrowWingY = size * 0.42;
    qreal arrowTipY = size * 0.68;
    qreal arrowCenterX = size * 0.50;
    qreal arrowLeftWingX = size * 0.24;
    qreal arrowRightWingX = size * 0.76;
    qreal stemHalfWidth = size * 0.09;

    arrowPath.moveTo(arrowCenterX, arrowTipY);
    arrowPath.lineTo(arrowLeftWingX, arrowWingY);
    arrowPath.lineTo(arrowCenterX - stemHalfWidth, arrowWingY);
    arrowPath.lineTo(arrowCenterX - stemHalfWidth, arrowStemTopY);
    arrowPath.lineTo(arrowCenterX + stemHalfWidth, arrowStemTopY);
    arrowPath.lineTo(arrowCenterX + stemHalfWidth, arrowWingY);
    arrowPath.lineTo(arrowRightWingX, arrowWingY);
    arrowPath.closeSubpath();

    painter.setPen(QPen(color, size * 0.02, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(color));
    painter.drawPath(arrowPath);

    return QIcon(pixmap);
}

QIcon IconUtils::CreateRedCrossIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(color, size * 0.16, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);

    qreal margin = size * 0.22;
    painter.drawLine(margin, margin, size - margin, size - margin);
    painter.drawLine(margin, size - margin, size - margin, margin);

    return QIcon(pixmap);
}

QIcon IconUtils::CreateSelectAllIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制外方框
    QPen boxPen(color, size * 0.08, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(boxPen);
    painter.setBrush(Qt::NoBrush);
    qreal m = size * 0.16;
    painter.drawRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), size * 0.1, size * 0.1);

    // 绘制内部醒目 Check 勾号
    QPen checkPen(color, size * 0.12, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(checkPen);
    QPainterPath checkPath;
    checkPath.moveTo(size * 0.28, size * 0.50);
    checkPath.lineTo(size * 0.44, size * 0.68);
    checkPath.lineTo(size * 0.74, size * 0.32);
    painter.drawPath(checkPath);

    return QIcon(pixmap);
}

QIcon IconUtils::CreateDeselectAllIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制灰色外方框
    QPen boxPen(color, size * 0.08, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(boxPen);
    painter.setBrush(Qt::NoBrush);
    qreal m = size * 0.16;
    painter.drawRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), size * 0.1, size * 0.1);

    // 绘制中间减号横杠
    QPen minusPen(color, size * 0.10, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(minusPen);
    painter.drawLine(QPointF(size * 0.30, size * 0.50), QPointF(size * 0.70, size * 0.50));

    return QIcon(pixmap);
}

QIcon IconUtils::CreateAiSparkleIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    auto drawSparkle = [&](qreal cx, qreal cy, qreal r, qreal w) {
        QPainterPath path;
        path.moveTo(cx, cy - r);
        path.quadTo(cx, cy, cx + w, cy);
        path.quadTo(cx, cy, cx, cy + r);
        path.quadTo(cx, cy, cx - w, cy);
        path.quadTo(cx, cy, cx, cy - r);
        path.closeSubpath();
        painter.fillPath(path, QBrush(color));
    };

    // 1. 主 AI 4角星芒 (居中偏左下)
    drawSparkle(size * 0.44, size * 0.52, size * 0.40, size * 0.40);

    // 2. 右上角伴随副星芒
    QColor secondaryColor = color.lighter(130);
    painter.setBrush(QBrush(secondaryColor));
    drawSparkle(size * 0.78, size * 0.26, size * 0.20, size * 0.20);

    // 3. 左下方微小星芒
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(color.lighter(115)));
    painter.drawEllipse(QPointF(size * 0.18, size * 0.24), size * 0.055, size * 0.055);

    return QIcon(pixmap);
}

QIcon IconUtils::CreateAiChipIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制芯片外围引脚 (上下左右)
    QPen pinPen(color.darker(110), size * 0.06, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(pinPen);

    qreal chipLeft = size * 0.24;
    qreal chipRight = size * 0.76;
    qreal chipTop = size * 0.24;
    qreal chipBottom = size * 0.76;

    // 左右引脚
    for (int i = 0; i < 3; ++i)
    {
        qreal y = chipTop + (chipBottom - chipTop) * (i + 1) / 4.0;
        painter.drawLine(QPointF(size * 0.10, y), QPointF(chipLeft, y));
        painter.drawLine(QPointF(chipRight, y), QPointF(size * 0.90, y));
    }
    // 上下引脚
    for (int i = 0; i < 3; ++i)
    {
        qreal x = chipLeft + (chipRight - chipLeft) * (i + 1) / 4.0;
        painter.drawLine(QPointF(x, size * 0.10), QPointF(x, chipTop));
        painter.drawLine(QPointF(x, chipBottom), QPointF(x, size * 0.90));
    }

    // 绘制中央芯片本体
    painter.setPen(QPen(color, size * 0.06, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(color.darker(250)));
    painter.drawRoundedRect(QRectF(chipLeft, chipTop, chipRight - chipLeft, chipBottom - chipTop), size * 0.08, size * 0.08);

    // 芯片内部 AI 核心文字
    QFont font = painter.font();
    font.setPixelSize(static_cast<int>(size * 0.28));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(QRectF(chipLeft, chipTop, chipRight - chipLeft, chipBottom - chipTop), Qt::AlignCenter, "AI");

    return QIcon(pixmap);
}

QIcon IconUtils::CreateSettingsGearIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    qreal cx = size * 0.50;
    qreal cy = size * 0.50;
    qreal rOuter = size * 0.40;
    qreal rInner = size * 0.30;
    qreal rHole = size * 0.14;
    int teeth = 6;

    for (int i = 0; i < teeth; ++i)
    {
        double angleStep = 2.0 * M_PI / teeth;
        double a0 = i * angleStep;
        double a1 = a0 + angleStep * 0.28;
        double a2 = a0 + angleStep * 0.50;
        double a3 = a0 + angleStep * 0.78;

        QPointF p0(cx + rInner * std::cos(a0), cy + rInner * std::sin(a0));
        QPointF p1(cx + rOuter * std::cos(a1), cy + rOuter * std::sin(a1));
        QPointF p2(cx + rOuter * std::cos(a2), cy + rOuter * std::sin(a2));
        QPointF p3(cx + rInner * std::cos(a3), cy + rInner * std::sin(a3));

        if (i == 0)
        {
            path.moveTo(p0);
        }
        else
        {
            path.lineTo(p0);
        }
        path.lineTo(p1);
        path.lineTo(p2);
        path.lineTo(p3);
    }
    path.closeSubpath();

    // 镂空中孔
    QPainterPath holePath;
    holePath.addEllipse(QPointF(cx, cy), rHole, rHole);
    path = path.subtracted(holePath);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(color));
    painter.drawPath(path);

    return QIcon(pixmap);
}

QIcon IconUtils::CreateInspectionReticleIcon(const QColor& color, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. 绘制 4 个角落标定瞄准框
    QPen bracketPen(color, size * 0.08, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(bracketPen);

    qreal m = size * 0.14;
    qreal arm = size * 0.22;

    // 左上角
    painter.drawLine(QPointF(m, m + arm), QPointF(m, m));
    painter.drawLine(QPointF(m, m), QPointF(m + arm, m));

    // 右上角
    painter.drawLine(QPointF(size - m - arm, m), QPointF(size - m, m));
    painter.drawLine(QPointF(size - m, m), QPointF(size - m, m + arm));

    // 左下角
    painter.drawLine(QPointF(m, size - m - arm), QPointF(m, size - m));
    painter.drawLine(QPointF(m, size - m), QPointF(m + arm, size - m));

    // 右下角
    painter.drawLine(QPointF(size - m - arm, size - m), QPointF(size - m, size - m));
    painter.drawLine(QPointF(size - m, size - m), QPointF(size - m, size - m - arm));

    // 2. 中央靶心十字与小圆圈
    qreal cx = size * 0.50;
    qreal cy = size * 0.50;
    qreal crossArm = size * 0.16;

    QPen crossPen(color, size * 0.065, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(crossPen);
    painter.drawLine(QPointF(cx - crossArm, cy), QPointF(cx + crossArm, cy));
    painter.drawLine(QPointF(cx, cy - crossArm), QPointF(cx, cy + crossArm));

    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(cx, cy), size * 0.08, size * 0.08);

    return QIcon(pixmap);
}
