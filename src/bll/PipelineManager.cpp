#include "PipelineManager.h"
#include "../utils/Logger.h"
#include "../core/PipelineContext.h"
#include <chrono>
#include <exception>
#include <algorithm>

PipelineManager::PipelineManager(QObject* parent)
    : QObject(parent)
{
    EnsureDefaultBranch();
}

void PipelineManager::EnsureDefaultBranch()
{
    if (_branches.empty())
    {
        PipelineBranch mainBranch;
        mainBranch.Name = "主流水线 (Main)";
        _branches.push_back(std::move(mainBranch));
        _activeBranchIndex = 0;
    }
}

void PipelineManager::SetActiveBranchIndex(int index)
{
    EnsureDefaultBranch();
    if (index >= 0 && index < static_cast<int>(_branches.size()) && index != _activeBranchIndex)
    {
        _activeBranchIndex = index;
        emit ActiveBranchChanged(_activeBranchIndex);
        emit PipelineChanged();
    }
}

std::string PipelineManager::GetBranchName(int index) const
{
    if (index >= 0 && index < static_cast<int>(_branches.size()))
    {
        return _branches[index].Name;
    }
    return "";
}

void PipelineManager::SetBranchName(int index, const std::string& name)
{
    if (index >= 0 && index < static_cast<int>(_branches.size()) && !name.empty())
    {
        _branches[index].Name = name;
        emit BranchListChanged();
    }
}

int PipelineManager::CreateBranch(const std::string& name)
{
    EnsureDefaultBranch();

    // 获取当前活跃分支的最新输出图像作为新分支的原始基准输入
    cv::Mat branchSrc = GetFinalOutput();
    if (branchSrc.empty())
    {
        branchSrc = GetSourceImage();
    }

    PipelineBranch newBranch;
    if (!name.empty())
    {
        newBranch.Name = name;
    }
    else
    {
        newBranch.Name = "分支 " + std::to_string(_branches.size());
    }

    newBranch.SourceImage = branchSrc.empty() ? cv::Mat() : branchSrc.clone();

    _branches.push_back(std::move(newBranch));
    int newIdx = static_cast<int>(_branches.size()) - 1;
    _activeBranchIndex = newIdx;

    Logger::Info("PipelineManager", "已创建新分支: " + _branches[newIdx].Name);

    emit BranchListChanged();
    emit ActiveBranchChanged(_activeBranchIndex);
    emit PipelineChanged();

    return newIdx;
}

bool PipelineManager::RemoveBranch(int index)
{
    EnsureDefaultBranch();
    if (index <= 0 || index >= static_cast<int>(_branches.size()))
    {
        // 0 号主流水线不可删除
        return false;
    }

    std::string removedName = _branches[index].Name;
    _branches.erase(_branches.begin() + index);

    if (_activeBranchIndex >= static_cast<int>(_branches.size()))
    {
        _activeBranchIndex = static_cast<int>(_branches.size()) - 1;
    }

    Logger::Info("PipelineManager", "已删除分支: " + removedName);

    emit BranchListChanged();
    emit ActiveBranchChanged(_activeBranchIndex);
    emit PipelineChanged();
    Execute();

    return true;
}

const cv::Mat& PipelineManager::GetBranchSourceImage(int branchIndex) const
{
    if (branchIndex >= 0 && branchIndex < static_cast<int>(_branches.size()))
    {
        return _branches[branchIndex].SourceImage;
    }
    return _emptyMat;
}

const cv::Mat& PipelineManager::GetBranchFinalOutput(int branchIndex) const
{
    if (branchIndex < 0 || branchIndex >= static_cast<int>(_branches.size()))
    {
        return _emptyMat;
    }

    const auto& branch = _branches[branchIndex];
    if (branch.Steps.empty())
    {
        return branch.SourceImage;
    }

    for (int i = static_cast<int>(branch.Steps.size()) - 1; i >= 0; --i)
    {
        if (!branch.Steps[i]->GetOutputCache().empty())
        {
            return branch.Steps[i]->GetOutputCache();
        }
    }
    return branch.SourceImage;
}

const cv::Mat& PipelineManager::GetBranchFinalOutput(const std::string& branchName) const
{
    for (size_t i = 0; i < _branches.size(); ++i)
    {
        if (_branches[i].Name == branchName ||
            (branchName == "主流水线" && i == 0) ||
            (branchName == "Main" && i == 0) ||
            (branchName == "Branch " + std::to_string(i)) ||
            (branchName == "分支 " + std::to_string(i)))
        {
            return GetBranchFinalOutput(static_cast<int>(i));
        }
    }
    return _emptyMat;
}

int PipelineManager::GetBranchStepCount(int branchIndex) const
{
    if (branchIndex >= 0 && branchIndex < static_cast<int>(_branches.size()))
    {
        return static_cast<int>(_branches[branchIndex].Steps.size());
    }
    return 0;
}

PipelineStep* PipelineManager::GetBranchStep(int branchIndex, int stepIndex)
{
    if (branchIndex >= 0 && branchIndex < static_cast<int>(_branches.size()))
    {
        auto& steps = _branches[branchIndex].Steps;
        if (stepIndex >= 0 && stepIndex < static_cast<int>(steps.size()))
        {
            return steps[stepIndex].get();
        }
    }
    return nullptr;
}

const PipelineStep* PipelineManager::GetBranchStep(int branchIndex, int stepIndex) const
{
    if (branchIndex >= 0 && branchIndex < static_cast<int>(_branches.size()))
    {
        const auto& steps = _branches[branchIndex].Steps;
        if (stepIndex >= 0 && stepIndex < static_cast<int>(steps.size()))
        {
            return steps[stepIndex].get();
        }
    }
    return nullptr;
}

// --- 当前活跃分支的方法实现 ---

void PipelineManager::SetSourceImage(const cv::Mat& img)
{
    EnsureDefaultBranch();
    _branches[0].SourceImage = img.clone();
    PipelineContext::Instance().SetSourceImage(_branches[0].SourceImage);

    // 如果是活跃分支，标记脏
    MarkDirtyFrom(0);
    emit PipelineChanged();
}

const cv::Mat& PipelineManager::GetSourceImage() const
{
    if (_activeBranchIndex >= 0 && _activeBranchIndex < static_cast<int>(_branches.size()))
    {
        const cv::Mat& bSrc = _branches[_activeBranchIndex].SourceImage;
        if (!bSrc.empty())
        {
            return bSrc;
        }
    }
    if (!_branches.empty())
    {
        return _branches[0].SourceImage;
    }
    return _emptyMat;
}

int PipelineManager::AddStep(std::unique_ptr<IOpNode> opNode)
{
    if (!opNode)
    {
        return -1;
    }

    EnsureDefaultBranch();
    auto& steps = _branches[_activeBranchIndex].Steps;
    auto step = std::make_unique<PipelineStep>(std::move(opNode));
    int idx = static_cast<int>(steps.size());
    steps.push_back(std::move(step));

    MarkDirtyFrom(idx);
    emit PipelineChanged();
    return idx;
}

bool PipelineManager::InsertStep(int index, std::unique_ptr<IOpNode> opNode)
{
    if (!opNode)
    {
        return false;
    }

    EnsureDefaultBranch();
    auto& steps = _branches[_activeBranchIndex].Steps;
    if (index < 0 || index > static_cast<int>(steps.size()))
    {
        return false;
    }

    auto step = std::make_unique<PipelineStep>(std::move(opNode));
    steps.insert(steps.begin() + index, std::move(step));

    MarkDirtyFrom(index);
    emit PipelineChanged();
    return true;
}

bool PipelineManager::RemoveStep(int index)
{
    EnsureDefaultBranch();
    auto& steps = _branches[_activeBranchIndex].Steps;
    if (index < 0 || index >= static_cast<int>(steps.size()))
    {
        return false;
    }

    steps.erase(steps.begin() + index);
    MarkDirtyFrom(index);
    emit PipelineChanged();
    return true;
}

bool PipelineManager::MoveStepUp(int index)
{
    EnsureDefaultBranch();
    auto& steps = _branches[_activeBranchIndex].Steps;
    if (index <= 0 || index >= static_cast<int>(steps.size()))
    {
        return false;
    }

    std::swap(steps[index], steps[index - 1]);
    MarkDirtyFrom(index - 1);
    emit PipelineChanged();
    return true;
}

bool PipelineManager::MoveStepDown(int index)
{
    EnsureDefaultBranch();
    auto& steps = _branches[_activeBranchIndex].Steps;
    if (index < 0 || index >= static_cast<int>(steps.size()) - 1)
    {
        return false;
    }

    std::swap(steps[index], steps[index + 1]);
    MarkDirtyFrom(index);
    emit PipelineChanged();
    return true;
}

void PipelineManager::Clear()
{
    EnsureDefaultBranch();
    _branches[_activeBranchIndex].Steps.clear();
    _branches[_activeBranchIndex].TotalTimeMs = 0.0;
    emit PipelineChanged();
}

void PipelineManager::SetAllStepsEnabled(bool enabled)
{
    EnsureDefaultBranch();
    auto& steps = _branches[_activeBranchIndex].Steps;
    if (steps.empty())
    {
        return;
    }

    bool changed = false;
    for (auto& step : steps)
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

bool PipelineManager::AreAllStepsEnabled() const
{
    if (_activeBranchIndex < 0 || _activeBranchIndex >= static_cast<int>(_branches.size()))
    {
        return true;
    }
    const auto& steps = _branches[_activeBranchIndex].Steps;
    if (steps.empty())
    {
        return true;
    }
    for (const auto& step : steps)
    {
        if (!step->IsEnabled())
        {
            return false;
        }
    }
    return true;
}

int PipelineManager::GetStepCount() const
{
    if (_activeBranchIndex >= 0 && _activeBranchIndex < static_cast<int>(_branches.size()))
    {
        return static_cast<int>(_branches[_activeBranchIndex].Steps.size());
    }
    return 0;
}

PipelineStep* PipelineManager::GetStep(int index)
{
    return GetBranchStep(_activeBranchIndex, index);
}

const PipelineStep* PipelineManager::GetStep(int index) const
{
    return GetBranchStep(_activeBranchIndex, index);
}

void PipelineManager::InvalidateCrossBranchDependents()
{
    for (auto& branch : _branches)
    {
        for (size_t i = 0; i < branch.Steps.size(); ++i)
        {
            IOpNode* op = branch.Steps[i]->GetOpNode();
            if (!op)
            {
                continue;
            }
            const std::string& opName = op->GetName();
            if (opName == "BlendWithSource" || opName == "MaskApply")
            {
                for (size_t k = i; k < branch.Steps.size(); ++k)
                {
                    branch.Steps[k]->MarkDirty();
                }
                break;
            }
        }
    }
}

void PipelineManager::MarkDirtyFrom(int index)
{
    if (_activeBranchIndex >= 0 && _activeBranchIndex < static_cast<int>(_branches.size()))
    {
        auto& steps = _branches[_activeBranchIndex].Steps;
        for (size_t i = static_cast<size_t>(std::max(0, index)); i < steps.size(); ++i)
        {
            steps[i]->MarkDirty();
        }
    }
    // 跨分支级联标记脏：当任何分支发生改变，包含双图/掩码复合算子的其他分支步骤自动重新计算
    InvalidateCrossBranchDependents();
}

bool PipelineManager::ExecuteBranch(int branchIdx, bool forceAll)
{
    if (branchIdx < 0 || branchIdx >= static_cast<int>(_branches.size()))
    {
        return false;
    }

    auto& branch = _branches[branchIdx];
    cv::Mat branchSrc = branch.SourceImage;
    if (branchSrc.empty() && !_branches.empty())
    {
        branchSrc = _branches[0].SourceImage;
    }

    if (branchSrc.empty())
    {
        return false;
    }

    if (forceAll)
    {
        for (auto& s : branch.Steps)
        {
            s->MarkDirty();
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    bool allSuccess = true;

    for (size_t i = 0; i < branch.Steps.size(); ++i)
    {
        PipelineStep* currentStep = branch.Steps[i].get();
        if (!currentStep->IsDirty())
        {
            if (branchIdx == _activeBranchIndex)
            {
                PipelineContext::Instance().SetStepOutput(static_cast<int>(i), currentStep->GetOutputCache());
            }
            continue;
        }

        const cv::Mat& prevOutput = (i == 0) ? branchSrc : branch.Steps[i - 1]->GetOutputCache();
        if (prevOutput.empty())
        {
            currentStep->SetErrorMessage("上一步输出为空");
            currentStep->SetOutputCache(cv::Mat());
            allSuccess = false;
            if (branchIdx == _activeBranchIndex)
            {
                emit ExecutionError(static_cast<int>(i), "上一步输出为空");
            }
            continue;
        }

        if (!currentStep->IsEnabled())
        {
            currentStep->SetOutputCache(prevOutput.clone());
            currentStep->SetExecutionTimeMs(0.0);
            currentStep->SetErrorMessage("");
            currentStep->ClearDirty();
            if (branchIdx == _activeBranchIndex)
            {
                PipelineContext::Instance().SetStepOutput(static_cast<int>(i), currentStep->GetOutputCache());
            }
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
            if (branchIdx == _activeBranchIndex)
            {
                PipelineContext::Instance().SetStepOutput(static_cast<int>(i), output);
                emit StepExecuted(static_cast<int>(i), elapsedMs);
            }
        }
        else
        {
            currentStep->SetErrorMessage(errMsg.empty() ? "计算失败" : errMsg);
            currentStep->SetOutputCache(prevOutput.clone());
            allSuccess = false;
            Logger::Warn("PipelineManager", branch.Name + " 步骤 " + std::to_string(i + 1) + " 计算异常: " + errMsg);
            if (branchIdx == _activeBranchIndex)
            {
                emit ExecutionError(static_cast<int>(i), QString::fromStdString(errMsg));
            }

            for (size_t downstream = i + 1; downstream < branch.Steps.size(); ++downstream)
            {
                branch.Steps[downstream]->SetOutputCache(cv::Mat());
                branch.Steps[downstream]->SetErrorMessage("因前置步骤失败而跳过");
                branch.Steps[downstream]->MarkDirty();
            }
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    branch.TotalTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

    // 将本分支最终输出注册到全局上下文，供其他分支双图复合直接引用
    cv::Mat finalOut = GetBranchFinalOutput(branchIdx);
    if (!finalOut.empty())
    {
        PipelineContext::Instance().SetBranch(branch.Name, finalOut);
        PipelineContext::Instance().SetBranch("Branch " + std::to_string(branchIdx), finalOut);
        PipelineContext::Instance().SetBranch("分支 " + std::to_string(branchIdx), finalOut);
        if (branchIdx == 0)
        {
            PipelineContext::Instance().SetBranch("主流水线", finalOut);
            PipelineContext::Instance().SetBranch("Main", finalOut);
        }
    }

    return allSuccess;
}

bool PipelineManager::Execute(bool forceAll)
{
    EnsureDefaultBranch();

    if (!_branches.empty() && !_branches[0].SourceImage.empty())
    {
        PipelineContext::Instance().SetSourceImage(_branches[0].SourceImage);
    }

    auto totalStart = std::chrono::high_resolution_clock::now();
    bool allOk = true;

    // 第一轮：计算所有分支基础输出并同步全局上下文注册
    for (size_t b = 0; b < _branches.size(); ++b)
    {
        allOk &= ExecuteBranch(static_cast<int>(b), forceAll);
    }

    // 第二轮：级联执行依赖了其他分支最新输出的复合算子分支
    InvalidateCrossBranchDependents();
    for (size_t b = 0; b < _branches.size(); ++b)
    {
        allOk &= ExecuteBranch(static_cast<int>(b), false);
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    _totalTimeMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

    emit ExecutionCompleted(_totalTimeMs);
    return allOk;
}

const cv::Mat& PipelineManager::GetFinalOutput() const
{
    return GetBranchFinalOutput(_activeBranchIndex);
}

const cv::Mat& PipelineManager::GetStepOutput(int index) const
{
    if (_activeBranchIndex >= 0 && _activeBranchIndex < static_cast<int>(_branches.size()))
    {
        const auto& steps = _branches[_activeBranchIndex].Steps;
        if (index >= 0 && index < static_cast<int>(steps.size()))
        {
            const cv::Mat& cache = steps[index]->GetOutputCache();
            if (!cache.empty())
            {
                return cache;
            }
        }
        return GetSourceImage();
    }
    return _emptyMat;
}

