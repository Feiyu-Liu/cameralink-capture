#pragma once

#ifndef _SAPERAUSE_H_
#define _SAPERAUSE_H_

#include <SapClassBasic.h>
#include "conio.h"
#include "RealtimeView.h"
#include "RecordFromBuffer.h"

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cmath>


#include "config.h"

struct CameraObj {
    std::string grabberName;
    std::string deviceName;
    std::string cameraName;

    std::unique_ptr<SapAcquisition> Acq;
    std::unique_ptr<SapAcqDevice> AcqDevice;
    std::unique_ptr<SapBufferWithTrash> Buffers;
    std::unique_ptr<SapTransfer> AcqToBuf;
    std::unique_ptr<SapTransfer> AcqDeviceToBuf;
    std::unique_ptr<SapTransfer> Xfer;
    std::unique_ptr<SapView> View;

};

class SaperaUse
{
public:
    SaperaUse();
    ~SaperaUse();

    bool GrabbersInit();  // 初始化采集卡

    bool CreateDevice(int grabberIndex, int deviceIndex, const char* configFilePath);  // 初始化相机

    //bool ReleaseDevice(); // 释放相机

    // callback
    static void XferCallback(SapXferCallbackInfo* pInfo);  //transfer call back

    static void ProCallback(SapProCallbackInfo* pInfo);  //process call back


private:

    int _errorStaus = -1;  // 错误状态 -1: no error

    /* GrabbersInit */
    int _availableGrabberCount = 0; // 可用的采集卡数
    std::vector<std::tuple<std::string, std::vector<std::string>>> _devicesInfo;  // 保存可用采集卡和可用设备名:1级：采集卡名称;2级：设备名称
    
	float _FrameRateDisp(SapXferFrameRateInfo* FrameRateInfo);  // 显示实时帧率
	float _SteadyFrameRate;  // 稳定帧率

    CameraObj _cameraA;

	void _KeyToBufferRecord(SapBufferWithTrash* mBuffer, SapTransfer* Xfer, int beginBufferIdx);  // 键盘触发非流式录制
    bool _TriggerToBufferRecord(SapBufferWithTrash* mBuffer);  // trigger触发非流式录制

    bool _isKeyToRecording = false; // 监控非流式录制
    bool _isTriggerToRecording = false; // 监控非流式trigger录制
    // void
    // void _DestroyCameraObj();


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