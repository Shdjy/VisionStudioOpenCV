# VisionStudio OpenCV - 算子库全景清单 (Operator Catalog)

VisionStudio OpenCV 内置了 **60+ 个** 工业级通用及高级机器视觉算子，涵盖图像预处理、滤波去噪、特征提取、测量拟合、模板匹配、形态学运算到图像增强等各大领域。

---

## 📑 算子分类概览

| 序号 | 算子分类 | 英文名称 | 算子数量 | 典型应用场景 |
| :---: | :--- | :--- | :---: | :--- |
| **01** | 🎨 颜色与通道操作 | Color & Channels | 4 | 色彩空间转换 (灰度/HSV/Lab)、通道分离与合并、查表 LUT |
| **02** | 🌫️ 滤波与平滑去噪 | Filtering & Denoise | 7 | 高斯/均值/中值/双边保边滤波、自定义卷积核、可分离卷积 |
| **03** | 🌓 阈值分割与二值化 | Thresholding & Segmentation | 5 | 全局固定阈值、大津法 (Otsu)、自适应局部阈值、HSV 颜色范围过滤 |
| **04** | 🎭 掩码与双图复合 | Mask & Image Arithmetic | 3 | 交互式选区涂抹、掩码应用、原图与处理图加权融合 |
| **05** | 🧱 形态学与结构运算 | Morphology Operations | 4 | 膨胀、腐蚀、开运算(去毛刺)、闭运算(填孔洞)、骨架细化 |
| **06** | 📐 几何与空间变换 | Geometry & Transformations | 9 | 图像缩放、旋转、翻转、ROI 裁剪、仿射变换、透视矫正、极坐标展开、图像金字塔 |
| **07** | 🔍 边缘与梯度检测 | Edge & Gradient Detection | 4 | Canny 双阈值边缘检测、Sobel/Scharr 梯度算子、Laplacian 二阶微分 |
| **08** | 📊 轮廓与连通域分析 | Contours & Connected Components | 3 | 轮廓提取与外接几何拟合、连通域打标与面积过滤、距离变换 |
| **09** | 📏 霍夫特征与几何检测 | Hough Transforms | 4 | 累计概率霍夫直线、霍夫圆检测、最小二乘直线拟合、RANSAC 鲁棒抗噪拟合 |
| **10** | 🎯 特征提取与模板匹配 | Features & Matching | 4 | Harris 角点、Shi-Tomasi 特征点、单/多模板匹配 (含 NMS 抑制)、条码/QR 解码 |
| **11** | ✨ 图像增强与修复仿真 | Enhancement & Restoration | 8 | 亮度/对比度调节、直方图均衡化、CLAHE 局部自适应均衡、伽马校正、图像锐化、去噪与修补、加噪仿真 |
| **12** | 📈 通用统计与矩特征 | General Statistics & Moments | 4 | 图像均值/方差/极值统计、直方图比对、直方图规定化、Hu 不变矩计算 |

---

## 1. 🎨 颜色与通道操作 (Color & Channels)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `CvtColor` | 颜色空间转换 | `cv::cvtColor` | 支持 `BGR2GRAY`, `BGR2HSV`, `BGR2Lab`, `BGR2YCrCb` 等，用于特征预处理与光照解耦。 |
| `ChannelSplit` | 通道分离 | `cv::split` | 提取 R、G、B、H、S、V 或 L、a、b 单独通道，便于针对特定颜色分量进行处理。 |
| `ChannelMerge` | 通道合并 | `cv::merge` | 将多通道图像重新组合或交换通道顺序。 |
| `LUT` | 查表映射 | `cv::LUT` | 通过 256 级查找表实现快速非线性色调映射与伪彩色增强。 |

---

## 2. 🌫️ 滤波与平滑去噪 (Filtering & Denoise)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `GaussianBlur` | 高斯滤波 | `cv::GaussianBlur` | 消除高斯白噪声，支持调节核大小 `ksize` 与标准差 $\sigma_x, \sigma_y$。 |
| `Blur` | 均值滤波 | `cv::blur` | 快速平滑图像背景与高频纹理。 |
| `BoxFilter` | 盒式滤波 | `cv::boxFilter` | 支持非归一化与归一化积分图快速求和。 |
| `MedianBlur` | 中值滤波 | `cv::medianBlur` | 强效去除工业图像中的椒盐噪声与亮点毛刺，有效保护物体边缘。 |
| `BilateralFilter` | 双边保边滤波 | `cv::bilateralFilter` | 结合空间邻近度与像素值相似度，在彻底平滑平坦区域的同时完美保留高对比边缘。 |
| `Filter2D` | 自定义二维卷积 | `cv::filter2D` | 支持自定义卷积核矩阵（如锐化核、浮雕核、高通/低通滤波核）。 |
| `SeparableFilter` | 可分离二维滤波 | `cv::sepFilter2D` | 行列独立一维核卷积，计算效率显著高于全二维卷积。 |

---

## 3. 🌓 阈值分割与二值化 (Thresholding & Segmentation)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `Threshold` | 固定阈值二值化 | `cv::threshold` | 支持二值化（Binary）、反二值化（Binary Inverted）、截断（Truncate）等模式。 |
| `OtsuThreshold` | 大津法自动阈值 | `cv::threshold(..., THRESH_OTSU)` | 基于类间方差最大化自动搜寻最佳全局双峰分割阈值。 |
| `AdaptiveThreshold` | 自适应局部阈值 | `cv::adaptiveThreshold` | 解决工件表面光照不均、阴影干扰难题，支持均值与高斯局部加权。 |
| `InRange` | 颜色/数值范围二值化 | `cv::inRange` | 在 HSV/RGB 空间设定上下界，精确提取特定颜色区域。 |
| `ThresholdTruncate` | 阈值截断操作 | `cv::threshold(..., THRESH_TRUNC)` | 抑制图像高亮过曝区域，平滑灰度极值。 |

---

## 4. 🎭 掩码与双图复合 (Mask & Image Arithmetic)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `InteractiveMask` | 交互式掩码选区 | 自研交互引擎 | 支持通过鼠标画笔、矩形、圆形自由涂抹 ROI 掩码，或根据灰度自动提取。 |
| `MaskApply` | 掩码提取应用 | `cv::Mat::copyTo(..., mask)` | 将掩码应用于目标图像，抠取感兴趣目标区域，背景置黑。 |
| `BlendWithSource` | 原图混合加权 | `cv::addWeighted` | 将滤波或增强后的结果与原始图像按权重 $\alpha, \beta$ 融合，保留真实纹理。 |

---

## 5. 🧱 形态学与结构运算 (Morphology Operations)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `Dilate` | 形态学膨胀 | `cv::dilate` | 扩张前景白色区域，连接断开细线，填补小空洞。 |
| `Erode` | 形态学腐蚀 | `cv::erode` | 消除孤立噪点与细小毛刺，分离粘连目标。 |
| `MorphologyEx` | 高级形态学变换 | `cv::morphologyEx` | 支持开运算 (Open)、闭运算 (Close)、形态学梯度 (Gradient)、顶帽 (TopHat)、黑帽 (BlackHat)。 |
| `SkeletonThinning` | 骨架细化提取 | 细化迭代算法 | 将二值物体收缩为单像素宽度的拓扑骨架线，用于字符与线状缺陷分析。 |

---

## 6. 📐 几何与空间变换 (Geometry & Transformations)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `Resize` | 尺寸缩放 | `cv::resize` | 支持最近邻、双线性、双三次、Lanczos4 等多种高质量插值模式。 |
| `Rotate` | 任意角度旋转 | `cv::getRotationMatrix2D` + `cv::warpAffine` | 支持绕中心或任意参考点精确微调角度与缩放尺度。 |
| `Flip` | 图像翻转 | `cv::flip` | 水平镜像、垂直翻转或双向对称镜像。 |
| `CropRoi` | 矩形 ROI 裁剪 | `cv::Mat(Rect)` | 裁切特定工位视觉区域，提升后续算子运算吞吐量。 |
| `AffineTransform` | 仿射变换 | `cv::warpAffine` | 基于三点映射实现平移、缩放、错切与旋转复合几何矫正。 |
| `PerspectiveTransform` | 透视投影变换 | `cv::warpPerspective` | 基于四点透视重投影，将倾斜工件/车牌/条码矫正为正俯视图。 |
| `PolarTransform` | 极坐标变换 | `cv::warpPolar` | 将圆形零件、晶圆表面纹理展开为直角矩形长条，便于环形缺陷检测。 |
| `CylindricalUnroll` | 柱面展平映射 | 柱面展开投影 | 将圆柱形瓶身/罐体标签透视畸变投影展平。 |
| `ImagePyramid` | 图像高斯/拉普拉斯金字塔 | `cv::pyrUp`, `cv::pyrDown` | 多尺度空间下采样与上采样，加速多尺度模板匹配。 |

---

## 7. 🔍 边缘与梯度检测 (Edge & Gradient Detection)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `Canny` | Canny 边缘检测 | `cv::Canny` | 业界最经典的抗噪边缘算子，包含高低双阈值滞后跟踪与非极大值抑制。 |
| `Sobel` | Sobel 梯度算子 | `cv::Sobel` | 一阶微分梯度计算，支持水平 X 方向与垂直 Y 方向独立边缘检测。 |
| `Scharr` | Scharr 高精度边缘 | `cv::Scharr` | 优于 Sobel 的 3x3 紧凑高阶梯度核，边缘定位精度极高。 |
| `Laplacian` | 拉普拉斯二阶微分 | `cv::Laplacian` | 通过二阶微分过零点快速寻找图像全向突变边缘。 |

---

## 8. 📊 轮廓与连通域分析 (Contours & Connected Components)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `FindContours` | 轮廓查找与几何绘制 | `cv::findContours` | 提取所有闭合曲线，支持自动计算面积、周长、最小外接矩形、凸包并可视化。 |
| `ConnectedComponents` | 连通域标记与分析 | `cv::connectedComponentsWithStats` | 高速统计视野内所有连通区域的中心坐标、面积、边界框，并按颜色伪彩区分。 |
| `DistanceTransform` | 距离变换 | `cv::distanceTransform` | 计算前景像素到最近背景边界的欧氏距离，用于分水岭分割初值与线宽测量。 |

---

## 9. 📏 霍夫特征与几何检测 (Hough Transforms)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `HoughLinesP` | 累计概率霍夫直线 | `cv::HoughLinesP` | 在边缘图上检测线段端点坐标，支持最大断隙与最小线段长度过滤。 |
| `HoughCircles` | 霍夫梯度圆检测 | `cv::HoughCircles` | 基于梯度相位检测圆心与半径，适用于工业零件孔位定位。 |
| `FitLine` | 最小二乘直线拟合 | `cv::fitLine` | 亚像素级直线参数估计（斜率与通过点）。 |
| `RansacFitCircle` | RANSAC 鲁棒圆拟合 | 自研 RANSAC 算法 | 强噪点、毛刺与边缘缺损干扰下的鲁棒抗噪圆拟合与中心精密定位。 |

---

## 10. 🎯 特征提取与模板匹配 (Features & Matching)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `CornerHarris` | Harris 角点检测 | `cv::cornerHarris` | 提取图像局部自相关矩阵极值点作为角点特征。 |
| `GoodFeaturesToTrack` | Shi-Tomasi 优质角点 | `cv::goodFeaturesToTrack` | 优化版角点检测，常用于目标跟踪与立体标定。 |
| `MatchTemplate` | 模板匹配 (单/多目标) | `cv::matchTemplate` | 支持 6 种匹配模式（相关性/归一化互相关），支持多目标 NMS 抑制与阈值过滤。 |
| `BarcodeQr` | 条码与二维码识别 | `cv::QRCodeDetector` / `cv::barcode` | 一维条形码与二维码定位与文本解码。 |

---

## 11. ✨ 图像增强与修复仿真 (Enhancement & Restoration)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `BrightnessContrast` | 亮度与对比度调节 | 线性变换 $\alpha x + \beta$ | 快速调整全局明暗与动态范围。 |
| `EqualizeHist` | 全局直方图均衡化 | `cv::equalizeHist` | 拉伸灰度动态范围，提升整体对比度。 |
| `CLAHE` | 自适应对比度限制直方图均衡 | `cv::createCLAHE` | 局部瓦片直方图均衡化，抑制背景噪点过曝，工业视觉首选增强算法。 |
| `GammaCorrection` | 伽马非线性校正 | 非线性幂次映射 | 提亮暗部细节或压制高光过曝。 |
| `Invert` | 图像反相 | `cv::bitwise_not` | 黑白颠倒，快速适配白底黑字或黑底白字要求。 |
| `Sharpen` | 图像锐化增强 | 拉普拉斯卷积核 | 增强边缘与微小细节纹理。 |
| `Inpaint` | 缺陷修补与消除 | `cv::inpaint` | 基于 Telea / Navier-Stokes 算法修复图像划痕、污点或遮挡物。 |
| `NoiseInjection` | 工业噪声仿真 | 高斯/椒盐噪声发生器 | 在理想图像上注入噪声，用于算法鲁棒性极限测试。 |

---

## 12. 📈 通用统计与特征分析 (General Statistics)

| 算子标识 | 中文名称 | OpenCV 核心 API | 核心参数与功能说明 |
| :--- | :--- | :--- | :--- |
| `ImageStatistics` | 图像统计特性 | `cv::meanStdDev`, `cv::minMaxLoc` | 统计计算均值、标准差、最大/最小值，支持在视口绘制信息遮罩。 |
| `HistogramCompare` | 直方图相似度比对 | `cv::compareHist` | 支持相关性、卡方、交叉核、巴氏距离比对，用于颜色分类与缺陷判定。 |
| `HistogramMatch` | 直方图规定化匹配 | 累积分布映射 | 强制将当前图像色彩分布匹配至参考标准图。 |
| `MomentsHu` | 图像几何矩与 Hu 不变矩 | `cv::moments`, `cv::HuMoments` | 提取旋转、平移、尺度不变的 7 个 Hu 不变矩特征，用于工件形状分类。 |

---

> 💡 **提示**：所有算子均支持一键生成规范的 C++ (OpenCV) 与 Python (cv2) 源码！
