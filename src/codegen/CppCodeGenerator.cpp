#include "CppCodeGenerator.h"
#include <sstream>

std::string CppCodeGenerator::Generate(const PipelineManager& pipeline, const std::string& inputImagePath) const
{
    std::stringstream ss;
    ss << "/**\n"
       << " * ============================================================================\n"
       << " * 由 VisionStudio OpenCV 自动生成的 C++ 机器视觉处理流水线代码\n"
       << " * 环境依赖: OpenCV 4.x / 3.x, C++11 及以上\n"
       << " * ============================================================================\n"
       << " */\n\n"
       << "#include <opencv2/opencv.hpp>\n"
       << "#include <iostream>\n"
       << "#include <vector>\n"
       << "#include <chrono>\n\n"
       << "int main(int argc, char** argv)\n"
       << "{\n"
       << "    // 1. 读取输入图像\n"
       << "    std::string imagePath = \"" << inputImagePath << "\";\n"
       << "    if (argc > 1)\n"
       << "    {\n"
       << "        imagePath = argv[1];\n"
       << "    }\n\n"
       << "    cv::Mat src = cv::imread(imagePath);\n"
       << "    if (src.empty())\n"
       << "    {\n"
       << "        std::cerr << \"[Error] 无法读取图像文件: \" << imagePath << std::endl;\n"
       << "        return -1;\n"
       << "    }\n\n"
       << "    std::cout << \"[Info] 图像加载成功，尺寸: \" << src.cols << \"x\" << src.rows << \" 通道: \" << src.channels() << std::endl;\n\n"
       << "    // 2. 初始化流水线变量与计时\n"
       << "    auto startTime = std::chrono::high_resolution_clock::now();\n\n"
       << "    cv::Mat current = src.clone();\n\n";

    int activeStepCount = 0;
    for (int i = 0; i < pipeline.GetStepCount(); ++i)
    {
        const PipelineStep* step = pipeline.GetStep(i);
        if (!step || !step->GetOpNode() || !step->IsEnabled())
        {
            continue;
        }

        activeStepCount++;
        IOpNode* op = step->GetOpNode();
        std::string inVar = (activeStepCount == 1) ? "src" : ("step_" + std::to_string(activeStepCount - 1) + "_out");
        std::string outVar = "step_" + std::to_string(activeStepCount) + "_out";

        ss << "    // ---------------- Step " << activeStepCount << ": " << op->GetDisplayName() << " ----------------\n"
           << "    cv::Mat " << outVar << ";\n"
           << "    {\n";

        std::string codeChunk = op->GenerateCppCode(inVar, outVar);
        std::istringstream codeStream(codeChunk);
        std::string line;
        while (std::getline(codeStream, line))
        {
            ss << "        " << line << "\n";
        }

        ss << "    }\n\n";
    }

    std::string finalVar = (activeStepCount == 0) ? "src" : ("step_" + std::to_string(activeStepCount) + "_out");

    ss << "    auto endTime = std::chrono::high_resolution_clock::now();\n"
       << "    double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();\n"
       << "    std::cout << \"[Success] 流水线执行完毕，总耗时: \" << elapsedMs << \" ms\" << std::endl;\n\n"
       << "    // 3. 结果显示与保存\n"
       << "    cv::namedWindow(\"VisionStudio Pipeline Output\", cv::WINDOW_NORMAL);\n"
       << "    cv::imshow(\"VisionStudio Pipeline Output\", " << finalVar << ");\n"
       << "    cv::imwrite(\"output_result.jpg\", " << finalVar << ");\n"
       << "    std::cout << \"[Info] 结果已保存至 output_result.jpg，按任意键退出...\" << std::endl;\n"
       << "    cv::waitKey(0);\n"
       << "    cv::destroyAllWindows();\n\n"
       << "    return 0;\n"
       << "}\n";

    return ss.str();
}
