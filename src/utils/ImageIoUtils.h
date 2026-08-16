#pragma once

#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 支持 UTF-8 Windows 路径的 OpenCV 图像文件读写工具
 */
class ImageIoUtils
{
public:
    /**
     * @brief 从 UTF-8 路径读取图像
     * @param utf8Path UTF-8 编码的文件路径
     * @param flags OpenCV ImreadModes 标志
     * @param errorMsg 失败原因
     */
    static cv::Mat ReadImage(const std::string& utf8Path, int flags, std::string& errorMsg);

    /**
     * @brief 将图像写入 UTF-8 路径
     * @param utf8Path UTF-8 编码的目标路径，扩展名决定编码格式
     * @param image 待写入图像
     * @param errorMsg 失败原因
     */
    static bool WriteImage(const std::string& utf8Path, const cv::Mat& image, std::string& errorMsg);
};
