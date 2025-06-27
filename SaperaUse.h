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

    bool GrabbersInit();  // ��ʼ���ɼ���

    bool CreateDevice(int grabberIndex, int deviceIndex, const char* configFilePath);  // ��ʼ�����

    //bool ReleaseDevice(); // �ͷ����

    // callback
    static void XferCallback(SapXferCallbackInfo* pInfo);  //transfer call back

    static void ProCallback(SapProCallbackInfo* pInfo);  //process call back


private:

    int _errorStaus = -1;  // ����״̬ -1: no error

    /* GrabbersInit */
    int _availableGrabberCount = 0; // ���õĲɼ�����
    std::vector<std::tuple<std::string, std::vector<std::string>>> _devicesInfo;  // ������òɼ����Ϳ����豸��:1�����ɼ�������;2�����豸����
    
	float _FrameRateDisp(SapXferFrameRateInfo* FrameRateInfo);  // ��ʾʵʱ֡��
	float _SteadyFrameRate;  // �ȶ�֡��

    CameraObj _cameraA;

	void _KeyToBufferRecord(SapBufferWithTrash* mBuffer, SapTransfer* Xfer, int beginBufferIdx);  // ���̴�������ʽ¼��
    bool _TriggerToBufferRecord(SapBufferWithTrash* mBuffer);  // trigger��������ʽ¼��

    bool _isKeyToRecording = false; // ��ط���ʽ¼��
    bool _isTriggerToRecording = false; // ��ط���ʽtrigger¼��
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