#pragma once
#include "ConfigManager.h"
#include <opencv2/videoio.hpp>

// ��ȡ����ʵ��
#define CONFIG ConfigManager::getInstance()

// ������ӳ��
inline int GetEncoder(const std::string& encoderName) {
    if (encoderName == "H264") return cv::VideoWriter::fourcc('H', '2', '6', '4'); // MP4
    if (encoderName == "HEVC") return cv::VideoWriter::fourcc('H', 'E', 'V', '1'); // �޷�����д��
    if (encoderName == "MJPG") return cv::VideoWriter::fourcc('M', 'J', 'P', 'G'); // avi
    if (encoderName == "YUYV") return cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V'); 
    if (encoderName == "AZPR") return cv::VideoWriter::fourcc('A', 'Z', 'P', 'R');
    return cv::VideoWriter::fourcc('H', '2', '6', '4'); // Ĭ��ʹ��H264
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
��ݼ�˵����
q���˳�����
g����ʼ������
p����ͣ������
i����ʾͼ����Ϣ
r����ʼ¼��
s��ֹͣ¼��
*/