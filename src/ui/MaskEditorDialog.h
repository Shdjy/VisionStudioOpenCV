#pragma once

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <opencv2/opencv.hpp>
#include "../core/operators/InteractiveMaskOp.h"

namespace Ui {
class MaskEditorDialog;
}

class MaskEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MaskEditorDialog(const cv::Mat& currentImg, const cv::Mat& sourceImg, QWidget* parent = nullptr);
    ~MaskEditorDialog() override;

    std::unique_ptr<InteractiveMaskOp> GetConfiguredOperator() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void OnParamChanged();
    void OnApplyClicked();

private:
    Ui::MaskEditorDialog* ui;
    cv::Mat _currentImg;
    cv::Mat _sourceImg;

    QGraphicsScene* _scene = nullptr;
    QGraphicsPixmapItem* _pixmapItem = nullptr;

    bool _isDrawing = false;
    QPoint _startPos;
    QRect _currentRoi;

    void UpdateDisplayPreview();
};
