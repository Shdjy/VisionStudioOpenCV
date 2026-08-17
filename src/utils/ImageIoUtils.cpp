#include "ImageIoUtils.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

cv::Mat ImageIoUtils::ReadImage(const std::string& utf8Path, int flags, std::string& errorMsg)
{
    errorMsg.clear();
    if (utf8Path.empty())
    {
        errorMsg = "图像路径为空";
        return cv::Mat();
    }

    try
    {
        const std::filesystem::path path = std::filesystem::u8path(utf8Path);
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream)
        {
            errorMsg = "无法打开图像文件: " + utf8Path;
            return cv::Mat();
        }

        const std::streamsize fileSize = stream.tellg();
        if (fileSize <= 0)
        {
            errorMsg = "图像文件为空或大小为 0 字节";
            return cv::Mat();
        }

        stream.seekg(0, std::ios::beg);

        // 预分配内存进行高性能连续块读取 (大幅提升数十MB至GB级别线扫大图的载入吞吐率)
        std::vector<unsigned char> bytes(static_cast<size_t>(fileSize));
        if (!stream.read(reinterpret_cast<char*>(bytes.data()), fileSize))
        {
            errorMsg = "读取图像文件数据流失败";
            return cv::Mat();
        }

        cv::Mat image = cv::imdecode(bytes, flags);
        if (image.empty())
        {
            errorMsg = "OpenCV 无法解码该图像格式，请检查文件完整性";
        }
        return image;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
    }
    catch (const std::exception& ex)
    {
        errorMsg = ex.what();
    }
    return cv::Mat();
}

bool ImageIoUtils::WriteImage(const std::string& utf8Path, const cv::Mat& image, std::string& errorMsg)
{
    errorMsg.clear();
    if (utf8Path.empty() || image.empty())
    {
        errorMsg = "输出路径或图像为空";
        return false;
    }

    try
    {
        const std::filesystem::path path = std::filesystem::u8path(utf8Path);
        const std::string extension = path.extension().u8string();
        if (extension.empty())
        {
            errorMsg = "输出路径缺少图像扩展名";
            return false;
        }

        std::vector<unsigned char> bytes;
        if (!cv::imencode(extension, image, bytes))
        {
            errorMsg = "OpenCV 无法编码该图像格式";
            return false;
        }

        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            errorMsg = "无法创建输出图像文件";
            return false;
        }
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!stream)
        {
            errorMsg = "图像文件写入不完整";
            return false;
        }
        return true;
    }
    catch (const cv::Exception& ex)
    {
        errorMsg = ex.what();
    }
    catch (const std::exception& ex)
    {
        errorMsg = ex.what();
    }
    return false;
}
