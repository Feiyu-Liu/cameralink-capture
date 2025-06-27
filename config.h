#pragma once
#include "ConfigManager.h"
#include <opencv2/videoio.hpp>

// 获取配置实例
#define CONFIG ConfigManager::getInstance()

// 编码器映射
inline int GetEncoder(const std::string& encoderName) {
    if (encoderName == "H264") return cv::VideoWriter::fourcc('H', '2', '6', '4'); // MP4
    if (encoderName == "HEVC") return cv::VideoWriter::fourcc('H', 'E', 'V', '1'); // 无法持续写入
    if (encoderName == "MJPG") return cv::VideoWriter::fourcc('M', 'J', 'P', 'G'); // avi
    if (encoderName == "YUYV") return cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'); 
    if (encoderName == "AZPR") return cv::VideoWriter::fourcc('A', 'Z', 'P', 'R');
    return cv::VideoWriter::fourcc('H', '2', '6', '4'); // 默认使用H264
}

inline int GetCvFormat(int depth) {
    switch (depth) {
    case 8: return CV_8UC1;
    case 16: return CV_16UC1;
    case 32: return CV_32FC1;
    default: return CV_8UC1;
    }
}
/*
快捷键说明：
q：退出程序
g：开始捕获画面
p：暂停捕获画面
i：显示图像信息
r：开始录制
s：停止录制
*/