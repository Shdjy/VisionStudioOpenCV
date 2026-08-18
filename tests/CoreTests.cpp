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
#include "utils/ImageIoUtils.h"
#include "utils/MatUtils.h"
#include "utils/AppSettings.h"
#include <QCoreApplication>
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
        std::cout << "All VisionStudio core tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Test failure: " << ex.what() << std::endl;
        return 1;
    }
}
