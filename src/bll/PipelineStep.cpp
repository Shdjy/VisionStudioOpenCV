#include "PipelineStep.h"

PipelineStep::PipelineStep(std::unique_ptr<IOpNode> opNode)
    : _opNode(std::move(opNode))
    , _enabled(true)
    , _isDirty(true)
    , _executionTimeMs(0.0)
{
}
