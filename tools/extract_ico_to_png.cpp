#include <iostream>
#include <fstream>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

int main()
{
    std::ifstream in("VisionStudio_Icon_Uncompressed_DIB.ico", std::ios::binary);
    if (!in.is_open()) return 1;

    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buf(size);
    in.read(reinterpret_cast<char*>(buf.data()), size);
    in.close();

    // 寻找 PNG 魔法头 0x89 0x50 0x4E 0x47
    for (size_t i = 0; i < buf.size() - 8; ++i)
    {
        if (buf[i] == 0x89 && buf[i+1] == 0x50 && buf[i+2] == 0x4E && buf[i+3] == 0x47)
        {
            std::vector<uint8_t> pngData(buf.begin() + i, buf.end());
            cv::Mat img = cv::imdecode(pngData, cv::IMREAD_UNCHANGED);
            if (!img.empty())
            {
                cv::imwrite("src/resources/icons/app_icon.png", img);
                std::cout << "Successfully extracted PNG of size: " << img.cols << "x" << img.rows << std::endl;
                return 0;
            }
        }
    }

    // 寻找最大尺寸的 DIB 或 PNG
    uint16_t count = *reinterpret_cast<uint16_t*>(&buf[4]);
    std::cout << "ICO contains " << count << " sub-images" << std::endl;

    int maxW = 0;
    size_t bestIdx = 0;
    for (size_t i = 0; i < count; ++i)
    {
        uint8_t w = buf[6 + i * 16];
        uint8_t h = buf[7 + i * 16];
        int width = (w == 0) ? 256 : w;
        if (width > maxW)
        {
            maxW = width;
            bestIdx = i;
        }
    }

    if (maxW > 0)
    {
        uint32_t offset = *reinterpret_cast<uint32_t*>(&buf[6 + bestIdx * 16 + 12]);
        uint32_t bytesInRes = *reinterpret_cast<uint32_t*>(&buf[6 + bestIdx * 16 + 8]);
        uint8_t w = buf[6 + bestIdx * 16];
        uint8_t h = buf[7 + bestIdx * 16];
        int width = (w == 0) ? 256 : w;
        int height = (h == 0) ? 256 : h;

        // 判断是否为 PNG 压缩
        if (offset + 4 <= buf.size() && buf[offset] == 0x89 && buf[offset+1] == 0x50)
        {
            std::vector<uint8_t> pngData(buf.begin() + offset, buf.begin() + offset + bytesInRes);
            cv::Mat img = cv::imdecode(pngData, cv::IMREAD_UNCHANGED);
            if (!img.empty())
            {
                cv::imwrite("src/resources/icons/app_icon.png", img);
                std::cout << "Extracted max PNG of size: " << img.cols << "x" << img.rows << std::endl;
                return 0;
            }
        }
        else
        {
            const uint8_t* pDIB = buf.data() + offset;
            const uint8_t* pXor = pDIB + 40; // 跳过 BITMAPINFOHEADER

            cv::Mat outImg(height, width, CV_8UC4);
            int rowBytes = width * 4;
            for (int y = 0; y < height; ++y)
            {
                cv::Vec4b* row = outImg.ptr<cv::Vec4b>(height - 1 - y);
                const uint8_t* srcRow = pXor + y * rowBytes;
                for (int x = 0; x < width; ++x)
                {
                    row[x][0] = srcRow[x * 4 + 0];
                    row[x][1] = srcRow[x * 4 + 1];
                    row[x][2] = srcRow[x * 4 + 2];
                    row[x][3] = srcRow[x * 4 + 3];
                }
            }
            cv::imwrite("src/resources/icons/app_icon.png", outImg);
            std::cout << "Extracted max DIB to app_icon.png: " << width << "x" << height << std::endl;
        }
    }
    return 0;
}
