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
    return "### 📘 算子原理：LUT 颜色查找表与科学伪彩色 (applyColorMap & LUT)\n\n"
           "基于色彩映射表将单通道灰度数据非线性转换为三通道伪彩色图像：\n"
           "* **热力图可视化**：使用 `COLORMAP_JET` 或 `COLORMAP_TURBO` 快速凸显微弱亮度差异；\n"
           "* **医学与红外热成像**：使用 `COLORMAP_HOT` / `COLORMAP_BONE` 强化工业无损探伤与温度梯度分布。";
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
    return "### 📘 算子原理：透视变换与单应性投影 (warpPerspective)\n\n"
           "基于 $3 \\times 3$ 单应性变换矩阵 $H$ 纠正倾斜相机的透视畸变：\n"
           "$$\\begin{bmatrix} x' \\\\ y' \\\\ 1 \\end{bmatrix} = H \\begin{bmatrix} x \\\\ y \\\\ 1 \\end{bmatrix}$$\n"
           "实现对倾斜拍摄的铭牌、二维码、输送带工件进行鸟瞰图正射校准。";
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
    return "### 📘 算子原理：极坐标变换与圆周展开 (warpPolar)\n\n"
           "将以图像中心为极点的圆形区域映射到直角极坐标空间 $(r, \\theta)$：\n"
           "* **圆环字符识别与瑕疵检测**：将圆盘轴承、圆形瓶盖或齿轮表面的圆周曲线展开为平直水平矩形，使标准 OCR 和缺陷检测算法可直接处理。";
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
    return "### 📘 算子原理：图像金字塔与多尺度表达 (pyrDown & pyrUp)\n\n"
           "* **pyrDown**：先用高斯核卷积平滑，再剔除偶数行和偶数列，图像宽高减半；\n"
           "* **pyrUp**：在行列插入零像素后进行插值平滑，图像宽高翻倍；\n"
           "* 用于机器视觉中由粗到精的多分辨率快速模板匹配与尺度空间构建。";
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
           "对二值图像执行两遍扫描连通域标记：\n"
           "* **输出多色标签图**：为每个独立连通块赋予唯一伪彩，快速辨识紧密排列目标；\n"
           "* **统计量计算**：输出每个连通块的面积 $Area$、外接矩形与重心 $Centroid$。";
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
           "计算二值图每个像素点到最近背景零像素的最小距离：\n"
           "* **中心定位与骨架提取**：亮度最高的像素点即为工件最大内切圆圆心与厚度主轴中心；\n"
           "* **分水岭算法核心**：为分水岭算法提供前景精准种子点。";
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
           "通过 M-估计极小化点集到拟合直线的加权残差距离：\n"
           "$$\\min \\sum \\rho(d_i)$$\n"
           "在工业尺寸测量中即使存在毛刺或划痕噪点，依然能够稳定输出真实边缘直线方程。";
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
           "基于结构与纹理传播偏微分方程填充瑕疵缺损区域：\n"
           "* **INPAINT_TELEA**：沿等照度线梯度方向由外向内插值加权；\n"
           "* **INPAINT_NS**：模拟流体力学流函数传播平滑曲率，消除划痕与水印。";
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
           "基于积分图实现快速常数时间 $O(1)$ 邻域积分卷积平滑：\n"
           "* **归一化模式**：等价于均值平滑；\n"
           "* **非归一化模式**：输出邻域像素累加和，用于局部能量与特征密度分析。";
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
           "模拟工业产线恶劣成像环境（打光闪烁、传感器热噪、粉尘污渍）：\n"
           "* **椒盐噪声**：随机黑白二值坏点，用于检验中值滤波器的去除效果；\n"
           "* **高斯噪声**：正态分布热电噪声，用于检验双边与高斯滤波器的平滑能力。";
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
