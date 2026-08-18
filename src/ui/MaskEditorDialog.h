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

public slots:
    void ZoomIn();
    void ZoomOut();
    void ZoomFit();
    void ZoomOriginal();

protected:
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void OnShapeChanged(int index);
    void OnParamChanged();
    void OnApplyClicked();

private:
    Ui::MaskEditorDialog* ui;
    cv::Mat _currentImg;
    cv::Mat _sourceImg;

    QGraphicsScene* _scene = nullptr;
    QGraphicsPixmapItem* _pixmapItem = nullptr;

    double _scaleFactor = 1.0;
    bool _isPanning = false;
    QPoint _panStartPos;

    bool _isDrawing = false;
    QPoint _startPos;
    QRect _currentRoi;

    void UpdateDisplayPreview();
    void UpdateZoomLabel();
    void UpdateShapeControlsVisibility();
};
