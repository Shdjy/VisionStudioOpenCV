#include "FeatureOps.h"
#include "../PipelineContext.h"
#include "../../utils/ImageIoUtils.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
std::string EscapeCppString(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value)
    {
        if (ch == '\\' || ch == '"')
        {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}

std::string EscapePythonString(std::string value)
{
    std::replace(value.begin(), value.end(), '\\', '/');
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value)
    {
        if (ch == '\'')
        {
            escaped.push_back('\\');
        }
        escaped.push_back(ch);
    }
    return escaped;
}
}

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
    // 0. 匹配度量方法
    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "匹配度量算法 (Match Method)";
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

    // 1. 模板来源
    OpParameter tplSrcParam;
    tplSrcParam.Name = "tplSource";
    tplSrcParam.DisplayName = "模板来源方式 (Template Source)";
    tplSrcParam.Type = ParamType::ComboBox;
    tplSrcParam.Value = 0;
    tplSrcParam.EnumOptions = {
        "1. 在当前输入图像中框选 ROI (Crop from Input)",
        "2. 在原始基准图像中框选 ROI (Crop from Original)",
        "3. 加载本地外部模板图片文件 (Load Local Image File)"
    };
    _params.push_back(tplSrcParam);

    // 2. 本地模板图片路径
    OpParameter tplPathParam;
    tplPathParam.Name = "tplPath";
    tplPathParam.DisplayName = "本地模板图片 (Local Template Image)";
    tplPathParam.Type = ParamType::FilePath;
    tplPathParam.Value = std::string("");
    tplPathParam.Tooltip = "当模板来源选择【加载本地外部模板图片】时，点击浏览选择本地模板图像文件";
    _params.push_back(tplPathParam);

    // 3. 模板 ROI X
    OpParameter xParam;
    xParam.Name = "roiX";
    xParam.DisplayName = "模板区域 X (px)";
    xParam.Type = ParamType::IntSlider;
    xParam.Value = 50;
    xParam.MinValue = 0;
    xParam.MaxValue = 4000;
    xParam.Step = 5;
    _params.push_back(xParam);

    // 4. 模板 ROI Y
    OpParameter yParam;
    yParam.Name = "roiY";
    yParam.DisplayName = "模板区域 Y (px)";
    yParam.Type = ParamType::IntSlider;
    yParam.Value = 50;
    yParam.MinValue = 0;
    yParam.MaxValue = 4000;
    yParam.Step = 5;
    _params.push_back(yParam);

    // 5. 模板 ROI W
    OpParameter wParam;
    wParam.Name = "roiW";
    wParam.DisplayName = "模板宽度 Width (px)";
    wParam.Type = ParamType::IntSlider;
    wParam.Value = 120;
    wParam.MinValue = 10;
    wParam.MaxValue = 2000;
    wParam.Step = 5;
    _params.push_back(wParam);

    // 6. 模板 ROI H
    OpParameter hParam;
    hParam.Name = "roiH";
    hParam.DisplayName = "模板高度 Height (px)";
    hParam.Type = ParamType::IntSlider;
    hParam.Value = 100;
    hParam.MinValue = 10;
    hParam.MaxValue = 2000;
    hParam.Step = 5;
    _params.push_back(hParam);

    // 7. 匹配得分阈值
    OpParameter scoreParam;
    scoreParam.Name = "scoreThreshold";
    scoreParam.DisplayName = "置信度得分门限 (Score Threshold)";
    scoreParam.Type = ParamType::DoubleSlider;
    scoreParam.Value = 0.80;
    scoreParam.MinValue = 0.10;
    scoreParam.MaxValue = 1.00;
    scoreParam.Step = 0.02;
    _params.push_back(scoreParam);

    // 8. 多目标匹配
    OpParameter multiParam;
    multiParam.Name = "multiMatch";
    multiParam.DisplayName = "多目标检测模式 (Multi-Match All)";
    multiParam.Tooltip = "开启后框选画面中所有超过阈值的相似目标，关闭则仅定位单个最高分目标";
    multiParam.Type = ParamType::CheckBox;
    multiParam.Value = false;
    _params.push_back(multiParam);
}

std::string MatchTemplateOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：模板匹配与目标定位 (matchTemplate)\n\n"
           "基于滑窗互相关矩阵在图像中遍历搜索指定 ROI 模板或本地模板图像：\n"
           "* **本地外部模板支持**：支持直接加载本地高精度标准品切片进行跨图检测；\n"
           "* **度量方法**：归一化相关系数 $R(x, y) = \\frac{\\sum (T - \\bar{T})(I - \\bar{I})}{\\sqrt{\\sum (T - \\bar{T})^2 \\sum (I - \\bar{I})^2}} \\in [-1, 1]$；\n"
           "* **单目标定位**：`cv::minMaxLoc` 寻找全局响应峰值点；\n"
           "* **多目标检测与非极大值抑制**：基于置信度门限与距离邻域批量定位所有工件。";
}

void MatchTemplateOp::SetTemplateRoi(int x, int y, int w, int h)
{
    SetParameter("roiX", x);
    SetParameter("roiY", y);
    SetParameter("roiW", w);
    SetParameter("roiH", h);
}

bool MatchTemplateOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int methodIdx = _params[0].GetInt();
    int tplSrc = _params[1].GetInt();
    std::string tplPath = _params[2].GetString();
    int rx = _params[3].GetInt();
    int ry = _params[4].GetInt();
    int rw = _params[5].GetInt();
    int rh = _params[6].GetInt();
    double scoreThresh = _params[7].GetDouble();
    bool multiMatch = _params[8].GetBool();

    const int methods[] = {
        cv::TM_SQDIFF,
        cv::TM_SQDIFF_NORMED,
        cv::TM_CCORR,
        cv::TM_CCORR_NORMED,
        cv::TM_CCOEFF,
        cv::TM_CCOEFF_NORMED
    };
    const int cvMethod = methods[methodIdx];
    const bool isSqDiff = methodIdx <= 1;
    const bool isNormalizedMethod = methodIdx == 1 || methodIdx == 3 || methodIdx == 5;

    cv::Mat tpl;
    cv::Rect tplRoi(0, 0, 0, 0);

    if (tplSrc == 2)
    {
        // 加载本地外部模板文件
        if (tplPath.empty())
        {
            errorMsg = "请选择本地模板图像文件";
            return false;
        }

        std::string imageIoError;
        tpl = ImageIoUtils::ReadImage(tplPath, cv::IMREAD_COLOR, imageIoError);
        if (tpl.empty())
        {
            errorMsg = "无法读取指定的本地模板图像文件: " + imageIoError;
            output = input.clone();
            return false;
        }
        rw = tpl.cols;
        rh = tpl.rows;
    }
    else
    {
        // 框选 ROI 模式
        cv::Mat baseImg = input;
        if (tplSrc == 1)
        {
            cv::Mat orig = PipelineContext::Instance().GetSourceImage();
            if (!orig.empty()) baseImg = orig;
        }

        if (baseImg.cols < 10 || baseImg.rows < 10)
        {
            errorMsg = "参考图像尺寸过小，无法提取模板";
            return false;
        }
        rx = std::max(0, std::min(rx, baseImg.cols - 10));
        ry = std::max(0, std::min(ry, baseImg.rows - 10));
        rw = std::max(10, std::min(rw, baseImg.cols - rx));
        rh = std::max(10, std::min(rh, baseImg.rows - ry));

        tplRoi = cv::Rect(rx, ry, rw, rh);
        tpl = baseImg(tplRoi).clone();
    }

    if (tpl.cols > input.cols || tpl.rows > input.rows)
    {
        errorMsg = "模板尺寸大于待匹配图像尺寸";
        output = input.clone();
        return false;
    }

    try
    {
        // 灰度化匹配提升效率与稳定性
        cv::Mat grayInput, grayTpl;
        if (input.channels() == 4)
        {
            cv::cvtColor(input, grayInput, cv::COLOR_BGRA2GRAY);
        }
        else if (input.channels() > 1)
        {
            cv::cvtColor(input, grayInput, cv::COLOR_BGR2GRAY);
        }
        else
        {
            grayInput = input;
        }

        if (tpl.channels() == 4)
        {
            cv::cvtColor(tpl, grayTpl, cv::COLOR_BGRA2GRAY);
        }
        else if (tpl.channels() > 1)
        {
            cv::cvtColor(tpl, grayTpl, cv::COLOR_BGR2GRAY);
        }
        else
        {
            grayTpl = tpl;
        }

        cv::Mat result;
        cv::matchTemplate(grayInput, grayTpl, result, cvMethod);

        if (input.channels() == 1)
        {
            cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
        }
        else if (input.channels() == 4)
        {
            cv::cvtColor(input, output, cv::COLOR_BGRA2BGR);
        }
        else
        {
            output = input.clone();
        }

        // 若是框选模式，绘制洋红色参考框
        if (tplSrc != 2 && tplRoi.width > 0)
        {
            cv::rectangle(output, tplRoi, cv::Scalar(255, 0, 255), 1, cv::LINE_AA);
        }

        if (!multiMatch)
        {
            // 单目标最优匹配
            double minVal, maxVal;
            cv::Point minLoc, maxLoc;
            cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

            cv::Point matchLoc = isSqDiff ? minLoc : maxLoc;
            double matchScore = isSqDiff ? minVal : maxVal;
            double similarity = isSqDiff ? 1.0 - minVal : maxVal;
            const bool accepted = !isNormalizedMethod || similarity >= scoreThresh;
            AddResult("match.rawScore", "模板匹配原始得分", matchScore);
            AddResult("match.similarity", "模板匹配相似度", isNormalizedMethod ? similarity : matchScore);
            AddResult("match.x", "匹配位置 X", static_cast<std::int64_t>(matchLoc.x), "px");
            AddResult("match.y", "匹配位置 Y", static_cast<std::int64_t>(matchLoc.y), "px");

            if (accepted)
            {
                cv::Rect matchRect(matchLoc.x, matchLoc.y, rw, rh);
                cv::rectangle(output, matchRect, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

                std::stringstream ss;
                ss << "Match: " << std::fixed << std::setprecision(4)
                   << (isNormalizedMethod ? similarity : matchScore)
                   << " (" << matchLoc.x << "," << matchLoc.y << ")";
                cv::putText(output, ss.str(), cv::Point(matchLoc.x, std::max(16, matchLoc.y - 6)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
            }
        }
        else
        {
            // 多目标阈值匹配
            cv::Mat similarityMap;
            if (isNormalizedMethod)
            {
                similarityMap = isSqDiff ? (1.0f - result) : result.clone();
            }
            else
            {
                cv::normalize(result, similarityMap, 0.0, 1.0, cv::NORM_MINMAX);
                if (isSqDiff)
                {
                    similarityMap = 1.0f - similarityMap;
                }
            }

            cv::Mat suppressedScores = similarityMap.clone();
            const int suppressionRadiusX = std::max(1, rw / 2);
            const int suppressionRadiusY = std::max(1, rh / 2);
            int matchCount = 0;
            while (matchCount < 200)
            {
                double peakScore = 0.0;
                cv::Point location;
                cv::minMaxLoc(suppressedScores, nullptr, &peakScore, nullptr, &location);
                if (peakScore < scoreThresh)
                {
                    break;
                }

                cv::rectangle(output, cv::Rect(location.x, location.y, rw, rh), cv::Scalar(0, 165, 255), 2);
                ++matchCount;

                const int left = std::max(0, location.x - suppressionRadiusX);
                const int top = std::max(0, location.y - suppressionRadiusY);
                const int right = std::min(suppressedScores.cols, location.x + suppressionRadiusX + 1);
                const int bottom = std::min(suppressedScores.rows, location.y + suppressionRadiusY + 1);
                suppressedScores(cv::Rect(left, top, right - left, bottom - top)).setTo(-1.0f);
            }
            AddResult("match.count", "多目标匹配数量", static_cast<std::int64_t>(matchCount));

            std::stringstream ss;
            ss << "Found " << matchCount << " matches";
            cv::putText(output, ss.str(), cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        }

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
    int tplSrc = _params[1].GetInt();
    std::string tplPath = _params[2].GetString();
    int rx = _params[3].GetInt();
    int ry = _params[4].GetInt();
    int rw = _params[5].GetInt();
    int rh = _params[6].GetInt();
    double scoreThresh = _params[7].GetDouble();
    bool multiMatch = _params[8].GetBool();

    const char* methods[] = {
        "cv::TM_SQDIFF", "cv::TM_SQDIFF_NORMED", "cv::TM_CCORR",
        "cv::TM_CCORR_NORMED", "cv::TM_CCOEFF", "cv::TM_CCOEFF_NORMED"
    };
    const std::string methodStr = methods[methodIdx];
    const bool isSqDiff = methodIdx <= 1;
    const bool isNormalizedMethod = methodIdx == 1 || methodIdx == 3 || methodIdx == 5;

    std::stringstream ss;
    ss << "// 模板匹配与目标定位\n";
    if (tplSrc == 2 && !tplPath.empty())
    {
        ss << "cv::Mat templateImg = cv::imread(\"" << EscapeCppString(tplPath) << "\");\n";
    }
    else
    {
        ss << "cv::Mat templateBase = " << (tplSrc == 1 ? "source_image" : inVar) << ";\n"
           << "int templateX = std::clamp(" << rx << ", 0, templateBase.cols - 10);\n"
           << "int templateY = std::clamp(" << ry << ", 0, templateBase.rows - 10);\n"
           << "cv::Rect tplRoi(templateX, templateY, std::min(" << rw << ", templateBase.cols - templateX), std::min(" << rh << ", templateBase.rows - templateY));\n"
           << "cv::Mat templateImg = templateBase(tplRoi).clone();\n";
    }
    ss << "cv::Mat matchGray, templateGray;\n"
       << "if (" << inVar << ".channels() == 1) matchGray = " << inVar << "; else cv::cvtColor(" << inVar << ", matchGray, cv::COLOR_BGR2GRAY);\n"
       << "if (templateImg.channels() == 1) templateGray = templateImg; else cv::cvtColor(templateImg, templateGray, cv::COLOR_BGR2GRAY);\n"
       << "cv::Mat result_" << outVar << ";\n"
       << "cv::matchTemplate(matchGray, templateGray, result_" << outVar << ", " << methodStr << ");\n"
       << "double minVal, maxVal; cv::Point minLoc, maxLoc;\n"
       << "cv::minMaxLoc(result_" << outVar << ", &minVal, &maxVal, &minLoc, &maxLoc);\n"
       << outVar << " = " << inVar << ".clone();\n";
    if (!multiMatch)
    {
        ss << "cv::Point matchLocation = " << (isSqDiff ? "minLoc" : "maxLoc") << ";\n"
           << "double matchSimilarity = " << (isSqDiff ? "1.0 - minVal" : "maxVal") << ";\n"
           << "if (" << (!isNormalizedMethod ? "true" : ("matchSimilarity >= " + std::to_string(scoreThresh))) << ")\n"
           << "{\n"
           << "    cv::rectangle(" << outVar << ", cv::Rect(matchLocation.x, matchLocation.y, templateImg.cols, templateImg.rows), cv::Scalar(0, 255, 0), 2);\n"
           << "}";
    }
    else
    {
        ss << "cv::Mat matchSimilarityMap;\n";
        if (isNormalizedMethod)
        {
            ss << "matchSimilarityMap = " << (isSqDiff ? "1.0f - result_" + outVar : "result_" + outVar + ".clone()") << ";\n";
        }
        else
        {
            ss << "cv::normalize(result_" << outVar << ", matchSimilarityMap, 0.0, 1.0, cv::NORM_MINMAX);\n";
            if (isSqDiff)
            {
                ss << "matchSimilarityMap = 1.0f - matchSimilarityMap;\n";
            }
        }
        ss << "cv::Mat suppressedScores = matchSimilarityMap.clone(); int matchCount = 0;\n"
           << "while (matchCount < 200)\n"
           << "{\n"
           << "    double peakScore = 0.0; cv::Point location; cv::minMaxLoc(suppressedScores, nullptr, &peakScore, nullptr, &location);\n"
           << "    if (peakScore < " << scoreThresh << ") break;\n"
           << "    cv::rectangle(" << outVar << ", cv::Rect(location.x, location.y, templateImg.cols, templateImg.rows), cv::Scalar(0,165,255), 2); ++matchCount;\n"
           << "    int radiusX = std::max(1, templateImg.cols / 2), radiusY = std::max(1, templateImg.rows / 2);\n"
           << "    int left = std::max(0, location.x - radiusX), top = std::max(0, location.y - radiusY);\n"
           << "    int right = std::min(suppressedScores.cols, location.x + radiusX + 1), bottom = std::min(suppressedScores.rows, location.y + radiusY + 1);\n"
           << "    suppressedScores(cv::Rect(left, top, right - left, bottom - top)).setTo(-1.0f);\n"
           << "}";
    }
    return ss.str();
}

std::string MatchTemplateOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int methodIdx = _params[0].GetInt();
    int tplSrc = _params[1].GetInt();
    std::string tplPath = _params[2].GetString();
    int rx = _params[3].GetInt();
    int ry = _params[4].GetInt();
    int rw = _params[5].GetInt();
    int rh = _params[6].GetInt();
    double scoreThresh = _params[7].GetDouble();
    bool multiMatch = _params[8].GetBool();
    const char* methods[] = {
        "cv2.TM_SQDIFF", "cv2.TM_SQDIFF_NORMED", "cv2.TM_CCORR",
        "cv2.TM_CCORR_NORMED", "cv2.TM_CCOEFF", "cv2.TM_CCOEFF_NORMED"
    };
    const bool isSqDiff = methodIdx <= 1;
    const bool isNormalizedMethod = methodIdx == 1 || methodIdx == 3 || methodIdx == 5;

    std::stringstream ss;
    ss << "# 模板匹配与目标定位\n";
    if (tplSrc == 2 && !tplPath.empty())
    {
        ss << "template_img = cv2.imread('" << EscapePythonString(tplPath) << "')\n";
    }
    else
    {
        const std::string baseVar = tplSrc == 1 ? "source_image" : inVar;
        ss << "template_img = " << baseVar << "[" << ry << ":" << (ry + rh) << ", " << rx << ":" << (rx + rw) << "].copy()\n";
    }
    ss << "match_gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "template_gray = template_img if len(template_img.shape) == 2 else cv2.cvtColor(template_img, cv2.COLOR_BGR2GRAY)\n"
       << "res = cv2.matchTemplate(match_gray, template_gray, " << methods[methodIdx] << ")\n"
       << "min_val, max_val, min_loc, max_loc = cv2.minMaxLoc(res)\n"
       << outVar << " = " << inVar << ".copy()\n";
    if (!multiMatch)
    {
        ss << "match_location = " << (isSqDiff ? "min_loc" : "max_loc") << "\n"
           << "match_similarity = " << (isSqDiff ? "1.0 - min_val" : "max_val") << "\n"
           << "if " << (!isNormalizedMethod ? "True" : ("match_similarity >= " + std::to_string(scoreThresh))) << ":\n"
           << "    cv2.rectangle(" << outVar << ", match_location, (match_location[0] + template_img.shape[1], match_location[1] + template_img.shape[0]), (0, 255, 0), 2)";
    }
    else
    {
        if (isNormalizedMethod)
        {
            ss << "match_similarity_map = " << (isSqDiff ? "1.0 - res" : "res.copy()") << "\n";
        }
        else
        {
            ss << "match_similarity_map = cv2.normalize(res, None, 0, 1, cv2.NORM_MINMAX)\n";
            if (isSqDiff)
            {
                ss << "match_similarity_map = 1.0 - match_similarity_map\n";
            }
        }
        ss << "suppressed_scores = match_similarity_map.copy()\n"
           << "suppression_radius_x = max(1, template_img.shape[1] // 2)\n"
           << "suppression_radius_y = max(1, template_img.shape[0] // 2)\n"
           << "match_count = 0\n"
           << "while match_count < 200:\n"
           << "    _, peak_score, _, location = cv2.minMaxLoc(suppressed_scores)\n"
           << "    if peak_score < " << scoreThresh << ": break\n"
           << "    cv2.rectangle(" << outVar << ", location, (location[0] + template_img.shape[1], location[1] + template_img.shape[0]), (0,165,255), 2)\n"
           << "    match_count += 1\n"
           << "    left, top = max(0, location[0] - suppression_radius_x), max(0, location[1] - suppression_radius_y)\n"
           << "    right = min(suppressed_scores.shape[1], location[0] + suppression_radius_x + 1)\n"
           << "    bottom = min(suppressed_scores.shape[0], location[1] + suppression_radius_y + 1)\n"
           << "    suppressed_scores[top:bottom, left:right] = -1.0";
    }
    return ss.str();
}
