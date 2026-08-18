#include "PipelineManager.h"
#include "../utils/Logger.h"
#include "../core/PipelineContext.h"
#include <chrono>
#include <exception>

PipelineManager::PipelineManager(QObject* parent)
    : QObject(parent)
{
}

void PipelineManager::SetSourceImage(const cv::Mat& img)
{
    _sourceImage = img.clone();
    PipelineContext::Instance().SetSourceImage(_sourceImage);
    MarkDirtyFrom(0);
    emit PipelineChanged();
}

int PipelineManager::AddStep(std::unique_ptr<IOpNode> opNode)
{
    if (!opNode)
    {
        return -1;
    }

    auto step = std::make_unique<PipelineStep>(std::move(opNode));
    int idx = static_cast<int>(_steps.size());
    _steps.push_back(std::move(step));
    MarkDirtyFrom(idx);
    emit PipelineChanged();
    return idx;
}

bool PipelineManager::InsertStep(int index, std::unique_ptr<IOpNode> opNode)
{
    if (!opNode || index < 0 || index > static_cast<int>(_steps.size()))
    {
        return false;
    }

    auto step = std::make_unique<PipelineStep>(std::move(opNode));
    _steps.insert(_steps.begin() + index, std::move(step));
    MarkDirtyFrom(index);
    emit PipelineChanged();
    return true;
}

bool PipelineManager::RemoveStep(int index)
{
    if (index < 0 || index >= static_cast<int>(_steps.size()))
    {
        return false;
    }

    _steps.erase(_steps.begin() + index);
    MarkDirtyFrom(index);
    emit PipelineChanged();
    return true;
}

bool PipelineManager::MoveStepUp(int index)
{
    if (index <= 0 || index >= static_cast<int>(_steps.size()))
    {
        return false;
    }

    std::swap(_steps[index], _steps[index - 1]);
    MarkDirtyFrom(index - 1);
    emit PipelineChanged();
    return true;
}

bool PipelineManager::MoveStepDown(int index)
{
    if (index < 0 || index >= static_cast<int>(_steps.size()) - 1)
    {
        return false;
    }

    std::swap(_steps[index], _steps[index + 1]);
    MarkDirtyFrom(index);
    emit PipelineChanged();
    return true;
}

void PipelineManager::Clear()
{
    _steps.clear();
    _totalTimeMs = 0.0;
    PipelineContext::Instance().Clear();
    emit PipelineChanged();
}

void PipelineManager::SetAllStepsEnabled(bool enabled)
{
    if (_steps.empty())
    {
        return;
    }

    bool changed = false;
    for (auto& step : _steps)
    {
        if (step->IsEnabled() != enabled)
        {
            step->SetEnabled(enabled);
            changed = true;
        }
    }

    if (changed)
    {
        MarkDirtyFrom(0);
        Execute();
        emit PipelineChanged();
    }
}

PipelineStep* PipelineManager::GetStep(int index)
{
    if (index >= 0 && index < static_cast<int>(_steps.size()))
    {
        return _steps[index].get();
    }
    return nullptr;
}

const PipelineStep* PipelineManager::GetStep(int index) const
{
    if (index >= 0 && index < static_cast<int>(_steps.size()))
    {
        return _steps[index].get();
    }
    return nullptr;
}

void PipelineManager::MarkDirtyFrom(int index)
{
    for (size_t i = static_cast<size_t>(std::max(0, index)); i < _steps.size(); ++i)
    {
        _steps[i]->MarkDirty();
    }
}

bool PipelineManager::Execute(bool forceAll)
{
    if (_sourceImage.empty())
    {
        return false;
    }

    PipelineContext::Instance().SetSourceImage(_sourceImage);

    if (forceAll)
    {
        MarkDirtyFrom(0);
    }

    auto totalStart = std::chrono::high_resolution_clock::now();
    bool allSuccess = true;

    for (size_t i = 0; i < _steps.size(); ++i)
    {
        PipelineStep* currentStep = _steps[i].get();
        if (!currentStep->IsDirty())
        {
            // 已有缓存且未脏，直接更新上下文后跳过计算
            PipelineContext::Instance().SetStepOutput(static_cast<int>(i), currentStep->GetOutputCache());
            continue;
        }

        // 确定上一步输入
        const cv::Mat& prevOutput = (i == 0) ? _sourceImage : _steps[i - 1]->GetOutputCache();
        if (prevOutput.empty())
        {
            currentStep->SetErrorMessage("上一步输出为空");
            currentStep->SetOutputCache(cv::Mat());
            allSuccess = false;
            emit ExecutionError(static_cast<int>(i), "上一步输出为空");
            continue;
        }

        if (!currentStep->IsEnabled())
        {
            // 步骤被禁用，直通传递上一步输出
            currentStep->SetOutputCache(prevOutput.clone());
            currentStep->SetExecutionTimeMs(0.0);
            currentStep->SetErrorMessage("");
            currentStep->ClearDirty();
            PipelineContext::Instance().SetStepOutput(static_cast<int>(i), currentStep->GetOutputCache());
            continue;
        }

        auto stepStart = std::chrono::high_resolution_clock::now();
        cv::Mat output;
        std::string errMsg;
        bool ok = false;
        try
        {
            ok = currentStep->GetOpNode()->Process(prevOutput, output, errMsg);
        }
        catch (const cv::Exception& ex)
        {
            errMsg = ex.what();
        }
        catch (const std::exception& ex)
        {
            errMsg = ex.what();
        }
        catch (...)
        {
            errMsg = "算子抛出了未知异常";
        }
        auto stepEnd = std::chrono::high_resolution_clock::now();

        double elapsedMs = std::chrono::duration<double, std::milli>(stepEnd - stepStart).count();
        currentStep->SetExecutionTimeMs(elapsedMs);

        if (ok && !output.empty())
        {
            currentStep->SetOutputCache(output);
            currentStep->SetErrorMessage("");
            currentStep->ClearDirty();
            PipelineContext::Instance().SetStepOutput(static_cast<int>(i), output);
            emit StepExecuted(static_cast<int>(i), elapsedMs);
        }
        else
        {
            currentStep->SetErrorMessage(errMsg.empty() ? "计算失败" : errMsg);
            currentStep->SetOutputCache(prevOutput.clone()); // 失败时保留上一帧防黑屏
            allSuccess = false;
            Logger::Warn("PipelineManager", "步骤 " + std::to_string(i + 1) + " 计算异常: " + errMsg);
            emit ExecutionError(static_cast<int>(i), QString::fromStdString(errMsg));

            for (size_t downstreamIndex = i + 1; downstreamIndex < _steps.size(); ++downstreamIndex)
            {
                _steps[downstreamIndex]->SetOutputCache(cv::Mat());
                _steps[downstreamIndex]->SetErrorMessage("因前置步骤失败而跳过");
                _steps[downstreamIndex]->MarkDirty();
            }
            break;
        }
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    _totalTimeMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    emit ExecutionCompleted(_totalTimeMs);
    return allSuccess;
}

const cv::Mat& PipelineManager::GetFinalOutput() const
{
    if (_steps.empty())
    {
        return _sourceImage;
    }

    for (int i = static_cast<int>(_steps.size()) - 1; i >= 0; --i)
    {
        if (!_steps[i]->GetOutputCache().empty())
        {
            return _steps[i]->GetOutputCache();
        }
    }
    return _sourceImage;
}

const cv::Mat& PipelineManager::GetStepOutput(int index) const
{
    if (index < 0 || index >= static_cast<int>(_steps.size()))
    {
        return _sourceImage;
    }
    const cv::Mat& cache = _steps[index]->GetOutputCache();
    return cache.empty() ? _sourceImage : cache;
}
