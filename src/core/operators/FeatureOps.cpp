#include "FeatureOps.h"
#include <sstream>

// ---------------- CornerHarrisOp ----------------
CornerHarrisOp::CornerHarrisOp()
{
    OpParameter bsParam;
    bsParam.Name = "blockSize";
    bsParam.DisplayName = "邻域块大小 (BlockSize)";
    bsParam.Type = ParamType::IntSlider;
    bsParam.Value = 2;
    bsParam.MinValue = 2;
    bsParam.MaxValue = 10;
    bsParam.Step = 1;
    _params.push_back(bsParam);

    OpParameter ksParam;
    ksParam.Name = "ksize";
    ksParam.DisplayName = "Sobel核大小 (KSize)";
    ksParam.Type = ParamType::IntSlider;
    ksParam.Value = 3;
    ksParam.MinValue = 1;
    ksParam.MaxValue = 7;
    ksParam.Step = 2;
    _params.push_back(ksParam);

    OpParameter kParam;
    kParam.Name = "k";
    kParam.DisplayName = "Harris常数 (k)";
    kParam.Type = ParamType::DoubleSlider;
    kParam.Value = 0.04;
    kParam.MinValue = 0.01;
    kParam.MaxValue = 0.1;
    kParam.Step = 0.01;
    _params.push_back(kParam);

    OpParameter thParam;
    thParam.Name = "threshold";
    thParam.DisplayName = "响应阈值 (Threshold %)";
    thParam.Type = ParamType::DoubleSlider;
    thParam.Value = 0.01;
    thParam.MinValue = 0.001;
    thParam.MaxValue = 0.1;
    thParam.Step = 0.005;
    _params.push_back(thParam);
}

std::string CornerHarrisOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Harris 角点检测 (cornerHarris)\n\n"
           "通过局部自相关矩阵 $M = \\sum_{x,y} w(x,y) \\begin{bmatrix} I_x^2 & I_x I_y \\\\ I_x I_y & I_y^2 \\end{bmatrix}$ 计算角点响应度：\n"
           "$$R = \\det(M) - k \\cdot (\\text{trace}(M))^2$$\n"
           "* **$R > 0$ 且幅值极大**：角点（两个正交方向灰度变化均极剧烈）；\n"
           "* **$R < 0$**：直线边缘；\n"
           "* **$|R|$ 接近 0**：平坦区域。";
}

bool CornerHarrisOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
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

    int blockSize = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double k = _params[2].GetDouble();
    double threshRatio = _params[3].GetDouble();

    try
    {
        cv::Mat dst;
        cv::cornerHarris(gray, dst, blockSize, ksize, k);

        cv::Mat dst_norm;
        cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());

        for (int j = 0; j < dst_norm.rows; j++)
        {
            for (int i = 0; i < dst_norm.cols; i++)
            {
                if (dst_norm.at<float>(j, i) > 255.0 * threshRatio * 10.0)
                {
                    cv::circle(output, cv::Point(i, j), 4, cv::Scalar(0, 0, 255), 2, 8, 0);
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
    if (ksize % 2 == 0) ksize += 1;
    double k = _params[2].GetDouble();
    double threshRatio = _params[3].GetDouble();

    std::stringstream ss;
    ss << "// Harris角点检测参数配置\n"
       << "int blockSize = " << blockSize << ";            // 邻域窗口尺寸\n"
       << "int ksize = " << ksize << ";                // Sobel微分算子孔径\n"
       << "double k = " << k << ";                  // Harris经验常数\n"
       << "double threshRatio = " << threshRatio << ";    // 响应相对门限\n"
       << "cv::Mat dst_" << outVar << ", dstNorm_" << outVar << ";\n"
       << "cv::cornerHarris(" << inVar << ", dst_" << outVar << ", blockSize, ksize, k);\n"
       << "cv::normalize(dst_" << outVar << ", dstNorm_" << outVar << ", 0, 255, cv::NORM_MINMAX, CV_32FC1);\n"
       << "cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "for (int r = 0; r < dstNorm_" << outVar << ".rows; ++r) {\n"
       << "    for (int c = 0; c < dstNorm_" << outVar << ".cols; ++c) {\n"
       << "        if (dstNorm_" << outVar << ".at<float>(r, c) > 255.0 * threshRatio * 10.0) {\n"
       << "            cv::circle(" << outVar << ", cv::Point(c, r), 4, cv::Scalar(0, 0, 255), 2);\n"
       << "        }\n"
       << "    }\n"
       << "}";
    return ss.str();
}

std::string CornerHarrisOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int blockSize = _params[0].GetInt();
    int ksize = _params[1].GetInt();
    if (ksize % 2 == 0) ksize += 1;
    double k = _params[2].GetDouble();
    double threshRatio = _params[3].GetDouble();

    std::stringstream ss;
    ss << "# Harris角点检测参数配置\n"
       << "block_size = " << blockSize << "            # 邻域窗口尺寸\n"
       << "ksize = " << ksize << "                 # Sobel微分算子孔径\n"
       << "k = " << k << "                     # Harris经验常数\n"
       << "thresh_ratio = " << threshRatio << "          # 响应相对门限\n"
       << "gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "dst = cv2.cornerHarris(np.float32(gray), block_size, ksize, k)\n"
       << "dst = cv2.dilate(dst, None)\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << outVar << "[dst > thresh_ratio * dst.max()] = [0, 0, 255]";
    return ss.str();
}

// ---------------- GoodFeaturesToTrackOp ----------------
GoodFeaturesToTrackOp::GoodFeaturesToTrackOp()
{
    OpParameter mcParam;
    mcParam.Name = "maxCorners";
    mcParam.DisplayName = "最大角点数量 (MaxCorners)";
    mcParam.Type = ParamType::IntSlider;
    mcParam.Value = 50;
    mcParam.MinValue = 1;
    mcParam.MaxValue = 500;
    mcParam.Step = 5;
    _params.push_back(mcParam);

    OpParameter qlParam;
    qlParam.Name = "qualityLevel";
    qlParam.DisplayName = "质量水平门限 (QualityLevel)";
    qlParam.Type = ParamType::DoubleSlider;
    qlParam.Value = 0.01;
    qlParam.MinValue = 0.001;
    qlParam.MaxValue = 0.5;
    qlParam.Step = 0.005;
    _params.push_back(qlParam);

    OpParameter mdParam;
    mdParam.Name = "minDistance";
    mdParam.DisplayName = "角点最小欧氏间距 (MinDistance px)";
    mdParam.Type = ParamType::DoubleSlider;
    mdParam.Value = 10.0;
    mdParam.MinValue = 1.0;
    mdParam.MaxValue = 100.0;
    mdParam.Step = 1.0;
    _params.push_back(mdParam);
}

std::string GoodFeaturesToTrackOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：Shi-Tomasi 优质角点检测 (goodFeaturesToTrack)\n\n"
           "基于自相关矩阵的**最小特征值准则** $\\min(\\lambda_1, \\lambda_2) > \\lambda_{min}$。\n"
           "相比 Harris 算子，Shi-Tomasi 在特征点跟踪与光流法 (KLT) 中表现更鲁棒、分布更均匀。";
}

bool GoodFeaturesToTrackOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
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

    int maxCorners = _params[0].GetInt();
    double qualityLevel = _params[1].GetDouble();
    double minDistance = _params[2].GetDouble();

    try
    {
        std::vector<cv::Point2f> corners;
        cv::goodFeaturesToTrack(gray, corners, maxCorners, qualityLevel, minDistance);

        for (size_t i = 0; i < corners.size(); ++i)
        {
            cv::circle(output, corners[i], 4, cv::Scalar(0, 255, 0), -1);
            cv::circle(output, corners[i], 6, cv::Scalar(0, 0, 255), 1);
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
    double qualityLevel = _params[1].GetDouble();
    double minDistance = _params[2].GetDouble();

    std::stringstream ss;
    ss << "// Shi-Tomasi优质特征点检测参数配置\n"
       << "int maxCorners = " << maxCorners << ";           // 返回角点最大数量上限\n"
       << "double qualityLevel = " << qualityLevel << ";     // 角点质量最小相对比例\n"
       << "double minDistance = " << minDistance << ";       // 两相邻角点最小物理间距 (px)\n"
       << "std::vector<cv::Point2f> corners;\n"
       << "cv::goodFeaturesToTrack(" << inVar << ", corners, maxCorners, qualityLevel, minDistance);\n"
       << "cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "for (size_t i = 0; i < corners.size(); ++i) {\n"
       << "    cv::circle(" << outVar << ", corners[i], 4, cv::Scalar(0, 255, 0), -1);\n"
       << "}";
    return ss.str();
}

std::string GoodFeaturesToTrackOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int maxCorners = _params[0].GetInt();
    double qualityLevel = _params[1].GetDouble();
    double minDistance = _params[2].GetDouble();

    std::stringstream ss;
    ss << "# Shi-Tomasi优质特征点检测参数配置\n"
       << "max_corners = " << maxCorners << "           # 最大返回角点数量\n"
       << "quality_level = " << qualityLevel << "       # 最小质量阈值比例\n"
       << "min_distance = " << minDistance << "         # 角点最小欧氏间距 (px)\n"
       << "gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "corners = cv2.goodFeaturesToTrack(gray, max_corners, quality_level, min_distance)\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "if corners is not None:\n"
       << "    for pt in corners:\n"
       << "        x, y = pt.ravel()\n"
       << "        cv2.circle(" << outVar << ", (int(x), int(y)), 4, (0, 255, 0), -1)";
    return ss.str();
}

// ---------------- MatchTemplateOp ----------------
MatchTemplateOp::MatchTemplateOp()
{
    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "匹配度量方法 (Method)";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 5; // TM_CCOEFF_NORMED
    methodParam.EnumOptions = {
        "TM_SQDIFF (平方差匹配 - 越小越优)",
        "TM_SQDIFF_NORMED (标准归一化平方差)",
        "TM_CCORR (相关性匹配)",
        "TM_CCORR_NORMED (标准归一化相关性)",
        "TM_CCOEFF (相关系数匹配)",
        "TM_CCOEFF_NORMED (标准归一化相关系数 - 推荐)"
    };
    _params.push_back(methodParam);
}

std::string MatchTemplateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：模板匹配 (matchTemplate)\n\n"
           "滑动窗口遍历整幅目标图，计算滑动子窗口与模板图像的相似度度量（如归一化相关系数 $R(x, y) \\in [-1, 1]$），最后通过 `minMaxLoc` 快速定位最佳响应峰值坐标。";
}

bool MatchTemplateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int methodIdx = _params[0].GetInt();
    int cvMethod = cv::TM_CCOEFF_NORMED;
    if (methodIdx == 0) cvMethod = cv::TM_SQDIFF;
    else if (methodIdx == 1) cvMethod = cv::TM_SQDIFF_NORMED;
    else if (methodIdx == 2) cvMethod = cv::TM_CCORR;
    else if (methodIdx == 3) cvMethod = cv::TM_CCORR_NORMED;
    else if (methodIdx == 4) cvMethod = cv::TM_CCOEFF;

    try
    {
        // 截取中心区域作为演示模板
        int tw = input.cols / 4;
        int th = input.rows / 4;
        if (tw < 10 || th < 10)
        {
            output = input.clone();
            return true;
        }

        cv::Rect tplRoi(input.cols / 2 - tw / 2, input.rows / 2 - th / 2, tw, th);
        cv::Mat tpl = input(tplRoi).clone();

        cv::Mat result;
        cv::matchTemplate(input, tpl, result, cvMethod);

        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

        cv::Point matchLoc = (cvMethod == cv::TM_SQDIFF || cvMethod == cv::TM_SQDIFF_NORMED) ? minLoc : maxLoc;

        if (input.channels() == 1)
        {
            cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
        }
        else
        {
            output = input.clone();
        }

        cv::rectangle(output, matchLoc, cv::Point(matchLoc.x + tw, matchLoc.y + th), cv::Scalar(0, 0, 255), 2);
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
    ss << "// 模板匹配参数配置\n"
       << "int matchMethod = " << methodStr << ";\n"
       << "cv::Mat result_" << outVar << ";\n"
       << "cv::matchTemplate(" << inVar << ", templateImg, result_" << outVar << ", matchMethod);\n"
       << "double minVal, maxVal; cv::Point minLoc, maxLoc;\n"
       << "cv::minMaxLoc(result_" << outVar << ", &minVal, &maxVal, &minLoc, &maxLoc);\n"
       << "cv::Point matchLoc = (matchMethod == cv::TM_SQDIFF || matchMethod == cv::TM_SQDIFF_NORMED) ? minLoc : maxLoc;\n"
       << "cv::rectangle(" << outVar << ", matchLoc, cv::Point(matchLoc.x + templateImg.cols, matchLoc.y + templateImg.rows), cv::Scalar(0, 0, 255), 2);";
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
    ss << "# 模板匹配参数配置\n"
       << "match_method = " << methodStr << "\n"
       << "res = cv2.matchTemplate(" << inVar << ", template_img, match_method)\n"
       << "min_val, max_val, min_loc, max_loc = cv2.minMaxLoc(res)\n"
       << "top_left = min_loc if match_method in [cv2.TM_SQDIFF, cv2.TM_SQDIFF_NORMED] else max_loc\n"
       << "h_tpl, w_tpl = template_img.shape[:2]\n"
       << "bottom_right = (top_left[0] + w_tpl, top_left[1] + h_tpl)\n"
       << outVar << " = " << inVar << ".copy()\n"
       << "cv2.rectangle(" << outVar << ", top_left, bottom_right, (0, 0, 255), 2)";
    return ss.str();
}
