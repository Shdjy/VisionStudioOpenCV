#include "core/OpRegistry.h"
#include "core/PipelineContext.h"
#include "core/operators/FeatureOps.h"
#include "core/operators/FilterOps.h"
#include "core/operators/GeneralOps.h"
#include "core/operators/IndustrialOps.h"
#include "bll/PipelineManager.h"
#include "bll/ProjectManager.h"
#include "utils/ImageIoUtils.h"
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
        std::cout << "All VisionStudio core tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Test failure: " << ex.what() << std::endl;
        return 1;
    }
}
