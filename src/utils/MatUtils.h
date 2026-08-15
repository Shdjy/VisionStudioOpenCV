#pragma once

#include <opencv2/opencv.hpp>
#include <QImage>
#include <QPixmap>
#include <string>
#include <vector>

/**
 * @brief 像素采样探针信息
 */
struct PixelProbeInfo
{
    int X = 0;
    int Y = 0;
    int Channels = 0;
    int Depth = 0;
    int GrayVal = 0;
    int R = 0;
    int G = 0;
    int B = 0;
    int H = 0;
    int S = 0;
    int V = 0;
    std::string FormattedText;
};

/**
 * @brief OpenCV Mat 与 Qt 图像及格式转换工具类
 */
class MatUtils
{
public:
    /**
     * @brief 将 cv::Mat 转换为 QImage (深拷贝安全机制)
     * @param mat 输入 OpenCV 矩阵
     * @return 转换后的 QImage
     */
    static QImage MatToQImage(const cv::Mat& mat);

    /**
     * @brief 将 QImage 转换为 cv::Mat
     * @param image 输入 QImage
     * @return 转换后的 cv::Mat
     */
    static cv::Mat QImageToMat(const QImage& image);

    /**
     * @brief 提取指定坐标处的像素探针详细信息
     * @param mat 图像矩阵
     * @param x X 坐标
     * @param y Y 坐标
     * @return 结构化的探针信息
     */
    static PixelProbeInfo GetPixelInfo(const cv::Mat& mat, int x, int y);

    /**
     * @brief 图像类型与通道校验
     * @param mat 输入矩阵
     * @param expectedChannels 期望的通道数 (0表示任意)
     * @param errorMsg 错误提示
     * @return 校验是否通过
     */
    static bool ValidateMat(const cv::Mat& mat, int expectedChannels, std::string& errorMsg);

    /**
     * @brief 计算图像的颜色直方图（灰度或BGR三通道）
     * @param mat 输入矩阵
     * @param histData 输出的直方图统计数据（归一化为0~1）
     * @return 是否计算成功
     */
    static bool CalculateHistogram(const cv::Mat& mat, std::vector<std::vector<float>>& histData);
};
