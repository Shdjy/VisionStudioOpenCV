#include "InteractiveMaskOp.h"
#include "../PipelineContext.h"
#include <sstream>

InteractiveMaskOp::InteractiveMaskOp()
{
    // 0. 掩码生成模式
    OpParameter shapeParam;
    shapeParam.Name = "shape";
    shapeParam.DisplayName = "掩码生成模式 (Mask Shape/Mode)";
    shapeParam.Type = ParamType::ComboBox;
    shapeParam.Value = 0; // 矩形
    shapeParam.EnumOptions = {
        "矩形形状掩码 (Rectangle ROI)",
        "圆形/椭圆形状掩码 (Circle/Ellipse ROI)",
        "原图亮度阈值自动掩码 (Threshold Auto Mask)"
    };
    _params.push_back(shapeParam);

    // 1. 作用目标
    OpParameter targetParam;
    targetParam.Name = "targetSource";
    targetParam.DisplayName = "作用目标图像 (Target Image)";
    targetParam.Type = ParamType::ComboBox;
    targetParam.Value = 0;
    targetParam.EnumOptions = {
        "当前流水线输入图像 (Current Input)",
        "原始基准输入图像 (Original Source Image)"
    };
    _params.push_back(targetParam);

    // 2. 复合操作动作
    OpParameter actionParam;
    actionParam.Name = "action";
    actionParam.DisplayName = "掩码复合操作 (Mask Action)";
    actionParam.Type = ParamType::ComboBox;
    actionParam.Value = 0; // 仅保留ROI
    actionParam.EnumOptions = {
        "仅提取ROI区域 (纯黑背景)",
        "仅提取ROI区域 (纯白背景)",
        "反转剔除ROI区域 (挖空目标)",
        "掩码内局部高斯滤波 (Local Blur)",
        "掩码内局部USM锐化 (Local Sharpen)",
        "掩码内局部对比度增强 (Local Contrast)",
        "半透明红色高亮标注 (Highlight Mask Overlay)",
        "仅输出单通道二值掩码 (Output Mask Only)"
    };
    _params.push_back(actionParam);

    // 3. ROI X
    OpParameter xParam;
    xParam.Name = "roiX";
    xParam.DisplayName = "ROI 左上角 X (px)";
    xParam.Type = ParamType::IntSlider;
    xParam.Value = 50;
    xParam.MinValue = 0;
    xParam.MaxValue = 4000;
    xParam.Step = 5;
    _params.push_back(xParam);

    // 4. ROI Y
    OpParameter yParam;
    yParam.Name = "roiY";
    yParam.DisplayName = "ROI 左上角 Y (px)";
    yParam.Type = ParamType::IntSlider;
    yParam.Value = 50;
    yParam.MinValue = 0;
    yParam.MaxValue = 4000;
    yParam.Step = 5;
    _params.push_back(yParam);

    // 5. ROI Width
    OpParameter wParam;
    wParam.Name = "roiW";
    wParam.DisplayName = "ROI 宽度 Width (px)";
    wParam.Type = ParamType::IntSlider;
    wParam.Value = 200;
    wParam.MinValue = 10;
    wParam.MaxValue = 4000;
    wParam.Step = 5;
    _params.push_back(wParam);

    // 6. ROI Height
    OpParameter hParam;
    hParam.Name = "roiH";
    hParam.DisplayName = "ROI 高度 Height (px)";
    hParam.Type = ParamType::IntSlider;
    hParam.Value = 150;
    hParam.MinValue = 10;
    hParam.MaxValue = 4000;
    hParam.Step = 5;
    _params.push_back(hParam);

    // 7. 阈值数值
    OpParameter threshParam;
    threshParam.Name = "threshVal";
    threshParam.DisplayName = "自动掩码分割阈值 (Threshold Cutoff)";
    threshParam.Type = ParamType::IntSlider;
    threshParam.Value = 128;
    threshParam.MinValue = 1;
    threshParam.MaxValue = 254;
    threshParam.Step = 1;
    _params.push_back(threshParam);

    // 8. 边缘羽化
    OpParameter featherParam;
    featherParam.Name = "featherSize";
    featherParam.DisplayName = "边缘羽化平滑 (Feathering px)";
    featherParam.Type = ParamType::IntSlider;
    featherParam.Value = 0;
    featherParam.MinValue = 0;
    featherParam.MaxValue = 50;
    featherParam.Step = 2;
    _params.push_back(featherParam);
}

std::string InteractiveMaskOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：交互式掩码创建与局部作用 (Interactive Mask ROI)\n\n"
           "掩码（Mask）是工业机器视觉中控制算法计算区域的核心工具：\n"
           "* **几何形状掩码**：在图像指定坐标 $[x, y, w, h]$ 生成矩形/椭圆 8 位单通道二进制矩阵；\n"
           "* **阈值自动掩码**：基于当前图像灰度特征自动分离前景连通域；\n"
           "* **局部作用域运算**：仅对掩码白色像素覆盖的 ROI 区域执行局部滤波、锐化、增强或缺陷高亮标注，避免背景干扰。";
}

void InteractiveMaskOp::SetRoiGeometry(int x, int y, int w, int h)
{
    SetParameter("roiX", x);
    SetParameter("roiY", y);
    SetParameter("roiW", w);
    SetParameter("roiH", h);
}

void InteractiveMaskOp::SetShape(MaskShape shape)
{
    SetParameter("shape", static_cast<int>(shape));
}

void InteractiveMaskOp::SetAction(MaskAction action)
{
    SetParameter("action", static_cast<int>(action));
}

bool InteractiveMaskOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int shape = _params[0].GetInt();
    int targetSrc = _params[1].GetInt();
    int action = _params[2].GetInt();
    int rx = _params[3].GetInt();
    int ry = _params[4].GetInt();
    int rw = _params[5].GetInt();
    int rh = _params[6].GetInt();
    int threshVal = _params[7].GetInt();
    int feather = _params[8].GetInt();

    // 确定底图
    cv::Mat targetImg = input;
    if (targetSrc == 1)
    {
        cv::Mat orig = PipelineContext::Instance().GetSourceImage();
        if (!orig.empty()) targetImg = orig;
    }
    if (targetImg.size() != input.size())
    {
        cv::resize(targetImg, targetImg, input.size());
    }

    try
    {
        // 1. 生成单通道 0/255 掩码
        cv::Mat mask = cv::Mat::zeros(input.size(), CV_8UC1);

        if (shape == 0) // 矩形
        {
            cv::Rect roi(rx, ry, rw, rh);
            roi = roi & cv::Rect(0, 0, input.cols, input.rows);
            if (roi.width > 0 && roi.height > 0)
            {
                mask(roi).setTo(255);
            }
        }
        else if (shape == 1) // 圆形/椭圆
        {
            cv::Point center(rx + rw / 2, ry + rh / 2);
            cv::Size axes(rw / 2, rh / 2);
            cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
        }
        else // 阈值自动
        {
            cv::Mat gray = input;
            if (input.channels() > 1) cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
            cv::threshold(gray, mask, threshVal, 255, cv::THRESH_BINARY);
        }

        // 羽化处理
        if (feather > 0)
        {
            int ksize = (feather % 2 == 0) ? (feather + 1) : feather;
            cv::GaussianBlur(mask, mask, cv::Size(ksize, ksize), 0);
        }

        // 2. 根据 Action 执行复合操作
        if (action == 7) // 仅输出掩码
        {
            cv::cvtColor(mask, output, cv::COLOR_GRAY2BGR);
            return true;
        }

        if (action == 0) // 仅保留ROI (纯黑背景)
        {
            output = cv::Mat::zeros(targetImg.size(), targetImg.type());
            targetImg.copyTo(output, mask);
        }
        else if (action == 1) // 仅保留ROI (纯白背景)
        {
            output = cv::Mat(targetImg.size(), targetImg.type(), cv::Scalar::all(255));
            targetImg.copyTo(output, mask);
        }
        else if (action == 2) // 反转剔除ROI
        {
            cv::Mat invMask;
            cv::bitwise_not(mask, invMask);
            output = cv::Mat::zeros(targetImg.size(), targetImg.type());
            targetImg.copyTo(output, invMask);
        }
        else if (action == 3) // 局部高斯模糊
        {
            cv::Mat localBlurred;
            cv::GaussianBlur(targetImg, localBlurred, cv::Size(21, 21), 5.0);
            output = targetImg.clone();
            localBlurred.copyTo(output, mask);
        }
        else if (action == 4) // 局部USM锐化
        {
            cv::Mat blurred, localSharpened;
            cv::GaussianBlur(targetImg, blurred, cv::Size(0, 0), 3.0);
            cv::addWeighted(targetImg, 2.0, blurred, -1.0, 0, localSharpened);
            output = targetImg.clone();
            localSharpened.copyTo(output, mask);
        }
        else if (action == 5) // 局部对比度增强
        {
            cv::Mat localEnhanced;
            targetImg.convertTo(localEnhanced, -1, 1.5, 20);
            output = targetImg.clone();
            localEnhanced.copyTo(output, mask);
        }
        else if (action == 6) // 半透明红色高亮标注覆盖
        {
            cv::Mat redOverlay = targetImg.clone();
            cv::Mat redColor(targetImg.size(), targetImg.type(), cv::Scalar(0, 0, 255));
            cv::addWeighted(targetImg, 0.6, redColor, 0.4, 0, redOverlay);
            output = targetImg.clone();
            redOverlay.copyTo(output, mask);
            // 绘制黄色边框辅助线
            cv::Rect roi(rx, ry, rw, rh);
            roi = roi & cv::Rect(0, 0, input.cols, input.rows);
            if (roi.width > 0 && roi.height > 0)
            {
                cv::rectangle(output, roi, cv::Scalar(0, 255, 255), 2);
            }
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string InteractiveMaskOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int shape = _params[0].GetInt();
    int action = _params[2].GetInt();
    int rx = _params[3].GetInt();
    int ry = _params[4].GetInt();
    int rw = _params[5].GetInt();
    int rh = _params[6].GetInt();

    std::stringstream ss;
    ss << "// 交互式掩码创建与局部作用运算\n"
       << "cv::Mat mask = cv::Mat::zeros(" << inVar << ".size(), CV_8UC1);\n";
    if (shape == 0)
    {
        ss << "cv::Rect roi(" << rx << ", " << ry << ", " << rw << ", " << rh << ");\n"
           << "mask(roi).setTo(255);\n";
    }
    else if (shape == 1)
    {
        ss << "cv::ellipse(mask, cv::Point(" << (rx + rw / 2) << ", " << (ry + rh / 2) << "), cv::Size(" << (rw / 2) << ", " << (rh / 2) << "), 0, 0, 360, cv::Scalar(255), -1);\n";
    }
    else
    {
        ss << "cv::Mat gray_" << inVar << "; cv::cvtColor(" << inVar << ", gray_" << inVar << ", cv::COLOR_BGR2GRAY);\n"
           << "cv::threshold(gray_" << inVar << ", mask, 128, 255, cv::THRESH_BINARY);\n";
    }

    if (action == 0)
    {
        ss << outVar << " = cv::Mat::zeros(" << inVar << ".size(), " << inVar << ".type());\n"
           << inVar << ".copyTo(" << outVar << ", mask); // 仅提取ROI";
    }
    else if (action == 3)
    {
        ss << "cv::Mat localBlurred; cv::GaussianBlur(" << inVar << ", localBlurred, cv::Size(21, 21), 5.0);\n"
           << outVar << " = " << inVar << ".clone();\n"
           << "localBlurred.copyTo(" << outVar << ", mask); // 局部高斯滤波";
    }
    else
    {
        ss << outVar << " = " << inVar << ".clone();\n"
           << inVar << ".copyTo(" << outVar << ", mask);";
    }
    return ss.str();
}

std::string InteractiveMaskOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int shape = _params[0].GetInt();
    int rx = _params[3].GetInt();
    int ry = _params[4].GetInt();
    int rw = _params[5].GetInt();
    int rh = _params[6].GetInt();

    std::stringstream ss;
    ss << "# 交互式掩码创建与局部作用运算\n"
       << "mask = np.zeros(" << inVar << ".shape[:2], dtype=np.uint8)\n";
    if (shape == 0)
    {
        ss << "mask[" << ry << ":" << (ry + rh) << ", " << rx << ":" << (rx + rw) << "] = 255\n";
    }
    else
    {
        ss << "cv2.ellipse(mask, (" << (rx + rw / 2) << ", " << (ry + rh / 2) << "), (" << (rw / 2) << ", " << (rh / 2) << "), 0, 0, 360, 255, -1)\n";
    }
    ss << outVar << " = np.zeros_like(" << inVar << ")\n"
       << outVar << " = cv2.bitwise_and(" << inVar << ", " << inVar << ", mask=mask)";
    return ss.str();
}
