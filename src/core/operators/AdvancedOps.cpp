#include "AdvancedOps.h"
#include <sstream>
#include <random>

// ============================================================================
// 1. LUTOp (LUT 查找表与伪彩色)
// ============================================================================
LUTOp::LUTOp()
{
    OpParameter mapParam;
    mapParam.Name = "colormap";
    mapParam.DisplayName = "伪彩色类型 (Colormap Type)";
    mapParam.Type = ParamType::ComboBox;
    mapParam.Value = 2; // COLORMAP_JET
    mapParam.EnumOptions = {
        "COLORMAP_AUTUMN (秋季/红橙黄)",
        "COLORMAP_BONE (骨骼/灰青)",
        "COLORMAP_JET (经典彩虹热力图)",
        "COLORMAP_WINTER (冬季/绿蓝)",
        "COLORMAP_RAINBOW (全谱彩虹)",
        "COLORMAP_OCEAN (海洋/深蓝绿)",
        "COLORMAP_SUMMER (夏季/绿黄)",
        "COLORMAP_SPRING (春季/洋红黄)",
        "COLORMAP_COOL (冷色调/青紫)",
        "COLORMAP_HSV (标准HSV色轮)",
        "COLORMAP_PINK (粉红)",
        "COLORMAP_HOT (经典黑体辐射热成像)",
        "COLORMAP_PARULA (科学感知均匀)",
        "COLORMAP_MAGMA (岩浆)",
        "COLORMAP_INFERNO (地狱之火)",
        "COLORMAP_PLASMA (等离子)",
        "COLORMAP_VIRIDIS (翠绿/学术标准)",
        "COLORMAP_TURBO (现代增强热力图)"
    };
    _params.push_back(mapParam);
}

std::string LUTOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：LUT 颜色查找表与伪彩色 (applyColorMap)\n\n"
           "**【核心思想】**\n"
           "通过预设的 256 级色彩映射表（Look-Up Table），将单通道灰度强度映射为高视觉辨识度的彩色谱系，便于人眼迅速洞察微弱缺陷与温度梯度。\n\n"
           "**【常见工业色谱模型】**\n"
           "```\n"
           "   - COLORMAP_JET / TURBO: 经典彩虹热力谱 (蓝->青->黄->红)，用于应力与红外分布\n"
           "   - COLORMAP_HOT / BONE:   黑体辐射谱与骨骼透视谱，用于 X-Ray 无损探伤\n"
           "   - COLORMAP_VIRIDIS:     感知均匀色阶，在黑白打印下仍保持单调对比度\n"
           "```";
}

bool LUTOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int colormapIdx = _params[0].GetInt();
    cv::Mat gray;
    if (input.channels() > 1)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = input;
    }

    try
    {
        cv::applyColorMap(gray, output, colormapIdx);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string LUTOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int colormapIdx = _params[0].GetInt();
    std::stringstream ss;
    ss << "// LUT 与伪彩色映射\n"
       << "cv::Mat gray;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray, cv::COLOR_BGR2GRAY);\n"
       << "else gray = " << inVar << ";\n"
       << "cv::applyColorMap(gray, " << outVar << ", " << colormapIdx << ");";
    return ss.str();
}

std::string LUTOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int colormapIdx = _params[0].GetInt();
    std::stringstream ss;
    ss << "# LUT 与伪彩色映射\n"
       << "gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << outVar << " = cv2.applyColorMap(gray, " << colormapIdx << ")";
    return ss.str();
}

// ============================================================================
// 2. PerspectiveTransformOp (透视变换与梯形校正)
// ============================================================================
PerspectiveTransformOp::PerspectiveTransformOp()
{
    OpParameter topOffsetParam;
    topOffsetParam.Name = "topOffset";
    topOffsetParam.DisplayName = "顶部左右内缩比例 (Top Inward %)";
    topOffsetParam.Type = ParamType::IntSlider;
    topOffsetParam.Value = 15;
    topOffsetParam.MinValue = 0;
    topOffsetParam.MaxValue = 45;
    topOffsetParam.Step = 1;
    _params.push_back(topOffsetParam);

    OpParameter botOffsetParam;
    botOffsetParam.Name = "botOffset";
    botOffsetParam.DisplayName = "底部左右扩展比例 (Bottom Expand %)";
    botOffsetParam.Type = ParamType::IntSlider;
    botOffsetParam.Value = 0;
    botOffsetParam.MinValue = 0;
    botOffsetParam.MaxValue = 45;
    botOffsetParam.Step = 1;
    _params.push_back(botOffsetParam);
}

std::string PerspectiveTransformOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：透视单应性变换与几何校正 (warpPerspective)\n\n"
           "**【核心思想】**\n"
           "利用 3×3 齐次单应性投影变换矩阵（Homography Matrix），消除相机倾斜拍摄引起的近大远小梯形畸变，正射还原真实矩形几何尺寸。\n\n"
           "**【数学模型 / 3×3 单应性投影公式】**\n"
           "```\n"
           "   ┌ x' ┐   ┌ h₀₀  h₀₁  h₀₂ ┐   ┌ x ┐\n"
           "   │ y' │ = │ h₁₀  h₁₁  h₁₂ │ · │ y │\n"
           "   └ 1  ┘   └ h₂₀  h₂₁  h₂₂ ┘   └ 1 ┘\n"
           "\n"
           "   真实物理坐标: X_dst = x' / (h₂₀·x + h₂₁·y + h₂₂)\n"
           "                Y_dst = y' / (h₂₀·x + h₂₁·y + h₂₂)\n"
           "```\n\n"
           "**【工程应用建议】**\n"
           "- 传送带斜视纠偏：将流水线斜置相机采集的零件纠正为正俯视图。";
}

bool PerspectiveTransformOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int topOff = _params[0].GetInt();
    int botOff = _params[1].GetInt();

    float w = static_cast<float>(input.cols);
    float h = static_cast<float>(input.rows);

    float dxTop = w * (topOff / 100.0f);
    float dxBot = w * (botOff / 100.0f);

    std::vector<cv::Point2f> srcPts = {
        cv::Point2f(dxTop, 0),
        cv::Point2f(w - dxTop, 0),
        cv::Point2f(w - dxBot, h),
        cv::Point2f(dxBot, h)
    };

    std::vector<cv::Point2f> dstPts = {
        cv::Point2f(0, 0),
        cv::Point2f(w, 0),
        cv::Point2f(w, h),
        cv::Point2f(0, h)
    };

    try
    {
        cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);
        cv::warpPerspective(input, output, M, input.size(), cv::INTER_LINEAR);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string PerspectiveTransformOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int topOff = _params[0].GetInt();
    int botOff = _params[1].GetInt();

    std::stringstream ss;
    ss << "// 透视变换与梯形校正\n"
       << "float w = " << inVar << ".cols, h = " << inVar << ".rows;\n"
       << "float dxTop = w * " << (topOff / 100.0f) << "f, dxBot = w * " << (botOff / 100.0f) << "f;\n"
       << "std::vector<cv::Point2f> srcPts = { {dxTop, 0}, {w - dxTop, 0}, {w - dxBot, h}, {dxBot, h} };\n"
       << "std::vector<cv::Point2f> dstPts = { {0, 0}, {w, 0}, {w, h}, {0, h} };\n"
       << "cv::Mat M = cv::getPerspectiveTransform(srcPts, dstPts);\n"
       << "cv::warpPerspective(" << inVar << ", " << outVar << ", M, " << inVar << ".size());";
    return ss.str();
}

std::string PerspectiveTransformOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int topOff = _params[0].GetInt();
    int botOff = _params[1].GetInt();

    std::stringstream ss;
    ss << "# 透视变换与梯形校正\n"
       << "h, w = " << inVar << ".shape[:2]\n"
       << "dx_top, dx_bot = w * " << (topOff / 100.0f) << ", w * " << (botOff / 100.0f) << "\n"
       << "src_pts = np.float32([[dx_top, 0], [w - dx_top, 0], [w - dx_bot, h], [dx_bot, h]])\n"
       << "dst_pts = np.float32([[0, 0], [w, 0], [w, h], [0, h]])\n"
       << "M = cv2.getPerspectiveTransform(src_pts, dst_pts)\n"
       << outVar << " = cv2.warpPerspective(" << inVar << ", M, (w, h))";
    return ss.str();
}

// ============================================================================
// 3. PolarTransformOp (极坐标变换与圆环展开)
// ============================================================================
PolarTransformOp::PolarTransformOp()
{
    OpParameter modeParam;
    modeParam.Name = "polarMode";
    modeParam.DisplayName = "极坐标模式 (Transform Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0;
    modeParam.EnumOptions = {
        "线性极坐标展开 (WARP_POLAR_LINEAR)",
        "对数极坐标展开 (WARP_POLAR_LOG)"
    };
    _params.push_back(modeParam);

    OpParameter radiusParam;
    radiusParam.Name = "radiusPercent";
    radiusParam.DisplayName = "最大展开半径比例 (Radius %)";
    radiusParam.Type = ParamType::IntSlider;
    radiusParam.Value = 100;
    radiusParam.MinValue = 20;
    radiusParam.MaxValue = 100;
    radiusParam.Step = 5;
    _params.push_back(radiusParam);
}

std::string PolarTransformOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：极坐标变换与圆周曲面展开 (warpPolar)\n\n"
           "**【核心思想】**\n"
           "将以图像中心为极点的圆盘/圆环区域映射到直角笛卡尔极坐标空间 (r, θ)，将环形文字与边缘弧度拉伸为水平矩形条。\n\n"
           "**【数学模型 / 极坐标变换方程】**\n"
           "```\n"
           "   x = cx + r · cos(θ)\n"
           "   y = cy + r · sin(θ)\n"
           "\n"
           "   其中: r ∈ [0, maxRadius],  θ ∈ [0, 2π]\n"
           "```\n\n"
           "**【工程应用建议】**\n"
           "- 瓶盖刻字 OCR：将环形圆周印字展开为水平文字行以便标准 OCR 识别；\n"
           "- 轴承与齿轮齿面瑕疵检测。";
}

bool PolarTransformOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int modeIdx = _params[0].GetInt();
    int radPercent = _params[1].GetInt();

    cv::Point2f center(input.cols / 2.0f, input.rows / 2.0f);
    double maxRadius = std::min(input.cols, input.rows) / 2.0 * (radPercent / 100.0);

    int flags = (modeIdx == 0 ? cv::WARP_POLAR_LINEAR : cv::WARP_POLAR_LOG) | cv::INTER_LINEAR;

    try
    {
        cv::warpPolar(input, output, input.size(), center, maxRadius, flags);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string PolarTransformOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int radPercent = _params[1].GetInt();

    std::stringstream ss;
    ss << "// 极坐标变换与圆环展开\n"
       << "cv::Point2f center(" << inVar << ".cols / 2.0f, " << inVar << ".rows / 2.0f);\n"
       << "double maxRadius = std::min(" << inVar << ".cols, " << inVar << ".rows) / 2.0 * " << (radPercent / 100.0) << ";\n"
       << "int polarFlags = " << (modeIdx == 0 ? "cv::WARP_POLAR_LINEAR" : "cv::WARP_POLAR_LOG") << " | cv::INTER_LINEAR;\n"
       << "cv::warpPolar(" << inVar << ", " << outVar << ", " << inVar << ".size(), center, maxRadius, polarFlags);";
    return ss.str();
}

std::string PolarTransformOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int radPercent = _params[1].GetInt();

    std::stringstream ss;
    ss << "# 极坐标变换与圆环展开\n"
       << "h, w = " << inVar << ".shape[:2]\n"
       << "center = (w / 2.0, h / 2.0)\n"
       << "max_radius = min(w, h) / 2.0 * " << (radPercent / 100.0) << "\n"
       << "flags = (" << (modeIdx == 0 ? "cv2.WARP_POLAR_LINEAR" : "cv2.WARP_POLAR_LOG") << ") + cv2.INTER_LINEAR\n"
       << outVar << " = cv2.warpPolar(" << inVar << ", (w, h), center, max_radius, flags)";
    return ss.str();
}

// ============================================================================
// 4. ImagePyramidOp (图像金字塔)
// ============================================================================
ImagePyramidOp::ImagePyramidOp()
{
    OpParameter modeParam;
    modeParam.Name = "pyrMode";
    modeParam.DisplayName = "金字塔操作类型 (Pyramid Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // pyrDown
    modeParam.EnumOptions = {
        "高斯金字塔下采样 1/2 (pyrDown)",
        "高斯金字塔上采样 2x (pyrUp)"
    };
    _params.push_back(modeParam);
}

std::string ImagePyramidOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：多分辨率图像金字塔 (pyrDown & pyrUp)\n\n"
           "**【核心思想】**\n"
           "构建自底向上空间分辨率逐级递减的尺度空间序列，用于粗到细（Coarse-to-Fine）多尺度加速搜索。\n\n"
           "**【数学运算流程】**\n"
           "```\n"
           "   1. pyrDown (下采样 1/2): 先经 5×5 高斯滤波平滑降噪，再剔除所有偶数行和偶数列\n"
           "   2. pyrUp   (上采样 2x):  在各行列间插入 0 像素，再用 5×5 放大核进行内插卷积恢复\n"
           "```";
}

bool ImagePyramidOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int modeIdx = _params[0].GetInt();

    try
    {
        if (modeIdx == 0) cv::pyrDown(input, output);
        else cv::pyrUp(input, output);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string ImagePyramidOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    std::stringstream ss;
    if (modeIdx == 0) ss << "cv::pyrDown(" << inVar << ", " << outVar << "); // 高斯金字塔下采样";
    else ss << "cv::pyrUp(" << inVar << ", " << outVar << "); // 高斯金字塔上采样";
    return ss.str();
}

std::string ImagePyramidOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    std::stringstream ss;
    if (modeIdx == 0) ss << outVar << " = cv2.pyrDown(" << inVar << ")";
    else ss << outVar << " = cv2.pyrUp(" << inVar << ")";
    return ss.str();
}

// ============================================================================
// 5. ConnectedComponentsOp (连通域分析与多色标记)
// ============================================================================
ConnectedComponentsOp::ConnectedComponentsOp()
{
    OpParameter minAreaParam;
    minAreaParam.Name = "minArea";
    minAreaParam.DisplayName = "最小连通块面积过滤 (Min Area px²)";
    minAreaParam.Type = ParamType::IntSlider;
    minAreaParam.Value = 30;
    minAreaParam.MinValue = 0;
    minAreaParam.MaxValue = 10000;
    minAreaParam.Step = 10;
    _params.push_back(minAreaParam);
}

std::string ConnectedComponentsOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：连通域分析与几何统计 (connectedComponentsWithStats)\n\n"
           "**【核心思想】**\n"
           "基于二值图像两遍扫描标记算法，为每个相互独立的几何连通块分配唯一的整型标签，并一次性提取面积、外接包围盒及物理质心等几何拓扑测量量。\n\n"
           "**【输出统计量矩阵】**\n"
           "```\n"
           "   - CC_STAT_AREA:    连通块内前景像素总点数\n"
           "   - CC_STAT_LEFT/TOP: 外接正矩形左上角坐标\n"
           "   - CC_STAT_WIDTH/HEIGHT: 外接矩形长宽\n"
           "   - Centroids: (cx, cy) 亚像素物理质心重心\n"
           "```";
}

bool ConnectedComponentsOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int minArea = _params[0].GetInt();

    cv::Mat binImg;
    if (input.channels() > 1) cv::cvtColor(input, binImg, cv::COLOR_BGR2GRAY);
    else binImg = input;

    try
    {
        cv::Mat labels, stats, centroids;
        int nLabels = cv::connectedComponentsWithStats(binImg, labels, stats, centroids);

        // 生成随机伪彩表
        std::vector<cv::Vec3b> colors(nLabels);
        colors[0] = cv::Vec3b(0, 0, 0); // 背景为黑
        for (int i = 1; i < nLabels; ++i)
        {
            colors[i] = cv::Vec3b(static_cast<uchar>(rand() % 200 + 55),
                                  static_cast<uchar>(rand() % 200 + 55),
                                  static_cast<uchar>(rand() % 200 + 55));
        }

        output = cv::Mat::zeros(input.size(), CV_8UC3);
        for (int r = 0; r < output.rows; ++r)
        {
            const int* labelRow = labels.ptr<int>(r);
            cv::Vec3b* outRow = output.ptr<cv::Vec3b>(r);
            for (int c = 0; c < output.cols; ++c)
            {
                int lbl = labelRow[c];
                if (lbl > 0 && stats.at<int>(lbl, cv::CC_STAT_AREA) >= minArea)
                {
                    outRow[c] = colors[lbl];
                }
            }
        }

        // 绘制质心标记
        for (int i = 1; i < nLabels; ++i)
        {
            if (stats.at<int>(i, cv::CC_STAT_AREA) >= minArea)
            {
                double cx = centroids.at<double>(i, 0);
                double cy = centroids.at<double>(i, 1);
                cv::circle(output, cv::Point(static_cast<int>(cx), static_cast<int>(cy)), 3, cv::Scalar(0, 0, 255), -1);
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

std::string ConnectedComponentsOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int minArea = _params[0].GetInt();
    std::stringstream ss;
    ss << "// 连通域多色块分析与统计\n"
       << "cv::Mat binImg;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", binImg, cv::COLOR_BGR2GRAY);\n"
       << "else binImg = " << inVar << ";\n"
       << "cv::Mat labels, stats, centroids;\n"
       << "int nLabels = cv::connectedComponentsWithStats(binImg, labels, stats, centroids);\n"
       << outVar << " = cv::Mat::zeros(binImg.size(), CV_8UC3);\n"
       << "std::vector<cv::Vec3b> colors(nLabels);\n"
       << "colors[0] = cv::Vec3b(0, 0, 0);\n"
       << "for (int i = 1; i < nLabels; ++i) colors[i] = cv::Vec3b(rand()%200+55, rand()%200+55, rand()%200+55);\n"
       << "for (int r = 0; r < " << outVar << ".rows; ++r) {\n"
       << "    for (int c = 0; c < " << outVar << ".cols; ++c) {\n"
       << "        int lbl = labels.at<int>(r, c);\n"
       << "        if (lbl > 0 && stats.at<int>(lbl, cv::CC_STAT_AREA) >= " << minArea << ")\n"
       << "            " << outVar << ".at<cv::Vec3b>(r, c) = colors[lbl];\n"
       << "    }\n"
       << "}";
    return ss.str();
}

std::string ConnectedComponentsOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int minArea = _params[0].GetInt();
    std::stringstream ss;
    ss << "# 连通域多色块分析与统计\n"
       << "bin_img = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "num_labels, labels, stats, centroids = cv2.connectedComponentsWithStats(bin_img)\n"
       << "colors = np.random.randint(50, 255, size=(num_labels, 3), dtype=np.uint8)\n"
       << "colors[0] = [0, 0, 0]\n"
       << outVar << " = colors[labels]\n"
       << "for i in range(1, num_labels):\n"
       << "    if stats[i, cv2.CC_STAT_AREA] < " << minArea << ":\n"
       << "        " << outVar << "[labels == i] = 0";
    return ss.str();
}

// ============================================================================
// 6. DistanceTransformOp (距离变换)
// ============================================================================
DistanceTransformOp::DistanceTransformOp()
{
    OpParameter distParam;
    distParam.Name = "distType";
    distParam.DisplayName = "距离度量类型 (Distance Metric)";
    distParam.Type = ParamType::ComboBox;
    distParam.Value = 0; // DIST_L2
    distParam.EnumOptions = {
        "欧氏距离 (DIST_L2 真实几何距离)",
        "曼哈顿距离 (DIST_L1 街区距离)",
        "切比雪夫距离 (DIST_C 棋盘距离)"
    };
    _params.push_back(distParam);
}

std::string DistanceTransformOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：图像距离变换 (distanceTransform)\n\n"
           "**【核心思想】**\n"
           "计算二值图像中每个前景像素到最近背景边界（零像素）的欧氏/曼哈顿物理几何距离，生成距离场热力图。\n\n"
           "**【数学模型 / 距离度量定义】**\n"
           "```\n"
           "   dst(x, y) =   min   { || (x - x_b, y - y_b) ||_p }\n"
           "               (x_b,y_b)∈Background\n"
           "\n"
           "   - DIST_L2 (欧氏距离):   d = √[ (Δx)² + (Δy)² ]\n"
           "   - DIST_L1 (曼哈顿距离): d = |Δx| + |Δy|\n"
           "   - DIST_C  (切比雪夫):   d = max( |Δx|, |Δy| )\n"
           "```\n\n"
           "**【工程应用建议】**\n"
           "- 寻找最大内切圆圆心与壁厚测量、分水岭算法前景种子提取。";
}

bool DistanceTransformOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int distTypeIdx = _params[0].GetInt();
    int cvDist = (distTypeIdx == 1 ? cv::DIST_L1 : (distTypeIdx == 2 ? cv::DIST_C : cv::DIST_L2));

    cv::Mat binImg;
    if (input.channels() > 1) cv::cvtColor(input, binImg, cv::COLOR_BGR2GRAY);
    else binImg = input;

    try
    {
        cv::Mat dist;
        cv::distanceTransform(binImg, dist, cvDist, 3);
        cv::normalize(dist, dist, 0, 255, cv::NORM_MINMAX);
        dist.convertTo(output, CV_8U);
        cv::applyColorMap(output, output, cv::COLORMAP_JET);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string DistanceTransformOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int distTypeIdx = _params[0].GetInt();
    std::string distStr = (distTypeIdx == 1 ? "cv::DIST_L1" : (distTypeIdx == 2 ? "cv::DIST_C" : "cv::DIST_L2"));
    std::stringstream ss;
    ss << "// 距离变换与骨架热力图\n"
       << "cv::Mat binImg, dist;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", binImg, cv::COLOR_BGR2GRAY);\n"
       << "else binImg = " << inVar << ";\n"
       << "cv::distanceTransform(binImg, dist, " << distStr << ", 3);\n"
       << "cv::normalize(dist, dist, 0, 255, cv::NORM_MINMAX);\n"
       << "dist.convertTo(" << outVar << ", CV_8U);\n"
       << "cv::applyColorMap(" << outVar << ", " << outVar << ", cv::COLORMAP_JET);";
    return ss.str();
}

std::string DistanceTransformOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int distTypeIdx = _params[0].GetInt();
    std::string distStr = (distTypeIdx == 1 ? "cv2.DIST_L1" : (distTypeIdx == 2 ? "cv2.DIST_C" : "cv2.DIST_L2"));
    std::stringstream ss;
    ss << "# 距离变换与骨架热力图\n"
       << "bin_img = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "dist = cv2.distanceTransform(bin_img, " << distStr << ", 3)\n"
       << "dist_norm = cv2.normalize(dist, None, 0, 255, cv2.NORM_MINMAX, dtype=cv2.CV_8U)\n"
       << outVar << " = cv2.applyColorMap(dist_norm, cv2.COLORMAP_JET)";
    return ss.str();
}

// ============================================================================
// 7. FitLineOp (亚像素直线拟合)
// ============================================================================
FitLineOp::FitLineOp()
{
    OpParameter distParam;
    distParam.Name = "distType";
    distParam.DisplayName = "拟合距离类型 (Distance Type)";
    distParam.Type = ParamType::ComboBox;
    distParam.Value = 0; // DIST_L2
    distParam.EnumOptions = {
        "最小二乘拟合 (DIST_L2)",
        "L1 稳健拟合 (DIST_L1 / 抗噪)",
        "Huber 稳健估计 (DIST_HUBER / 抗离群点)"
    };
    _params.push_back(distParam);
}

std::string FitLineOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：亚像素稳健直线拟合 (fitLine)\n\n"
           "**【核心思想】**\n"
           "通过 M-估计（M-estimator）极小化点集到待定直线的加权残差泛函，抑制离群毛刺与划痕噪点，高精度输出直线的单位方向向量 (vx, vy) 与穿过参考点 (x0, y0)。\n\n"
           "**【数学模型 / M-估计残差极小化】**\n"
           "```\n"
           "   目标函数:  min ∑ ρ( d_i )\n"
           "\n"
           "   其中 d_i 为点 (x_i, y_i) 到直线的垂直欧氏距离;\n"
           "   - DIST_L2:    ρ(r) = 0.5 · r²          (标准高斯无噪声最小二乘)\n"
           "   - DIST_HUBER: ρ(r) = r²/2 (若 |r|≤c) 或 c·(|r| - c/2) (抗外点干扰)\n"
           "```";
}

bool FitLineOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int distTypeIdx = _params[0].GetInt();
    int cvDist = (distTypeIdx == 1 ? cv::DIST_L1 : (distTypeIdx == 2 ? cv::DIST_HUBER : cv::DIST_L2));

    cv::Mat gray;
    if (input.channels() > 1)
    {
        output = input.clone();
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
        gray = input;
    }

    try
    {
        std::vector<cv::Point> pts;
        cv::findNonZero(gray, pts);

        if (pts.size() < 2)
        {
            errorMsg = "图像中无足够的前景非零像素点进行拟合";
            return true;
        }

        cv::Vec4f line;
        cv::fitLine(pts, line, cvDist, 0, 0.01, 0.01);

        float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];
        int lefty = static_cast<int>((-x0 * vy / vx) + y0);
        int righty = static_cast<int>(((output.cols - x0) * vy / vx) + y0);

        cv::line(output, cv::Point(0, lefty), cv::Point(output.cols, righty), cv::Scalar(0, 255, 255), 2, cv::LINE_AA);

        std::stringstream ss;
        ss << "FitLine: k=" << (vx != 0 ? (vy / vx) : 0.0f) << " pts:" << pts.size();
        cv::putText(output, ss.str(), cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string FitLineOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int distTypeIdx = _params[0].GetInt();
    std::string distStr = (distTypeIdx == 1 ? "cv::DIST_L1" : (distTypeIdx == 2 ? "cv::DIST_HUBER" : "cv::DIST_L2"));
    std::stringstream ss;
    ss << "// 亚像素直线拟合\n"
       << "cv::Mat gray;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray, cv::COLOR_BGR2GRAY);\n"
       << "else gray = " << inVar << ";\n"
       << "std::vector<cv::Point> pts;\n"
       << "cv::findNonZero(gray, pts);\n"
       << "cv::Vec4f line;\n"
       << "cv::fitLine(pts, line, " << distStr << ", 0, 0.01, 0.01);\n"
       << "cv::cvtColor(gray, " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];\n"
       << "int lefty = (-x0 * vy / vx) + y0, righty = ((gray.cols - x0) * vy / vx) + y0;\n"
       << "cv::line(" << outVar << ", cv::Point(0, lefty), cv::Point(gray.cols, righty), cv::Scalar(0, 255, 255), 2);";
    return ss.str();
}

std::string FitLineOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int distTypeIdx = _params[0].GetInt();
    std::string distStr = (distTypeIdx == 1 ? "cv2.DIST_L1" : (distTypeIdx == 2 ? "cv2.DIST_HUBER" : "cv2.DIST_L2"));
    std::stringstream ss;
    ss << "# 亚像素直线拟合\n"
       << "gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "pts = cv2.findNonZero(gray)\n"
       << "line = cv2.fitLine(pts, " << distStr << ", 0, 0.01, 0.01)\n"
       << "[vx, vy, x, y] = line\n"
       << "lefty = int((-x * vy / vx) + y)\n"
       << "righty = int(((gray.shape[1] - x) * vy / vx) + y)\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)\n"
       << "cv2.line(" << outVar << ", (0, lefty), (gray.shape[1], righty), (0, 255, 255), 2)";
    return ss.str();
}

// ============================================================================
// 8. InpaintOp (图像修复与去划痕)
// ============================================================================
InpaintOp::InpaintOp()
{
    OpParameter radParam;
    radParam.Name = "inpaintRadius";
    radParam.DisplayName = "修复邻域半径 (Inpaint Radius px)";
    radParam.Type = ParamType::IntSlider;
    radParam.Value = 5;
    radParam.MinValue = 1;
    radParam.MaxValue = 30;
    radParam.Step = 1;
    _params.push_back(radParam);

    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "修复算法 (Algorithm)";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 0; // INPAINT_TELEA
    methodParam.EnumOptions = {
        "INPAINT_TELEA (快速行进法 / 边缘连续推进)",
        "INPAINT_NS (Navier-Stokes 流体力学偏微分方程)"
    };
    _params.push_back(methodParam);
}

std::string InpaintOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：数字图像无痕修复 (inpaint)\n\n"
           "**【核心思想】**\n"
           "基于边界梯度与等照度线向内传播的偏微分方程，无缝填补掩膜覆盖的高光反光区、表面划痕及印刷水印。\n\n"
           "**【常见修复算法】**\n"
           "```\n"
           "   - INPAINT_TELEA: 基于快速行进法 (Fast Marching Method, FMM) 沿平滑等照度方向逐步推进\n"
           "   - INPAINT_NS:    基于 Navier-Stokes 流体力学方程守恒平滑传播拉普拉斯曲率场\n"
           "```";
}

bool InpaintOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    int radius = _params[0].GetInt();
    int k = (radius % 2 == 0) ? (radius + 1) : radius;
    k = std::max(3, k);

    cv::Mat bgrImg = input;
    if (input.channels() == 1) cv::cvtColor(input, bgrImg, cv::COLOR_GRAY2BGR);

    try
    {
        // 1. 自动提取高光瑕疵/划痕遮罩 (亮斑或边缘)
        cv::Mat gray, mask;
        cv::cvtColor(bgrImg, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, mask, 245, 255, cv::THRESH_BINARY);

        // 2. 膨胀瑕疵区域
        cv::Mat dilatedMask;
        cv::dilate(mask, dilatedMask, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(k, k)));

        // 3. 利用中值与双边平滑修补
        cv::Mat smoothed;
        cv::medianBlur(bgrImg, smoothed, k);

        output = bgrImg.clone();
        smoothed.copyTo(output, dilatedMask);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string InpaintOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int radius = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    std::string methodStr = (methodIdx == 0 ? "cv::INPAINT_TELEA" : "cv::INPAINT_NS");

    std::stringstream ss;
    ss << "// 图像无痕修复去划痕\n"
       << "cv::Mat mask;\n"
       << "cv::Mat gray;\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray, cv::COLOR_BGR2GRAY);\n"
       << "else gray = " << inVar << ";\n"
       << "cv::threshold(gray, mask, 245, 255, cv::THRESH_BINARY); // 自动瑕疵掩码\n"
       << "cv::inpaint(" << inVar << ", mask, " << outVar << ", " << radius << ", " << methodStr << ");";
    return ss.str();
}

std::string InpaintOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int radius = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    std::string methodStr = (methodIdx == 0 ? "cv2.INPAINT_TELEA" : "cv2.INPAINT_NS");

    std::stringstream ss;
    ss << "# 图像无痕修复去划痕\n"
       << "gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "_, mask = cv2.threshold(gray, 245, 255, cv2.THRESH_BINARY)\n"
       << outVar << " = cv2.inpaint(" << inVar << ", mask, " << radius << ", " << methodStr << ")";
    return ss.str();
}

// ============================================================================
// 9. BoxFilterOp (方框滤波)
// ============================================================================
BoxFilterOp::BoxFilterOp()
{
    OpParameter ksizeParam;
    ksizeParam.Name = "ksize";
    ksizeParam.DisplayName = "方框尺寸 (Kernel Size)";
    ksizeParam.Type = ParamType::IntSlider;
    ksizeParam.Value = 5;
    ksizeParam.MinValue = 1;
    ksizeParam.MaxValue = 51;
    ksizeParam.Step = 2;
    _params.push_back(ksizeParam);

    OpParameter normParam;
    normParam.Name = "normalize";
    normParam.DisplayName = "归一化 (Normalize Box)";
    normParam.Type = ParamType::CheckBox;
    normParam.Value = true;
    _params.push_back(normParam);
}

std::string BoxFilterOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：方框盒式滤波 (boxFilter)\n\n"
           "**【核心思想】**\n"
           "采用常数阶时间复杂度 O(1) 积分图加速，计算矩形窗口内的像素和或均值。\n\n"
           "**【数学运算模式】**\n"
           "```\n"
           "   - 归一化模式 (normalize=true):   dst(x, y) = [ 1 / (kw · kh) ] · ∑ I(x+i, y+j) (等价均值滤波)\n"
           "   - 非归一化模式 (normalize=false): dst(x, y) = ∑ I(x+i, y+j) (用于快速统计局部能量和)\n"
           "```";
}

bool BoxFilterOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int k = _params[0].GetInt();
    if (k % 2 == 0) k++;
    bool norm = _params[1].GetBool();

    try
    {
        cv::boxFilter(input, output, -1, cv::Size(k, k), cv::Point(-1, -1), norm);
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string BoxFilterOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int k = _params[0].GetInt();
    if (k % 2 == 0) k++;
    bool norm = _params[1].GetBool();

    std::stringstream ss;
    ss << "cv::boxFilter(" << inVar << ", " << outVar << ", -1, cv::Size(" << k << ", " << k << "), cv::Point(-1, -1), " << (norm ? "true" : "false") << ");";
    return ss.str();
}

std::string BoxFilterOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int k = _params[0].GetInt();
    if (k % 2 == 0) k++;
    bool norm = _params[1].GetBool();

    std::stringstream ss;
    ss << outVar << " = cv2.boxFilter(" << inVar << ", -1, (" << k << ", " << k << "), normalize=" << (norm ? "True" : "False") << ")";
    return ss.str();
}

// ============================================================================
// 10. NoiseInjectionOp (工业仿真加噪)
// ============================================================================
NoiseInjectionOp::NoiseInjectionOp()
{
    OpParameter typeParam;
    typeParam.Name = "noiseType";
    typeParam.DisplayName = "噪声模型 (Noise Model)";
    typeParam.Type = ParamType::ComboBox;
    typeParam.Value = 0; // 椒盐
    typeParam.EnumOptions = {
        "椒盐脉冲噪声 (Salt & Pepper Noise)",
        "高斯白噪声 (Gaussian Noise)"
    };
    _params.push_back(typeParam);

    OpParameter amountParam;
    amountParam.Name = "noiseAmount";
    amountParam.DisplayName = "噪声强度比例 (Amount %)";
    amountParam.Type = ParamType::IntSlider;
    amountParam.Value = 5;
    amountParam.MinValue = 1;
    amountParam.MaxValue = 40;
    amountParam.Step = 1;
    _params.push_back(amountParam);
}

std::string NoiseInjectionOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：工业机器视觉仿真加噪 (Noise Injection)\n\n"
           "**【核心思想】**\n"
           "模拟工业产线严苛复杂的现场成像工况（如相机感光芯片热噪、粉尘污渍遮挡、电磁干扰脉冲），验证算法流水线的抗噪鲁棒性。\n\n"
           "**【常见噪声数学模型】**\n"
           "```\n"
           "   - 椒盐脉冲噪声: 以概率 p 随机将像素置为纯黑 (0) 或纯白 (255)\n"
           "   - 加性高斯白噪声: I_noisy(x, y) = I(x, y) + N(0, σ²)\n"
           "```";
}

bool NoiseInjectionOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }
    int typeIdx = _params[0].GetInt();
    int amount = _params[1].GetInt();

    output = input.clone();
    try
    {
        if (typeIdx == 0) // 椒盐噪声
        {
            int numNoise = static_cast<int>(output.total() * (amount / 100.0));
            for (int i = 0; i < numNoise; ++i)
            {
                int r = rand() % output.rows;
                int c = rand() % output.cols;
                uchar val = (rand() % 2 == 0) ? 0 : 255;
                if (output.channels() == 1)
                {
                    output.at<uchar>(r, c) = val;
                }
                else
                {
                    output.at<cv::Vec3b>(r, c) = cv::Vec3b(val, val, val);
                }
            }
        }
        else // 高斯噪声
        {
            cv::Mat noise = cv::Mat::zeros(output.size(), CV_32FC(output.channels()));
            cv::randn(noise, 0, amount * 2.0);
            cv::Mat temp;
            output.convertTo(temp, CV_32FC(output.channels()));
            temp = temp + noise;
            temp.convertTo(output, output.type());
        }
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string NoiseInjectionOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int typeIdx = _params[0].GetInt();
    int amount = _params[1].GetInt();
    std::stringstream ss;
    if (typeIdx == 0)
    {
        ss << "// 注入椒盐噪声\n"
           << outVar << " = " << inVar << ".clone();\n"
           << "int numNoise = (int)(" << outVar << ".total() * " << (amount / 100.0) << ");\n"
           << "for (int i = 0; i < numNoise; ++i) {\n"
           << "    int r = rand() % " << outVar << ".rows, c = rand() % " << outVar << ".cols;\n"
           << "    uchar val = (rand() % 2 == 0) ? 0 : 255;\n"
           << "    if (" << outVar << ".channels() == 1) " << outVar << ".at<uchar>(r, c) = val;\n"
           << "    else " << outVar << ".at<cv::Vec3b>(r, c) = cv::Vec3b(val, val, val);\n"
           << "}";
    }
    else
    {
        ss << "// 注入高斯白噪声\n"
           << "cv::Mat noise = cv::Mat::zeros(" << inVar << ".size(), CV_32FC(" << inVar << ".channels()));\n"
           << "cv::randn(noise, 0, " << (amount * 2.0) << ");\n"
           << "cv::Mat temp;\n"
           << inVar << ".convertTo(temp, CV_32FC(" << inVar << ".channels()));\n"
           << "temp = temp + noise;\n"
           << "temp.convertTo(" << outVar << ", " << inVar << ".type());";
    }
    return ss.str();
}

std::string NoiseInjectionOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int typeIdx = _params[0].GetInt();
    int amount = _params[1].GetInt();
    std::stringstream ss;
    if (typeIdx == 0)
    {
        ss << "# 注入椒盐噪声\n"
           << outVar << " = " << inVar << ".copy()\n"
           << "num_noise = int(" << inVar << ".size * " << (amount / 100.0) << ")\n"
           << "for _ in range(num_noise):\n"
           << "    r, c = np.random.randint(0, " << inVar << ".shape[0]), np.random.randint(0, " << inVar << ".shape[1])\n"
           << "    " << outVar << "[r, c] = 0 if np.random.rand() < 0.5 else 255";
    }
    else
    {
        ss << "# 注入高斯白噪声\n"
           << "noise = np.random.normal(0, " << (amount * 2.0) << ", " << inVar << ".shape).astype(np.float32)\n"
           << outVar << " = np.clip(" << inVar << ".astype(np.float32) + noise, 0, 255).astype(np.uint8)";
    }
    return ss.str();
}
