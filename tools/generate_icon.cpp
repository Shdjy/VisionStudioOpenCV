#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

int main()
{
    std::string srcPath = "C:/Users/SHDha/.gemini/antigravity/brain/d61fb316-e3c4-4a31-9f85-66d074e00a15/.user_uploaded/media_1786794338094.png";
    cv::Mat src = cv::imread(srcPath, cv::IMREAD_UNCHANGED);
    if (src.empty())
    {
        std::cerr << "Failed to read source image: " << srcPath << std::endl;
        return 1;
    }

    // 确保为 4 通道 BGRA
    cv::Mat bgra;
    if (src.channels() == 3)
    {
        cv::cvtColor(src, bgra, cv::COLOR_BGR2BGRA);
    }
    else
    {
        bgra = src.clone();
    }

    // 自动裁剪居中聚焦主体 (去除边缘多余黑边，让准心和 VS 充满画布)
    // 裁剪中心 88% 的区域
    int cropMarginX = static_cast<int>(bgra.cols * 0.05);
    int cropMarginY = static_cast<int>(bgra.rows * 0.05);
    cv::Rect roi(cropMarginX, cropMarginY, bgra.cols - 2 * cropMarginX, bgra.rows - 2 * cropMarginY);
    cv::Mat cropped = bgra(roi).clone();

    // 缩放到标准的 256x256
    cv::Mat icon256;
    cv::resize(cropped, icon256, cv::Size(256, 256), 0, 0, cv::INTER_AREA);

    // 为圆角矩形外圈生成平滑 Alpha 透明通道 (消除四周方块黑角)
    int w = icon256.cols;
    int h = icon256.rows;
    float cornerRadius = 56.0f; // 标准平滑圆角半径

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float dx = 0.0f, dy = 0.0f;
            if (x < cornerRadius) dx = cornerRadius - x;
            else if (x > w - cornerRadius) dx = x - (w - cornerRadius);

            if (y < cornerRadius) dy = cornerRadius - y;
            else if (y > h - cornerRadius) dy = y - (h - cornerRadius);

            if (dx > 0 && dy > 0)
            {
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist > cornerRadius)
                {
                    icon256.at<cv::Vec4b>(y, x)[3] = 0; // 完全透明
                }
                else if (dist > cornerRadius - 1.5f)
                {
                    float alpha = (cornerRadius - dist) / 1.5f;
                    icon256.at<cv::Vec4b>(y, x)[3] = static_cast<uchar>(255 * alpha);
                }
            }
        }
    }

    // 保存主 PNG 图标
    cv::imwrite("src/resources/icons/app_icon.png", icon256);
    std::cout << "Successfully generated high-contrast transparent app_icon.png" << std::endl;

    // 生成各尺寸数据并写入标准多分辨率 ICO
    std::vector<int> sizes = { 256, 128, 64, 48, 32, 24, 16 };
    struct IconImage
    {
        int size;
        std::vector<uchar> pngBytes;
    };
    std::vector<IconImage> images;

    for (int s : sizes)
    {
        cv::Mat resized;
        cv::resize(icon256, resized, cv::Size(s, s), 0, 0, cv::INTER_AREA);
        std::vector<uchar> buf;
        cv::imencode(".png", resized, buf);
        images.push_back({ s, buf });
    }

    // 组装 ICO 文件
    std::ofstream icoFile("src/resources/icons/app_icon.ico", std::ios::binary);
    if (!icoFile.is_open())
    {
        std::cerr << "Failed to open app_icon.ico for writing" << std::endl;
        return 1;
    }

    // ICO Header: 0, 1 (type=icon), count
    uint16_t reserved = 0;
    uint16_t type = 1;
    uint16_t count = static_cast<uint16_t>(images.size());
    icoFile.write(reinterpret_cast<const char*>(&reserved), 2);
    icoFile.write(reinterpret_cast<const char*>(&type), 2);
    icoFile.write(reinterpret_cast<const char*>(&count), 2);

    uint32_t offset = 6 + count * 16;
    for (const auto& img : images)
    {
        uint8_t width = (img.size == 256) ? 0 : static_cast<uint8_t>(img.size);
        uint8_t height = (img.size == 256) ? 0 : static_cast<uint8_t>(img.size);
        uint8_t colorCount = 0;
        uint8_t res = 0;
        uint16_t planes = 1;
        uint16_t bitCount = 32;
        uint32_t bytesInRes = static_cast<uint32_t>(img.pngBytes.size());

        icoFile.write(reinterpret_cast<const char*>(&width), 1);
        icoFile.write(reinterpret_cast<const char*>(&height), 1);
        icoFile.write(reinterpret_cast<const char*>(&colorCount), 1);
        icoFile.write(reinterpret_cast<const char*>(&res), 1);
        icoFile.write(reinterpret_cast<const char*>(&planes), 2);
        icoFile.write(reinterpret_cast<const char*>(&bitCount), 2);
        icoFile.write(reinterpret_cast<const char*>(&bytesInRes), 4);
        icoFile.write(reinterpret_cast<const char*>(&offset), 4);

        offset += bytesInRes;
    }

    for (const auto& img : images)
    {
        icoFile.write(reinterpret_cast<const char*>(img.pngBytes.data()), img.pngBytes.size());
    }
    icoFile.close();

    std::cout << "Successfully generated multi-size app_icon.ico with 7 resolutions!" << std::endl;
    return 0;
}
