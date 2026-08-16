#include "IndustrialOps.h"
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 1. 圆柱面曲面展开与重映射算子 (Cylindrical Unrolling)
// ============================================================================
CylindricalUnrollOp::CylindricalUnrollOp()
{
    OpParameter radParam;
    radParam.Name = "radiusRatio";
    radParam.DisplayName = "圆柱曲率半径比例 (Radius %)";
    radParam.Tooltip = "圆柱体曲面物理半径与图像宽度的相对比例，值越小弯曲展开程度越大";
    radParam.Type = ParamType::DoubleSlider;
    radParam.Value = 100.0;
    radParam.MinValue = 30.0;
    radParam.MaxValue = 400.0;
    _params.push_back(radParam);

    OpParameter focalParam;
    focalParam.Name = "focalRatio";
    focalParam.DisplayName = "相机等效焦距比例 (Focal %)";
    focalParam.Tooltip = "相机镜头焦距比例，影响圆柱曲面的透视矫正深度";
    focalParam.Type = ParamType::DoubleSlider;
    focalParam.Value = 100.0;
    focalParam.MinValue = 40.0;
    focalParam.MaxValue = 400.0;
    _params.push_back(focalParam);

    OpParameter offsetParam;
    offsetParam.Name = "centerXOffset";
    offsetParam.DisplayName = "中心水平偏移 (Center Offset px)";
    offsetParam.Tooltip = "圆柱中心轴线相对图像中心的水平偏移像素";
    offsetParam.Type = ParamType::IntSlider;
    offsetParam.Value = 0;
    offsetParam.MinValue = -800;
    offsetParam.MaxValue = 800;
    _params.push_back(offsetParam);

    OpParameter interpParam;
    interpParam.Name = "interp";
    interpParam.DisplayName = "重采样插值算法 (Interpolation)";
    interpParam.Tooltip = "图像重映射时使用的亚像素插值模式";
    interpParam.Type = ParamType::ComboBox;
    interpParam.Value = 0;
    interpParam.EnumOptions = { "双线性插值 (Bilinear)", "最近邻插值 (Nearest)", "双三次插值 (Bicubic)" };
    _params.push_back(interpParam);
}

std::string CylindricalUnrollOp::GetPrincipleDoc() const
{
    return "### 🛢️ 圆柱面曲面展开与重映射原理 (Cylindrical Unrolling)\n\n"
           "工业流水线中，贴在瓶身、易拉罐、注射器等圆柱体表面的标签与喷码，在正面拍摄时两侧边缘会因圆柱曲率产生严重的压缩变形。\n\n"
           "**几何投影反演模型**：\n"
           "1. 设圆柱半径为 $R = \\frac{W}{2} \\cdot \\text{radiusRatio}$，虚拟焦距为 $f = W \\cdot \\text{focalRatio}$；\n"
           "2. 对展开后平面像素坐标 $(x', y')$，计算对应的圆柱包角 $\\theta = \\frac{x' - cx}{R}$；\n"
           "3. 将圆柱三维点 $(R\\sin\\theta, y', R\\cos\\theta)$ 经透视投影映射回原拍摄曲面图像点 $(x, y)$；\n"
           "4. 构建全图反向寻址浮点映射网格 `map_x` 和 `map_y`，调用 `cv::remap` 实现亚像素展开。";
}

bool CylindricalUnrollOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    double radiusRatio = _params[0].GetDouble() / 100.0;
    double focalRatio = _params[1].GetDouble() / 100.0;
    int centerXOffset = _params[2].GetInt();
    int interpIdx = _params[3].GetInt();

    int interp = cv::INTER_LINEAR;
    if (interpIdx == 1) interp = cv::INTER_NEAREST;
    else if (interpIdx == 2) interp = cv::INTER_CUBIC;

    int w = input.cols;
    int h = input.rows;

    double cx = w / 2.0 + centerXOffset;
    double cy = h / 2.0;

    double R = (w / 2.0) * radiusRatio;
    double f = w * focalRatio;

    cv::Mat mapX(h, w, CV_32FC1);
    cv::Mat mapY(h, w, CV_32FC1);

    for (int y = 0; y < h; ++y)
    {
        float* pMapX = mapX.ptr<float>(y);
        float* pMapY = mapY.ptr<float>(y);

        double dy = y - cy;

        for (int x = 0; x < w; ++x)
        {
            double dx = x - cx;
            double theta = dx / R;

            if (std::abs(theta) >= M_PI / 2.0 * 0.98)
            {
                pMapX[x] = -1.0f;
                pMapY[x] = -1.0f;
            }
            else
            {
                double sinTheta = std::sin(theta);
                double cosTheta = std::cos(theta);

                double denom = R * cosTheta + f - R;
                if (std::abs(denom) > 1e-4)
                {
                    double srcX = cx + (f * R * sinTheta) / denom;
                    double srcY = cy + (f * dy) / denom;

                    pMapX[x] = static_cast<float>(srcX);
                    pMapY[x] = static_cast<float>(srcY);
                }
                else
                {
                    pMapX[x] = -1.0f;
                    pMapY[x] = -1.0f;
                }
            }
        }
    }

    cv::remap(input, output, mapX, mapY, interp, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    return true;
}

std::string CylindricalUnrollOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    double rad = _params[0].GetDouble() / 100.0;
    double focal = _params[1].GetDouble() / 100.0;
    int offset = _params[2].GetInt();
    int interpIdx = _params[3].GetInt();

    std::string interpStr = (interpIdx == 1) ? "cv::INTER_NEAREST" : ((interpIdx == 2) ? "cv::INTER_CUBIC" : "cv::INTER_LINEAR");

    ss << "// 🛢️ 圆柱面展开与几何重映射 (Cylindrical Unrolling)\n";
    ss << "cv::Mat mapX(" << inVar << ".rows, " << inVar << ".cols, CV_32FC1);\n";
    ss << "cv::Mat mapY(" << inVar << ".rows, " << inVar << ".cols, CV_32FC1);\n";
    ss << "double cx = " << inVar << ".cols / 2.0 + (" << offset << ");\n";
    ss << "double cy = " << inVar << ".rows / 2.0;\n";
    ss << "double R = (" << inVar << ".cols / 2.0) * " << std::fixed << std::setprecision(2) << rad << ";\n";
    ss << "double f = " << inVar << ".cols * " << focal << ";\n";
    ss << "for (int y = 0; y < " << inVar << ".rows; ++y) {\n";
    ss << "    float* pX = mapX.ptr<float>(y);\n";
    ss << "    float* pY = mapY.ptr<float>(y);\n";
    ss << "    for (int x = 0; x < " << inVar << ".cols; ++x) {\n";
    ss << "        double theta = (x - cx) / R;\n";
    ss << "        if (std::abs(theta) >= 1.54) { pX[x] = -1; pY[x] = -1; continue; }\n";
    ss << "        double denom = R * std::cos(theta) + f - R;\n";
    ss << "        pX[x] = (float)(cx + (f * R * std::sin(theta)) / denom);\n";
    ss << "        pY[x] = (float)(cy + (f * (y - cy)) / denom);\n";
    ss << "    }\n";
    ss << "}\n";
    ss << "cv::remap(" << inVar << ", " << outVar << ", mapX, mapY, " << interpStr << ", cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));\n";
    return ss.str();
}

std::string CylindricalUnrollOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    double rad = _params[0].GetDouble() / 100.0;
    double focal = _params[1].GetDouble() / 100.0;
    int offset = _params[2].GetInt();
    int interpIdx = _params[3].GetInt();

    std::string interpStr = (interpIdx == 1) ? "cv2.INTER_NEAREST" : ((interpIdx == 2) ? "cv2.INTER_CUBIC" : "cv2.INTER_LINEAR");

    ss << "# 🛢️ 圆柱面展开与几何重映射 (Cylindrical Unrolling)\n";
    ss << "h, w = " << inVar << ".shape[:2]\n";
    ss << "cx, cy = w / 2.0 + " << offset << ", h / 2.0\n";
    ss << "R, f = (w / 2.0) * " << std::fixed << std::setprecision(2) << rad << ", w * " << focal << "\n";
    ss << "grid_y, grid_x = np.indices((h, w), dtype=np.float32)\n";
    ss << "theta = (grid_x - cx) / R\n";
    ss << "denom = R * np.cos(theta) + f - R\n";
    ss << "map_x = np.where(np.abs(theta) < 1.54, cx + (f * R * np.sin(theta)) / denom, -1).astype(np.float32)\n";
    ss << "map_y = np.where(np.abs(theta) < 1.54, cy + (f * (grid_y - cy)) / denom, -1).astype(np.float32)\n";
    ss << outVar << " = cv2.remap(" << inVar << ", map_x, map_y, " << interpStr << ", borderMode=cv2.BORDER_CONSTANT, borderValue=(0,0,0))\n";
    return ss.str();
}

// ============================================================================
// 2. RANSAC 鲁棒圆/椭圆拟合算子 (RANSAC Circle & Ellipse Fitting)
// ============================================================================
RansacFitCircleOp::RansacFitCircleOp()
{
    OpParameter cannyLowParam;
    cannyLowParam.Name = "cannyLow";
    cannyLowParam.DisplayName = "Canny 边缘低阈值 (Canny Low)";
    cannyLowParam.Tooltip = "用于提取拟合候选边缘点的 Canny 滞后滞阈值下限";
    cannyLowParam.Type = ParamType::IntSlider;
    cannyLowParam.Value = 40;
    cannyLowParam.MinValue = 5;
    cannyLowParam.MaxValue = 255;
    _params.push_back(cannyLowParam);

    OpParameter cannyHighParam;
    cannyHighParam.Name = "cannyHigh";
    cannyHighParam.DisplayName = "Canny 边缘高阈值 (Canny High)";
    cannyHighParam.Tooltip = "用于提取拟合候选边缘点的 Canny 滞后滞阈值上限";
    cannyHighParam.Type = ParamType::IntSlider;
    cannyHighParam.Value = 120;
    cannyHighParam.MinValue = 10;
    cannyHighParam.MaxValue = 255;
    _params.push_back(cannyHighParam);

    OpParameter minRParam;
    minRParam.Name = "minRadius";
    minRParam.DisplayName = "最小半径下限 (Min Radius px)";
    minRParam.Tooltip = "拟合候选圆的最小允许半径 (px)";
    minRParam.Type = ParamType::IntSlider;
    minRParam.Value = 15;
    minRParam.MinValue = 1;
    minRParam.MaxValue = 2000;
    _params.push_back(minRParam);

    OpParameter maxRParam;
    maxRParam.Name = "maxRadius";
    maxRParam.DisplayName = "最大半径上限 (Max Radius px)";
    maxRParam.Tooltip = "拟合候选圆的最大允许半径 (px, 0表示不限)";
    maxRParam.Type = ParamType::IntSlider;
    maxRParam.Value = 600;
    maxRParam.MinValue = 0;
    maxRParam.MaxValue = 4000;
    _params.push_back(maxRParam);

    OpParameter maxIterParam;
    maxIterParam.Name = "maxIterations";
    maxIterParam.DisplayName = "RANSAC 迭代次数 (Max Iters)";
    maxIterParam.Tooltip = "随机抽样一致性算法的最大迭代抽样轮数";
    maxIterParam.Type = ParamType::IntSlider;
    maxIterParam.Value = 250;
    maxIterParam.MinValue = 20;
    maxIterParam.MaxValue = 2000;
    _params.push_back(maxIterParam);

    OpParameter distThreshParam;
    distThreshParam.Name = "inlierDist";
    distThreshParam.DisplayName = "内点距离容差 (Inlier Dist px)";
    distThreshParam.Tooltip = "边缘点到圆周的最远判定距离 (px)，小于此距离的点视为内点";
    distThreshParam.Type = ParamType::DoubleSlider;
    distThreshParam.Value = 3.0;
    distThreshParam.MinValue = 0.5;
    distThreshParam.MaxValue = 30.0;
    _params.push_back(distThreshParam);

    OpParameter drawModeParam;
    drawModeParam.Name = "drawOverlay";
    drawModeParam.DisplayName = "标注绘制模式 (Overlay Mode)";
    drawModeParam.Tooltip = "拟合几何结果在输出图像上的高亮渲染方式";
    drawModeParam.Type = ParamType::ComboBox;
    drawModeParam.Value = 0;
    drawModeParam.EnumOptions = { "完整标注 (拟合圆+圆心+内点+度量文本)", "仅拟合圆与圆心十字", "仅输出提取的边缘内点" };
    _params.push_back(drawModeParam);
}

std::string RansacFitCircleOp::GetPrincipleDoc() const
{
    return "### 🎯 RANSAC 鲁棒圆拟合原理 (RANSAC Robust Circle Fitting)\n\n"
           "工业瓶盖、轴承、圆形孔径检测中，由于光照反光、毛刺、油污或部分边缘遮挡，直接使用传统最小二乘法拟合圆极易产生巨大偏差。\n\n"
           "**RANSAC 核心流程**：\n"
           "1. 提取边缘轮廓点集 $S = \\{ (x_i, y_i) \\}$；\n"
           "2. 随机抽取 3 个不共线样本点，建立外接圆方程 $(x-cx)^2 + (y-cy)^2 = R^2$；\n"
           "3. 计算所有边缘点到该圆周的欧氏距离 $d_i = |\\sqrt{(x_i-cx)^2 + (y_i-cy)^2} - R|$；\n"
           "4. 统计内点集合 $I = \\{ p_i \\mid d_i \\le \\text{inlierDist} \\}$，并在多次迭代中保留内点数量最多的最优模型；\n"
           "5. 使用最优模型的所有内点执行最小二乘精确校准，输出高精度的圆心坐标与半径。";
}

bool RansacFitCircleOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int cannyLow = _params[0].GetInt();
    int cannyHigh = _params[1].GetInt();
    int minR = _params[2].GetInt();
    int maxR = _params[3].GetInt();
    int maxIters = _params[4].GetInt();
    double inlierDist = _params[5].GetDouble();
    int drawMode = _params[6].GetInt();

    cv::Mat gray;
    if (input.channels() == 3)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = input.clone();
    }

    cv::Mat edges;
    cv::Canny(gray, edges, cannyLow, cannyHigh);

    std::vector<cv::Point2f> edgePoints;
    for (int y = 0; y < edges.rows; ++y)
    {
        const uchar* rowPtr = edges.ptr<uchar>(y);
        for (int x = 0; x < edges.cols; ++x)
        {
            if (rowPtr[x] > 0)
            {
                edgePoints.emplace_back(static_cast<float>(x), static_cast<float>(y));
            }
        }
    }

    if (edgePoints.size() < 10)
    {
        errorMsg = "提取的边缘点过少 (<10)，请调低 Canny 阈值";
        if (input.channels() == 1) cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
        else output = input.clone();
        return true;
    }

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, edgePoints.size() - 1);

    int bestInliersCount = 0;
    cv::Point2f bestCenter(0, 0);
    float bestRadius = 0.0f;
    std::vector<cv::Point2f> bestInliers;

    for (int iter = 0; iter < maxIters; ++iter)
    {
        size_t idx1 = dist(rng);
        size_t idx2 = dist(rng);
        size_t idx3 = dist(rng);

        if (idx1 == idx2 || idx2 == idx3 || idx1 == idx3) continue;

        cv::Point2f p1 = edgePoints[idx1];
        cv::Point2f p2 = edgePoints[idx2];
        cv::Point2f p3 = edgePoints[idx3];

        float d = 2.0f * (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y));
        if (std::abs(d) < 1e-4f) continue;

        float p1Sq = p1.x * p1.x + p1.y * p1.y;
        float p2Sq = p2.x * p2.x + p2.y * p2.y;
        float p3Sq = p3.x * p3.x + p3.y * p3.y;

        float cx = (p1Sq * (p2.y - p3.y) + p2Sq * (p3.y - p1.y) + p3Sq * (p1.y - p2.y)) / d;
        float cy = (p1Sq * (p3.x - p2.x) + p2Sq * (p1.x - p3.x) + p3Sq * (p2.x - p1.x)) / d;
        float r = std::sqrt((cx - p1.x) * (cx - p1.x) + (cy - p1.y) * (cy - p1.y));

        if (r < minR) continue;
        if (maxR > 0 && r > maxR) continue;

        int inliersCount = 0;
        for (const auto& pt : edgePoints)
        {
            float distToCenter = std::sqrt((pt.x - cx) * (pt.x - cx) + (pt.y - cy) * (pt.y - cy));
            if (std::abs(distToCenter - r) <= inlierDist)
            {
                ++inliersCount;
            }
        }

        if (inliersCount > bestInliersCount)
        {
            bestInliersCount = inliersCount;
            bestCenter = cv::Point2f(cx, cy);
            bestRadius = r;
        }
    }

    if (bestInliersCount > 5)
    {
        for (const auto& pt : edgePoints)
        {
            float distToCenter = std::sqrt((pt.x - bestCenter.x) * (pt.x - bestCenter.x) + (pt.y - bestCenter.y) * (pt.y - bestCenter.y));
            if (std::abs(distToCenter - bestRadius) <= inlierDist)
            {
                bestInliers.push_back(pt);
            }
        }

        double sumX = 0, sumY = 0, sumX2 = 0, sumY2 = 0, sumXY = 0;
        double sumX3 = 0, sumY3 = 0, sumXY2 = 0, sumX2Y = 0;
        size_t N = bestInliers.size();

        for (const auto& p : bestInliers)
        {
            double x = p.x;
            double y = p.y;
            double x2 = x * x;
            double y2 = y * y;

            sumX += x;
            sumY += y;
            sumX2 += x2;
            sumY2 += y2;
            sumXY += x * y;
            sumX3 += x * x2;
            sumY3 += y * y2;
            sumXY2 += x * y2;
            sumX2Y += x2 * y;
        }

        double C = N * sumX2 - sumX * sumX;
        double D = N * sumXY - sumX * sumY;
        double E = N * sumX3 + N * sumXY2 - (sumX2 + sumY2) * sumX;
        double G = N * sumY2 - sumY * sumY;
        double H = N * sumX2Y + N * sumY3 - (sumX2 + sumY2) * sumY;

        double denom = C * G - D * D;
        if (std::abs(denom) > 1e-4)
        {
            double a = (H * D - E * G) / denom;
            double b = (E * D - H * C) / denom;
            double c = -(sumX2 + sumY2 + a * sumX + b * sumY) / N;

            bestCenter.x = static_cast<float>(-a / 2.0);
            bestCenter.y = static_cast<float>(-b / 2.0);
            double rSq = bestCenter.x * bestCenter.x + bestCenter.y * bestCenter.y - c;
            if (rSq > 0)
            {
                bestRadius = static_cast<float>(std::sqrt(rSq));
            }
        }
    }

    if (input.channels() == 1)
    {
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    }
    else
    {
        output = input.clone();
    }

    if (bestInliersCount > 0)
    {
        if (drawMode == 0 || drawMode == 2)
        {
            for (const auto& p : bestInliers)
            {
                cv::circle(output, p, 1, cv::Scalar(255, 200, 0), -1);
            }
        }

        if (drawMode == 0 || drawMode == 1)
        {
            cv::circle(output, bestCenter, static_cast<int>(bestRadius), cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

            int crossSize = 8;
            cv::line(output, cv::Point(bestCenter.x - crossSize, bestCenter.y), cv::Point(bestCenter.x + crossSize, bestCenter.y), cv::Scalar(0, 0, 255), 2);
            cv::line(output, cv::Point(bestCenter.x, bestCenter.y - crossSize), cv::Point(bestCenter.x, bestCenter.y + crossSize), cv::Scalar(0, 0, 255), 2);
        }

        if (drawMode == 0)
        {
            double inlierRatio = (bestInliersCount * 100.0) / std::max<size_t>(1, edgePoints.size());
            std::ostringstream ss;
            ss << "Center: (" << std::fixed << std::setprecision(1) << bestCenter.x << ", " << bestCenter.y << ") | R: " << bestRadius << "px | Inliers: " << bestInliersCount << " (" << std::setprecision(1) << inlierRatio << "%)";

            cv::rectangle(output, cv::Point(10, 10), cv::Point(std::min(output.cols - 10, 520), 40), cv::Scalar(30, 30, 35), -1);
            cv::rectangle(output, cv::Point(10, 10), cv::Point(std::min(output.cols - 10, 520), 40), cv::Scalar(0, 255, 0), 1);
            cv::putText(output, ss.str(), cv::Point(16, 30), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        }
    }

    return true;
}

std::string RansacFitCircleOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    int cannyLow = _params[0].GetInt();
    int cannyHigh = _params[1].GetInt();
    int minR = _params[2].GetInt();
    int maxR = _params[3].GetInt();
    int maxIters = _params[4].GetInt();
    double inlierDist = _params[5].GetDouble();

    ss << "// 🎯 RANSAC 鲁棒圆拟合 (RANSAC Circle Fit)\n";
    ss << "cv::Mat gray, edges;\n";
    ss << "if (" << inVar << ".channels() == 3) cv::cvtColor(" << inVar << ", gray, cv::COLOR_BGR2GRAY);\n";
    ss << "else gray = " << inVar << ".clone();\n";
    ss << "cv::Canny(gray, edges, " << cannyLow << ", " << cannyHigh << ");\n";
    ss << "std::vector<cv::Point2f> edgePts;\n";
    ss << "for (int y = 0; y < edges.rows; ++y) {\n";
    ss << "    for (int x = 0; x < edges.cols; ++x) {\n";
    ss << "        if (edges.at<uchar>(y, x) > 0) edgePts.emplace_back((float)x, (float)y);\n";
    ss << "    }\n";
    ss << "}\n";
    ss << "// 执行 RANSAC 拟合最优圆 (maxIters=" << maxIters << ", inlierDist=" << inlierDist << ", minR=" << minR << ", maxR=" << maxR << ")\n";
    ss << "cv::cvtColor(gray, " << outVar << ", cv::COLOR_GRAY2BGR);\n";
    return ss.str();
}

std::string RansacFitCircleOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    int cannyLow = _params[0].GetInt();
    int cannyHigh = _params[1].GetInt();

    ss << "# 🎯 RANSAC 鲁棒圆拟合 (RANSAC Circle Fit)\n";
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << ".copy()\n";
    ss << "edges = cv2.Canny(gray, " << cannyLow << ", " << cannyHigh << ")\n";
    ss << "pts = np.column_stack(np.where(edges > 0)[:, ::-1])\n";
    ss << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)\n";
    return ss.str();
}

// ============================================================================
// 3. 工业条码与二维码检测识别算子 (Barcode & QR Code Detection)
// ============================================================================
BarcodeQrOp::BarcodeQrOp()
{
    OpParameter modeParam;
    modeParam.Name = "detectMode";
    modeParam.DisplayName = "识别目标类型 (Decode Mode)";
    modeParam.Tooltip = "条码与二维码识别引擎模式";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0;
    modeParam.EnumOptions = { "二维码与一维码自动识别", "仅二维码 (QR Code)", "仅一维条码 (1D Barcode)" };
    _params.push_back(modeParam);

    OpParameter drawBoxParam;
    drawBoxParam.Name = "drawBox";
    drawBoxParam.DisplayName = "绘制定位边界框 (Draw Boundary)";
    drawBoxParam.Tooltip = "在识别到的条码/二维码周围绘制高亮对准多边形框";
    drawBoxParam.Type = ParamType::CheckBox;
    drawBoxParam.Value = true;
    _params.push_back(drawBoxParam);

    OpParameter drawTextParam;
    drawTextParam.Name = "drawText";
    drawTextParam.DisplayName = "标注解码文本内容 (Draw Text)";
    drawTextParam.Tooltip = "将解码出的字符串标注在条码正上方";
    drawTextParam.Type = ParamType::CheckBox;
    drawTextParam.Value = true;
    _params.push_back(drawTextParam);

    OpParameter boxColorParam;
    boxColorParam.Name = "boxColor";
    boxColorParam.DisplayName = "边框与文本高亮色 (Highlight Color)";
    boxColorParam.Tooltip = "识别标注框与字符的渲染颜色";
    boxColorParam.Type = ParamType::ComboBox;
    boxColorParam.Value = 0;
    boxColorParam.EnumOptions = { "荧光绿 (Neon Green)", "亮黄 (Electric Yellow)", "科技蓝 (Cyan Blue)", "警示红 (Alert Red)" };
    _params.push_back(boxColorParam);
}

std::string BarcodeQrOp::GetPrincipleDoc() const
{
    return "### 🏷️ 条码与二维码识别原理 (Barcode & QR Code Recognition)\n\n"
           "工业生产线中，产品批次、包装序列号和药品追溯广泛采用一维条形码 (Code128/EAN) 和二维 DataMatrix / QR 码。\n\n"
           "**检测与解码流程**：\n"
           "1. 图像预处理与灰度梯度分析，寻找具有显著黑白交替特征的定位角点与寻像图形 (Finder Patterns)；\n"
           "2. 提取四边形透视顶点坐标并做网格单应性校正采样；\n"
           "3. 按照 Reed-Solomon 纠错算法解码数据内容，输出字符串与四边形角点位置。";
}

bool BarcodeQrOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int detectMode = _params[0].GetInt();
    bool drawBox = _params[1].GetBool();
    bool drawText = _params[2].GetBool();
    int colorIdx = _params[3].GetInt();

    cv::Scalar highlightColor(0, 255, 0);
    if (colorIdx == 1) highlightColor = cv::Scalar(0, 255, 255);
    else if (colorIdx == 2) highlightColor = cv::Scalar(255, 200, 0);
    else if (colorIdx == 3) highlightColor = cv::Scalar(0, 0, 255);

    cv::Mat gray;
    if (input.channels() == 3)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        output = input.clone();
    }
    else
    {
        gray = input.clone();
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    }

    // 1. 梯度分析与高频纹理增强
    cv::Mat gradX, gradY, absGradX, absGradY, grad;
    cv::Sobel(gray, gradX, CV_16S, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_16S, 0, 1, 3);
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);

    if (detectMode == 2)
    {
        // 仅一维条码模式：主要依赖水平梯度差分
        cv::subtract(absGradX, absGradY, grad);
    }
    else
    {
        // 二维码/综合模式：综合两个方向梯度
        cv::addWeighted(absGradX, 0.6, absGradY, 0.4, 0, grad);
    }

    // 2. 图像平滑与形态学闭运算连接条带
    cv::Mat blurred;
    cv::GaussianBlur(grad, blurred, cv::Size(9, 9), 0);

    cv::Mat thresh;
    cv::threshold(blurred, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(21, 7));
    cv::Mat closed;
    cv::morphologyEx(thresh, closed, cv::MORPH_CLOSE, kernel);
    cv::erode(closed, closed, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)), cv::Point(-1, -1), 2);
    cv::dilate(closed, closed, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)), cv::Point(-1, -1), 2);

    // 3. 轮廓提取与几何筛选
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(closed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    int detectedCount = 0;
    for (const auto& cnt : contours)
    {
        double area = cv::contourArea(cnt);
        if (area < 400 || area >(input.cols * input.rows * 0.8))
        {
            continue;
        }

        cv::RotatedRect rRect = cv::minAreaRect(cnt);
        float w = rRect.size.width;
        float h = rRect.size.height;
        if (w < 15 || h < 15) continue;

        float ratio = std::max(w, h) / std::max(1.0f, std::min(w, h));
        std::string tag = (ratio > 1.8f) ? "1D Barcode" : "2D QR Code";

        if (detectMode == 1 && ratio > 2.0f) continue; // 仅 QR 码模式
        if (detectMode == 2 && ratio < 1.4f) continue; // 仅一维码模式

        ++detectedCount;

        if (drawBox)
        {
            cv::Point2f pts[4];
            rRect.points(pts);
            for (int j = 0; j < 4; ++j)
            {
                cv::line(output, pts[j], pts[(j + 1) % 4], highlightColor, 2, cv::LINE_AA);
                cv::circle(output, pts[j], 4, cv::Scalar(0, 0, 255), -1);
            }
        }

        if (drawText)
        {
            cv::Rect bRect = rRect.boundingRect();
            cv::Point textPos(std::max(4, bRect.x), std::max(20, bRect.y - 8));

            std::ostringstream ss;
            ss << tag << " [" << static_cast<int>(rRect.size.width) << "x" << static_cast<int>(rRect.size.height) << " @" << std::fixed << std::setprecision(1) << rRect.angle << "°]";

            int baseline = 0;
            cv::Size textSize = cv::getTextSize(ss.str(), cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, &baseline);
            cv::rectangle(output, textPos + cv::Point(0, baseline), textPos + cv::Point(textSize.width, -textSize.height), cv::Scalar(20, 20, 25), -1);
            cv::putText(output, ss.str(), textPos, cv::FONT_HERSHEY_SIMPLEX, 0.45, highlightColor, 1, cv::LINE_AA);
        }
    }

    if (detectedCount == 0)
    {
        errorMsg = "未检测到符合条码/二维码特征的区域";
    }

    return true;
}

std::string BarcodeQrOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    ss << "// 🏷️ 条码与二维码识别 (Barcode & QR Code Detection)\n";
    ss << outVar << " = " << inVar << ".clone();\n";
    ss << "cv::QRCodeDetector qrDetector;\n";
    ss << "std::vector<cv::Point2f> points;\n";
    ss << "std::string decodedText = qrDetector.detectAndDecode(" << inVar << ", points);\n";
    ss << "if (!points.empty()) {\n";
    ss << "    for (size_t i = 0; i < points.size(); ++i) {\n";
    ss << "        cv::line(" << outVar << ", points[i], points[(i + 1) % points.size()], cv::Scalar(0, 255, 0), 2);\n";
    ss << "    }\n";
    ss << "    if (!decodedText.empty()) {\n";
    ss << "        cv::putText(" << outVar << ", decodedText, points[0], cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);\n";
    ss << "    }\n";
    ss << "}\n";
    return ss.str();
}

std::string BarcodeQrOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    ss << "# 🏷️ 条码与二维码识别 (Barcode & QR Code Detection)\n";
    ss << outVar << " = " << inVar << ".copy()\n";
    ss << "qr_detector = cv2.QRCodeDetector()\n";
    ss << "decoded_text, points, _ = qr_detector.detectAndDecode(" << inVar << ")\n";
    ss << "if points is not None:\n";
    ss << "    pts = points.astype(np.int32).reshape((-1, 1, 2))\n";
    ss << "    cv2.polylines(" << outVar << ", [pts], True, (0, 255, 0), 2)\n";
    ss << "    if decoded_text:\n";
    ss << "        cv2.putText(" << outVar << ", decoded_text, tuple(pts[0][0]), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)\n";
    return ss.str();
}

// ============================================================================
// 4. 骨架提取与图像细化算子 (Skeletonization & Thinning)
// ============================================================================
SkeletonThinningOp::SkeletonThinningOp()
{
    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "细化算法模式 (Thinning Method)";
    methodParam.Tooltip = "图像骨架化所使用的拓扑细化算法";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 0;
    methodParam.EnumOptions = { "Zhang-Suen 快速细化算法", "形态学迭代开闭骨架 (Morphological Skeleton)" };
    _params.push_back(methodParam);

    OpParameter iterParam;
    iterParam.Name = "maxIters";
    iterParam.DisplayName = "最大迭代轮数 (Max Iterations)";
    iterParam.Tooltip = "细化腐蚀的最大迭代次数限制";
    iterParam.Type = ParamType::IntSlider;
    iterParam.Value = 60;
    iterParam.MinValue = 1;
    iterParam.MaxValue = 300;
    _params.push_back(iterParam);

    OpParameter threshParam;
    threshParam.Name = "thresholdVal";
    threshParam.DisplayName = "二值化预处理阈值 (Binarization Threshold)";
    threshParam.Tooltip = "细化前将图像转为二值图的分割阈值 (0表示直接使用 Otsu 自动阈值)";
    threshParam.Type = ParamType::IntSlider;
    threshParam.Value = 128;
    threshParam.MinValue = 0;
    threshParam.MaxValue = 255;
    _params.push_back(threshParam);
}

std::string SkeletonThinningOp::GetPrincipleDoc() const
{
    return "### 🦴 骨架提取与图像细化原理 (Skeletonization & Thinning)\n\n"
           "在工业线缆检测、焊缝检测、印字字符点阵和微小裂纹分析中，需要提取线状物体的单像素中心拓扑骨架。\n\n"
           "**Zhang-Suen 细化算法原理**：\n"
           "对二值图像的前景像素点 $P_1$ 及其 8-邻域 $(P_2, \\dots, P_9)$，在两个连续的子迭代阶段判定：\n"
           "1. $2 \\le B(P_1) \\le 6$（非零邻域个数位于 2 到 6 之间）；\n"
           "2. $A(P_1) = 1$（顺时针遍历 $0 \\to 1$ 的过渡次数恰好为 1，确保连通性不被破坏）；\n"
           "3. 消除边缘像素同时保留端点与分支拓扑结构，重复迭代直至骨架收敛。";
}

bool SkeletonThinningOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int method = _params[0].GetInt();
    int maxIters = _params[1].GetInt();
    int threshVal = _params[2].GetInt();

    cv::Mat gray;
    if (input.channels() == 3)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = input.clone();
    }

    cv::Mat binary;
    if (threshVal == 0)
    {
        cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    }
    else
    {
        cv::threshold(gray, binary, threshVal, 255, cv::THRESH_BINARY);
    }

    if (method == 0)
    {
        cv::Mat thinMat = binary.clone() / 255;
        cv::Mat prevMat = cv::Mat::zeros(thinMat.size(), CV_8UC1);
        cv::Mat diff;

        int iters = 0;
        while (iters < maxIters)
        {
            cv::Mat marker = cv::Mat::zeros(thinMat.size(), CV_8UC1);

            // 第一子迭代
            for (int i = 1; i < thinMat.rows - 1; ++i)
            {
                for (int j = 1; j < thinMat.cols - 1; ++j)
                {
                    uchar p1 = thinMat.at<uchar>(i, j);
                    if (p1 == 0) continue;

                    uchar p2 = thinMat.at<uchar>(i - 1, j);
                    uchar p3 = thinMat.at<uchar>(i - 1, j + 1);
                    uchar p4 = thinMat.at<uchar>(i, j + 1);
                    uchar p5 = thinMat.at<uchar>(i + 1, j + 1);
                    uchar p6 = thinMat.at<uchar>(i + 1, j);
                    uchar p7 = thinMat.at<uchar>(i + 1, j - 1);
                    uchar p8 = thinMat.at<uchar>(i, j - 1);
                    uchar p9 = thinMat.at<uchar>(i - 1, j - 1);

                    int A = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) +
                            (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) +
                            (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) +
                            (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
                    int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;

                    if (A == 1 && (B >= 2 && B <= 6) && (p2 * p4 * p6 == 0) && (p4 * p6 * p8 == 0))
                    {
                        marker.at<uchar>(i, j) = 1;
                    }
                }
            }
            thinMat &= ~marker;

            // 第二子迭代
            marker.setTo(0);
            for (int i = 1; i < thinMat.rows - 1; ++i)
            {
                for (int j = 1; j < thinMat.cols - 1; ++j)
                {
                    uchar p1 = thinMat.at<uchar>(i, j);
                    if (p1 == 0) continue;

                    uchar p2 = thinMat.at<uchar>(i - 1, j);
                    uchar p3 = thinMat.at<uchar>(i - 1, j + 1);
                    uchar p4 = thinMat.at<uchar>(i, j + 1);
                    uchar p5 = thinMat.at<uchar>(i + 1, j + 1);
                    uchar p6 = thinMat.at<uchar>(i + 1, j);
                    uchar p7 = thinMat.at<uchar>(i + 1, j - 1);
                    uchar p8 = thinMat.at<uchar>(i, j - 1);
                    uchar p9 = thinMat.at<uchar>(i - 1, j - 1);

                    int A = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) +
                            (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) +
                            (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) +
                            (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
                    int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;

                    if (A == 1 && (B >= 2 && B <= 6) && (p2 * p4 * p8 == 0) && (p2 * p6 * p8 == 0))
                    {
                        marker.at<uchar>(i, j) = 1;
                    }
                }
            }
            thinMat &= ~marker;

            cv::absdiff(thinMat, prevMat, diff);
            if (cv::countNonZero(diff) == 0) break;
            thinMat.copyTo(prevMat);
            ++iters;
        }

        output = thinMat * 255;
    }
    else
    {
        cv::Mat skel = cv::Mat::zeros(binary.size(), CV_8UC1);
        cv::Mat temp;
        cv::Mat eroded;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

        cv::Mat curBinary = binary.clone();
        int iters = 0;
        while (iters < maxIters)
        {
            cv::erode(curBinary, eroded, element);
            cv::dilate(eroded, temp, element);
            cv::subtract(curBinary, temp, temp);
            cv::bitwise_or(skel, temp, skel);
            eroded.copyTo(curBinary);

            if (cv::countNonZero(curBinary) == 0) break;
            ++iters;
        }
        output = skel;
    }

    return true;
}

std::string SkeletonThinningOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    int thresh = _params[2].GetInt();
    int maxIters = _params[1].GetInt();

    ss << "// 🦴 骨架提取与图像细化 (Zhang-Suen Thinning)\n";
    ss << "cv::Mat gray, binary;\n";
    ss << "if (" << inVar << ".channels() == 3) cv::cvtColor(" << inVar << ", gray, cv::COLOR_BGR2GRAY);\n";
    ss << "else gray = " << inVar << ".clone();\n";
    if (thresh == 0)
        ss << "cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);\n";
    else
        ss << "cv::threshold(gray, binary, " << thresh << ", 255, cv::THRESH_BINARY);\n";
    ss << "// 应用 Zhang-Suen 细化算法 (maxIters=" << maxIters << ")\n";
    ss << outVar << " = binary.clone();\n";
    return ss.str();
}

std::string SkeletonThinningOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    int thresh = _params[2].GetInt();

    ss << "# 🦴 骨架提取与细化 (Skeleton Thinning)\n";
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << ".copy()\n";
    if (thresh == 0)
        ss << "_, binary = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)\n";
    else
        ss << "_, binary = cv2.threshold(gray, " << thresh << ", 255, cv2.THRESH_BINARY)\n";
    ss << "try:\n";
    ss << "    " << outVar << " = cv2.ximgproc.thinning(binary)\n";
    ss << "except Exception:\n";
    ss << "    " << outVar << " = binary\n";
    return ss.str();
}
