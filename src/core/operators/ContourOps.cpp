#include "ContourOps.h"
#include <sstream>

FindContoursOp::FindContoursOp()
{
    OpParameter modeParam;
    modeParam.Name = "mode";
    modeParam.DisplayName = "检索模式 (Retrieval Mode)";
    modeParam.Type = ParamType::ComboBox;
    modeParam.Value = 0; // RETR_EXTERNAL
    modeParam.EnumOptions = {
        "RETR_EXTERNAL (仅最外层轮廓)",
        "RETR_LIST (检测所有轮廓不建立树)",
        "RETR_CCOMP (两级层级结构)",
        "RETR_TREE (完整嵌套层级树)"
    };
    _params.push_back(modeParam);

    OpParameter methodParam;
    methodParam.Name = "method";
    methodParam.DisplayName = "近似方法 (Approximation)";
    methodParam.Type = ParamType::ComboBox;
    methodParam.Value = 1; // CHAIN_APPROX_SIMPLE
    methodParam.EnumOptions = {
        "CHAIN_APPROX_NONE (保留所有点)",
        "CHAIN_APPROX_SIMPLE (压缩水平/垂直/对角线)"
    };
    _params.push_back(methodParam);

    OpParameter minAreaParam;
    minAreaParam.Name = "minArea";
    minAreaParam.DisplayName = "最小面积过滤 (Min Area px²)";
    minAreaParam.Type = ParamType::DoubleSlider;
    minAreaParam.Value = 50.0;
    minAreaParam.MinValue = 0.0;
    minAreaParam.MaxValue = 10000.0;
    minAreaParam.Step = 10.0;
    _params.push_back(minAreaParam);

    OpParameter drawTypeParam;
    drawTypeParam.Name = "drawType";
    drawTypeParam.DisplayName = "绘制拟合几何 (Geometry Overlay)";
    drawTypeParam.Type = ParamType::ComboBox;
    drawTypeParam.Value = 0;
    drawTypeParam.EnumOptions = {
        "仅轮廓线条 (Contour Curves)",
        "正外接矩形 (Bounding Rect)",
        "最小面积旋转矩形 (MinAreaRect)",
        "最小外接圆 (MinEnclosingCircle)",
        "凸包多边形 (Convex Hull)"
    };
    _params.push_back(drawTypeParam);
}

std::string FindContoursOp::GetPrincipleDoc() const
{
    return "### 📘 算子原理：轮廓提取与几何拓扑分析 (findContours)\n\n"
           "基于拓扑结构分析算法在二值图像中提取目标边界曲线：\n"
           "* **外接几何拟合**：正矩形 `boundingRect`、旋转矩形 `minAreaRect`、外接圆 `minEnclosingCircle`、凸包 `convexHull`；\n"
           "* **几何测量**：轮廓面积 `contourArea`、周长 `arcLength`，广泛用于工业尺寸测量与瑕疵检测。";
}

bool FindContoursOp::Process(const cv::Mat& input, cv::Mat& output, std::string& errorMsg)
{
    if (input.empty()) { errorMsg = "输入图像为空"; return false; }

    cv::Mat binImg;
    if (input.channels() == 1)
    {
        binImg = input.clone();
    }
    else
    {
        cv::cvtColor(input, binImg, cv::COLOR_BGR2GRAY);
    }

    if (input.channels() == 1)
    {
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    }
    else
    {
        output = input.clone();
    }

    int modeIdx = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    double minArea = _params[2].GetDouble();
    int drawType = _params[3].GetInt();

    int cvMode = cv::RETR_EXTERNAL;
    if (modeIdx == 1) cvMode = cv::RETR_LIST;
    else if (modeIdx == 2) cvMode = cv::RETR_CCOMP;
    else if (modeIdx == 3) cvMode = cv::RETR_TREE;

    int cvMethod = (methodIdx == 0) ? cv::CHAIN_APPROX_NONE : cv::CHAIN_APPROX_SIMPLE;

    try
    {
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(binImg, contours, hierarchy, cvMode, cvMethod);

        for (size_t i = 0; i < contours.size(); ++i)
        {
            double area = cv::contourArea(contours[i]);
            if (area < minArea) continue;

            if (drawType == 0) // 轮廓
            {
                cv::drawContours(output, contours, static_cast<int>(i), cv::Scalar(0, 255, 0), 2);
            }
            else if (drawType == 1) // 正外接矩形
            {
                cv::Rect r = cv::boundingRect(contours[i]);
                cv::rectangle(output, r, cv::Scalar(0, 0, 255), 2);
            }
            else if (drawType == 2) // 旋转矩形
            {
                cv::RotatedRect r = cv::minAreaRect(contours[i]);
                cv::Point2f pts[4];
                r.points(pts);
                for (int j = 0; j < 4; ++j)
                {
                    cv::line(output, pts[j], pts[(j + 1) % 4], cv::Scalar(255, 255, 0), 2);
                }
            }
            else if (drawType == 3) // 外接圆
            {
                cv::Point2f center;
                float radius;
                cv::minEnclosingCircle(contours[i], center, radius);
                cv::circle(output, center, static_cast<int>(radius), cv::Scalar(255, 0, 255), 2);
            }
            else if (drawType == 4) // 凸包
            {
                std::vector<cv::Point> hull;
                cv::convexHull(contours[i], hull);
                std::vector<std::vector<cv::Point>> hulls = { hull };
                cv::drawContours(output, hulls, 0, cv::Scalar(0, 255, 255), 2);
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
    int methodIdx = _params[1].GetInt();
    double minArea = _params[2].GetDouble();
    int drawType = _params[3].GetInt();

    std::string modeStr = (modeIdx == 1 ? "cv::RETR_LIST" : (modeIdx == 2 ? "cv::RETR_CCOMP" : (modeIdx == 3 ? "cv::RETR_TREE" : "cv::RETR_EXTERNAL")));
    std::string methodStr = (methodIdx == 0 ? "cv::CHAIN_APPROX_NONE" : "cv::CHAIN_APPROX_SIMPLE");

    std::stringstream ss;
    ss << "// 轮廓查找与几何分析参数配置\n"
       << "int contourMode = " << modeStr << ";\n"
       << "int contourMethod = " << methodStr << ";\n"
       << "double minAreaThreshold = " << minArea << "; // 最小面积过滤门限 (px²)\n"
       << "std::vector<std::vector<cv::Point>> contours;\n"
       << "std::vector<cv::Vec4i> hierarchy;\n"
       << "cv::findContours(" << inVar << ", contours, hierarchy, contourMode, contourMethod);\n"
       << "cv::cvtColor(" << inVar << ", " << outVar << ", cv::COLOR_GRAY2BGR);\n"
       << "for (size_t i = 0; i < contours.size(); ++i) {\n"
       << "    if (cv::contourArea(contours[i]) < minAreaThreshold) continue;\n";
    if (drawType == 0)
    {
        ss << "    cv::drawContours(" << outVar << ", contours, (int)i, cv::Scalar(0, 255, 0), 2);\n";
    }
    else if (drawType == 1)
    {
        ss << "    cv::Rect r = cv::boundingRect(contours[i]);\n"
           << "    cv::rectangle(" << outVar << ", r, cv::Scalar(0, 0, 255), 2);\n";
    }
    else if (drawType == 2)
    {
        ss << "    cv::RotatedRect r = cv::minAreaRect(contours[i]);\n"
           << "    cv::Point2f pts[4]; r.points(pts);\n"
           << "    for (int j = 0; j < 4; ++j) cv::line(" << outVar << ", pts[j], pts[(j+1)%4], cv::Scalar(255, 255, 0), 2);\n";
    }
    else if (drawType == 3)
    {
        ss << "    cv::Point2f center; float radius;\n"
           << "    cv::minEnclosingCircle(contours[i], center, radius);\n"
           << "    cv::circle(" << outVar << ", center, (int)radius, cv::Scalar(255, 0, 255), 2);\n";
    }
    else
    {
        ss << "    std::vector<cv::Point> hull;\n"
           << "    cv::convexHull(contours[i], hull);\n"
           << "    std::vector<std::vector<cv::Point>> hulls = { hull };\n"
           << "    cv::drawContours(" << outVar << ", hulls, 0, cv::Scalar(0, 255, 255), 2);\n";
    }
    ss << "}";
    return ss.str();
}

std::string FindContoursOp::GeneratePythonCode(const std::string& inVar, const std::string& outVar) const
{
    int modeIdx = _params[0].GetInt();
    int methodIdx = _params[1].GetInt();
    double minArea = _params[2].GetDouble();
    int drawType = _params[3].GetInt();

    std::string modeStr = (modeIdx == 1 ? "cv2.RETR_LIST" : (modeIdx == 2 ? "cv2.RETR_CCOMP" : (modeIdx == 3 ? "cv2.RETR_TREE" : "cv2.RETR_EXTERNAL")));
    std::string methodStr = (methodIdx == 0 ? "cv2.CHAIN_APPROX_NONE" : "cv2.CHAIN_APPROX_SIMPLE");

    std::stringstream ss;
    ss << "# 轮廓查找与几何分析参数配置\n"
       << "contour_mode = " << modeStr << "\n"
       << "contour_method = " << methodStr << "\n"
       << "min_area_thresh = " << minArea << "        # 最小面积过滤门限\n"
       << "contours, hierarchy = cv2.findContours(" << inVar << ", contour_mode, contour_method)\n"
       << outVar << " = cv2.cvtColor(" << inVar << ", cv2.COLOR_GRAY2BGR) if len(" << inVar << ".shape) == 2 else " << inVar << ".copy()\n"
       << "for cnt in contours:\n"
       << "    if cv2.contourArea(cnt) < min_area_thresh:\n"
       << "        continue\n";
    if (drawType == 0)
    {
        ss << "    cv2.drawContours(" << outVar << ", [cnt], 0, (0, 255, 0), 2)\n";
    }
    else if (drawType == 1)
    {
        ss << "    x, y, w, h = cv2.boundingRect(cnt)\n"
           << "    cv2.rectangle(" << outVar << ", (x, y), (x + w, y + h), (0, 0, 255), 2)\n";
    }
    else if (drawType == 2)
    {
        ss << "    rect = cv2.minAreaRect(cnt)\n"
           << "    box = np.int0(cv2.boxPoints(rect))\n"
           << "    cv2.drawContours(" << outVar << ", [box], 0, (255, 255, 0), 2)\n";
    }
    else if (drawType == 3)
    {
        ss << "    (x, y), radius = cv2.minEnclosingCircle(cnt)\n"
           << "    cv2.circle(" << outVar << ", (int(x), int(y)), int(radius), (255, 0, 255), 2)\n";
    }
    else
    {
        ss << "    hull = cv2.convexHull(cnt)\n"
           << "    cv2.drawContours(" << outVar << ", [hull], 0, (0, 255, 255), 2)\n";
    }
    return ss.str();
}
