#pragma once

#ifndef _SAPERAUSE_H_
#define _SAPERAUSE_H_

#include <SapClassBasic.h>
#include "CaptureWindow.h"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

class SaperaUse
{
public:
    SaperaUse();
    ~SaperaUse();

    bool GrabbersInit();  // 初始化采集卡

    bool CreateDevice(int grabberIndex, int deviceIndex, const char* configFilePath);  // 初始化相机

    bool Shutdown() noexcept;

    // callback
    static void XferCallback(SapXferCallbackInfo* pInfo);  //transfer call back

private:
    struct CameraSession;

    int _errorStaus = -1;  // 错误状态 -1: no error

    /* GrabbersInit */
    int _availableGrabberCount = 0; // 可用的采集卡数
    std::vector<std::tuple<std::string, std::vector<std::string>>> _devicesInfo;  // 保存可用采集卡和可用设备名:1级：采集卡名称;2级：设备名称

	float _FrameRateDisp(SapXferFrameRateInfo* FrameRateInfo);  // 显示实时帧率
	float _SteadyFrameRate = 0.0f;  // 稳定帧率

    bool _ValidateRuntimeConfig(std::string& error) const;
    bool _StopTransfer(CameraSession& session, std::string& error) const;
    bool _KeyToBufferRecord(CameraSession& session);
    bool _TriggerToBufferRecord(CameraSession& session);
    bool _SaveCaptureWindow(CameraSession& session, const CaptureWindow& window);

    std::unique_ptr<CameraSession> _session;
    bool _isTriggerToRecording = false; // 监控非流式trigger录制
};

#endif

/* error codes:
-1:no error
0:no grabber found
1:no grabbers is available
2:fail to creat an acquisition object
3:fail to creat buffter
4:fail to creat Xfer
5.fail to creat view
6.fail to creat process
*/
