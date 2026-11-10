#include "core/OpRegistry.h"
#include "core/PipelineContext.h"
#include "core/operators/ContourOps.h"
#include "core/operators/FeatureOps.h"
#include "core/operators/FilterOps.h"
#include "core/operators/GeneralOps.h"
#include "core/operators/IndustrialOps.h"
#include "core/operators/InteractiveMaskOp.h"
#include "bll/PipelineManager.h"
#include "bll/ProjectManager.h"
#include "codegen/CppCodeGenerator.h"
#include "codegen/PythonCodeGenerator.h"
#include "utils/ImageIoUtils.h"
#include "utils/MatUtils.h"
#include "utils/AppSettings.h"
#include "core/OperatorMetadata.h"
#include "bll/PipelineValidator.h"
#include "detection/DetectionTask.h"
#include "detection/InspectionRuleEngine.h"
#include "detection/ObjectCounter.h"
#include "detection/DefectDetector.h"
#include "detection/GeometryMeasurement.h"
#include "detection/DetectionExecutor.h"
#include "detection/DetectionTaskManager.h"
#include "ai/IAiProvider.h"
#include "ai/OpenAiCompatibleProvider.h"
#include "ai/AiConfigManager.h"
#include "ai/OperatorKnowledgeBase.h"
#include "ai/AiResponseValidator.h"
#include "ai/AiDetectionPlanner.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void Require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

class ThrowingOp final : public IOpNode
{
public:
    OpCategory GetCategory() const override { return OpCategory::ImageFilter; }
    std::string GetName() const override { return "ThrowingTest"; }
    std::string GetDisplayName() const override { return "Throwing Test"; }
    std::string GetDescription() const override { return "Test operator"; }
    std::string GetPrincipleDoc() const override { return "Test"; }
    bool Process(const cv::Mat&, cv::Mat&, std::string&) override
    {
        throw std::runtime_error("expected test exception");
    }
    std::vector<OpParameter>& GetParameters() override { return _parameters; }
    const std::vector<OpParameter>& GetParameters() const override { return _parameters; }
    std::string GenerateCppCode(const std::string&, const std::string&) const override { return ""; }
    std::string GeneratePythonCode(const std::string&, const std::string&) const override { return ""; }
    std::unique_ptr<IOpNode> Clone() const override { return std::make_unique<ThrowingOp>(*this); }

private:
    std::vector<OpParameter> _parameters;
};

void TestRegistryAndParameterValidation()
{
    OpRegistry::Instance().RegisterAllBuiltinOperators();
    Require(OpRegistry::Instance().GetAllOperators().size() >= 60, "expected at least 60 registered operators");

    GaussianBlurOp gaussian;
    Require(gaussian.SetParameter("ksize", 7), "valid integer parameter was rejected");
    Require(!gaussian.SetParameter("ksize", 1000), "out-of-range parameter was accepted");
    Require(!gaussian.SetParameter("ksize", 7.0), "wrong parameter type was accepted");
}

void TestGeneralOperators()
{
    std::string error;
    cv::Mat input(64, 64, CV_8UC3, cv::Scalar(10, 80, 220));
    cv::rectangle(input, cv::Rect(16, 16, 24, 24), cv::Scalar(200, 40, 20), -1);
    PipelineContext::Instance().SetSourceImage(input);

    ChannelMergeOp merge;
    Require(merge.SetParameter("blueSource", 2), "failed to configure channel merge");
    cv::Mat merged;
    Require(merge.Process(input, merged, error), "channel merge failed: " + error);
    Require(merged.type() == CV_8UC3 && merged.at<cv::Vec3b>(0, 0)[0] == 220, "channel merge result is incorrect");

    NormalizeOp normalize;
    cv::Mat normalized;
    Require(normalize.Process(input, normalized, error), "normalize failed: " + error);
    double minValue = 0.0;
    double maxValue = 0.0;
    cv::minMaxLoc(normalized.reshape(1), &minValue, &maxValue);
    Require(minValue == 0.0 && maxValue == 255.0, "MINMAX normalization range is incorrect");

    SeparableFilterOp separable;
    cv::Mat filtered;
    Require(separable.Process(input, filtered, error), "separable filter failed: " + error);
    Require(filtered.size() == input.size(), "separable filter changed image size");

    AffineTransformOp affine;
    cv::Mat transformed;
    Require(affine.Process(input, transformed, error), "affine transform failed: " + error);
    Require(cv::norm(input, transformed, cv::NORM_INF) == 0.0, "identity affine transform changed pixels");

    ImageStatisticsOp statistics;
    Require(statistics.SetParameter("drawOverlay", false), "failed to disable statistics overlay");
    cv::Mat statisticsOutput;
    Require(statistics.Process(input, statisticsOutput, error), "statistics failed: " + error);
    Require(statistics.GetResults().size() == 12, "statistics did not produce four results per channel");

    HistogramCompareOp compare;
    cv::Mat comparisonOutput;
    Require(compare.Process(input, comparisonOutput, error), "histogram comparison failed: " + error);
    Require(compare.GetResults().size() == 1, "histogram comparison score is missing");

    HistogramMatchOp histogramMatch;
    cv::Mat matched;
    Require(histogramMatch.Process(input, matched, error), "histogram matching failed: " + error);
    Require(matched.size() == input.size(), "histogram matching changed image size");

    cv::Mat shapeImage = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::circle(shapeImage, cv::Point(50, 50), 20, cv::Scalar(255), -1);
    MomentsHuOp moments;
    cv::Mat momentsOutput;
    Require(moments.Process(shapeImage, momentsOutput, error), "moments calculation failed: " + error);
    Require(moments.GetResults().size() == 10, "moments result set is incomplete");
}

void TestTemplateMatching()
{
    cv::Mat input = cv::Mat::zeros(120, 120, CV_8UC1);
    cv::Mat pattern = cv::Mat::zeros(16, 16, CV_8UC1);
    cv::rectangle(pattern, cv::Rect(2, 2, 5, 11), cv::Scalar(180), -1);
    cv::circle(pattern, cv::Point(11, 5), 3, cv::Scalar(255), -1);
    cv::line(pattern, cv::Point(8, 14), cv::Point(15, 8), cv::Scalar(90), 2);
    pattern.copyTo(input(cv::Rect(12, 18, pattern.cols, pattern.rows)));
    pattern.copyTo(input(cv::Rect(76, 72, pattern.cols, pattern.rows)));
    PipelineContext::Instance().SetSourceImage(input);

    MatchTemplateOp match;
    Require(match.SetParameter("method", 1), "failed to select SQDIFF_NORMED");
    Require(match.SetParameter("roiX", 12), "failed to set template X");
    Require(match.SetParameter("roiY", 18), "failed to set template Y");
    Require(match.SetParameter("roiW", pattern.cols), "failed to set template width");
    Require(match.SetParameter("roiH", pattern.rows), "failed to set template height");
    Require(match.SetParameter("scoreThreshold", 0.99), "failed to set template score threshold");

    std::string error;
    cv::Mat output;
    Require(match.Process(input, output, error), "single template matching failed: " + error);
    Require(match.GetResults().size() >= 4, "single template matching results are missing");

    Require(match.SetParameter("multiMatch", true), "failed to enable multi match");
    Require(match.Process(input, output, error), "multi template matching failed: " + error);
    const auto countResult = std::find_if(match.GetResults().begin(), match.GetResults().end(), [](const OpResultItem& item) {
        return item.Name == "match.count";
    });
    Require(countResult != match.GetResults().end(), "multi template count is missing");
    Require(std::get<std::int64_t>(countResult->Value) == 2, "multi template suppression returned duplicate matches");
}

void TestRansacCircle()
{
    cv::Mat circleImage = cv::Mat::zeros(180, 180, CV_8UC1);
    cv::circle(circleImage, cv::Point(90, 90), 45, cv::Scalar(255), 2, cv::LINE_AA);
    RansacFitCircleOp ransac;
    Require(ransac.SetParameter("maxIterations", 400), "failed to configure RANSAC iterations");
    Require(ransac.SetParameter("minRadius", 30), "failed to configure RANSAC minimum radius");
    Require(ransac.SetParameter("maxRadius", 60), "failed to configure RANSAC maximum radius");

    std::string error;
    cv::Mat output;
    Require(ransac.Process(circleImage, output, error), "RANSAC circle processing failed: " + error);
    Require(!ransac.GetResults().empty(), "RANSAC circle results are missing");
    const auto& results = ransac.GetResults();
    const auto radiusResult = std::find_if(results.begin(), results.end(), [](const OpResultItem& item) {
        return item.Name == "circle.radius";
    });
    Require(radiusResult != results.end(), "RANSAC did not find the synthetic circle");
    Require(std::abs(std::get<double>(radiusResult->Value) - 45.0) < 5.0, "RANSAC radius is outside tolerance");

    cv::Mat blankImage = cv::Mat::zeros(80, 80, CV_8UC1);
    Require(ransac.Process(blankImage, output, error), "RANSAC blank-image processing failed: " + error);
    const auto foundResult = std::find_if(ransac.GetResults().begin(), ransac.GetResults().end(), [](const OpResultItem& item) {
        return item.Name == "circle.found";
    });
    Require(foundResult != ransac.GetResults().end(), "RANSAC blank-image result is missing");
    Require(!std::get<bool>(foundResult->Value), "RANSAC reported a circle in a blank image");
}

void TestUtf8ImageIo()
{
    QTemporaryDir temporaryDirectory;
    Require(temporaryDirectory.isValid(), "failed to create temporary directory");
    const QString imagePath = temporaryDirectory.filePath(QString::fromUtf8("测试图像.png"));
    const cv::Mat expected(16, 20, CV_8UC3, cv::Scalar(15, 45, 90));

    std::string error;
    Require(ImageIoUtils::WriteImage(imagePath.toUtf8().constData(), expected, error), "UTF-8 image write failed: " + error);
    cv::Mat actual = ImageIoUtils::ReadImage(imagePath.toUtf8().constData(), cv::IMREAD_COLOR, error);
    Require(!actual.empty(), "UTF-8 image read failed: " + error);
    Require(cv::norm(expected, actual, cv::NORM_INF) == 0.0, "UTF-8 image round trip changed pixels");
}

void TestProjectRoundTripAndTransactionalLoad()
{
    PipelineManager sourcePipeline;
    auto match = std::make_unique<MatchTemplateOp>();
    const std::string templatePath = u8"D:/模板/参考图像.png";
    Require(match->SetParameter("tplSource", 2), "failed to set template source");
    Require(match->SetParameter("tplPath", templatePath), "failed to set template path");
    sourcePipeline.AddStep(std::move(match));

    QTemporaryDir temporaryDirectory;
    Require(temporaryDirectory.isValid(), "failed to create project temporary directory");
    const QString projectPath = temporaryDirectory.filePath("pipeline.json");
    QString error;
    Require(ProjectManager::SaveProject(sourcePipeline, projectPath, error), "project save failed: " + error.toStdString());

    PipelineManager loadedPipeline;
    Require(ProjectManager::LoadProject(loadedPipeline, projectPath, error), "project load failed: " + error.toStdString());
    Require(loadedPipeline.GetStepCount() == 1, "project step count changed after round trip");
    const OpParameter* loadedPath = loadedPipeline.GetStep(0)->GetOpNode()->FindParameter("tplPath");
    Require(loadedPath && loadedPath->GetString() == templatePath, "file path parameter was not preserved");

    const QString invalidPath = temporaryDirectory.filePath("invalid.json");
    QFile invalidFile(invalidPath);
    Require(invalidFile.open(QIODevice::WriteOnly | QIODevice::Text), "failed to create invalid project");
    invalidFile.write("{\"version\":\"1.0\",\"pipeline\":[{\"name\":\"UnknownOperator\",\"parameters\":{}}]}");
    invalidFile.close();
    Require(!ProjectManager::LoadProject(loadedPipeline, invalidPath, error), "invalid project was accepted");
    Require(loadedPipeline.GetStepCount() == 1, "failed project load modified the current pipeline");
}

void TestPipelineExceptionBoundary()
{
    PipelineManager pipeline;
    pipeline.SetSourceImage(cv::Mat(20, 20, CV_8UC1, cv::Scalar(100)));
    pipeline.AddStep(std::make_unique<ThrowingOp>());
    pipeline.AddStep(std::make_unique<NormalizeOp>());
    Require(!pipeline.Execute(true), "pipeline reported success after an operator exception");
    Require(pipeline.GetStep(0)->HasError(), "throwing step has no error message");
    Require(pipeline.GetStep(1)->GetOutputCache().empty(), "downstream cache was not cleared after failure");
}

void TestMatUtilsColorIntegrity()
{
    // 构造 OpenCV 格式 BGR 图像：左半边纯红 (B=0,G=0,R=255)，右半边纯蓝 (B=255,G=0,R=0)
    cv::Mat bgrMat(40, 40, CV_8UC3, cv::Scalar(0, 0, 255));
    cv::rectangle(bgrMat, cv::Rect(20, 0, 20, 40), cv::Scalar(255, 0, 0), -1);

    // 转换至 QImage
    QImage qimg = MatUtils::MatToQImage(bgrMat);
    Require(!qimg.isNull(), "MatToQImage returned null image");
    Require(qimg.width() == 40 && qimg.height() == 40, "QImage dimension mismatch");

    // 验证左半部红色区域 (Qt 中的 RGB 像素必须 Red=255, Blue=0, Green=0，严禁红蓝反转)
    QColor redPixel = qimg.pixelColor(5, 20);
    Require(redPixel.red() == 255 && redPixel.blue() == 0 && redPixel.green() == 0,
            "MatToQImage red-blue inverted: red pixel showed Blue=" + std::to_string(redPixel.blue()) + ", Red=" + std::to_string(redPixel.red()));

    // 验证右半部蓝色区域 (Qt 中的 RGB 像素必须 Blue=255, Red=0, Green=0)
    QColor bluePixel = qimg.pixelColor(25, 20);
    Require(bluePixel.blue() == 255 && bluePixel.red() == 0 && bluePixel.green() == 0,
            "MatToQImage red-blue inverted: blue pixel showed Red=" + std::to_string(bluePixel.red()) + ", Blue=" + std::to_string(bluePixel.blue()));

    // 验证从 QImage 还原回 cv::Mat
    cv::Mat roundTrip = MatUtils::QImageToMat(qimg);
    Require(!roundTrip.empty() && roundTrip.size() == bgrMat.size() && roundTrip.type() == bgrMat.type(),
            "QImageToMat failed to recreate correct cv::Mat");
    Require(cv::norm(bgrMat, roundTrip, cv::NORM_INF) == 0.0, "QImageToMat round-trip introduced pixel differences");
}

void TestContourVisibilityAndCanvasMode()
{
    std::string error;
    // 构造白底黑目标的二值图 (模拟二值化后纯白前景/背景)
    cv::Mat whiteBinary(100, 100, CV_8UC1, cv::Scalar(255));
    cv::rectangle(whiteBinary, cv::Rect(25, 25, 50, 50), cv::Scalar(0), -1);

    FindContoursOp op;
    Require(op.SetParameter("minArea", 100.0), "failed to set minArea");
    Require(op.SetParameter("thickness", 3), "failed to set line thickness");
    Require(op.SetParameter("highContrast", true), "failed to enable high contrast outline");

    // 1. 默认叠加模式
    Require(op.SetParameter("canvasMode", 0), "failed to set canvasMode overlay");
    cv::Mat outputOverlay;
    Require(op.Process(whiteBinary, outputOverlay, error), "FindContoursOp overlay process failed: " + error);
    Require(outputOverlay.type() == CV_8UC3 && outputOverlay.size() == whiteBinary.size(), "overlay output mismatch");

    // 2. 纯黑画布模式
    Require(op.SetParameter("canvasMode", 1), "failed to set canvasMode black canvas");
    cv::Mat outputBlack;
    Require(op.Process(whiteBinary, outputBlack, error), "FindContoursOp black canvas process failed: " + error);
    Require(outputBlack.type() == CV_8UC3 && outputBlack.size() == whiteBinary.size(), "black canvas output mismatch");

    // 3. 纯白画布模式
    Require(op.SetParameter("canvasMode", 2), "failed to set canvasMode white canvas");
    cv::Mat outputWhite;
    Require(op.Process(whiteBinary, outputWhite, error), "FindContoursOp white canvas process failed: " + error);
    Require(outputWhite.type() == CV_8UC3 && outputWhite.size() == whiteBinary.size(), "white canvas output mismatch");

    // 验证代码生成
    std::string cppCode = op.GenerateCppCode("srcMat", "dstMat");
    Require(cppCode.find("lineThickness") != std::string::npos, "Cpp code missing lineThickness");
    std::string pyCode = op.GeneratePythonCode("src_mat", "dst_mat");
    Require(pyCode.find("line_thickness") != std::string::npos, "Python code missing line_thickness");
}

void TestAppSettingsPathPersistence()
{
    QTemporaryDir tempDir;
    Require(tempDir.isValid(), "failed to create temporary dir for settings test");
    const QString fakeFilePath = tempDir.filePath("test_sample_image.png");

    // 1. 设置文件路径，应自动提取目录
    AppSettings::SetLastDirectory(AppSettings::KeyImageDir, fakeFilePath);
    QString retrievedDir = AppSettings::GetLastDirectory(AppSettings::KeyImageDir);
    Require(!retrievedDir.isEmpty(), "AppSettings returned empty directory");
    Require(QDir(retrievedDir).absolutePath() == QDir(tempDir.path()).absolutePath(),
            "AppSettings did not store expected parent directory");

    // 2. 获取默认拼接路径
    QString defaultFilePath = AppSettings::GetDefaultFilePath(AppSettings::KeyImageDir, "result.png");
    Require(defaultFilePath.endsWith("result.png"), "AppSettings GetDefaultFilePath returned unexpected filename");
}

void TestPipelineBulkEnableDisable()
{
    PipelineManager pipeline;
    pipeline.SetSourceImage(cv::Mat(50, 50, CV_8UC1, cv::Scalar(128)));

    auto op1 = std::make_unique<GaussianBlurOp>();
    auto op2 = std::make_unique<NormalizeOp>();
    pipeline.AddStep(std::move(op1));
    pipeline.AddStep(std::move(op2));
    Require(pipeline.GetStepCount() == 2, "pipeline step count mismatch");

    // 一键取消全选/全部禁用
    pipeline.SetAllStepsEnabled(false);
    Require(!pipeline.GetStep(0)->IsEnabled(), "step 0 should be disabled");
    Require(!pipeline.GetStep(1)->IsEnabled(), "step 1 should be disabled");

    // 一键全选/全部启用
    pipeline.SetAllStepsEnabled(true);
    Require(pipeline.GetStep(0)->IsEnabled(), "step 0 should be enabled");
    Require(pipeline.GetStep(1)->IsEnabled(), "step 1 should be enabled");
}

void TestInteractiveMaskMultipleShapes()
{
    cv::Mat input(120, 120, CV_8UC3, cv::Scalar(50, 50, 50));
    std::string error;

    // 1. 测试圆环 (Ring)
    InteractiveMaskOp ringOp;
    Require(ringOp.SetParameter("shape", static_cast<int>(MaskShape::Ring)), "failed to set shape to Ring");
    Require(ringOp.SetParameter("roiX", 20), "failed to set roiX");
    Require(ringOp.SetParameter("roiY", 20), "failed to set roiY");
    Require(ringOp.SetParameter("roiW", 80), "failed to set roiW");
    Require(ringOp.SetParameter("roiH", 80), "failed to set roiH");
    Require(ringOp.SetParameter("innerRatio", 50), "failed to set innerRatio");
    Require(ringOp.SetParameter("action", static_cast<int>(MaskAction::OutputMaskOnly)), "failed to set action OutputMaskOnly");

    cv::Mat ringMask;
    Require(ringOp.Process(input, ringMask, error), "Ring mask generation failed: " + error);
    Require(!ringMask.empty() && ringMask.size() == input.size(), "Ring mask size mismatch");

    // 2. 测试正六边形 (Regular Polygon 6 sides)
    InteractiveMaskOp polyOp;
    Require(polyOp.SetParameter("shape", static_cast<int>(MaskShape::RegularPolygon)), "failed to set shape to Polygon");
    Require(polyOp.SetParameter("polySides", 6), "failed to set polySides to 6");
    Require(polyOp.SetParameter("roiAngle", 30), "failed to set roiAngle to 30");

    cv::Mat polyMask;
    Require(polyOp.Process(input, polyMask, error), "Polygon mask generation failed: " + error);
    Require(!polyMask.empty() && polyMask.size() == input.size(), "Polygon mask size mismatch");

    // 3. 测试旋转矩形 (Rotated Rectangle)
    InteractiveMaskOp rotOp;
    Require(rotOp.SetParameter("shape", static_cast<int>(MaskShape::RotatedRect)), "failed to set shape to RotatedRect");
    Require(rotOp.SetParameter("roiAngle", 45), "failed to set roiAngle to 45");

    cv::Mat rotMask;
    Require(rotOp.Process(input, rotMask, error), "Rotated rect mask generation failed: " + error);
    Require(!rotMask.empty() && rotMask.size() == input.size(), "Rotated rect mask size mismatch");
}

void TestBranchPipelineAndDualImageArithmetic()
{
    std::string error;
    cv::Mat srcImg(100, 100, CV_8UC3, cv::Scalar(50, 50, 50));
    // 在中心画一个亮色方块
    cv::rectangle(srcImg, cv::Rect(30, 30, 40, 40), cv::Scalar(200, 200, 200), -1);

    PipelineManager pipeline;
    pipeline.SetSourceImage(srcImg);

    // 1. 在主流水线添加灰度化算子
    auto grayOp = OpRegistry::Instance().Create("CvtColor");
    Require(grayOp != nullptr, "failed to create CvtColor operator");
    Require(grayOp->SetParameter("code", 0), "failed to set code to BGR2GRAY");
    pipeline.AddStep(std::move(grayOp));

    // 2. 一键创建新分支 "分支 1"
    int b1Idx = pipeline.CreateBranch("分支 1");
    Require(b1Idx == 1, "created branch index mismatch");
    Require(pipeline.GetBranchCount() == 2, "branch count mismatch");

    // 3. 在 "分支 1" 中添加 Canny 边缘检测
    auto cannyOp = OpRegistry::Instance().Create("Canny");
    Require(cannyOp != nullptr, "failed to create Canny operator");
    pipeline.AddStep(std::move(cannyOp));

    // 4. 切换回主流水线 (Branch 0)
    pipeline.SetActiveBranchIndex(0);
    Require(pipeline.GetActiveBranchIndex() == 0, "failed to switch back to Main branch");

    // 5. 在主流水线添加双图复合算子 (合并目标选 分支 1，模式选 AbsDiff 差分)
    auto blendOp = OpRegistry::Instance().Create("BlendWithSource");
    Require(blendOp != nullptr, "failed to create BlendWithSource operator");
    Require(blendOp->SetParameter("secondSource", 1), "failed to set secondSource to Branch 1"); // 1: 分支 1
    Require(blendOp->SetParameter("opMode", 1), "failed to set opMode to AbsDiff"); // 1: AbsDiff
    pipeline.AddStep(std::move(blendOp));

    // 6. 执行整个多分支流水线
    Require(pipeline.Execute(true), "multi-branch pipeline execution failed");

    cv::Mat mainFinal = pipeline.GetFinalOutput();
    Require(!mainFinal.empty(), "main pipeline output is empty");
    Require(mainFinal.size() == srcImg.size(), "main pipeline output size mismatch");

    cv::Mat b1Final = pipeline.GetBranchFinalOutput(1);
    Require(!b1Final.empty(), "branch 1 output is empty");

    // 7. 测试分支流程修改时，主流水线的复合结果自动级联刷新
    cv::Mat oldMainOut = mainFinal.clone();
    // 切换到分支 1 并禁用 Canny
    pipeline.SetActiveBranchIndex(1);
    PipelineStep* b1CannyStep = pipeline.GetStep(0);
    Require(b1CannyStep != nullptr, "b1 canny step is null");
    b1CannyStep->SetEnabled(false);
    pipeline.MarkDirtyFrom(0);
    Require(pipeline.Execute(), "pipeline execute failed after branch modification");

    // 切换回主流水线验证结果是否已经刷新（不再等于旧复合图）
    pipeline.SetActiveBranchIndex(0);
    cv::Mat newMainOut = pipeline.GetFinalOutput();
    Require(!newMainOut.empty(), "new main output is empty");
    cv::Mat diff;
    cv::absdiff(oldMainOut, newMainOut, diff);
    Require(cv::countNonZero(diff.reshape(1)) > 0, "main pipeline failed to cascade update when branch 1 was modified");

    // 8. 测试一键全选与全部取消切换
    pipeline.SetActiveBranchIndex(1);
    Require(!pipeline.AreAllStepsEnabled(), "branch 1 has a disabled step");
    pipeline.SetAllStepsEnabled(true);
    Require(pipeline.AreAllStepsEnabled(), "branch 1 steps should be re-enabled");

    // 9. 测试多分支工程持久化与导入
    QTemporaryDir tempDir;
    QString projFile = tempDir.filePath("branch_project.json");
    QString saveErr;
    Require(ProjectManager::SaveProject(pipeline, projFile, saveErr), "SaveProject failed: " + saveErr.toStdString());

    PipelineManager reloadedPipeline;
    reloadedPipeline.SetSourceImage(srcImg);
    QString loadErr;
    Require(ProjectManager::LoadProject(reloadedPipeline, projFile, loadErr), "LoadProject failed: " + loadErr.toStdString());
    Require(reloadedPipeline.GetBranchCount() == 2, "reloaded branch count mismatch");
    Require(reloadedPipeline.GetBranchName(1) == "分支 1", "reloaded branch name mismatch");

    // 9. 验证 C++ 与 Python 代码生成包含分支信息
    CppCodeGenerator cppGen;
    std::string cppCode = cppGen.Generate(pipeline, "test.png");
    Require(cppCode.find("branch_1") != std::string::npos, "C++ code missing branch_1");
    Require(cppCode.find("main_pipeline_out") != std::string::npos, "C++ code missing main_pipeline_out");

    PythonCodeGenerator pyGen;
    std::string pyCode = pyGen.Generate(pipeline, "test.png");
    Require(pyCode.find("branch_1") != std::string::npos, "Python code missing branch_1");
    Require(pyCode.find("main_pipeline_out") != std::string::npos, "Python code missing main_pipeline_out");
}

void TestOperatorMetadataAndValidation()
{
    OperatorMetadata meta = OpRegistry::Instance().GetOperatorMetadata("GaussianBlur");
    Require(!meta.Name.empty(), "expected GaussianBlur metadata");
    Require(meta.SupportsMultiThread, "expected multi-thread support flag");
    Require(!meta.Parameters.empty(), "expected GaussianBlur parameters");

    QJsonArray metaJson = OpRegistry::Instance().ExportAllMetadataJson();
    Require(metaJson.size() >= 60, "expected at least 60 exported operator metadata items");

    // 校验合法流水线
    PipelineManager pipeline;
    auto cvt = OpRegistry::Instance().Create("CvtColor");
    auto blur = OpRegistry::Instance().Create("GaussianBlur");
    auto canny = OpRegistry::Instance().Create("Canny");
    pipeline.AddStep(std::move(cvt));
    pipeline.AddStep(std::move(blur));
    pipeline.AddStep(std::move(canny));

    cv::Mat colorImg(100, 100, CV_8UC3, cv::Scalar(128, 128, 128));
    auto report = PipelineValidator::Validate(pipeline, colorImg);
    Require(report.IsValid, "expected valid pipeline report");

    // 校验偶数核大小等参数违背
    PipelineManager invalidPipeline;
    auto badBlur = OpRegistry::Instance().Create("GaussianBlur");
    auto* param = badBlur->FindParameter("ksize");
    if (param)
    {
        param->Value = 4; // 偶数核大小非法
    }
    invalidPipeline.AddStep(std::move(badBlur));
    auto badReport = PipelineValidator::Validate(invalidPipeline, colorImg);
    Require(!badReport.IsValid, "expected invalid pipeline report for even kernel size");
    Require(badReport.ErrorCount > 0, "expected error count > 0");
}

void TestObjectCounter()
{
    // 生成 12 个独立瓶盖/零件靶标演示图
    cv::Mat img(400, 500, CV_8UC3, cv::Scalar(20, 20, 20));
    for (int r = 0; r < 3; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            cv::Point center(60 + c * 110, 60 + r * 110);
            cv::circle(img, center, 32, cv::Scalar(240, 240, 240), -1);
            cv::circle(img, center, 20, cv::Scalar(160, 160, 160), -1);
        }
    }

    ObjectCountConfig cfg;
    cfg.Method = "ConnectedComponents";
    cfg.ThresholdMethod = "Otsu";
    cfg.MinArea = 50.0;
    cfg.MaxArea = 500000.0;
    cfg.TargetClass = "bottle_cap";

    DetectionResult result;
    std::string err;
    Require(ObjectCounter::Execute(img, cfg, result, err), "ObjectCounter failed: " + err);
    Require(result.ObjectCount == 12, "expected exactly 12 detected objects, got: " + std::to_string(result.ObjectCount));
    Require(result.Objects.size() == 12, "objects vector size mismatch");
    Require(!result.Visualization.empty(), "expected non-empty visualization");

    // 规则评估: expectedCount == 12 -> PASS
    QJsonArray passRules;
    QJsonObject r1;
    r1["name"] = "满数12个检验";
    r1["ruleType"] = "count";
    r1["operator"] = "==";
    r1["expectedCount"] = 12;
    passRules.append(r1);

    InspectionDecision passDecision = InspectionRuleEngine::Evaluate(result, passRules);
    Require(passDecision.IsOK(), "expected inspection pass for 12 objects");
    Require(passDecision.Status == InspectionStatus::Pass, "status should be Pass");

    // 规则评估: expectedCount == 10 -> FAIL / NG
    QJsonArray failRules;
    QJsonObject r2;
    r2["name"] = "满数10个检验";
    r2["ruleType"] = "count";
    r2["operator"] = "==";
    r2["expectedCount"] = 10;
    failRules.append(r2);

    InspectionDecision failDecision = InspectionRuleEngine::Evaluate(result, failRules);
    Require(!failDecision.IsOK(), "expected inspection fail for mismatching count");
    Require(failDecision.Status == InspectionStatus::Fail, "status should be Fail");
    Require(!failDecision.Reasons.empty(), "reasons should not be empty");
}

void TestDefectDetector()
{
    // 生成金属工件表面与缺陷模拟图 (底色灰度 140)
    // 包含 1 处长划痕与 2 处凹坑
    cv::Mat workpiece(300, 400, CV_8UC3, cv::Scalar(140, 140, 140));
    // 细长暗色划痕
    cv::line(workpiece, cv::Point(50, 50), cv::Point(180, 70), cv::Scalar(30, 30, 30), 2);
    // 暗色凹坑
    cv::circle(workpiece, cv::Point(280, 120), 8, cv::Scalar(25, 25, 25), -1);
    // 亮色凹坑/反光点
    cv::circle(workpiece, cv::Point(150, 220), 7, cv::Scalar(250, 250, 250), -1);

    DefectDetectConfig cfg;
    cfg.Method = "BackgroundSubtraction";
    cfg.BackgroundBlurKernel = 31;
    cfg.DiffThreshold = 25;
    cfg.MinDefectArea = 10.0;
    cfg.MinDefectLength = 4.0;

    DetectionResult result;
    std::string err;
    Require(DefectDetector::Execute(workpiece, cfg, result, err), "DefectDetector failed: " + err);
    Require(result.DefectCount >= 3, "expected at least 3 defects found, got: " + std::to_string(result.DefectCount));

    bool foundScratch = false;
    bool foundPit = false;
    for (const auto& d : result.Defects)
    {
        if (d.DefectType == "scratch")
        {
            foundScratch = true;
        }
        else if (d.DefectType == "pit")
        {
            foundPit = true;
        }
    }
    Require(foundScratch, "expected scratch defect to be identified");
    Require(foundPit, "expected pit defect to be identified");

    // 业务规则判定: 零划痕限制
    QJsonArray rules;
    QJsonObject r1;
    r1["name"] = "零划痕限制";
    r1["ruleType"] = "defect";
    r1["defectClass"] = "scratch";
    r1["maxCount"] = 0;
    rules.append(r1);

    InspectionDecision decision = InspectionRuleEngine::Evaluate(result, rules);
    Require(!decision.IsOK(), "expected fail on scratch detected");
    Require(decision.Status == InspectionStatus::Fail, "decision status should be Fail");
}

void TestGeometryMeasurement()
{
    cv::Mat canvas(300, 300, CV_8UC3, cv::Scalar(0, 0, 0));
    // 绘制清晰实心白色圆 (半径 40，直径 80)
    cv::circle(canvas, cv::Point(150, 150), 40, cv::Scalar(255, 255, 255), -1);

    MeasurementConfig cfg;
    cfg.Mode = "FindCircles";
    cfg.MinRadius = 20.0;
    cfg.MaxRadius = 60.0;
    cfg.PixelToMmRatio = 0.5; // 0.5 mm/px -> 直径 80 px = 40 mm

    DetectionResult result;
    std::string err;
    Require(GeometryMeasurement::Execute(canvas, cfg, result, err), "GeometryMeasurement failed: " + err);
    Require(result.Measurements.contains("Circle_1"), "expected Circle_1 in measurements");
    double dMm = result.Measurements["Circle_1"].toObject()["diameterMm"].toDouble();
    Require(std::abs(dMm - 40.0) < 5.0, "circle diameter measurement out of range");
}

void TestProjectManagerV2WithTasks()
{
    cv::Mat dummy(100, 100, CV_8UC3, cv::Scalar(100, 100, 100));
    PipelineManager pipeline;
    pipeline.SetSourceImage(dummy);
    pipeline.AddStep(OpRegistry::Instance().Create("GaussianBlur"));

    DetectionTaskManager taskManager;
    taskManager.Clear();
    taskManager.AddTask(DetectionTaskManager::CreateDefaultCountTask(24));
    taskManager.AddTask(DetectionTaskManager::CreateDefaultDefectTask());

    QTemporaryDir tempDir;
    QString projPath = tempDir.filePath("v2_project.json");
    QString saveErr;
    Require(ProjectManager::SaveProject(pipeline, &taskManager, projPath, saveErr), "SaveProject V2 failed: " + saveErr.toStdString());

    PipelineManager reloadedPipeline;
    DetectionTaskManager reloadedTaskManager;
    reloadedTaskManager.Clear();
    QString loadErr;
    Require(ProjectManager::LoadProject(reloadedPipeline, &reloadedTaskManager, projPath, loadErr), "LoadProject V2 failed: " + loadErr.toStdString());
    Require(reloadedTaskManager.GetTaskCount() == 2, "expected 2 loaded detection tasks");
    Require(reloadedTaskManager.GetTask(0)->Type == DetectionTaskType::ObjectCounting, "task 0 should be ObjectCounting");
    Require(reloadedTaskManager.GetTask(1)->Type == DetectionTaskType::DefectDetection, "task 1 should be DefectDetection");
}

void TestAiResponseValidator()
{
    // 1. 验证目标计数任务 JSON 解析与参数自愈
    QString validAiCountingJson = R"(
    Here is the recommended configuration:
    ```json
    {
      "taskType": "ObjectCounting",
      "name": "药片计数检测",
      "configuration": {
        "method": "ConnectedComponents",
        "thresholdMethod": "Otsu",
        "morphKernelSize": 4,
        "minArea": 30.0
      },
      "rules": [
        {
          "name": "数量要求24个",
          "ruleType": "count",
          "operator": "==",
          "expectedCount": 24
        }
      ]
    }
    ```
    )";

    ValidatedAiPlan plan = AiResponseValidator::ValidateAndParse(validAiCountingJson);
    Require(plan.IsValid, "expected valid AI plan");
    Require(plan.HasDetectionTask, "expected detection task");
    Require(plan.Task.Name == "药片计数检测", "task name mismatch");
    Require(plan.Task.Configuration["morphKernelSize"].toInt() == 5, "kernel size should be adjusted to odd");

    // 2. 验证未知算子拦截
    QString hallucinatedPipeline = R"(
    ```json
    {
      "pipeline": [
        {"name": "NonExistentSuperAiFilter", "parameters": {}}
      ]
    }
    ```
    )";
    ValidatedAiPlan badPlan = AiResponseValidator::ValidateAndParse(hallucinatedPipeline);
    Require(!badPlan.IsValid, "hallucinated operator should be rejected");
}

void TestLiveAiProviderAndModelsFetch()
{
    QString baseUrl = "https://api.tokenskingdom.com/v1";
    QString apiKey = "sk-ec53d4c931f986190063a14a5ab287bfa443971621ce97f52d4622c951811ffb";

    OpenAiCompatibleProvider provider(baseUrl, apiKey);

    QEventLoop loop;
    bool reqSuccess = false;
    QStringList modelsList;
    QString reqError;

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    provider.FetchModelsAsync([&](bool success, const QStringList& models, const QString& error) {
        reqSuccess = success;
        modelsList = models;
        reqError = error;
        loop.quit();
    });

    timer.start(15000);
    loop.exec();

    Require(reqSuccess, "Live API model fetch failed: " + reqError.toStdString());
    Require(!modelsList.isEmpty(), "models list should not be empty");
    std::cout << "Live AI models auto-fetched (" << modelsList.size() << " models available): ";
    for (int i = 0; i < std::min(5, (int)modelsList.size()); ++i)
    {
        std::cout << modelsList[i].toStdString() << " ";
    }
    std::cout << std::endl;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    try
    {
        TestRegistryAndParameterValidation();
        TestGeneralOperators();
        TestTemplateMatching();
        TestRansacCircle();
        TestUtf8ImageIo();
        TestProjectRoundTripAndTransactionalLoad();
        TestPipelineExceptionBoundary();
        TestMatUtilsColorIntegrity();
        TestContourVisibilityAndCanvasMode();
        TestAppSettingsPathPersistence();
        TestPipelineBulkEnableDisable();
        TestInteractiveMaskMultipleShapes();
        TestBranchPipelineAndDualImageArithmetic();
        TestOperatorMetadataAndValidation();
        TestObjectCounter();
        TestDefectDetector();
        TestGeometryMeasurement();
        TestProjectManagerV2WithTasks();
        TestAiResponseValidator();
        TestLiveAiProviderAndModelsFetch();
        std::cout << "All VisionStudio core tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Test failure: " << ex.what() << std::endl;
        return 1;
    }
}
