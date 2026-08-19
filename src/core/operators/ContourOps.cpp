#include "ContourOps.h"
#include <sstream>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FindContoursOp::FindContoursOp()
{
    // 0. 检索模式
    OpParameter modeParam;
    modeParam.Name = "mode";
    modeParam.DisplayName = "检索层级模式 (Retrieval Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // RETR_EXTERNAL
    modeParam.EnumOptions = {
        "RETR_EXTERNAL (仅最外层轮廓)",
        "RETR_LIST (检测所有轮廓不建立树)",
        "RETR_CCOMP (两级层级结构)",
        "RETR_TREE (完整嵌套层级树)"
    };
    _params.push_back(modeParam);

    // 1. 过滤策略
    OpParameter filterStrategyParam;
    filterStrategyParam.Name = "filterStrategy";
    filterStrategyParam.DisplayName = "过滤筛选策略 (Filter Strategy)";
    filterStrategyParam.Type = ParamType::ComboBox;
    filterStrategyParam.Value = 0;
    filterStrategyParam.EnumOptions = {
        "保留所有符合阈值条件的轮廓 (All Valid)",
        "仅保留最大面积的轮廓 (Max Area Only / Largest)",
        "保留面积前 3 大轮廓 (Top-3 Largest)",
        "保留面积前 5 大轮廓 (Top-5 Largest)"
    };
    _params.push_back(filterStrategyParam);

    // 2. 最小面积
    OpParameter minAreaParam;
    minAreaParam.Name = "minArea";
    minAreaParam.DisplayName = "最小面积下限 (Min Area px²)";
    minAreaParam.Type = ParamType::DoubleSlider;
    minAreaParam.Value = 50.0;
    minAreaParam.MinValue = 0.0;
    minAreaParam.MaxValue = 50000.0;
    minAreaParam.Step = 50.0;
    _params.push_back(minAreaParam);

    // 3. 最大面积 (0表示不限)
    OpParameter maxAreaParam;
    maxAreaParam.Name = "maxArea";
    maxAreaParam.DisplayName = "最大面积上限 (Max Area px², 0为不限)";
    maxAreaParam.Type = ParamType::DoubleSlider;
    maxAreaParam.Value = 0.0;
    maxAreaParam.MinValue = 0.0;
    maxAreaParam.MaxValue = 200000.0;
    maxAreaParam.Step = 100.0;
    _params.push_back(maxAreaParam);

    // 4. 最小宽度
    OpParameter minWidthParam;
    minWidthParam.Name = "minWidth";
    minWidthParam.DisplayName = "最小宽度下限 (Min Width px)";
    minWidthParam.Type = ParamType::IntSlider;
    minWidthParam.Value = 0;
    minWidthParam.MinValue = 0;
    minWidthParam.MaxValue = 2000;
    minWidthParam.Step = 5;
    _params.push_back(minWidthParam);

    // 5. 最小高度
    OpParameter minHeightParam;
    minHeightParam.Name = "minHeight";
    minHeightParam.DisplayName = "最小高度下限 (Min Height px)";
    minHeightParam.Type = ParamType::IntSlider;
    minHeightParam.Value = 0;
    minHeightParam.MinValue = 0;
    minHeightParam.MaxValue = 2000;
    minHeightParam.Step = 5;
    _params.push_back(minHeightParam);

    // 6. 绘制拟合几何
    OpParameter drawTypeParam;
    drawTypeParam.Name = "drawType";
    drawTypeParam.DisplayName = "几何特征标注模式 (Overlay Geometry)";
    drawTypeParam.Type = ParamType::ComboBox;
    drawTypeParam.Value = 1; // 正外接矩形
    drawTypeParam.EnumOptions = {
        "仅绘制轮廓曲线 (Contour Curve Only)",
        "正外接矩形 (Bounding Box)",
        "最小外接旋转矩形 (MinAreaRect)",
        "最小外接圆 (MinEnclosingCircle)",
        "凸包多边形 (Convex Hull)",
        "全特征综合标注 (All Features: Box + Centroid)"
    };
    _params.push_back(drawTypeParam);

    // 7. 显示统计文本
    OpParameter showLabelsParam;
    showLabelsParam.Name = "showLabels";
    showLabelsParam.DisplayName = "标注统计文本 (ID/Area/Width/Height)";
    showLabelsParam.Tooltip = "在轮廓中心绘制编号与长宽面积测量值";
    showLabelsParam.Type = ParamType::CheckBox;
    showLabelsParam.Value = true;
    _params.push_back(showLabelsParam);

    // 8. 背景画布模式
    OpParameter canvasModeParam;
    canvasModeParam.Name = "canvasMode";
    canvasModeParam.DisplayName = "结果画布底图 (Canvas Background)";
    canvasModeParam.Tooltip = "选择绘制背景：叠加在原图/处理图上，或纯黑画布/纯白画布";
    canvasModeParam.Type = ParamType::ComboBox;
    canvasModeParam.Value = 0; // 叠加在输入图像上
    canvasModeParam.EnumOptions = {
        "叠加在输入图像上 (Overlay on Input Image)",
        "纯黑背景画布 (Black Canvas / Mask Mode)",
        "纯白背景画布 (White Canvas)"
    };
    _params.push_back(canvasModeParam);

    // 9. 标注线宽粗细
    OpParameter thicknessParam;
    thicknessParam.Name = "thickness";
    thicknessParam.DisplayName = "标注线条粗细 (Line Thickness px)";
    thicknessParam.Tooltip = "设置轮廓曲线、外接矩形与几何图形的绘制线宽";
    thicknessParam.Type = ParamType::IntSlider;
    thicknessParam.Value = 2;
    thicknessParam.MinValue = 1;
    thicknessParam.MaxValue = 10;
    thicknessParam.Step = 1;
    _params.push_back(thicknessParam);

    // 10. 高对比度复合描边
    OpParameter highContrastParam;
    highContrastParam.Name = "highContrast";
    highContrastParam.DisplayName = "高对比度双层描边 (High Contrast Outline)";
    highContrastParam.Tooltip = "采用反差底衬+荧光芯线复合绘制与文字胶囊框，确保在白底二值图及深色背景上均清晰锐利";
    highContrastParam.Type = ParamType::CheckBox;
    highContrastParam.Value = true;
    _params.push_back(highContrastParam);
}

std::string FindContoursOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：轮廓查找与多维度特征筛选分析 (findContours & Filter)\n\n"
           "**【核心思想】**\n"
           "基于拓扑结构分析提取二值图像中物体连通边界，并支持通过几何面积、包围盒长宽及 Top-K 策略快速筛选目标工件，实现质心坐标与几何测量。\n\n"
           "**【数学模型与计算指标】**\n"
           "```\n"
           "   1. 轮廓闭合面积 (格林公式):  S = 0.5 · | ∑ (x_i · y_{i+1} - x_{i+1} · y_i) |\n"
           "\n"
           "   2. 物理质心 (几何矩):        Cx = m₁₀ / m₀₀,   Cy = m₀₁ / m₀₀\n"
           "\n"
           "   3. 最小旋转矩形 (MinAreaRect): 基于旋转卡壳法计算真实方向角 θ 与主轴长宽\n"
           "\n"
           "   4. 凸包分析 (ConvexHull):     基于 Graham 扫描法计算包含轮廓的最小凸多边形\n"
           "```\n\n"
           "**【符号与参数解析】**\n"
           "- **mode**：检索拓扑层级（RETR_EXTERNAL 仅外层，RETR_TREE 嵌套树）；\n"
           "- **filterStrategy**：保留所有、仅最大面积（Max Area Only）或 Top-K；\n"
           "- **drawType**：标注模式（正矩形、旋转矩形、最小外接圆、凸包）。";
}

bool FindContoursOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat binImg;
    if (input.channels() == 1)
    {
        binImg = input.clone();
    }
    else if (input.channels() == 3)
    {
        cv::cvtColor(input, binImg, cv::COLOR_BGR2GRAY);
    }
    else
    {
        cv::cvtColor(input, binImg, cv::COLOR_BGRA2GRAY);
    }

    int modeIdx = _params[0].GetInt();
    int filterStrategy = _params[1].GetInt();
    double minArea = _params[2].GetDouble();
    double maxArea = _params[3].GetDouble();
    int minWidth = _params[4].GetInt();
    int minHeight = _params[5].GetInt();
    int drawType = _params[6].GetInt();
    bool showLabels = _params[7].GetBool();
    int canvasMode = _params.size() > 8 ? _params[8].GetInt() : 0;
    int thickness = _params.size() > 9 ? _params[9].GetInt() : 2;
    bool highContrast = _params.size() > 10 ? _params[10].GetBool() : true;

    if (thickness < 1)
    {
        thickness = 1;
    }

    // 初始化输出底图 (支持原图叠加/黑画布/白画布)
    if (canvasMode == 1)
    {
        // 纯黑背景画布
        output = cv::Mat::zeros(input.rows, input.cols, CV_8UC3);
    }
    else if (canvasMode == 2)
    {
        // 纯白背景画布
        output = cv::Mat(input.rows, input.cols, CV_8UC3, cv::Scalar(255, 255, 255));
    }
    else
    {
        // 叠加在输入图像上
        if (input.channels() == 1)
        {
            cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
        }
        else if (input.channels() == 3)
        {
            output = input.clone();
        }
        else
        {
            cv::cvtColor(input, output, cv::COLOR_BGRA2BGR);
        }
    }

    int cvMode = cv::RETR_EXTERNAL;
    if (modeIdx == 1) cvMode = cv::RETR_LIST;
    else if (modeIdx == 2) cvMode = cv::RETR_CCOMP;
    else if (modeIdx == 3) cvMode = cv::RETR_TREE;

    try
    {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binImg, contours, hierarchy, cvMode, cv::CHAIN_APPROX_SIMPLE);

        struct ContourItem
        {
            size_t Index;
            double Area;
            cv::Rect BBox;
        };

        std::vector<ContourItem> validItems;
        for (size_t i = 0; i < contours.size(); ++i)
        {
            double area = cv::contourArea(contours[i]);
            if (area < minArea) continue;
            if (maxArea > 0 && area > maxArea) continue;

            cv::Rect bbox = cv::boundingRect(contours[i]);
            if (minWidth > 0 && bbox.width < minWidth) continue;
            if (minHeight > 0 && bbox.height < minHeight) continue;

            validItems.push_back({ i, area, bbox });
        }

        // 根据策略排序或保留
        if (filterStrategy == 1) // 仅最大面积
        {
            if (!validItems.empty())
            {
                auto maxIt = std::max_element(validItems.begin(), validItems.end(), [](const ContourItem& a, const ContourItem& b) {
                    return a.Area < b.Area;
                });
                ContourItem maxItem = *maxIt;
                validItems.clear();
                validItems.push_back(maxItem);
            }
        }
        else if (filterStrategy == 2) // Top-3
        {
            std::sort(validItems.begin(), validItems.end(), [](const ContourItem& a, const ContourItem& b) {
                return a.Area > b.Area;
            });
            if (validItems.size() > 3) validItems.resize(3);
        }
        else if (filterStrategy == 3) // Top-5
        {
            std::sort(validItems.begin(), validItems.end(), [](const ContourItem& a, const ContourItem& b) {
                return a.Area > b.Area;
            });
            if (validItems.size() > 5) validItems.resize(5);
        }

        int haloThick = thickness + 2;
        cv::Scalar haloColor(0, 0, 0); // 黑色底衬反差色

        // 绘制结果
        for (size_t k = 0; k < validItems.size(); ++k)
        {
            const auto& item = validItems[k];
            const auto& cnt = contours[item.Index];

            // 1. 绘制轮廓线 (高亮绿)
            if (highContrast)
            {
                cv::drawContours(output, contours, static_cast<int>(item.Index), haloColor, haloThick, cv::LINE_AA);
            }
            cv::drawContours(output, contours, static_cast<int>(item.Index), cv::Scalar(0, 255, 0), thickness, cv::LINE_AA);

            // 2. 绘制外接几何
            if (drawType == 1 || drawType == 5) // 正矩形 (高亮橙)
            {
                if (highContrast)
                {
                    cv::rectangle(output, item.BBox, haloColor, haloThick, cv::LINE_AA);
                }
                cv::rectangle(output, item.BBox, cv::Scalar(0, 140, 255), thickness, cv::LINE_AA);
            }
            if (drawType == 2 || drawType == 5) // 旋转矩形 (天蓝/金黄)
            {
                cv::RotatedRect r = cv::minAreaRect(cnt);
                cv::Point2f pts[4];
                r.points(pts);
                for (int j = 0; j < 4; ++j)
                {
                    if (highContrast)
                    {
                        cv::line(output, pts[j], pts[(j + 1) % 4], haloColor, haloThick, cv::LINE_AA);
                    }
                    cv::line(output, pts[j], pts[(j + 1) % 4], cv::Scalar(255, 215, 0), thickness, cv::LINE_AA);
                }
            }
            if (drawType == 3) // 最小外接圆 (品红)
            {
                cv::Point2f center;
                float radius;
                cv::minEnclosingCircle(cnt, center, radius);
                if (highContrast)
                {
                    cv::circle(output, center, static_cast<int>(radius), haloColor, haloThick, cv::LINE_AA);
                }
                cv::circle(output, center, static_cast<int>(radius), cv::Scalar(255, 0, 255), thickness, cv::LINE_AA);
            }
            if (drawType == 4) // 凸包 (亮青)
            {
                std::vector<cv::Point> hull;
                cv::convexHull(cnt, hull);
                std::vector<std::vector<cv::Point>> hulls = { hull };
                if (highContrast)
                {
                    cv::drawContours(output, hulls, 0, haloColor, haloThick, cv::LINE_AA);
                }
                cv::drawContours(output, hulls, 0, cv::Scalar(255, 255, 0), thickness, cv::LINE_AA);
            }

            // 3. 绘制质心标记 (高亮红)
            cv::Moments m = cv::moments(cnt);
            cv::Point centroid(item.BBox.x + item.BBox.width / 2, item.BBox.y + item.BBox.height / 2);
            if (m.m00 > 1e-4)
            {
                centroid = cv::Point(static_cast<int>(m.m10 / m.m00), static_cast<int>(m.m01 / m.m00));
            }

            if (highContrast)
            {
                cv::drawMarker(output, centroid, haloColor, cv::MARKER_CROSS, 14, haloThick, cv::LINE_AA);
            }
            cv::drawMarker(output, centroid, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 12, thickness, cv::LINE_AA);

            // 4. 绘制统计文本 (增加半透明深色底框，确保在白底/黑底下均极其醒目)
            if (showLabels)
            {
                std::stringstream ss;
                ss << "#" << (k + 1) << " S:" << static_cast<int>(item.Area) << " " << item.BBox.width << "x" << item.BBox.height;
                std::string labelText = ss.str();

                int baseline = 0;
                double fontScale = 0.48;
                int fontThick = 1;
                cv::Size textSize = cv::getTextSize(labelText, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontThick, &baseline);

                int tx = std::clamp(item.BBox.x, 4, std::max(4, output.cols - textSize.width - 6));
                int ty = item.BBox.y - 6;
                if (ty - textSize.height - 4 < 0)
                {
                    ty = item.BBox.y + item.BBox.height + textSize.height + 6;
                }
                ty = std::clamp(ty, textSize.height + 4, output.rows - 4);

                cv::Rect badgeRect(tx - 3, ty - textSize.height - 3, textSize.width + 6, textSize.height + baseline + 4);
                badgeRect &= cv::Rect(0, 0, output.cols, output.rows);

                if (badgeRect.width > 0 && badgeRect.height > 0)
                {
                    cv::Mat roi = output(badgeRect);
                    cv::Mat darkBadge(roi.size(), roi.type(), cv::Scalar(20, 20, 25));
                    cv::addWeighted(roi, 0.25, darkBadge, 0.75, 0, roi);
                    cv::rectangle(output, badgeRect, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
                }

                cv::putText(output, labelText, cv::Point(tx, ty),
                            cv::FONT_HERSHEY_SIMPLEX, fontScale, cv::Scalar(0, 255, 255), fontThick, cv::LINE_AA);
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

std::string FindContoursOp::GenerateCppCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int filterStrategy = _params[1].GetInt();
    double minArea = _params[2].GetDouble();
    double maxArea = _params[3].GetDouble();
    int minWidth = _params[4].GetInt();
    int minHeight = _params[5].GetInt();
    int drawType = _params[6].GetInt();
    bool showLabels = _params[7].GetBool();
    int canvasMode = _params.size() > 8 ? _params[8].GetInt() : 0;
    int thickness = _params.size() > 9 ? _params[9].GetInt() : 2;
    bool highContrast = _params.size() > 10 ? _params[10].GetBool() : true;

    std::string modeStr = (modeIdx == 1 ? "cv::RETR_LIST" : (modeIdx == 2 ? "cv::RETR_CCOMP" : (modeIdx == 3 ? "cv::RETR_TREE" : "cv::RETR_EXTERNAL")));

    std::stringstream ss;
    ss << "// 轮廓查找与多维度特征筛选参数配置\n"
       << "int contourMode = " << modeStr << ";\n"
       << "double minArea = " << minArea << ";         // 最小面积阈值\n"
       << "double maxArea = " << maxArea << ";         // 最大面积阈值 (0为不限)\n"
       << "int minW = " << minWidth << ";               // 最小宽度阈值\n"
       << "int minH = " << minHeight << ";               // 最小高度阈值\n"
       << "int lineThickness = " << thickness << ";\n"
       << "bool highContrast = " << (highContrast ? "true" : "false") << ";\n"
       << "std::vector<std::vector<cv::Point>> contours;\n"
       << "std::vector<cv::Vec4i> hierarchy;\n"
       << "cv::Mat binImg = " << inVar << ".channels() == 1 ? " << inVar << ".clone() : cv::Mat();\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", binImg, cv::COLOR_BGR2GRAY);\n"
       << "cv::findContours(binImg, contours, hierarchy, contourMode, cv::CHAIN_APPROX_SIMPLE);\n";

    if (canvasMode == 1)
    {
        ss << outVar << " = cv::Mat::zeros(" << inVar << ".rows, " << inVar << ".cols, CV_8UC3);\n";
    }
    else if (canvasMode == 2)
    {
        ss << outVar << " = cv::Mat(" << inVar << ".rows, " << inVar << ".cols, CV_8UC3, cv::Scalar(255, 255, 255));\n";
    }
    else
    {
        ss << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
           << "else " << outVar << " = " << inVar << ".clone();\n";
    }

    ss << "std::vector<std::vector<cv::Point>> filteredContours;\n"
       << "for (const auto& cnt : contours) {\n"
       << "    double area = cv::contourArea(cnt);\n"
       << "    if (area < minArea) continue;\n"
       << "    if (maxArea > 0 && area > maxArea) continue;\n"
       << "    cv::Rect bbox = cv::boundingRect(cnt);\n"
       << "    if (minW > 0 && bbox.width < minW) continue;\n"
       << "    if (minH > 0 && bbox.height < minH) continue;\n"
       << "    filteredContours.push_back(cnt);\n"
       << "}\n";

    if (filterStrategy == 1)
    {
        ss << "// 仅保留最大面积轮廓\n"
           << "if (!filteredContours.empty()) {\n"
           << "    auto maxIt = std::max_element(filteredContours.begin(), filteredContours.end(), [](const auto& a, const auto& b) {\n"
           << "        return cv::contourArea(a) < cv::contourArea(b);\n"
           << "    });\n"
           << "    filteredContours = { *maxIt };\n"
           << "}\n";
    }

    ss << "// 绘制筛选出的特征轮廓与几何标注 (含反差描边)\n"
       << "for (size_t i = 0; i < filteredContours.size(); ++i) {\n"
       << "    if (highContrast) cv::drawContours(" << outVar << ", filteredContours, (int)i, cv::Scalar(0, 0, 0), lineThickness + 2, cv::LINE_AA);\n"
       << "    cv::drawContours(" << outVar << ", filteredContours, (int)i, cv::Scalar(0, 255, 0), lineThickness, cv::LINE_AA);\n"
       << "    cv::Rect r = cv::boundingRect(filteredContours[i]);\n";
    if (drawType == 1 || drawType == 5)
    {
        ss << "    if (highContrast) cv::rectangle(" << outVar << ", r, cv::Scalar(0, 0, 0), lineThickness + 2, cv::LINE_AA);\n"
           << "    cv::rectangle(" << outVar << ", r, cv::Scalar(0, 140, 255), lineThickness, cv::LINE_AA);\n";
    }
    if (drawType == 2 || drawType == 5)
    {
        ss << "    cv::RotatedRect rot = cv::minAreaRect(filteredContours[i]);\n"
           << "    cv::Point2f pts[4]; rot.points(pts);\n"
           << "    for (int j = 0; j < 4; ++j) {\n"
           << "        if (highContrast) cv::line(" << outVar << ", pts[j], pts[(j+1)%4], cv::Scalar(0, 0, 0), lineThickness + 2, cv::LINE_AA);\n"
           << "        cv::line(" << outVar << ", pts[j], pts[(j+1)%4], cv::Scalar(255, 215, 0), lineThickness, cv::LINE_AA);\n"
           << "    }\n";
    }
    if (showLabels)
    {
        ss << "    cv::putText(" << outVar << ", \"#\" + std::to_string(i+1) + \" Area:\" + std::to_string((int)cv::contourArea(filteredContours[i])),\n"
           << "                cv::Point(r.x, std::max(16, r.y - 5)), cv::FONT_HERSHEY_SIMPLEX, 0.48, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);\n";
    }
    ss << "}";
    return ss.str();
}

std::string FindContoursOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int filterStrategy = _params[1].GetInt();
    double minArea = _params[2].GetDouble();
    double maxArea = _params[3].GetDouble();
    int minWidth = _params[4].GetInt();
    int minHeight = _params[5].GetInt();
    int drawType = _params[6].GetInt();
    bool showLabels = _params[7].GetBool();
    int canvasMode = _params.size() > 8 ? _params[8].GetInt() : 0;
    int thickness = _params.size() > 9 ? _params[9].GetInt() : 2;
    bool highContrast = _params.size() > 10 ? _params[10].GetBool() : true;

    std::string modeStr = (modeIdx == 1 ? "cv2.RETR_LIST" : (modeIdx == 2 ? "cv2.RETR_CCOMP" : (modeIdx == 3 ? "cv2.RETR_TREE" : "cv2.RETR_EXTERNAL")));

    std::stringstream ss;
    ss << "# 轮廓查找与多维度特征筛选参数配置\n"
       << "contour_mode = " << modeStr << "\n"
       << "min_area = " << minArea << "               # 最小面积阈值\n"
       << "max_area = " << maxArea << "               # 最大面积阈值\n"
       << "min_w, min_h = " << minWidth << ", " << minHeight << "          # 最小长宽阈值\n"
       << "line_thickness = " << thickness << "\n"
       << "high_contrast = " << (highContrast ? "True" : "False") << "\n"
       << "bin_img = " << inVar << " if len(" << inVar << ".shape) == 2 else cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY)\n"
       << "contours, hierarchy = cv2.findContours(bin_img, contour_mode, cv2.CHAIN_APPROX_SIMPLE)\n";

    if (canvasMode == 1)
    {
        ss << outVar << " = np.zeros((" << inVar << ".shape[0], " << inVar << ".shape[1], 3), dtype=np.uint8)\n";
    }
    else if (canvasMode == 2)
    {
        ss << outVar << " = np.full((" << inVar << ".shape[0], " << inVar << ".shape[1], 3), 255, dtype=np.uint8)\n";
    }
    else
    {
        ss << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n";
    }

    ss << "filtered_contours = []\n"
       << "for cnt in contours:\n"
       << "    area = cv2.contourArea(cnt)\n"
       << "    if area < min_area or (max_area > 0 and area > max_area):\n"
       << "        continue\n"
       << "    x, y, w, h = cv2.boundingRect(cnt)\n"
       << "    if (min_w > 0 and w < min_w) or (min_h > 0 and h < min_h):\n"
       << "        continue\n"
       << "    filtered_contours.append(cnt)\n";

    if (filterStrategy == 1)
    {
        ss << "# 仅保留最大面积轮廓\n"
           << "if len(filtered_contours) > 0:\n"
           << "    filtered_contours = [max(filtered_contours, key=cv2.contourArea)]\n";
    }

    ss << "for i, cnt in enumerate(filtered_contours):\n"
       << "    if high_contrast:\n"
       << "        cv2.drawContours(" << outVar << ", [cnt], 0, (0, 0, 0), line_thickness + 2, cv2.LINE_AA)\n"
       << "    cv2.drawContours(" << outVar << ", [cnt], 0, (0, 255, 0), line_thickness, cv2.LINE_AA)\n"
       << "    x, y, w, h = cv2.boundingRect(cnt)\n";
    if (drawType == 1 || drawType == 5)
    {
        ss << "    if high_contrast:\n"
           << "        cv2.rectangle(" << outVar << ", (x, y), (x + w, y + h), (0, 0, 0), line_thickness + 2, cv2.LINE_AA)\n"
           << "    cv2.rectangle(" << outVar << ", (x, y), (x + w, y + h), (0, 140, 255), line_thickness, cv2.LINE_AA)\n";
    }
    if (drawType == 2 || drawType == 5)
    {
        ss << "    rect = cv2.minAreaRect(cnt)\n"
           << "    box = np.int0(cv2.boxPoints(rect))\n"
           << "    if high_contrast:\n"
           << "        cv2.drawContours(" << outVar << ", [box], 0, (0, 0, 0), line_thickness + 2, cv2.LINE_AA)\n"
           << "    cv2.drawContours(" << outVar << ", [box], 0, (255, 215, 0), line_thickness, cv2.LINE_AA)\n";
    }
    if (showLabels)
    {
        ss << "    cv2.putText(" << outVar << ", f'#{i+1} Area:{int(cv2.contourArea(cnt))} {w}x{h}',\n"
           << "                (x, max(16, y - 5)), cv2.FONT_HERSHEY_SIMPLEX, 0.48, (0, 255, 255), 1, cv2.LINE_AA)\n";
    }
    return ss.str();
}
