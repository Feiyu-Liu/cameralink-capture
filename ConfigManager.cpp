#include "ConfigManager.h"
#include <opencv2/videoio.hpp>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& iniPath) {
    char buffer[256];
    
    // 常用设置
    GetPrivateProfileStringA("Common", "GrabberConfigPath", "", buffer, sizeof(buffer), iniPath.c_str());
    m_grabberConfigPath = buffer;
    
    GetPrivateProfileStringA("Common", "SavePath", "", buffer, sizeof(buffer), iniPath.c_str());
    m_savePath = buffer;
    
    GetPrivateProfileStringA("Common", "VideoPrefix", "", buffer, sizeof(buffer), iniPath.c_str());
    m_videoPrefix = buffer;

    GetPrivateProfileStringA("Common", "VideoExt", ".mp4", buffer, sizeof(buffer), iniPath.c_str());
    m_videoExt = buffer;
    
    GetPrivateProfileStringA("Common", "VideoFileName", "test6.mp4", buffer, sizeof(buffer), iniPath.c_str());
    m_videoFileName = buffer;
    
    m_grabberIndex = GetPrivateProfileIntA("Common", "GrabberIndex", 0, iniPath.c_str());
    m_cameraIndex = GetPrivateProfileIntA("Common", "CameraIndex", 1, iniPath.c_str());
    
    // 显示设置
    GetPrivateProfileStringA("Display", "ViewerScale", "0.4", buffer, sizeof(buffer), iniPath.c_str());
    m_viewerScale = std::stof(buffer);

    m_cvPixelFormat = GetPrivateProfileIntA("Display", "CvPixelFormat", 8, iniPath.c_str());
    m_cvWaitKey = GetPrivateProfileIntA("Display", "CvWaitKey", 1, iniPath.c_str());
    m_focusPeakingLayer = GetPrivateProfileIntA("Display", "FocusPeakingLayer", 0, iniPath.c_str()) != 0;
    m_histLayer = GetPrivateProfileIntA("Display", "HistLayer", 0, iniPath.c_str()) != 0;
    m_motionDetectorLayer = GetPrivateProfileIntA("Display", "MotionDetectorLayer", 0, iniPath.c_str()) != 0;
    m_isRoundFramerate = GetPrivateProfileIntA("Display", "IsRoundFramerate", 1, iniPath.c_str()) != 0;
    // 录制设置
    m_recordMode = GetPrivateProfileIntA("Record", "RecordMode", 2, iniPath.c_str());
    m_frameRate = GetPrivateProfileIntA("Record", "FrameRate", 120, iniPath.c_str());
    m_saveAsFrameSequence = GetPrivateProfileIntA("Record", "SaveAsFrameSequence", 0, iniPath.c_str()) != 0;
    
    GetPrivateProfileStringA("Record", "Encoder", "H264", buffer, sizeof(buffer), iniPath.c_str());
    m_encoder = buffer;
    
    m_bufferCount = GetPrivateProfileIntA("Record", "BufferCount", 1000, iniPath.c_str());
    m_executeNext = GetPrivateProfileIntA("Record", "ExecuteNext", 1, iniPath.c_str()) != 0;
    m_pauseView = GetPrivateProfileIntA("Record", "PauseView", 1, iniPath.c_str()) != 0;
    m_recordFrame = GetPrivateProfileIntA("Record", "RecordFrame", 600, iniPath.c_str());
    
    
    return true;
}

