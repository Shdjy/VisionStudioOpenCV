#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

#pragma pack(push, 1)
struct ICONDIR
{
    uint16_t idReserved;
    uint16_t idType;
    uint16_t idCount;
};

struct ICONDIRENTRY
{
    uint8_t bWidth;
    uint8_t bHeight;
    uint8_t bColorCount;
    uint8_t bReserved;
    uint16_t wPlanes;
    uint16_t wBitCount;
    uint32_t dwBytesInRes;
    uint32_t dwImageOffset;
};

struct BITMAPINFOHEADER_ICO
{
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

int main()
{
    std::string srcPath = "C:/Users/SHDha/.gemini/antigravity/brain/d61fb316-e3c4-4a31-9f85-66d074e00a15/.user_uploaded/media_1786794338094.png";
    cv::Mat src = cv::imread(srcPath, cv::IMREAD_UNCHANGED);
    if (src.empty())
    {
        std::cerr << "Cannot open src image" << std::endl;
        return 1;
    }

    cv::Mat bgra;
    if (src.channels() == 3)
    {
        cv::cvtColor(src, bgra, cv::COLOR_BGR2BGRA);
    }
    else
    {
        bgra = src.clone();
    }

    // 适度裁剪四周黑底让准心和VS突出
    int marginX = static_cast<int>(bgra.cols * 0.05);
    int marginY = static_cast<int>(bgra.rows * 0.05);
    cv::Rect cropRoi(marginX, marginY, bgra.cols - 2 * marginX, bgra.rows - 2 * marginY);
    cv::Mat cropped = bgra(cropRoi).clone();

    cv::Mat iconMaster;
    cv::resize(cropped, iconMaster, cv::Size(256, 256), 0, 0, cv::INTER_AREA);

    // 圆角透明化
    int w = 256, h = 256;
    float cornerRadius = 56.0f;
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
                    iconMaster.at<cv::Vec4b>(y, x) = cv::Vec4b(0, 0, 0, 0);
                }
                else if (dist > cornerRadius - 1.5f)
                {
                    float alpha = (cornerRadius - dist) / 1.5f;
                    iconMaster.at<cv::Vec4b>(y, x)[3] = static_cast<uchar>(255 * alpha);
                }
            }
        }
    }

    // 保存 png
    cv::imwrite("src/resources/icons/app_icon.png", iconMaster);

    // 标准尺寸列表 (小尺寸全部采用原生 DIB 格式，256 采用 PNG 格式)
    std::vector<int> dibSizes = { 16, 24, 32, 48, 64, 128 };
    struct EntryData
    {
        int size;
        std::vector<uint8_t> bytes;
    };
    std::vector<EntryData> entries;

    for (int s : dibSizes)
    {
        cv::Mat resized;
        cv::resize(iconMaster, resized, cv::Size(s, s), 0, 0, cv::INTER_AREA);

        // 构造 DIB 数据
        int rowBytes = s * 4;
        int andRowBytes = ((s + 31) / 32) * 4;
        int dibImageSize = rowBytes * s + andRowBytes * s;

        BITMAPINFOHEADER_ICO bih;
        bih.biSize = sizeof(BITMAPINFOHEADER_ICO);
        bih.biWidth = s;
        bih.biHeight = s * 2; // Windows ICO 规范: biHeight 为高度的 2 倍
        bih.biPlanes = 1;
        bih.biBitCount = 32;
        bih.biCompression = 0; // BI_RGB
        bih.biSizeImage = dibImageSize;
        bih.biXPelsPerMeter = 0;
        bih.biYPelsPerMeter = 0;
        bih.biClrUsed = 0;
        bih.biClrImportant = 0;

        std::vector<uint8_t> data;
        data.resize(sizeof(bih) + dibImageSize, 0);
        std::memcpy(data.data(), &bih, sizeof(bih));

        uint8_t* pXor = data.data() + sizeof(bih);
        uint8_t* pAnd = pXor + (rowBytes * s);

        // DIB 像素是 Bottom-Up (从下往上存储)
        for (int y = 0; y < s; ++y)
        {
            const cv::Vec4b* srcRow = resized.ptr<cv::Vec4b>(s - 1 - y);
            uint8_t* dstRow = pXor + y * rowBytes;
            for (int x = 0; x < s; ++x)
            {
                dstRow[x * 4 + 0] = srcRow[x][0]; // B
                dstRow[x * 4 + 1] = srcRow[x][1]; // G
                dstRow[x * 4 + 2] = srcRow[x][2]; // R
                dstRow[x * 4 + 3] = srcRow[x][3]; // A

                if (srcRow[x][3] == 0)
                {
                    // 设置 AND mask 掩码
                    pAnd[y * andRowBytes + (x / 8)] |= (1 << (7 - (x % 8)));
                }
            }
        }
        entries.push_back({ s, data });
    }

    // 添加 256x256 PNG 格式
    {
        std::vector<uchar> pngBuf;
        cv::imencode(".png", iconMaster, pngBuf);
        std::vector<uint8_t> data(pngBuf.begin(), pngBuf.end());
        entries.push_back({ 256, data });
    }

    // 写入文件
    std::ofstream out("src/resources/icons/app_icon.ico", std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "Failed to open app_icon.ico" << std::endl;
        return 1;
    }

    ICONDIR dir;
    dir.idReserved = 0;
    dir.idType = 1;
    dir.idCount = static_cast<uint16_t>(entries.size());
    out.write(reinterpret_cast<const char*>(&dir), sizeof(dir));

    uint32_t offset = sizeof(ICONDIR) + sizeof(ICONDIRENTRY) * entries.size();
    for (const auto& e : entries)
    {
        ICONDIRENTRY de;
        de.bWidth = (e.size == 256) ? 0 : static_cast<uint8_t>(e.size);
        de.bHeight = (e.size == 256) ? 0 : static_cast<uint8_t>(e.size);
        de.bColorCount = 0;
        de.bReserved = 0;
        de.wPlanes = 1;
        de.wBitCount = 32;
        de.dwBytesInRes = static_cast<uint32_t>(e.bytes.size());
        de.dwImageOffset = offset;

        out.write(reinterpret_cast<const char*>(&de), sizeof(de));
        offset += de.dwBytesInRes;
    }

    for (const auto& e : entries)
    {
        out.write(reinterpret_cast<const char*>(e.bytes.data()), e.bytes.size());
    }
    out.close();

    std::cout << "Successfully generated 100% Windows Standard DIB + PNG multi-resolution ICO!" << std::endl;
    return 0;
}
