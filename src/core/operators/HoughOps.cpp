#include "HoughOps.h"
#include <sstream>

// ---------------- HoughLinesPOp ----------------
HoughLinesPOp::HoughLinesPOp()
{
    OpParameter thresholdParam;
    thresholdParam.Name = "threshold";
    thresholdParam.DisplayName = "累加器阈值 (Threshold)";
    thresholdParam.Tooltip = "投票累加器判决阈值 (大于此值判定为直线)";
    thresholdParam.Type = ParamType::IntSlider;
    thresholdParam.Value = 50;
    thresholdParam.MinValue = 10;
    thresholdParam.MaxValue = 300;
    thresholdParam.Step = 5;
    _params.push_back(thresholdParam);

    OpParameter minLineLengthParam;
    minLineLengthParam.Name = "minLineLength";
    minLineLengthParam.DisplayName = "最小线段长度 (MinLineLength)";
    minLineLengthParam.Tooltip = "被识别为线段的最小像素长度";
    minLineLengthParam.Type = ParamType::DoubleSlider;
    minLineLengthParam.Value = 50.0;
    minLineLengthParam.MinValue = 5.0;
    minLineLengthParam.MaxValue = 300.0;
    minLineLengthParam.Step = 5.0;
    _params.push_back(minLineLengthParam);

    OpParameter maxLineGapParam;
    maxLineGapParam.Name = "maxLineGap";
    maxLineGapParam.DisplayName = "最大线段间隙 (MaxLineGap)";
    maxLineGapParam.Tooltip = "连接同一线段的最大允许断裂间隙";
    maxLineGapParam.Type = ParamType::DoubleSlider;
    maxLineGapParam.Value = 10.0;
    maxLineGapParam.MinValue = 1.0;
    maxLineGapParam.MaxValue = 100.0;
    maxLineGapParam.Step = 1.0;
    _params.push_back(maxLineGapParam);

    OpParameter thicknessParam;
    thicknessParam.Name = "thickness";
    thicknessParam.DisplayName = "线条粗细 (Thickness)";
    thicknessParam.Tooltip = "绘制检测直线的线条宽度";
    thicknessParam.Type = ParamType::IntSlider;
    thicknessParam.Value = 2;
    thicknessParam.MinValue = 1;
    thicknessParam.MaxValue = 10;
    thicknessParam.Step = 1;
    _params.push_back(thicknessParam);
}

std::string HoughLinesPOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：累计概率霍夫直线变换 (HoughLinesP)\n\n"
           "霍夫变换将图像笛卡尔坐标系 $(x,y)$ 映射为极坐标霍夫参数空间 $(\\rho, \\theta)$：\n"
           "$$\\rho = x \\cos\\theta + y \\sin\\theta$$\n"
           "**概率霍夫算法 (PPHT)**：仅对边缘图像中随机采样的子集进行投票计算，大幅降低计算复杂度，并且直接输出线段的起点与终点坐标 $(x_1, y_1, x_2, y_2)$。";
}

bool HoughLinesPOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input.clone(); }

    int thresh = _params[0].GetInt();
    double minLen = _params[1].GetDouble();
    double maxGap = _params[2].GetDouble();
    int thickness = _params[3].GetInt();

    if (input.channels() == 1) { cv::cvtColor(input, output, cv::COLOR_GRAY2BGR); }
    else { output = input.clone(); }

    try
    {
        std::vector<cv::Vec4i> lines;
        cv::HoughLinesP(gray, lines, 1.0, CV_PI / 180.0, thresh, minLen, maxGap);

        for (const auto& l : lines)
        {
            cv::line(output, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 0, 255), thickness);
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string HoughLinesPOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int thresh = _params[0].GetInt();
    double minLen = _params[1].GetDouble();
    double maxGap = _params[2].GetDouble();
    int thickness = _params[3].GetInt();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ".clone();\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "else " << outVar << " = " << inVar << ".clone();\n"
       << "std::vector<cv::Vec4i> lines_" << outVar << ";\n"
       << "cv::HoughLinesP(gray_" << outVar << ", lines_" << outVar << ", 1.0, CV_PI / 180.0, " << thresh << ", " << minLen << ", " << maxGap << ");\n"
       << "for (const auto& l : lines_" << outVar << ")\n"
       << "{\n"
       << "    cv::line(" << outVar << ", cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 0, 255), " << thickness << ");\n"
       << "}";
    return ss.str();
}

std::string HoughLinesPOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int thresh = _params[0].GetInt();
    double minLen = _params[1].GetDouble();
    double maxGap = _params[2].GetDouble();
    int thickness = _params[3].GetInt();

    std::stringstream ss;
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "lines = cv2.HoughLinesP(gray, 1, np.pi / 180, " << thresh << ", minLineLength=" << minLen << ", maxLineGap=" << maxGap << ")\n"
       << "if lines is not None:\n"
       << "    for line in lines:\n"
       << "        x1, y1, x2, y2 = line[0]\n"
       << "        cv2.line(" << outVar << ", (x1, y1), (x2, y2), (0, 0, 255), " << thickness << ")";
    return ss.str();
}

// ---------------- HoughCirclesOp ----------------
HoughCirclesOp::HoughCirclesOp()
{
    OpParameter minDistParam;
    minDistParam.Name = "minDist";
    minDistParam.DisplayName = "圆心最小间距 (MinDist)";
    minDistParam.Tooltip = "两个检测圆心之间的最小允许像素距离";
    minDistParam.Type = ParamType::DoubleSlider;
    minDistParam.Value = 30.0;
    minDistParam.MinValue = 5.0;
    minDistParam.MaxValue = 200.0;
    minDistParam.Step = 5.0;
    _params.push_back(minDistParam);

    OpParameter param1;
    param1.Name = "param1";
    param1.DisplayName = "Canny高阈值 (Param1)";
    param1.Tooltip = "内部 Canny 边缘检测的高阈值";
    param1.Type = ParamType::DoubleSlider;
    param1.Value = 100.0;
    param1.MinValue = 10.0;
    param1.MaxValue = 300.0;
    param1.Step = 5.0;
    _params.push_back(param1);

    OpParameter param2;
    param2.Name = "param2";
    param2.DisplayName = "累加器阈值 (Param2)";
    param2.Tooltip = "圆心累加器检测阈值 (值越小检测越灵敏但误检多)";
    param2.Type = ParamType::DoubleSlider;
    param2.Value = 30.0;
    param2.MinValue = 5.0;
    param2.MaxValue = 100.0;
    param2.Step = 1.0;
    _params.push_back(param2);

    OpParameter minR;
    minR.Name = "minRadius";
    minR.DisplayName = "最小半径 (MinRadius)";
    minR.Tooltip = "允许检测的圆最小半径";
    minR.Type = ParamType::IntSlider;
    minR.Value = 10;
    minR.MinValue = 0;
    minR.MaxValue = 200;
    minR.Step = 2;
    _params.push_back(minR);

    OpParameter maxR;
    maxR.Name = "maxRadius";
    maxR.DisplayName = "最大半径 (MaxRadius)";
    maxR.Tooltip = "允许检测的圆最大半径";
    maxR.Type = ParamType::IntSlider;
    maxR.Value = 100;
    maxR.MinValue = 0;
    maxR.MaxValue = 500;
    maxR.Step = 5;
    _params.push_back(maxR);
}

std::string HoughCirclesOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：霍夫圆检测 (HoughCircles)\n\n"
           "基于 2-1 霍夫梯度法 (HOUGH_GRADIENT)：\n"
           "1. 首先进行 Canny 边缘提取与局部梯度方向计算；\n"
           "2. 沿所有边缘点的梯度法线方向进行 2D 圆心累加投票；\n"
           "3. 锁定候选圆心后，统计各候选半径距离上的边缘点支持度。";
}

bool HoughCirclesOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input.clone(); }

    double minDist = _params[0].GetDouble();
    double p1 = _params[1].GetDouble();
    double p2 = _params[2].GetDouble();
    int minR = _params[3].GetInt();
    int maxR = _params[4].GetInt();

    if (input.channels() == 1) { cv::cvtColor(input, output, cv::COLOR_GRAY2BGR); }
    else { output = input.clone(); }

    try
    {
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, minDist, p1, p2, minR, maxR);

        for (const auto& c : circles)
        {
            cv::Point center(cvRound(c[0]), cvRound(c[1]));
            int radius = cvRound(c[2]);
            // 绘制圆心
            cv::circle(output, center, 3, cv::Scalar(0, 255, 0), -1);
            // 绘制圆轮廓
            cv::circle(output, center, radius, cv::Scalar(0, 0, 255), 2);
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string HoughCirclesOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double minDist = _params[0].GetDouble();
    double p1 = _params[1].GetDouble();
    double p2 = _params[2].GetDouble();
    int minR = _params[3].GetInt();
    int maxR = _params[4].GetInt();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ".clone();\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "else " << outVar << " = " << inVar << ".clone();\n"
       << "std::vector<cv::Vec3f> circles_" << outVar << ";\n"
       << "cv::HoughCircles(gray_" << outVar << ", circles_" << outVar << ", cv::HOUGH_GRADIENT, 1, " << minDist << ", " << p1 << ", " << p2 << ", " << minR << ", " << maxR << ");\n"
       << "for (const auto& c : circles_" << outVar << ")\n"
       << "{\n"
       << "    cv::circle(" << outVar << ", cv::Point(cvRound(c[0]), cvRound(c[1])), 3, cv::Scalar(0, 255, 0), -1);\n"
       << "    cv::circle(" << outVar << ", cv::Point(cvRound(c[0]), cvRound(c[1])), cvRound(c[2]), cv::Scalar(0, 0, 255), 2);\n"
       << "}";
    return ss.str();
}

std::string HoughCirclesOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double minDist = _params[0].GetDouble();
    double p1 = _params[1].GetDouble();
    double p2 = _params[2].GetDouble();
    int minR = _params[3].GetInt();
    int maxR = _params[4].GetInt();

    std::stringstream ss;
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "circles = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT, 1, " << minDist << ", param1=" << p1 << ", param2=" << p2 << ", minRadius=" << minR << ", maxRadius=" << maxR << ")\n"
       << "if circles is not None:\n"
       << "    circles = np.uint16(np.around(circles))\n"
       << "    for i in circles[0, :]:\n"
       << "        cv2.circle(" << outVar << ", (i[0], i[1]), 3, (0, 255, 0), -1)\n"
       << "        cv2.circle(" << outVar << ", (i[0], i[1]), i[2], (0, 0, 255), 2)";
    return ss.str();
}
