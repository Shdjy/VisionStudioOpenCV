#include "PythonCodeGenerator.h"
#include <sstream>

std::string PythonCodeGenerator::Generate(const PipelineManager& pipeline, const std::string& inputImagePath) const
{
    std::stringstream ss;
    ss << "#!/usr/bin/env python3\n"
       << "# -*- coding: utf-8 -*-\n"
       << "\"\"\"\n"
       << "============================================================================\n"
       << "由 VisionStudio OpenCV 自动生成的 Python 机器视觉处理流水线代码\n"
       << "环境依赖: opencv-python, numpy\n"
       << "安装命令: pip install opencv-python numpy\n"
       << "============================================================================\n"
       << "\"\"\"\n\n"
       << "import cv2\n"
       << "import numpy as np\n"
       << "import sys\n"
       << "import time\n\n\n"
       << "def process_pipeline(image_path: str = \"" << inputImagePath << "\"):\n"
       << "    # 1. 加载图像\n"
       << "    src = cv2.imread(image_path)\n"
       << "    if src is None:\n"
       << "        print(f\"[Error] 无法读取图像: {image_path}\", file=sys.stderr)\n"
       << "        return\n\n"
       << "    h, w = src.shape[:2]\n"
       << "    print(f\"[Info] 成功加载图像: {w}x{h}, 通道数: {src.shape[2] if len(src.shape) == 3 else 1}\")\n\n"
       << "    # 2. 执行流水线处理并计时\n"
       << "    start_time = time.perf_counter()\n\n";

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

        ss << "    # --- Step " << activeStepCount << ": " << op->GetDisplayName() << " ---\n";

        std::string codeChunk = op->GeneratePythonCode(inVar, outVar);
        std::istringstream codeStream(codeChunk);
        std::string line;
        while (std::getline(codeStream, line))
        {
            ss << "    " << line << "\n";
        }
        ss << "\n";
    }

    std::string finalVar = (activeStepCount == 0) ? "src" : ("step_" + std::to_string(activeStepCount) + "_out");

    ss << "    elapsed_ms = (time.perf_counter() - start_time) * 1000\n"
       << "    print(f\"[Success] 流水线执行耗时: {elapsed_ms:.2f} ms\")\n\n"
       << "    # 3. 结果显示与存储\n"
       << "    cv2.namedWindow('VisionStudio Output', cv2.WINDOW_NORMAL)\n"
       << "    cv2.imshow('VisionStudio Output', " << finalVar << ")\n"
       << "    cv2.imwrite('output_result.jpg', " << finalVar << ")\n"
       << "    print(\"[Info] 结果已保存至 output_result.jpg，按任意键退出窗口...\")\n"
       << "    cv2.waitKey(0)\n"
       << "    cv2.destroyAllWindows()\n\n\n"
       << "if __name__ == '__main__':\n"
       << "    img_file = sys.argv[1] if len(sys.argv) > 1 else \"" << inputImagePath << "\"\n"
       << "    process_pipeline(img_file)\n";

    return ss.str();
}
