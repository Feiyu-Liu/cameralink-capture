#include "SaperaUse.h"

SaperaUse::SaperaUse()
{

}

SaperaUse::~SaperaUse()
{
    // 退出相机的时候记得写
}


bool SaperaUse::GrabbersInit()
{
    // 获取系统中的采集卡数量
    int grabberCount = SapManager::GetServerCount();
    if (grabberCount == 0)
    {
        this->_errorStaus = 0; //no grabber found
        return false;
    }

    // 遍历系统中的采集卡，找到支持采集的板卡及支持的设备
    bool serverFound = false;
    std::ostringstream oss;
    int tempCount = 0; 
    for (int serverIndex = 0; serverIndex < grabberCount; serverIndex++)
    {
        // 检查采集卡是否可用
        if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcq) != 0)
        {
            // 获取采集卡名称
            tempCount += 1;
            char serverName[CORSERVER_MAX_STRLEN];
            SapManager::GetServerName(serverIndex, serverName, sizeof(serverName));
            oss << serverName;

            // 获取设备名称
            int deviceCount = SapManager::GetResourceCount(serverName, SapManager::ResourceAcq);

            std::vector<std::string> devicesName;
            for (int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
            {
                char deviceName[CORPRM_GETSIZE(CORACQ_PRM_LABEL)];
                SapManager::GetResourceName(serverName, SapManager::ResourceAcq, deviceIndex, deviceName, sizeof(deviceName));
                devicesName.push_back(deviceName);
            }
            this->_devicesInfo.emplace_back(serverName, devicesName);

            oss.str("");
            oss.clear();
            serverFound = true;
        }
    }

    this->_availableGrabberCount = tempCount;
    if (!serverFound) // 至少有一个采集卡必须可用
    {
        this->_errorStaus = 1; // no grabbers is available
        return false;
    }

    return true;
}

bool SaperaUse::CreateDevice(int grabberIndex, int deviceIndex, const char* configFilePath)
{
    /* 初始化采集对象 */
    if (this->_availableGrabberCount <= 0)
    {
        this->_errorStaus = 1; // no grabber available, 退出
        return false;
    }

    // 获取采集卡名
    auto& deviceInfo = _devicesInfo[grabberIndex];
    std::string grabberName = std::get<0>(deviceInfo);

    SapLocation loc(grabberName.c_str(), deviceIndex);

    // 源传输节点
    SapAcquisition* Acq = new SapAcquisition(loc, configFilePath);

    /*缓冲对象可用作目标传输节点，允许从源节点（如采集或另一个缓冲区）传输数据到缓冲区资源。
     它还可以用作源传输节点，允许将数据从一个缓冲区资源转移到另一个缓冲区
    */ 
    SapBufferWithTrash* Buffers = new SapBufferWithTrash(2, Acq);
    if (RECORD_MODE == 1) { // 自由录制
        Buffers->SetCount(BUFFER_COUNT);
    }
    else if (RECORD_MODE == 2) { // 固定时长录制
        Buffers->SetCount(RECORD_FRAME+5);
        std::cout << "预估录制时长：" << RECORD_FRAME/FRAME_RATE << std::endl;
    }
    
    /*SapView类包括在窗口中显示SapBuffer对象资源的功能。它允许您显示当前缓冲区资源、
    特定资源或尚未显示的下一个资源。内部线程实时优化缓冲区显示。这使得主应用程序线程可
    以执行而不必担心显示任务。自动清空机制允许SapView和SapTransfer对象之间同步，以
    便实时显示缓冲区，不会丢失任何数据。 SapHwndDesktop/SapHwndAutomatic
    */
    //SapView* View = new SapView(Buffers, SapHwndAutomatic);
    SapView* View = new SapView(Buffers);

    /*实时处理层*/
    RealtimeView* Pro = new RealtimeView(Buffers, ProCallback, NULL);
    //VideoRecorder* Rec = new VideoRecorder(Buffers, ProCallback2, this);

    /*SapTransfer类实现了管理通用传输过程功能，即从一个源节点向目标节点传输数据的操作。
    所有继承自SapXferNode类的以下类都被视为传输节点：
    */
    SapTransfer* Xfer = new SapAcqToBuf(Acq, Buffers, this->XferCallback, Pro);
    //SapTransfer* Xfer2 = new SapAcqToBuf(Acq, Buffers, this->XferCallback2, Rec);


    RecordFromBuffer *bufferRecorder = new RecordFromBuffer(Buffers);


    /* 创建对象 */
    if (!Acq->Create()) {
        this->_errorStaus = 2; // fail to creat an acquisition object
        return false;
    }
    if (!Buffers->Create()) {
        this->_errorStaus = 3; // fail to creat buffter
        return false;
    }
    if (!View->Create()) {
        this->_errorStaus = 5; // fail to creat view
        return false;
    }
    if (!Pro->Create()) {
        this->_errorStaus = 6; // fail to creat process
        return false;
    }

    //if (!Rec->Create()) {
    //    this->_errorStaus = 7; // fail to creat process
    //    return false;
    //}

    if (Xfer && !Xfer->Create()) {
        this->_errorStaus = 4; // fail to creat Xfer
        return false;
    }
    //if (Xfer2 && !Xfer2->Create()) {
    //    this->_errorStaus = 4; // fail to creat Xfer
   //     return false;
    //}

    //if (Rec->AutoSetVideoParams("D:\\Batlab\\videos\\test11.mp4", Xfer2)) {
    //    std::cout << "ok\n";
     //   if (Rec->InitVideoRecorder()) {
    //        std::cout << "ok\n";
    //    }
    //}


    // Start continous grab
    //Xfer2->Grab();
    Xfer->Grab();
    //Xfer2->Grab();
    
    SapXferFrameRateInfo* pFrameRateInfo = Xfer->GetFrameRateStatistics();
    float frameRate = 0;
    float thisframeRate = 0;
    bool isQuit = false;
    while (1) {
        // 固定帧数录制模式下监控 buffer 满时暂停流传输
        if (_monitorRecording && RECORD_MODE == 2) {
            //std::cout << Buffers->GetIndex() << std::endl;
            if (Buffers->GetIndex() > RECORD_FRAME) {
                Xfer->Freeze();
                _monitorRecording = false;
                if (!Xfer->Wait(5000)) {
                    printf("Grab could not stop properly.\n");
                }
                std::cout << "暂停捕获: " << Buffers->GetIndex() << std::endl;

                std::stringstream ss;
                ss << SAVE_PATH << VIDEO_FILE_NAME;
                std::string filePath = ss.str();
                bool isSaved = bufferRecorder->SaveVideo(filePath, ENCODER, FRAME_RATE, 
                    Buffers->GetWidth(), Buffers->GetHeight(), false, Buffers->GetIndex());
                if (isSaved) {
                    std::cout << "视频已保存至: " << filePath << std::endl;
                }
                else {
                    std::cout << "视频保存失败" << std::endl;
                }
            }
        }

        
        if (pFrameRateInfo->IsLiveFrameRateAvailable()) {
            if (!pFrameRateInfo->IsLiveFrameRateStalled()) {
                // 帧率四舍五入
                if (IS_ROUND_FRAMERATE) {
                    thisframeRate = round(pFrameRateInfo->GetLiveFrameRate());
                }
                else {
                    thisframeRate = pFrameRateInfo->GetLiveFrameRate();
                }
                
                if (thisframeRate != frameRate) {
                    std::cout << "实时帧率：" << thisframeRate << std::endl;
                }

                frameRate = thisframeRate;
            }
        }

        if (_kbhit() != 0) {  //如果键盘被敲击
            char k = _getch();
            switch (k) {
                case 'q':
                    isQuit = 1;
                    break;
                case 'g':
                    Xfer->Grab();
                    std::cout << "\n\n开始捕获\n\n" << std::endl;
                    break;
                case 'p':
                    Xfer->Freeze();
                    std::cout << "\n\n暂停捕获\n\n" << std::endl;
                    break;
                case 'i':
                    Pro->keyControler = 1; // 显示信息
                    break;
                case 'r':

                    //Pro->keyControler = 2; // 开始录制
                    _monitorRecording = true;
                    Buffers->ResetIndex(); // 重置索引
                    Buffers->SetIndex(0);

                    Xfer->Grab();
                    
                    break;
                case 's':
                    Pro->keyControler = 3; // 停止录制
                    std::cout << "\n\n停止录制\n\n" << std::endl;
                    break;
                default:
                    break;
            }
            
        }

        if (isQuit) {
            break;
        }
        // Sleep(1); 
        
    }
    
    Xfer->Freeze();

    if (!Xfer->Wait(5000)) {
        printf("Grab could not stop properly.\n");
    }

    //unregister the acquisition callback
    Acq->UnregisterCallback();

    // Destroy view object
    if (!View->Destroy()) { return false; }
    if (Xfer && *Xfer && !Xfer->Destroy()) { return false; }
    if (!Buffers->Destroy()) { return false; }
    if (!Acq->Destroy()) { return false; }
    
}

void SaperaUse::XferCallback(SapXferCallbackInfo* pInfo)
{
    // 获取sapProcess对象
    RealtimeView* mPro = (RealtimeView*)pInfo->GetContext();
    
    // 执行pro进程（run方法在realtimeProcess中定义）Execute();FileName实时、ExecuteNext：非实时，依次读帧
    //mPro->Execute();
    //mPro->ExecuteNext();

    
    if (mPro->IsRecording()) {
        mPro->ExecuteNext();
    }
    else {
        mPro->Execute();
    }
    
    
}

void SaperaUse::XferCallback2(SapXferCallbackInfo* pInfo)
{
    VideoRecorder* mRec = (VideoRecorder*)pInfo->GetContext();
    //mPro->Execute();
    if (mRec->IsRecording()) {
        mRec->ExecuteNext();
    }
    
}

void SaperaUse::ProCallback(SapProCallbackInfo* pInfo)
{
    //SapView* mView = (SapView*)pInfo->GetContext();

    // 刷新视图
    //mView->Show();
    //this->Execute();
    /*
    RealtimeView* mPro = (RealtimeView*)pInfo->GetContext();

    if (mPro->IsRecording()) {
        mPro->ExecuteNext();
    }
    else {
        mPro->Execute();
    }
    */
}


void SaperaUse::ProCallback2(SapProCallbackInfo* pInfo)
{
    VideoRecorder* mRec = (VideoRecorder*)pInfo->GetContext();
    if (_kbhit() != 0) //如果键盘被敲击
    {
        char k = _getch();
        if (k == 'r')
        {
            mRec->StartRecording();
        }
        else if (k == 's') {
            mRec->StopRecording();
        }
    }
}

/* PRIVATE */

