#include "ContourOps.h"
#include <sstream>

FindContoursOp::FindContoursOp()
{
    OpParameter modeParam;
    modeParam.Name = "mode";
    modeParam.DisplayName = "检索模式 (RetrievalMode)";
    modeParam.Tooltip = "轮廓拓扑层级检索方式";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // RETR_EXTERNAL
    modeParam.EnumOptions = {
        "仅检测最外层轮廓 (RETR_EXTERNAL)",
        "检测所有轮廓并保存为列表 (RETR_LIST)",
        "检测所有轮廓并建立双层层级 (RETR_CCOMP)",
        "检测所有轮廓并重建完整树状层级 (RETR_TREE)"
    };
    _params.push_back(modeParam);

    OpParameter minAreaParam;
    minAreaParam.Name = "minArea";
    minAreaParam.DisplayName = "最小面积过滤 (MinArea)";
    minAreaParam.Tooltip = "过滤面积小于此阈值的细小噪点轮廓 (像素数)";
    minAreaParam.Type = ParamType::IntSlider;
    minAreaParam.Value = 100;
    minAreaParam.MinValue = 0;
    minAreaParam.MaxValue = 10000;
    minAreaParam.Step = 50;
    _params.push_back(minAreaParam);

    OpParameter drawTypeParam;
    drawTypeParam.Name = "drawType";
    drawTypeParam.DisplayName = "绘制特征类型 (DrawType)";
    drawTypeParam.Tooltip = "选择要在图像上绘制的几何特征图形";
    drawTypeParam.Type = ParamType::ComboBox;
    drawTypeParam.Value = 0;
    drawTypeParam.EnumOptions = {
        "绘制轮廓线条 (Contours)",
        "绘制外接正矩形 (Bounding Rect)",
        "绘制最小旋转矩形 (Min Area Rect)",
        "绘制最小外接圆 (Min Enclosing Circle)",
        "绘制凸包多边形 (Convex Hull)"
    };
    _params.push_back(drawTypeParam);

    OpParameter thicknessParam;
    thicknessParam.Name = "thickness";
    thicknessParam.DisplayName = "线条粗细 (Thickness)";
    thicknessParam.Tooltip = "绘制线条的像素宽度";
    thicknessParam.Type = ParamType::IntSlider;
    thicknessParam.Value = 2;
    thicknessParam.MinValue = 1;
    thicknessParam.MaxValue = 10;
    thicknessParam.Step = 1;
    _params.push_back(thicknessParam);
}

std::string FindContoursOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：轮廓查找与分析 (findContours)\n\n"
           "基于拓扑结构分析算法（如 Suzuki85 算法）从二值图像中提取边界像素序列。\n"
           "**后续几何测量能力**：\n"
           "* **面积与周长**：`contourArea()`, `arcLength()`；\n"
           "* **包络与拟合**：外接矩形 `boundingRect()`、旋转矩形 `minAreaRect()`、最小外接圆 `minEnclosingCircle()`、凸包 `convexHull()`。";
}

bool FindContoursOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty())
    {
        errorMsg = "输入图像为空";
        return false;
    }

    cv::Mat gray;
    if (input.channels() > 1) { cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY); }
    else { gray = input.clone(); }

    int modeIdx = _params[0].GetInt();
    int cvMode = cv::RETR_EXTERNAL;
    if (modeIdx == 1) cvMode = cv::RETR_LIST;
    else if (modeIdx == 2) cvMode = cv::RETR_CCOMP;
    else if (modeIdx == 3) cvMode = cv::RETR_TREE;

    double minArea = _params[1].GetInt();
    int drawType = _params[2].GetInt();
    int thickness = _params[3].GetInt();

    // 准备彩色画布
    if (input.channels() == 1)
    {
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    }
    else
    {
        output = input.clone();
    }

    try
    {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(gray, contours, hierarchy, cvMode, cv::CHAIN_APPROX_SIMPLE);

        for (size_t i = 0; i < contours.size(); ++i)
        {
            double area = cv::contourArea(contours[i]);
            if (area < minArea)
            {
                continue;
            }

            if (drawType == 0) // 绘制轮廓线
            {
                cv::drawContours(output, contours, static_cast<int>(i), cv::Scalar(0, 255, 0), thickness);
            }
            else if (drawType == 1) // 正矩形
            {
                cv::Rect rect = cv::boundingRect(contours[i]);
                cv::rectangle(output, rect, cv::Scalar(0, 0, 255), thickness);
            }
            else if (drawType == 2) // 最小旋转矩形
            {
                cv::RotatedRect rotRect = cv::minAreaRect(contours[i]);
                cv::Point2f pts[4];
                rotRect.points(pts);
                for (int j = 0; j < 4; ++j)
                {
                    cv::line(output, pts[j], pts[(j + 1) % 4], cv::Scalar(255, 0, 0), thickness);
                }
            }
            else if (drawType == 3) // 最小外接圆
            {
                cv::Point2f center;
                float radius;
                cv::minEnclosingCircle(contours[i], center, radius);
                cv::circle(output, center, static_cast<int>(radius), cv::Scalar(255, 255, 0), thickness);
            }
            else if (drawType == 4) // 凸包
            {
                std::vector<cv::Point> hull;
                cv::convexHull(contours[i], hull);
                std::vector<std::vector<cv::Point>> hullList = { hull };
                cv::drawContours(output, hullList, 0, cv::Scalar(255, 0, 255), thickness);
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
    std::string modeStr = "cv::RETR_EXTERNAL";
    if (modeIdx == 1) modeStr = "cv::RETR_LIST";
    else if (modeIdx == 2) modeStr = "cv::RETR_CCOMP";
    else if (modeIdx == 3) modeStr = "cv::RETR_TREE";

    int minArea = _params[1].GetInt();
    int drawType = _params[2].GetInt();
    int thickness = _params[3].GetInt();

    std::stringstream ss;
    ss << "cv::Mat gray_" << outVar << ";\n"
       << "if (" << inVar << ".channels() > 1) cv::cvtColor(" << inVar << ", gray_" << outVar << ", cv::COLOR_BGR2GRAY);\n"
       << "else gray_" << outVar << " = " << inVar << ".clone();\n"
       << "if (" << inVar << ".channels() == 1) cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "else " << outVar << " = " << inVar << ".clone();\n"
       << "std::vector<std::vector<cv::Point>> contours_" << outVar << ";\n"
       << "std::vector<cv::Vec4i> hierarchy_" << outVar << ";\n"
       << "cv::findContours(gray_" << outVar << ", contours_" << outVar << ", hierarchy_" << outVar << ", " << modeStr << ", cv::CHAIN_APPROX_SIMPLE);\n"
       << "for (size_t i = 0; i < contours_" << outVar << ".size(); ++i)\n"
       << "{\n"
       << "    if (cv::contourArea(contours_" << outVar << "[i]) < " << minArea << ") continue;\n";

    if (drawType == 0)
        ss << "    cv::drawContours(" << outVar << ", contours_" << outVar << ", static_cast<int>(i), cv::Scalar(0, 255, 0), " << thickness << ");\n";
    else if (drawType == 1)
        ss << "    cv::rectangle(" << outVar << ", cv::boundingRect(contours_" << outVar << "[i]), cv::Scalar(0, 0, 255), " << thickness << ");\n";
    else if (drawType == 2)
    {
        ss << "    cv::RotatedRect rr = cv::minAreaRect(contours_" << outVar << "[i]);\n"
           << "    cv::Point2f pts[4]; rr.points(pts);\n"
           << "    for(int j = 0; j < 4; ++j) cv::line(" << outVar << ", pts[j], pts[(j+1)%4], cv::Scalar(255, 0, 0), " << thickness << ");\n";
    }
    else if (drawType == 3)
    {
        ss << "    cv::Point2f c; float r;\n"
           << "    cv::minEnclosingCircle(contours_" << outVar << "[i], c, r);\n"
           << "    cv::circle(" << outVar << ", c, static_cast<int>(r), cv::Scalar(255, 255, 0), " << thickness << ");\n";
    }
    else if (drawType == 4)
    {
        ss << "    std::vector<cv::Point> hull; cv::convexHull(contours_" << outVar << "[i], hull);\n"
           << "    std::vector<std::vector<cv::Point>> hList = {hull};\n"
           << "    cv::drawContours(" << outVar << ", hList, 0, cv::Scalar(255, 0, 255), " << thickness << ");\n";
    }

    ss << "}";
    return ss.str();
}

std::string FindContoursOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    std::string modeStr = "cv2.RETR_EXTERNAL";
    if (modeIdx == 1) modeStr = "cv2.RETR_LIST";
    else if (modeIdx == 2) modeStr = "cv2.RETR_CCOMP";
    else if (modeIdx == 3) modeStr = "cv2.RETR_TREE";

    int minArea = _params[1].GetInt();
    int drawType = _params[2].GetInt();
    int thickness = _params[3].GetInt();

    std::stringstream ss;
    ss << "gray = cv2.cvtColor(" << inVar << ", cv2.COLOR_BGR2GRAY) if len(" << inVar << ".shape) == 3 else " << inVar << "\n"
       << outVar << " = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "contours, hierarchy = cv2.findContours(gray, " << modeStr << ", cv2.CHAIN_APPROX_SIMPLE)\n"
       << "for c in contours:\n"
       << "    if cv2.contourArea(c) < " << minArea << ":\n"
       << "        continue\n";

    if (drawType == 0)
        ss << "    cv2.drawContours(" << outVar << ", [c], -1, (0, 255, 0), " << thickness << ")\n";
    else if (drawType == 1)
        ss << "    x, y, w, h = cv2.boundingRect(c)\n    cv2.rectangle(" << outVar << ", (x, y), (x + w, y + h), (0, 0, 255), " << thickness << ")\n";
    else if (drawType == 2)
    {
        ss << "    rect = cv2.minAreaRect(c)\n    box = np.int0(cv2.boxPoints(rect))\n    cv2.drawContours(" << outVar << ", [box], 0, (255, 0, 0), " << thickness << ")\n";
    }
    else if (drawType == 3)
    {
        ss << "    (x, y), r = cv2.minEnclosingCircle(c)\n    cv2.circle(" << outVar << ", (int(x), int(y)), int(r), (255, 255, 0), " << thickness << ")\n";
    }
    else if (drawType == 4)
    {
        ss << "    hull = cv2.convexHull(c)\n    cv2.drawContours(" << outVar << ", [hull], 0, (255, 0, 255), " << thickness << ")\n";
    }

    return ss.str();
}
