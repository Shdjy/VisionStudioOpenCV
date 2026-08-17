#include "MatUtils.h"
#include <sstream>
#include <iomanip>

QString MatUtils::GetMatTypeString(const cv::Mat& mat)
{
    if (mat.empty())
    {
        return "Empty";
    }

    uchar depth = mat.type() & CV_MAT_DEPTH_MASK;
    uchar chans = 1 + (mat.type() >> CV_CN_SHIFT);

    QString r;
    switch (depth)
    {
        case CV_8U:  r = "8U"; break;
        case CV_8S:  r = "8S"; break;
        case CV_16U: r = "16U"; break;
        case CV_16S: r = "16S"; break;
        case CV_32S: r = "32S"; break;
        case CV_32F: r = "32F"; break;
        case CV_64F: r = "64F"; break;
        default:     r = "User"; break;
    }

    r += QString("C%1").arg(chans);
    return r;
}

QImage MatUtils::MatToQImage(const cv::Mat& mat)
{
    if (mat.empty())
    {
        return QImage();
    }

    switch (mat.type())
    {
        case CV_8UC1:
        {
            QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
            return image.copy();
        }
        case CV_8UC3:
        {
            // OpenCV 默认内存排布为 BGR，Qt 的 Format_BGR888 与之完全一致，直接安全深拷贝
            QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_BGR888);
            return image.copy();
        }
        case CV_8UC4:
        {
            // OpenCV BGRA 在小端机器上对应 Qt 的 Format_ARGB32 (BGRA)
            QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_ARGB32);
            return image.copy();
        }
        case CV_32FC1:
        {
            cv::Mat normalized;
            mat.convertTo(normalized, CV_8U, 255.0);
            QImage image(normalized.data, normalized.cols, normalized.rows, static_cast<int>(normalized.step), QImage::Format_Grayscale8);
            return image.copy();
        }
        default:
        {
            // 通用转换：转成 8 位无符号
            cv::Mat converted;
            if (mat.channels() == 1)
            {
                mat.convertTo(converted, CV_8U);
                QImage image(converted.data, converted.cols, converted.rows, static_cast<int>(converted.step), QImage::Format_Grayscale8);
                return image.copy();
            }
            else if (mat.channels() == 3)
            {
                mat.convertTo(converted, CV_8UC3);
                QImage image(converted.data, converted.cols, converted.rows, static_cast<int>(converted.step), QImage::Format_BGR888);
                return image.copy();
            }
            else if (mat.channels() == 4)
            {
                mat.convertTo(converted, CV_8UC4);
                QImage image(converted.data, converted.cols, converted.rows, static_cast<int>(converted.step), QImage::Format_ARGB32);
                return image.copy();
            }
            break;
        }
    }

    return QImage();
}

cv::Mat MatUtils::QImageToMat(const QImage& image)
{
    if (image.isNull())
    {
        return cv::Mat();
    }

    switch (image.format())
    {
        case QImage::Format_Grayscale8:
        case QImage::Format_Indexed8:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine()));
            return mat.clone();
        }
        case QImage::Format_BGR888:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC3, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine()));
            return mat.clone();
        }
        case QImage::Format_RGB888:
        {
            QImage swapped = image.rgbSwapped();
            cv::Mat mat(swapped.height(), swapped.width(), CV_8UC3, const_cast<uchar*>(swapped.bits()), static_cast<size_t>(swapped.bytesPerLine()));
            return mat.clone();
        }
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
        case QImage::Format_RGB32:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar*>(image.bits()), static_cast<size_t>(image.bytesPerLine()));
            cv::Mat matBgr;
            cv::cvtColor(mat, matBgr, cv::COLOR_BGRA2BGR);
            return matBgr;
        }
        default:
        {
            QImage converted = image.convertToFormat(QImage::Format_BGR888);
            cv::Mat mat(converted.height(), converted.width(), CV_8UC3, const_cast<uchar*>(converted.bits()), static_cast<size_t>(converted.bytesPerLine()));
            return mat.clone();
        }
    }
}

PixelProbeInfo MatUtils::GetPixelInfo(const cv::Mat& mat, int x, int y)
{
    PixelProbeInfo info;
    info.X = x;
    info.Y = y;

    if (mat.empty() || x < 0 || x >= mat.cols || y < 0 || y >= mat.rows)
    {
        info.FormattedText = "位置超出图像范围";
        return info;
    }

    info.Channels = mat.channels();
    info.Depth = mat.depth();

    std::stringstream ss;
    ss << "坐标: (" << x << ", " << y << ") | ";

    if (mat.channels() == 1)
    {
        uint8_t val = 0;
        if (mat.depth() == CV_8U)
        {
            val = mat.at<uint8_t>(y, x);
        }
        else if (mat.depth() == CV_32F)
        {
            val = static_cast<uint8_t>(mat.at<float>(y, x) * 255.0f);
        }
        info.GrayVal = val;
        info.R = val;
        info.G = val;
        info.B = val;
        ss << "灰度: " << static_cast<int>(val);
    }
    else if (mat.channels() == 3)
    {
        cv::Vec3b bgr = mat.at<cv::Vec3b>(y, x);
        info.B = bgr[0];
        info.G = bgr[1];
        info.R = bgr[2];
        info.GrayVal = static_cast<int>(0.299 * info.R + 0.587 * info.G + 0.114 * info.B);

        // 计算 HSV
        cv::Mat bgrMat(1, 1, CV_8UC3, cv::Scalar(info.B, info.G, info.R));
        cv::Mat hsvMat;
        cv::cvtColor(bgrMat, hsvMat, cv::COLOR_BGR2HSV);
        cv::Vec3b hsv = hsvMat.at<cv::Vec3b>(0, 0);
        info.H = hsv[0] * 2; // 0-360
        info.S = static_cast<int>(hsv[1] / 255.0 * 100); // 0-100%
        info.V = static_cast<int>(hsv[2] / 255.0 * 100); // 0-100%

        ss << "RGB: (" << info.R << ", " << info.G << ", " << info.B << ")"
           << " | HSV: (" << info.H << "°, " << info.S << "%, " << info.V << "%)";
    }
    else if (mat.channels() == 4)
    {
        cv::Vec4b bgra = mat.at<cv::Vec4b>(y, x);
        info.B = bgra[0];
        info.G = bgra[1];
        info.R = bgra[2];
        ss << "RGBA: (" << info.R << ", " << info.G << ", " << info.B << ", " << static_cast<int>(bgra[3]) << ")";
    }

    info.FormattedText = ss.str();
    return info;
}

bool MatUtils::ValidateMat(const cv::Mat& mat, int expectedChannels, std::string& errorMsg)
{
    if (mat.empty())
    {
        errorMsg = "输入图像为空 (Mat is empty)";
        return false;
    }

    if (expectedChannels > 0 && mat.channels() != expectedChannels)
    {
        std::stringstream ss;
        ss << "图像通道不匹配: 期望 " << expectedChannels << " 通道，当前为 " << mat.channels() << " 通道";
        errorMsg = ss.str();
        return false;
    }

    return true;
}

bool MatUtils::CalculateHistogram(const cv::Mat& mat, std::vector<std::vector<float>>& histData)
{
    histData.clear();
    if (mat.empty())
    {
        return false;
    }

    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };
    bool uniform = true;
    bool accumulate = false;

    if (mat.channels() == 1)
    {
        cv::Mat hist;
        cv::calcHist(&mat, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
        cv::normalize(hist, hist, 0, 1.0, cv::NORM_MINMAX, -1, cv::Mat());

        std::vector<float> grayHist(256);
        for (int i = 0; i < 256; ++i)
        {
            grayHist[i] = hist.at<float>(i);
        }
        histData.push_back(grayHist);
        return true;
    }
    else if (mat.channels() == 3)
    {
        std::vector<cv::Mat> bgrPlanes;
        cv::split(mat, bgrPlanes);

        for (int c = 0; c < 3; ++c)
        {
            cv::Mat hist;
            cv::calcHist(&bgrPlanes[c], 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
            cv::normalize(hist, hist, 0, 1.0, cv::NORM_MINMAX, -1, cv::Mat());

            std::vector<float> channelHist(256);
            for (int i = 0; i < 256; ++i)
            {
                channelHist[i] = hist.at<float>(i);
            }
            histData.push_back(channelHist);
        }
        return true;
    }

    return false;
}
