#include "IndustrialOps.h"
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

#if defined(VISIONSTUDIO_HAS_OPENCV_OBJDETECT)
#include <opencv2/objdetect.hpp>
#if __has_include(<opencv2/objdetect/barcode.hpp>)
#include <opencv2/objdetect/barcode.hpp>
#define VISIONSTUDIO_HAS_OPENCV_BARCODE 1
#endif
#endif

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
           "**【核心思想】**\n"
           "贴在瓶身、易拉罐、针管等圆柱体表面的标签与字符喷码，在正交拍摄时两侧边缘会因圆柱曲率产生严重的非线性压缩变形。通过几何反演重映射建立展开后平面与原曲面坐标的对应关系。\n\n"
           "**【数学几何反演模型】**\n"
           "```\n"
           "   1. 圆柱半径与虚拟焦距:  R = (W / 2) · radiusRatio,   f = W · focalRatio\n"
           "\n"
           "   2. 展开平面点 (x', y') 对应圆柱包角:  θ = (x' - cx) / R\n"
           "\n"
           "   3. 曲面反向寻址投影公式:\n"
           "        map_x(x', y') = cx + f · tan(θ)  = cx + f · [ sin(θ) / cos(θ) ]\n"
           "        map_y(x', y') = cy + (y' - cy) / cos(θ)\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **radiusRatio**：圆柱有效半径占图像半宽的百分比；\n"
           "- **focalRatio**：虚拟相机等效焦距系数；\n"
           "- **centerXOffset**：圆柱轴心在视野中的左右像素偏移。";
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

    try
    {
        const int w = input.cols;
        const int h = input.rows;
        const double cx = (w / 2.0) + centerXOffset;
        const double cy = h / 2.0;

        const double radius = (w / 2.0) * radiusRatio;
        const double focal = w * focalRatio;

        if (radius <= 1.0 || focal <= 1.0)
        {
            errorMsg = "圆柱展开半径或焦距参数无效";
            return false;
        }

        cv::Mat mapX(h, w, CV_32FC1);
        cv::Mat mapY(h, w, CV_32FC1);

        for (int y = 0; y < h; ++y)
        {
            float* pX = mapX.ptr<float>(y);
            float* pY = mapY.ptr<float>(y);
            for (int x = 0; x < w; ++x)
            {
                double dx = x - cx;
                double theta = dx / radius;

                if (std::abs(theta) < (CV_PI / 2.0 - 0.05))
                {
                    double srcX = cx + focal * std::tan(theta);
                    double srcY = cy + (y - cy) / std::cos(theta);

                    pX[x] = static_cast<float>(srcX);
                    pY[x] = static_cast<float>(srcY);
                }
                else
                {
                    pX[x] = -1.0f;
                    pY[x] = -1.0f;
                }
            }
        }

        cv::remap(input, output, mapX, mapY, interp, cv::BORDER_CONSTANT, cv::Scalar::all(0));
        return true;
    }
    catch (const cv::Exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

std::string CylindricalUnrollOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    double radiusRatio = _params[0].GetDouble() / 100.0;
    double focalRatio = _params[1].GetDouble() / 100.0;
    int centerXOffset = _params[2].GetInt();
    int interpIdx = _params[3].GetInt();

    std::stringstream ss;
    ss << "// 圆柱曲面展开参数配置\n"
       << "double radiusRatio = " << radiusRatio << ";\n"
       << "double focalRatio = " << focalRatio << ";\n"
       << "int centerXOffset = " << centerXOffset << ";\n"
       << "int interp = " << (interpIdx == 1 ? "cv::INTER_NEAREST" : (interpIdx == 2 ? "cv::INTER_CUBIC" : "cv::INTER_LINEAR")) << ";\n"
       << "int w = " << inVar << ".cols, h = " << inVar << ".rows;\n"
       << "double cx = (w / 2.0) + centerXOffset, cy = h / 2.0;\n"
       << "double radius = (w / 2.0) * radiusRatio, focal = w * focalRatio;\n"
       << "cv::Mat mapX(h, w, CV_32FC1), mapY(h, w, CV_32FC1);\n"
       << "for (int y = 0; y < h; ++y) {\n"
       << "    for (int x = 0; x < w; ++x) {\n"
       << "        double theta = (x - cx) / radius;\n"
       << "        if (std::abs(theta) < (CV_PI / 2.0 - 0.05)) {\n"
       << "            mapX.at<float>(y, x) = static_cast<float>(cx + focal * std::tan(theta));\n"
       << "            mapY.at<float>(y, x) = static_cast<float>(cy + (y - cy) / std::cos(theta));\n"
       << "        } else { mapX.at<float>(y, x) = -1.0f; mapY.at<float>(y, x) = -1.0f; }\n"
       << "    }\n"
       << "}\n"
       << "cv::remap(" << inVar << ", " << outVar << ", mapX, mapY, interp, cv::BORDER_CONSTANT, cv::Scalar::all(0));";
    return ss.str();
}

std::string CylindricalUnrollOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    double radiusRatio = _params[0].GetDouble() / 100.0;
    double focalRatio = _params[1].GetDouble() / 100.0;
    int centerXOffset = _params[2].GetInt();
    int interpIdx = _params[3].GetInt();

    std::stringstream ss;
    ss << "# 圆柱曲面展开参数配置\n"
       << "radius_ratio = " << radiusRatio << "\n"
       << "focal_ratio = " << focalRatio << "\n"
       << "center_x_offset = " << centerXOffset << "\n"
       << "interp = " << (interpIdx == 1 ? "cv2.INTER_NEAREST" : (interpIdx == 2 ? "cv2.INTER_CUBIC" : "cv2.INTER_LINEAR")) << "\n"
       << "h, w = " << inVar << ".shape[:2]\n"
       << "cx, cy = (w / 2.0) + center_x_offset, h / 2.0\n"
       << "radius, focal = (w / 2.0) * radius_ratio, w * focal_ratio\n"
       << "grid_x, grid_y = np.meshgrid(np.arange(w), np.arange(h))\n"
       << "theta = (grid_x - cx) / radius\n"
       << "valid_mask = np.abs(theta) < (np.pi / 2.0 - 0.05)\n"
       << "map_x = np.where(valid_mask, (cx + focal * np.tan(theta)).astype(np.float32), -1.0)\n"
       << "map_y = np.where(valid_mask, (cy + (grid_y - cy) / np.cos(theta)).astype(np.float32), -1.0)\n"
       << outVar << " = cv2.remap(" << inVar << ", map_x, map_y, interp, borderMode=cv2.BORDER_CONSTANT, borderValue=(0, 0, 0))";
    return ss.str();
}

// ============================================================================
// 2. RANSAC 鲁棒圆/椭圆拟合算子 (RANSAC Circle & Ellipse Fitting)
// ============================================================================
RansacFitCircleOp::RansacFitCircleOp()
{
    OpParameter cannyLowParam;
    cannyLowParam.Name = "cannyLow";
    cannyLowParam.DisplayName = "边缘提取低阈值 (Canny Low)";
    cannyLowParam.Type = ParamType::IntSlider;
    cannyLowParam.Value = 50;
    cannyLowParam.MinValue = 5;
    cannyLowParam.MaxValue = 255;
    cannyLowParam.Step = 5;
    _params.push_back(cannyLowParam);

    OpParameter cannyHighParam;
    cannyHighParam.Name = "cannyHigh";
    cannyHighParam.DisplayName = "边缘提取高阈值 (Canny High)";
    cannyHighParam.Type = ParamType::IntSlider;
    cannyHighParam.Value = 150;
    cannyHighParam.MinValue = 10;
    cannyHighParam.MaxValue = 300;
    cannyHighParam.Step = 5;
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
    maxIterParam.DisplayName = "RANSAC 迭代次数 (Iterations)";
    maxIterParam.Type = ParamType::IntSlider;
    maxIterParam.Value = 300;
    maxIterParam.MinValue = 50;
    maxIterParam.MaxValue = 2000;
    maxIterParam.Step = 50;
    _params.push_back(maxIterParam);

    OpParameter inlierDistParam;
    inlierDistParam.Name = "inlierDist";
    inlierDistParam.DisplayName = "内点容差阈值 (Inlier Distance px)";
    inlierDistParam.Type = ParamType::DoubleSlider;
    inlierDistParam.Value = 2.0;
    inlierDistParam.MinValue = 0.5;
    inlierDistParam.MaxValue = 10.0;
    inlierDistParam.Step = 0.5;
    _params.push_back(inlierDistParam);

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
    return "### 🎯 RANSAC 鲁棒圆拟合原理 (RANSAC Circle Fitting)\n\n"
           "**【核心思想】**\n"
           "在工件反光、毛刺、油污或边缘局部遮挡时，传统最小二乘法极易被离群噪点拉偏。RANSAC（随机抽样一致性）通过随机三点建立候选圆模型，以最大内点投票机制锁定真实工件圆周，最后对所有内点执行高精度最小二乘精修。\n\n"
           "**【数学模型 / 迭代流程】**\n"
           "```\n"
           "   1. 随机抽样: 随机抽取 3 个不共线边缘点 p₁, p₂, p₃ 求解三点共圆 (cx, cy, R)\n"
           "   2. 距离度量: 点到圆周欧氏径向残差 d_i = | √[(x_i - cx)² + (y_i - cy)²] - R |\n"
           "   3. 内点投票: 统计 { p_i | d_i ≤ inlierDist }，保留内点最多的模型\n"
           "   4. 最小二乘精修: 对全部有效内点求解超定方程矩阵，获得亚像素圆心与半径\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **inlierDist**：内点判定容差门限（px）；\n"
           "- **maxIterations**：RANSAC 随机采样迭代次数。";
}

bool RansacFitCircleOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    int cannyLow = _params[0].GetInt();
    int cannyHigh = _params[1].GetInt();
    if (cannyLow > cannyHigh)
    {
        std::swap(cannyLow, cannyHigh);
    }
    int minR = _params[2].GetInt();
    int maxR = _params[3].GetInt();
    int maxIters = _params[4].GetInt();
    double inlierDist = _params[5].GetDouble();
    int drawMode = _params[6].GetInt();

    cv::Mat gray;
    if (input.channels() == 4)
    {
        cv::cvtColor(input, gray, cv::COLOR_BGRA2GRAY);
    }
    else if (input.channels() == 3)
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
        else if (input.channels() == 4) cv::cvtColor(input, output, cv::COLOR_BGRA2BGR);
        else output = input.clone();
        AddResult("circle.found", "是否找到圆", false);
        AddResult("circle.edgePointCount", "边缘点数量", static_cast<std::int64_t>(edgePoints.size()));
        errorMsg.clear();
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

    const int minimumInliers = std::max(6, static_cast<int>(std::ceil(edgePoints.size() * 0.02)));
    const bool circleFound = bestInliersCount >= minimumInliers;
    if (circleFound)
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
    else if (input.channels() == 4)
    {
        cv::cvtColor(input, output, cv::COLOR_BGRA2BGR);
    }
    else
    {
        output = input.clone();
    }

    if (circleFound)
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

    AddResult("circle.found", "是否找到圆", circleFound);
    AddResult("circle.edgePointCount", "边缘点数量", static_cast<std::int64_t>(edgePoints.size()));
    AddResult("circle.inlierCount", "内点数量", static_cast<std::int64_t>(bestInliersCount));
    AddResult("circle.minimumInliers", "最小一致内点数", static_cast<std::int64_t>(minimumInliers));
    if (circleFound)
    {
        AddResult("circle.centerX", "圆心 X", static_cast<double>(bestCenter.x), "px");
        AddResult("circle.centerY", "圆心 Y", static_cast<double>(bestCenter.y), "px");
        AddResult("circle.radius", "半径", static_cast<double>(bestRadius), "px");
    }

    errorMsg.clear();
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
    int drawMode = _params[6].GetInt();

    if (cannyLow > cannyHigh)
    {
        std::swap(cannyLow, cannyHigh);
    }

    ss << "// RANSAC 鲁棒圆拟合\n"
       << "cv::Mat ransacGray, ransacEdges;\n"
       << "if (" << inVar << ".channels() == 1) ransacGray = " << inVar << "; else cv::cvtColor(" << inVar << ", ransacGray, cv::COLOR_BGR2GRAY);\n"
       << "cv::Canny(ransacGray, ransacEdges, " << cannyLow << ", " << cannyHigh << ");\n"
       << "std::vector<cv::Point2f> edgePts;\n"
       << "for (int y = 0; y < ransacEdges.rows; ++y) for (int x = 0; x < ransacEdges.cols; ++x) if (ransacEdges.at<uchar>(y, x)) edgePts.emplace_back((float)x, (float)y);\n"
       << "std::mt19937 ransacRng(42); cv::Point2f bestCenter; float bestRadius = 0; int bestInlierCount = 0; std::vector<cv::Point2f> bestInliers;\n"
       << "if (edgePts.size() >= 10)\n"
       << "{\n"
       << "    std::uniform_int_distribution<size_t> pickPoint(0, edgePts.size() - 1);\n"
       << "    for (int iteration = 0; iteration < " << maxIters << "; ++iteration)\n"
       << "    {\n"
       << "        size_t i1 = pickPoint(ransacRng), i2 = pickPoint(ransacRng), i3 = pickPoint(ransacRng); if (i1 == i2 || i2 == i3 || i1 == i3) continue;\n"
       << "        const cv::Point2f& p1 = edgePts[i1]; const cv::Point2f& p2 = edgePts[i2]; const cv::Point2f& p3 = edgePts[i3];\n"
       << "        float d = 2.0f * (p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y)); if (std::abs(d) < 1e-4f) continue;\n"
       << "        float q1 = p1.dot(p1), q2 = p2.dot(p2), q3 = p3.dot(p3);\n"
       << "        cv::Point2f center((q1 * (p2.y-p3.y) + q2 * (p3.y-p1.y) + q3 * (p1.y-p2.y)) / d, (q1 * (p3.x-p2.x) + q2 * (p1.x-p3.x) + q3 * (p2.x-p1.x)) / d);\n"
       << "        float radius = cv::norm(center - p1); if (radius < " << minR << " || (" << maxR << " > 0 && radius > " << maxR << ")) continue;\n"
       << "        int inlierCount = 0; for (const auto& point : edgePts) if (std::abs(cv::norm(point - center) - radius) <= " << inlierDist << ") ++inlierCount;\n"
       << "        if (inlierCount > bestInlierCount) { bestInlierCount = inlierCount; bestCenter = center; bestRadius = radius; }\n"
       << "    }\n"
       << "    int minimumInliers = std::max(6, (int)std::ceil(edgePts.size() * 0.02));\n"
       << "    if (bestInlierCount >= minimumInliers) for (const auto& point : edgePts) if (std::abs(cv::norm(point - bestCenter) - bestRadius) <= " << inlierDist << ") bestInliers.push_back(point);\n"
       << "}\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR); else " << outVar << " = " << inVar << ".clone();\n";
    if (drawMode == 0 || drawMode == 2)
    {
        ss << "for (const auto& point : bestInliers) cv::circle(" << outVar << ", point, 1, cv::Scalar(255,200,0), -1);\n";
    }
    if (drawMode == 0 || drawMode == 1)
    {
        ss << "if (!bestInliers.empty()) { cv::circle(" << outVar << ", bestCenter, (int)bestRadius, cv::Scalar(0,255,0), 2, cv::LINE_AA); cv::drawMarker(" << outVar << ", bestCenter, cv::Scalar(0,0,255), cv::MARKER_CROSS, 16, 2); }\n";
    }
    return ss.str();
}

std::string RansacFitCircleOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    std::ostringstream ss;
    int cannyLow = _params[0].GetInt();
    int cannyHigh = _params[1].GetInt();
    int minR = _params[2].GetInt();
    int maxR = _params[3].GetInt();
    int maxIters = _params[4].GetInt();
    double inlierDist = _params[5].GetDouble();
    int drawMode = _params[6].GetInt();

    if (cannyLow > cannyHigh)
    {
        std::swap(cannyLow, cannyHigh);
    }

    ss << "# RANSAC 鲁棒圆拟合\n"
       << "ransac_gray = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "ransac_edges = cv2.Canny(ransac_gray, " << cannyLow << ", " << cannyHigh << ")\n"
       << "edge_points = np.column_stack(np.where(ransac_edges > 0)[:, ::-1]).astype(np.float32)\n"
       << "best_center, best_radius, best_inlier_count = None, 0.0, 0\n"
       << "best_inliers = np.empty((0, 2), dtype=np.float32)\n"
       << "if len(edge_points) >= 10:\n"
       << "    ransac_rng = np.random.default_rng(42)\n"
       << "    for _ in range(" << maxIters << "):\n"
       << "        p1, p2, p3 = edge_points[ransac_rng.choice(len(edge_points), 3, replace=False)]\n"
       << "        d = 2.0 * (p1[0]*(p2[1]-p3[1]) + p2[0]*(p3[1]-p1[1]) + p3[0]*(p1[1]-p2[1]))\n"
       << "        if abs(d) < 1e-4: continue\n"
       << "        q1, q2, q3 = np.dot(p1,p1), np.dot(p2,p2), np.dot(p3,p3)\n"
       << "        center = np.array([(q1*(p2[1]-p3[1]) + q2*(p3[1]-p1[1]) + q3*(p1[1]-p2[1]))/d, (q1*(p3[0]-p2[0]) + q2*(p1[0]-p3[0]) + q3*(p2[0]-p1[0]))/d])\n"
       << "        radius = np.linalg.norm(center-p1)\n"
       << "        if radius < " << minR << " or (" << maxR << " > 0 and radius > " << maxR << "): continue\n"
       << "        distances = np.abs(np.linalg.norm(edge_points-center, axis=1)-radius)\n"
       << "        inliers = edge_points[distances <= " << inlierDist << "]\n"
       << "        if len(inliers) > best_inlier_count: best_center, best_radius, best_inlier_count = center, radius, len(inliers)\n"
       << "    minimum_inliers = max(6, int(np.ceil(len(edge_points) * 0.02)))\n"
       << "    if best_inlier_count >= minimum_inliers:\n"
       << "        distances = np.abs(np.linalg.norm(edge_points-best_center, axis=1)-best_radius)\n"
       << "        best_inliers = edge_points[distances <= " << inlierDist << "]\n"
       << "    else:\n"
       << "        best_center = None\n"
       << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n";
    if (drawMode == 0 || drawMode == 2)
    {
        ss << "for point in best_inliers.astype(np.int32): cv2.circle(" << outVar << ", tuple(point), 1, (255,200,0), -1)\n";
    }
    if (drawMode == 0 || drawMode == 1)
    {
        ss << "if best_center is not None:\n"
           << "    cv2.circle(" << outVar << ", tuple(best_center.astype(np.int32)), int(best_radius), (0,255,0), 2, cv2.LINE_AA)\n"
           << "    cv2.drawMarker(" << outVar << ", tuple(best_center.astype(np.int32)), (0,0,255), cv2.MARKER_CROSS, 16, 2)\n";
    }
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
    return "### 🏷️ 条码与二维码定位识别 (Barcode & QR Code Recognition)\n\n"
           "**【核心思想】**\n"
           "工业生产线中，产品批次、包装序列号和药品追溯广泛采用一维条形码 (Code128/EAN) 和二维 DataMatrix / QR 码。\n\n"
           "**【检测与解码流程】**\n"
           "```\n"
           "   1. 图形定位: 提取特征黑白方块比例 (1:1:3:1:1 寻像图形 Finder Patterns)\n"
           "   2. 透视矫正: 提取四边形顶点，计算单应矩阵并进行正交采样网格校正\n"
           "   3. 纠错解码: 采用 Reed-Solomon 纠错算法还原位流数据并解析字符串内容\n"
           "```";
}

bool BarcodeQrOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    ClearResults();
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    const int detectMode = _params[0].GetInt();
    bool drawBox = _params[1].GetBool();
    bool drawText = _params[2].GetBool();
    int colorIdx = _params[3].GetInt();

    cv::Scalar highlightColor(0, 255, 0);
    if (colorIdx == 1) highlightColor = cv::Scalar(0, 255, 255);
    else if (colorIdx == 2) highlightColor = cv::Scalar(255, 200, 0);
    else if (colorIdx == 3) highlightColor = cv::Scalar(0, 0, 255);

#if !defined(VISIONSTUDIO_HAS_OPENCV_OBJDETECT)
    (void)detectMode;
    (void)drawBox;
    (void)drawText;
    output = input.clone();
    errorMsg = "当前 OpenCV 构建未包含 objdetect 模块，无法执行 QR/条码解码";
    return false;
#else
    try
    {
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

        struct DecodedCode
        {
            std::string Text;
            std::string Type;
            std::vector<cv::Point2f> Points;
        };
        std::vector<DecodedCode> decodedCodes;

        if (detectMode == 0 || detectMode == 1)
        {
            cv::QRCodeDetector detector;
            std::vector<std::string> decodedInfo;
            std::vector<std::vector<cv::Point2f>> points;
            if (detector.detectAndDecodeMulti(input, decodedInfo, points))
            {
                for (size_t index = 0; index < decodedInfo.size() && index < points.size(); ++index)
                {
                    if (!decodedInfo[index].empty())
                    {
                        decodedCodes.push_back({ decodedInfo[index], "QR", points[index] });
                    }
                }
            }
        }

#if defined(VISIONSTUDIO_HAS_OPENCV_BARCODE)
        if (detectMode == 0 || detectMode == 2)
        {
            cv::barcode::BarcodeDetector detector;
            std::vector<std::string> decodedInfo;
            std::vector<std::string> decodedTypes;
            cv::Mat points;
            if (detector.detectAndDecodeWithType(input, decodedInfo, decodedTypes, points) && !points.empty())
            {
                cv::Mat pointRows = points.reshape(2, static_cast<int>(points.total() / 4));
                for (size_t codeIndex = 0; codeIndex < decodedInfo.size(); ++codeIndex)
                {
                    std::vector<cv::Point2f> codePoints;
                    for (int pointIndex = 0; pointIndex < 4; ++pointIndex)
                    {
                        codePoints.push_back(pointRows.at<cv::Point2f>(static_cast<int>(codeIndex), pointIndex));
                    }
                    decodedCodes.push_back({ decodedInfo[codeIndex], decodedTypes[codeIndex], codePoints });
                }
            }
        }
#else
        if (detectMode == 2)
        {
            errorMsg = "当前 OpenCV objdetect 构建未包含 BarcodeDetector";
            return false;
        }
#endif

        for (size_t codeIndex = 0; codeIndex < decodedCodes.size(); ++codeIndex)
        {
            const DecodedCode& code = decodedCodes[codeIndex];
            AddResult("code." + std::to_string(codeIndex) + ".text", "解码文本", code.Text);
            AddResult("code." + std::to_string(codeIndex) + ".type", "码类型", code.Type);

            if (drawBox && code.Points.size() >= 4)
            {
                for (size_t pointIndex = 0; pointIndex < code.Points.size(); ++pointIndex)
                {
                    cv::line(output, code.Points[pointIndex], code.Points[(pointIndex + 1) % code.Points.size()],
                             highlightColor, 2, cv::LINE_AA);
                }
            }
            if (drawText && !code.Points.empty())
            {
                const cv::Point textPosition(
                    std::max(4, static_cast<int>(code.Points[0].x)),
                    std::max(20, static_cast<int>(code.Points[0].y) - 8));
                cv::putText(output, code.Type + ": " + code.Text, textPosition,
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, highlightColor, 1, cv::LINE_AA);
            }
        }

        AddResult("code.count", "成功解码数量", static_cast<std::int64_t>(decodedCodes.size()));
        errorMsg.clear();
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
        return false;
    }
#endif
}

std::string BarcodeQrOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    const int detectMode = _params[0].GetInt();
    const bool drawBox = _params[1].GetBool();
    const bool drawText = _params[2].GetBool();
    const int colorIndex = _params[3].GetInt();
    const char* colors[] = {
        "cv::Scalar(0,255,0)", "cv::Scalar(0,255,255)",
        "cv::Scalar(255,200,0)", "cv::Scalar(0,0,255)"
    };
    std::ostringstream ss;
    ss << "// QR 与一维条码真实解码，需要 OpenCV objdetect 模块\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR); else " << outVar << " = " << inVar << ".clone();\n"
       << "cv::Scalar codeColor = " << colors[colorIndex] << ";\n";
    if (detectMode == 0 || detectMode == 1)
    {
        ss << "cv::QRCodeDetector qrDetector; std::vector<std::string> qrTexts; std::vector<std::vector<cv::Point2f>> qrPoints;\n"
           << "qrDetector.detectAndDecodeMulti(" << inVar << ", qrTexts, qrPoints);\n"
           << "for (size_t codeIndex = 0; codeIndex < qrTexts.size() && codeIndex < qrPoints.size(); ++codeIndex)\n"
           << "{\n";
        if (drawBox)
        {
            ss << "    for (size_t pointIndex = 0; pointIndex < qrPoints[codeIndex].size(); ++pointIndex) cv::line(" << outVar << ", qrPoints[codeIndex][pointIndex], qrPoints[codeIndex][(pointIndex + 1) % qrPoints[codeIndex].size()], codeColor, 2, cv::LINE_AA);\n";
        }
        if (drawText)
        {
            ss << "    if (!qrTexts[codeIndex].empty() && !qrPoints[codeIndex].empty()) cv::putText(" << outVar << ", \"QR: \" + qrTexts[codeIndex], qrPoints[codeIndex][0], cv::FONT_HERSHEY_SIMPLEX, 0.5, codeColor, 1, cv::LINE_AA);\n";
        }
        ss << "}\n";
    }
    if (detectMode == 0 || detectMode == 2)
    {
        ss << "cv::barcode::BarcodeDetector barcodeDetector; std::vector<std::string> barcodeTexts, barcodeTypes; cv::Mat barcodePoints;\n"
           << "barcodeDetector.detectAndDecodeWithType(" << inVar << ", barcodeTexts, barcodeTypes, barcodePoints);\n"
           << "// barcodePoints 包含每个一维码的四个定位顶点，可按 QR 绘制方式进行标注。\n";
    }
    return ss.str();
}

std::string BarcodeQrOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    const int detectMode = _params[0].GetInt();
    const bool drawBox = _params[1].GetBool();
    const bool drawText = _params[2].GetBool();
    const int colorIndex = _params[3].GetInt();
    const char* colors[] = { "(0,255,0)", "(0,255,255)", "(255,200,0)", "(0,0,255)" };
    std::ostringstream ss;
    ss << "# QR 与一维条码真实解码，需要带 objdetect 的 OpenCV\n"
       << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "code_color = " << colors[colorIndex] << "\n";
    if (detectMode == 0 || detectMode == 1)
    {
        ss << "qr_detector = cv2.QRCodeDetector()\n"
           << "qr_ok, qr_texts, qr_points, _ = qr_detector.detectAndDecodeMulti(" << inVar << ")\n"
           << "if qr_ok:\n"
           << "    for qr_text, points in zip(qr_texts, qr_points):\n"
           << "        points = points.astype(np.int32).reshape((-1, 1, 2))\n";
        if (drawBox)
        {
            ss << "        cv2.polylines(" << outVar << ", [points], True, code_color, 2)\n";
        }
        if (drawText)
        {
            ss << "        if qr_text: cv2.putText(" << outVar << ", 'QR: ' + qr_text, tuple(points[0][0]), cv2.FONT_HERSHEY_SIMPLEX, 0.5, code_color, 1)\n";
        }
    }
    if (detectMode == 0 || detectMode == 2)
    {
        ss << "barcode_detector = cv2.barcode_BarcodeDetector()\n"
           << "barcode_ok, barcode_texts, barcode_types, barcode_points = barcode_detector.detectAndDecodeWithType(" << inVar << ")\n";
    }
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
    return "### 🦴 骨架提取与细化算法 (Skeletonization & Thinning)\n\n"
           "**【核心思想】**\n"
           "在工业线缆检测、焊缝检测、印字字符点阵和微小裂纹分析中，通过拓扑骨架细化将二值斑块连通域逐步收缩为保持拓扑连通性的单像素中心线。\n\n"
           "**【Zhang-Suen 细化两阶段子迭代判决】**\n"
           "```\n"
           "   对中心前景点 P₁ 及其 8-邻域 (P₂ ~ P₉ 顺时针排列):\n"
           "     1. 邻域非零点数:  2 ≤ B(P₁) ≤ 6\n"
           "     2. 0->1 过渡次数: A(P₁) = 1 (确保骨架连通性不被破坏)\n"
           "     3. 子迭代 1: P₂ · P₄ · P₆ = 0  且  P₄ · P₆ · P₈ = 0 (消除东/南边缘点)\n"
           "     4. 子迭代 2: P₂ · P₄ · P₈ = 0  且  P₂ · P₆ · P₈ = 0 (消除西/北边缘点)\n"
           "```";
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
