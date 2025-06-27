#pragma once
#include <string>
#include <windows.h>

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    bool loadConfig(const std::string& iniPath = "config.ini");
    
    // 常用设置
    std::string getGrabberConfigPath() const { return m_grabberConfigPath; }
    std::string getSavePath() const { return m_savePath; }
    std::string getVideoPrefix() const { return m_videoPrefix; }
    std::string getVideoExt() const { return m_videoExt; }
    std::string getVideoFileName() const { return m_videoFileName; }
    int getGrabberIndex() const { return m_grabberIndex; }
    int getCameraIndex() const { return m_cameraIndex; }
    
    // 显示设置
    double getViewerScale() const { return m_viewerScale; }
    int getCvPixelFormat() const { return m_cvPixelFormat; }
    int getCvWaitKey() const { return m_cvWaitKey; }
    bool getFocusPeakingLayer() const { return m_focusPeakingLayer; }
    bool getHistLayer() const { return m_histLayer; }
    bool getMotionDetectorLayer() const { return m_motionDetectorLayer; }
    
    // 录制设置
    int getRecordMode() const { return m_recordMode; }
    int getFrameRate() const { return m_frameRate; }
    bool getSaveAsFrameSequence() const { return m_saveAsFrameSequence; }
    std::string getEncoder() const { return m_encoder; }
    int getBufferCount() const { return m_bufferCount; }
    bool getExecuteNext() const { return m_executeNext; }
    bool getPauseView() const { return m_pauseView; }
    int getRecordFrame() const { return m_recordFrame; }
    bool getIsRoundFramerate() const { return m_isRoundFramerate; }

private:
    ConfigManager() = default;
    
    // 常用设置
    std::string m_grabberConfigPath;
    std::string m_savePath;
    std::string m_videoPrefix;
    std::string m_videoExt;
    std::string m_videoFileName;
    int m_grabberIndex;
    int m_cameraIndex;
    
    // 显示设置
    float m_viewerScale;
    int m_cvPixelFormat;
    int m_cvWaitKey;
    bool m_focusPeakingLayer;
    bool m_histLayer;
    bool m_motionDetectorLayer;
    
    // 录制设置
    int m_recordMode;
    int m_frameRate;
    bool m_saveAsFrameSequence;
    std::string m_encoder;
    int m_bufferCount;
    bool m_executeNext;
    bool m_pauseView;
    int m_recordFrame;
    bool m_isRoundFramerate;
};