#include "HoughOps.h"
#include <sstream>

// ---------------- HoughLinesPOp ----------------
HoughLinesPOp::HoughLinesPOp()
{
    OpParameter rhoParam;
    rhoParam.Name = "rho";
    rhoParam.DisplayName = "距离分辨率 (Rho px)";
    rhoParam.Type = ParamType::DoubleSlider;
    rhoParam.Value = 1.0;
    rhoParam.MinValue = 0.5;
    rhoParam.MaxValue = 5.0;
    rhoParam.Step = 0.5;
    _params.push_back(rhoParam);

    OpParameter thetaParam;
    thetaParam.Name = "theta";
    thetaParam.DisplayName = "角度分辨率 (Theta °)";
    thetaParam.Type = ParamType::DoubleSlider;
    thetaParam.Value = 1.0;
    thetaParam.MinValue = 0.5;
    thetaParam.MaxValue = 5.0;
    thetaParam.Step = 0.5;
    _params.push_back(thetaParam);

    OpParameter thParam;
    thParam.Name = "threshold";
    thParam.DisplayName = "累加器阈值 (Threshold)";
    thParam.Type = ParamType::IntSlider;
    thParam.Value = 50;
    thParam.MinValue = 10;
    thParam.MaxValue = 300;
    thParam.Step = 5;
    _params.push_back(thParam);

    OpParameter lenParam;
    lenParam.Name = "minLineLength";
    lenParam.DisplayName = "最小线段长度 (MinLineLength px)";
    lenParam.Type = ParamType::DoubleSlider;
    lenParam.Value = 50.0;
    lenParam.MinValue = 5.0;
    lenParam.MaxValue = 500.0;
    lenParam.Step = 5.0;
    _params.push_back(lenParam);

    OpParameter gapParam;
    gapParam.Name = "maxLineGap";
    gapParam.DisplayName = "最大共线线段间隙 (MaxLineGap px)";
    gapParam.Type = ParamType::DoubleSlider;
    gapParam.Value = 10.0;
    gapParam.MinValue = 1.0;
    gapParam.MaxValue = 100.0;
    gapParam.Step = 1.0;
    _params.push_back(gapParam);
}

std::string HoughLinesPOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：累计概率霍夫直线变换 (HoughLinesP)\n\n"
           "基于极坐标对偶变换 $\\rho = x \\cos \\theta + y \\sin \\theta$ 将图像空间中共线点映射为累加器空间交点。\n"
           "**Progressive Probabilistic** 机制通过对边缘点随机采样，能够直接输出直线的**起点与终点坐标** $(x_1, y_1, x_2, y_2)$。";
}

bool HoughLinesPOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat edgeImg;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, edgeImg, cv::COLOR_BGR2GRAY);
        cv::Canny(edgeImg, edgeImg, 50, 150);
        output = input.clone();
    }
    else
    {
        edgeImg = input.clone();
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    }

    double rho = _params[0].GetDouble();
    double thetaDeg = _params[1].GetDouble();
    double theta = thetaDeg * CV_PI / 180.0;
    int threshold = _params[2].GetInt();
    double minLength = _params[3].GetDouble();
    double maxGap = _params[4].GetDouble();

    try
    {
        std::vector<cv::Vec4i> lines;
        cv::HoughLinesP(edgeImg, lines, rho, theta, threshold, minLength, maxGap);

        for (size_t i = 0; i < lines.size(); ++i)
        {
            cv::Vec4i l = lines[i];
            cv::line(output, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
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
    double rho = _params[0].GetDouble();
    double thetaDeg = _params[1].GetDouble();
    int threshold = _params[2].GetInt();
    double minLength = _params[3].GetDouble();
    double maxGap = _params[4].GetDouble();

    std::stringstream ss;
    ss << "// 累计概率霍夫直线检测参数配置\n"
       << "double rho = " << rho << ";                 // 距离分辨率 (px)\n"
       << "double theta = " << thetaDeg << " * CV_PI / 180.0; // 角度分辨率 (rad)\n"
       << "int houghThreshold = " << threshold << ";       // 累加器交点投票门限\n"
       << "double minLineLength = " << minLength << ";     // 最小线段长度 (px)\n"
       << "double maxLineGap = " << maxGap << ";        // 同一直线断裂合并间隙\n"
       << "std::vector<cv::Vec4i> lines;\n"
       << "cv::HoughLinesP(" << inVar << ", lines, rho, theta, houghThreshold, minLineLength, maxLineGap);\n"
       << "cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "for (size_t i = 0; i < lines.size(); ++i) {\n"
       << "    cv::line(" << outVar << ", cv::Point(lines[i][0], lines[i][1]), cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0, 0, 255), 2, cv::LINE_AA);\n"
       << "}";
    return ss.str();
}

std::string HoughLinesPOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double rho = _params[0].GetDouble();
    double thetaDeg = _params[1].GetDouble();
    int threshold = _params[2].GetInt();
    double minLength = _params[3].GetDouble();
    double maxGap = _params[4].GetDouble();

    std::stringstream ss;
    ss << "# 累计概率霍夫直线检测参数配置\n"
       << "rho = " << rho << "                       # 距离分辨率 (px)\n"
       << "theta = np.deg2rad(" << thetaDeg << ")              # 角度分辨率 (rad)\n"
       << "threshold = " << threshold << "                 # 累加器投票门限\n"
       << "min_line_len = " << minLength << "             # 最小线段长度\n"
       << "max_line_gap = " << maxGap << "             # 断裂合并间隙\n"
       << "lines = cv2.HoughLinesP(" << inVar << ", rho, theta, threshold, minLineLength=min_line_len, maxLineGap=max_line_gap)\n"
       << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "if lines is not None:\n"
       << "    for line in lines:\n"
       << "        x1, y1, x2, y2 = line[0]\n"
       << "        cv2.line(" << outVar << ", (x1, y1), (x2, y2), (0, 0, 255), 2, cv2.LINE_AA)";
    return ss.str();
}

// ---------------- HoughCirclesOp ----------------
HoughCirclesOp::HoughCirclesOp()
{
    OpParameter dpParam;
    dpParam.Name = "dp";
    dpParam.DisplayName = "累加器分辨率倒数 (dp)";
    dpParam.Type = ParamType::DoubleSlider;
    dpParam.Value = 1.2;
    dpParam.MinValue = 1.0;
    dpParam.MaxValue = 3.0;
    dpParam.Step = 0.1;
    _params.push_back(dpParam);

    OpParameter distParam;
    distParam.Name = "minDist";
    distParam.DisplayName = "圆心最小间距 (MinDist px)";
    distParam.Type = ParamType::DoubleSlider;
    distParam.Value = 30.0;
    distParam.MinValue = 5.0;
    distParam.MaxValue = 200.0;
    distParam.Step = 5.0;
    _params.push_back(distParam);

    OpParameter p1Param;
    p1Param.Name = "param1";
    p1Param.DisplayName = "Canny高阈值 (Param1)";
    p1Param.Type = ParamType::DoubleSlider;
    p1Param.Value = 100.0;
    p1Param.MinValue = 10.0;
    p1Param.MaxValue = 300.0;
    p1Param.Step = 5.0;
    _params.push_back(p1Param);

    OpParameter p2Param;
    p2Param.Name = "param2";
    p2Param.DisplayName = "圆心累加器阈值 (Param2)";
    p2Param.Type = ParamType::DoubleSlider;
    p2Param.Value = 30.0;
    p2Param.MinValue = 5.0;
    p2Param.MaxValue = 100.0;
    p2Param.Step = 1.0;
    _params.push_back(p2Param);

    OpParameter minRParam;
    minRParam.Name = "minRadius";
    minRParam.DisplayName = "最小半径 (MinRadius px)";
    minRParam.Type = ParamType::IntSlider;
    minRParam.Value = 10;
    minRParam.MinValue = 0;
    minRParam.MaxValue = 200;
    minRParam.Step = 2;
    _params.push_back(minRParam);

    OpParameter maxRParam;
    maxRParam.Name = "maxRadius";
    maxRParam.DisplayName = "最大半径 (MaxRadius px)";
    maxRParam.Type = ParamType::IntSlider;
    maxRParam.Value = 100;
    maxRParam.MinValue = 0;
    maxRParam.MaxValue = 500;
    maxRParam.Step = 5;
    _params.push_back(maxRParam);
}

std::string HoughCirclesOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：霍夫圆梯度变换 (HoughCircles)\n\n"
           "基于 **2-1 霍夫梯度法**：\n"
           "1. 估计边缘点梯度方向，在累加器平面沿法线投票确定圆心候选；\n"
           "2. 对各圆心计算到所有边缘点距离并统计半径分布，直接输出检测圆 $(x, y, r)$。";
}

bool HoughCirclesOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    cv::Mat gray;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        output = input.clone();
    }
    else
    {
        gray = input.clone();
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    }

    cv::GaussianBlur(gray, gray, cv::Size(9, 9), 2, 2);

    double dp = _params[0].GetDouble();
    double minDist = _params[1].GetDouble();
    double param1 = _params[2].GetDouble();
    double param2 = _params[3].GetDouble();
    int minR = _params[4].GetInt();
    int maxR = _params[5].GetInt();

    try
    {
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, dp, minDist, param1, param2, minR, maxR);

        for (size_t i = 0; i < circles.size(); ++i)
        {
            cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            int radius = cvRound(circles[i][2]);
            // 圆心
            cv::circle(output, center, 3, cv::Scalar(0, 255, 0), -1, 8, 0);
            // 圆轮廓
            cv::circle(output, center, radius, cv::Scalar(0, 0, 255), 2, 8, 0);
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
    double dp = _params[0].GetDouble();
    double minDist = _params[1].GetDouble();
    double param1 = _params[2].GetDouble();
    double param2 = _params[3].GetDouble();
    int minR = _params[4].GetInt();
    int maxR = _params[5].GetInt();

    std::stringstream ss;
    ss << "// 霍夫圆梯度变换检测参数配置\n"
       << "double dp = " << dp << ";                    // 累加器图像分辨率反比\n"
       << "double minDist = " << minDist << ";             // 两个检测圆心的最小物理间距 (px)\n"
       << "double param1 = " << param1 << ";              // 内部Canny边缘检测高阈值\n"
       << "double param2 = " << param2 << ";               // 圆心投票累加器阈值 (越小检测越灵敏)\n"
       << "int minRadius = " << minR << ";               // 最小检测半径 (px)\n"
       << "int maxRadius = " << maxR << ";              // 最大检测半径 (px)\n"
       << "std::vector<cv::Vec3f> circles;\n"
       << "cv::HoughCircles(" << inVar << ", circles, cv::HOUGH_GRADIENT, dp, minDist, param1, param2, minRadius, maxRadius);\n"
       << "cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "for (size_t i = 0; i < circles.size(); ++i) {\n"
       << "    cv::Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));\n"
       << "    int radius = cvRound(circles[i][2]);\n"
       << "    cv::circle(" << outVar << ", center, 3, cv::Scalar(0, 255, 0), -1);\n"
       << "    cv::circle(" << outVar << ", center, radius, cv::Scalar(0, 0, 255), 2);\n"
       << "}";
    return ss.str();
}

std::string HoughCirclesOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double dp = _params[0].GetDouble();
    double minDist = _params[1].GetDouble();
    double param1 = _params[2].GetDouble();
    double param2 = _params[3].GetDouble();
    int minR = _params[4].GetInt();
    int maxR = _params[5].GetInt();

    std::stringstream ss;
    ss << "# 霍夫圆梯度变换检测参数配置\n"
       << "dp = " << dp << "                         # 累加器图像分辨率反比\n"
       << "min_dist = " << minDist << "                 # 两个圆心最小物理间距 (px)\n"
       << "param1 = " << param1 << "                   # 内部Canny高阈值\n"
       << "param2 = " << param2 << "                    # 圆心投票累加器门限\n"
       << "min_radius = " << minR << "                  # 最小半径 (px)\n"
       << "max_radius = " << maxR << "                 # 最大半径 (px)\n"
       << "circles = cv2.HoughCircles(" << inVar << ", cv2.HOUGH_GRADIENT, dp, min_dist, param1=param1, param2=param2, minRadius=min_radius, maxRadius=max_radius)\n"
       << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "if circles is not None:\n"
       << "    circles = np.uint16(np.around(circles))\n"
       << "    for c in circles[0, :]:\n"
       << "        cv2.circle(" << outVar << ", (c[0], c[1]), 3, (0, 255, 0), -1)\n"
       << "        cv2.circle(" << outVar << ", (c[0], c[1]), c[2], (0, 0, 255), 2)";
    return ss.str();
}
