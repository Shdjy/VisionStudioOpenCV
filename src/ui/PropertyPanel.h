#pragma once

#include <QWidget>
#include "../bll/PipelineManager.h"

namespace Ui {
class PropertyPanel;
}

class PropertyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyPanel(QWidget* parent = nullptr);
    ~PropertyPanel() override;

    void SetPipelineManager(PipelineManager* pipelineMgr);

public slots:
    void SetCurrentStep(int stepIndex);
    void ShowOperatorPreview(const QString& opName);

signals:
    void ParameterChanged(int stepIndex);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Ui::PropertyPanel* ui;
    PipelineManager* _pipelineMgr = nullptr;
    int _currentStepIndex = -1;
    bool _isPreviewMode = false;
    std::unique_ptr<IOpNode> _previewOpNode;

    void ClearDynamicWidgets();
    void BuildParamControls(IOpNode* opNode, bool isPreview = false);
};
