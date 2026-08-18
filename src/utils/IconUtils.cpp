#include "IconUtils.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

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

    // 2. 绘制向下降落进入托盘的实心粗箭头 (Down Arrow)
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

    // 绘制空外方框
    QPen boxPen(color, size * 0.08, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(boxPen);
    painter.setBrush(Qt::NoBrush);
    qreal m = size * 0.16;
    painter.drawRoundedRect(QRectF(m, m, size - 2 * m, size - 2 * m), size * 0.1, size * 0.1);

    // 绘制内部减号/横线表示取消
    QPen dashPen(color, size * 0.10, Qt::SolidLine, Qt::RoundCap);
    painter.setPen(dashPen);
    painter.drawLine(QPointF(size * 0.32, size * 0.50), QPointF(size * 0.68, size * 0.50));

    return QIcon(pixmap);
}
