#include "FeatureOps.h"
#include <sstream>

// ---------------- CornerHarrisOp ----------------
CornerHarrisOp::CornerHarrisOp()
{
    OpParameter blockSizeParam;
    blockSizeParam.Name = "blockSize";
    blockSizeParam.DisplayName = "邻域大小 (BlockSize)";
    blockSizeParam.Tooltip = "考虑用于角点检测的邻域大小 (2, 3, 5...)";
    blockSizeParam.Type = ParamType::IntSlider;
    blockSizeParam.Value = 2;
    blockSizeParam.MinValue = 2;
    blockSizeParam.MaxValue = 10;
    blockSizeParam.Step = 1;
    _params.push_back(blockSizeParam);

    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "Sobel孔径 (Ksize)";
    ksizeParam.Tooltip = "Sobel 微分算子孔径大小";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 3;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 7;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter kParam;
    kParam.Name = "k";
    kParam.DisplayName = "Harris经验常数 (k)";
    kParam.Tooltip = "Harris 响应计算自由参数 (通常为 0.04 ~ 0.06)";
    kParam.Type = ParamType::DoubleSlider;
    kParam.Value = 0.04;
    kParam.MinValue = 0.01;
    kParam.MaxValue = 0.20;
    kParam.Step = 0.01;
    _params.push_back(kParam);

    OpParameter threshParam;
    threshParam.Name = "threshold";
    threshParam.DisplayName = "角点响应阈值 (Threshold)";
    threshParam.Tooltip = "角点响应归一化阈值 (0.01 ~ 0.50)";
    threshParam.Type = ParamType::DoubleSlider;
    threshParam.Value = 0.05;
    threshParam.MinValue = 0.001;
    threshParam.MaxValue = 0.5;
    threshParam.Step = 0.005;
    _params.push_back(threshParam);
}

std::string CornerHarrisOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Harris 角点检测 (cornerHarris)\n\n"
           "基于局部图像窗口自相关矩阵 $M$：\n"
           "$$M = \\sum_{(x,y)} w(x,y) \\begin{bmatrix} I_x^2 & I_x I_y \\\\ I_x I_y & I_y^2 \\end{bmatrix}$$\n"
           "计算角点响应值 $R = \\det(M) - k \\cdot (\\text{trace}(M))^2$：\n"
           "* $R > 0$ 且数值较大：判定为**角点**；\n"
           "* $R < 0$：判定为**边缘**；\n"
           "* $|R|$ 很小：判定为**平坦区域**。";
}

bool CornerHarrisOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input.clone(); }

    int blockSize = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double k = _params[2].GetDouble();
    double thresh = _params[3].GetDouble();

    if (input.channels() == 1) { cv::cvtColor(input, output, cv::COLOR_GRAY2BGR); }
    else { output = input.clone(); }

    try
    {
        cv::Mat dst;
        cv::cornerHarris(gray, dst, blockSize, ksize, k);
        cv::Mat dstNorm;
        cv::normalize(dst, dstNorm, 0, 1.0, cv::NORM_MINMAX, CV_32FC1, cv::Mat());

        for (int j = 0; j < dstNorm.rows; ++j)
        {
            for (int i = 0; i < dstNorm.cols; ++i)
            {
                if (dstNorm.at<float>(j, i) > thresh)
                {
                    cv::circle(output, cv::Point(i, j), 4, cv::Scalar(0, 0, 255), 2);
                }
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

std::string CornerHarrisOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int blockSize = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double k = _params[2].GetDouble();
    double thresh = _params[3].GetDouble();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ".clone();\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "else " << outVar << " = " << inVar << ".clone();\n"
       << "cv::Mat harris_" << outVar << ", harrisNorm_" << outVar << ";\n"
       << "cv::cornerHarris(gray_" << outVar << ", harris_" << outVar << ", " << blockSize << ", " << ksize << ", " << k << ");\n"
       << "cv::normalize(harris_" << outVar << ", harrisNorm_" << outVar << ", 0, 1.0, cv::NORM_MINMAX, CV_32FC1);\n"
       << "for (int r = 0; r < harrisNorm_" << outVar << ".rows; ++r)\n"
       << "{\n"
       << "    for (int c = 0; c < harrisNorm_" << outVar << ".cols; ++c)\n"
       << "    {\n"
       << "        if (harrisNorm_" << outVar << ".at<float>(r, c) > " << thresh << ") cv::circle(" << outVar << ", cv::Point(c, r), 4, cv::Scalar(0, 0, 255), 2);\n"
       << "    }\n"
       << "}";
    return ss.str();
}

std::string CornerHarrisOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int blockSize = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) { ksize += 1; }
    double k = _params[2].GetDouble();
    double thresh = _params[3].GetDouble();

    std::stringstream ss;
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "dst = cv2.cornerHarris(gray, " << blockSize << ", " << ksize << ", " << k << ")\n"
       << "dst = cv2.normalize(dst, None, 0, 1.0, cv2.NORM_MINMAX, dtype=cv2.CV_32F)\n"
       << outVar << "[dst > " << thresh << "] = [0, 0, 255]";
    return ss.str();
}

// ---------------- GoodFeaturesToTrackOp ----------------
GoodFeaturesToTrackOp::GoodFeaturesToTrackOp()
{
    OpParameter maxCornersParam;
    maxCornersParam.Name = "maxCorners";
    maxCornersParam.DisplayName = "最大角点数 (MaxCorners)";
    maxCornersParam.Tooltip = "返回的最强角点数量上限";
    maxCornersParam.Type = ParamType::IntSlider;
    maxCornersParam.Value = 50;
    maxCornersParam.MinValue = 1;
    maxCornersParam.MaxValue = 500;
    maxCornersParam.Step = 5;
    _params.push_back(maxCornersParam);

    OpParameter qualityParam;
    qualityParam.Name = "qualityLevel";
    qualityParam.DisplayName = "质量等级 (QualityLevel)";
    qualityParam.Tooltip = "角点质量阈值系数 (0.001 ~ 0.50)";
    qualityParam.Type = ParamType::DoubleSlider;
    qualityParam.Value = 0.01;
    qualityParam.MinValue = 0.001;
    qualityParam.MaxValue = 0.5;
    qualityParam.Step = 0.005;
    _params.push_back(qualityParam);

    OpParameter minDistParam;
    minDistParam.Name = "minDistance";
    minDistParam.DisplayName = "最小间距 (MinDistance)";
    minDistParam.Tooltip = "相邻两角点间的最小欧氏距离";
    minDistParam.Type = ParamType::DoubleSlider;
    minDistParam.Value = 10.0;
    minDistParam.MinValue = 1.0;
    minDistParam.MaxValue = 100.0;
    minDistParam.Step = 1.0;
    _params.push_back(minDistParam);
}

std::string GoodFeaturesToTrackOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Shi-Tomasi 角点检测 (goodFeaturesToTrack)\n\n"
           "Shi-Tomasi 发现角点的质量直接取决于自相关矩阵的两个特征值 $\\lambda_1, \\lambda_2$。\n"
           "定义评分函数 $R = \\min(\\lambda_1, \\lambda_2)$，只要该最小值大于阈值 $\\lambda_{\\min} > \\lambda_{\\text{threshold}}$，即确认为强角点。\n"
           "相比 Harris 角点，Shi-Tomasi 在特征点跟踪 (KLT) 与 SLAM 中性能更为稳定。";
}

bool GoodFeaturesToTrackOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input.clone(); }

    int maxCorners = _params[0].GetInt();
    double quality = _params[1].GetDouble();
    double minDist = _params[2].GetDouble();

    if (input.channels() == 1) { cv::cvtColor(input, output, cv::COLOR_GRAY2BGR); }
    else { output = input.clone(); }

    try
    {
        std::vector<cv::Point2f> corners;
        cv::goodFeaturesToTrack(gray, corners, maxCorners, quality, minDist);

        for (const auto& pt : corners)
        {
            cv::circle(output, pt, 4, cv::Scalar(0, 255, 0), -1);
            cv::circle(output, pt, 6, cv::Scalar(0, 0, 255), 1);
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string GoodFeaturesToTrackOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int maxCorners = _params[0].GetInt();
    double quality = _params[1].GetDouble();
    double minDist = _params[2].GetDouble();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ".clone();\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "else " << outVar << " = " << inVar << ".clone();\n"
       << "std::vector<cv::Point2f> corners_" << outVar << ";\n"
       << "cv::goodFeaturesToTrack(gray_" << outVar << ", corners_" << outVar << ", " << maxCorners << ", " << quality << ", " << minDist << ");\n"
       << "for (const auto& pt : corners_" << outVar << ")\n"
       << "{\n"
       << "    cv::circle(" << outVar << ", pt, 4, cv::Scalar(0, 255, 0), -1);\n"
       << "}";
    return ss.str();
}

std::string GoodFeaturesToTrackOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int maxCorners = _params[0].GetInt();
    double quality = _params[1].GetDouble();
    double minDist = _params[2].GetDouble();

    std::stringstream ss;
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "corners = cv2.goodFeaturesToTrack(gray, maxCorners=" << maxCorners << ", qualityLevel=" << quality << ", minDistance=" << minDist << ")\n"
       << "if corners is not None:\n"
       << "    for i in corners:\n"
       << "        x, y = i.ravel()\n"
       << "        cv2.circle(" << outVar << ", (int(x), int(y)), 4, (0, 255, 0), -1)";
    return ss.str();
}

// ---------------- MatchTemplateOp ----------------
MatchTemplateOp::MatchTemplateOp()
{
    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "匹配准则 (Method)";
    methodParam.Tooltip = "相似度计算相关性度量准则";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 5; // TM_CCOEFF_NORMED
    methodParam.EnumOptions = {
        "平方差匹配 (TM_SQDIFF)",
        "标准平方差匹配 (TM_SQDIFF_NORMED)",
        "相关匹配 (TM_CCORR)",
        "标准相关匹配 (TM_CCORR_NORMED)",
        "相关系数匹配 (TM_CCOEFF)",
        "标准相关系数匹配 (TM_CCOEFF_NORMED - 推荐)"
    };
    _params.push_back(methodParam);
}

std::string MatchTemplateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：模板匹配 (matchTemplate)\n\n"
           "模板图在待搜索图像上逐像素滑动做互相关计算，生成二维相似度矩阵：\n"
           "* **TM_SQDIFF**：数值越小匹配度越高（0 为完美匹配）；\n"
           "* **TM_CCOEFF_NORMED**：归一化相关系数（-1~1，1 为完美匹配），对全局光照亮度线性变化具有极强鲁棒性。";
}

bool MatchTemplateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int methodIdx = _params[0].GetInt();
    int cvMethod = cv::TM_CCOEFF_NORMED;
    if (methodIdx == 0) cvMethod = cv::TM_SQDIFF;
    else if (methodIdx == 1) cvMethod = cv::TM_SQDIFF_NORMED;
    else if (methodIdx == 2) cvMethod = cv::TM_CCORR;
    else if (methodIdx == 3) cvMethod = cv::TM_CCORR_NORMED;
    else if (methodIdx == 4) cvMethod = cv::TM_CCOEFF;

    // 默认选取输入图像中心 1/4 区域作为模板进行自匹配演示
    int tw = input.cols / 4;
    int th = input.rows / 4;
    if (tw <= 5 || th <= 5)
    {
        output = input.clone();
        return true;
    }

    cv::Rect tmplRoi(input.cols / 2 - tw / 2, input.rows / 2 - th / 2, tw, th);
    cv::Mat templ = input(tmplRoi).clone();

    if (input.channels() == 1) { cv::cvtColor(input, output, cv::COLOR_GRAY2BGR); }
    else { output = input.clone(); }

    try
    {
        cv::Mat result;
        cv::matchTemplate(input, templ, result, cvMethod);

        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

        cv::Point matchLoc = (cvMethod == cv::TM_SQDIFF || cvMethod == cv::TM_SQDIFF_NORMED) ? minLoc : maxLoc;

        // 绘制目标外框
        cv::rectangle(output, matchLoc, cv::Point(matchLoc.x + tw, matchLoc.y + th), cv::Scalar(0, 255, 0), 3);
        // 绘制中心十字
        cv::drawMarker(output, cv::Point(matchLoc.x + tw / 2, matchLoc.y + th / 2), cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 20, 2);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string MatchTemplateOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int methodIdx = _params[0].GetInt();
    std::string methodStr = "cv::TM_CCOEFF_NORMED";
    if (methodIdx == 0) methodStr = "cv::TM_SQDIFF";
    else if (methodIdx == 1) methodStr = "cv::TM_SQDIFF_NORMED";
    else if (methodIdx == 2) methodStr = "cv::TM_CCORR";
    else if (methodIdx == 3) methodStr = "cv::TM_CCORR_NORMED";
    else if (methodIdx == 4) methodStr = "cv::TM_CCOEFF";

    std::stringstream ss;
    ss << "// 模板匹配\n"
       << "cv::Mat templ;\n"
       << "cv::Mat result_" << outVar << ";\n"
       << "cv::matchTemplate(" << inVar << ", templ, result_" << outVar << ", " << methodStr << ");\n"
       << "double minVal, maxVal; cv::Point minLoc, maxLoc;\n"
       << "cv::minMaxLoc(result_" << outVar << ", &minVal, &maxVal, &minLoc, &maxLoc);\n"
       << outVar << " = " << inVar << ".clone();\n"
       << "cv::rectangle(" << outVar << ", maxLoc, cv::Point(maxLoc.x + templ.cols, maxLoc.y + templ.rows), cv::Scalar(0, 255, 0), 2);";
    return ss.str();
}

std::string MatchTemplateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int methodIdx = _params[0].GetInt();
    std::string methodStr = "cv2.TM_CCOEFF_NORMED";
    if (methodIdx == 0) methodStr = "cv2.TM_SQDIFF";
    else if (methodIdx == 1) methodStr = "cv2.TM_SQDIFF_NORMED";
    else if (methodIdx == 2) methodStr = "cv2.TM_CCORR";
    else if (methodIdx == 3) methodStr = "cv2.TM_CCORR_NORMED";
    else if (methodIdx == 4) methodStr = "cv2.TM_CCOEFF";

    std::stringstream ss;
    ss << "# 模板匹配示例\n"
       << "# templ = cv2.imread('template.jpg')\n"
       << "res = cv2.matchTemplate(" << inVar << ", templ, " << methodStr << ")\n"
       << "min_val, max_val, min_loc, max_loc = cv2.minMaxLoc(res)\n"
       << outVar << " = " << inVar << ".copy()\n"
       << "top_left = max_loc\n"
       << "bottom_right = (top_left[0] + templ.shape[1], top_left[1] + templ.shape[0])\n"
       << "cv2.rectangle(" << outVar << ", top_left, bottom_right, (0, 255, 0), 2)";
    return ss.str();
}
