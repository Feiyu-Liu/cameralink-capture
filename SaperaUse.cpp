#include "SaperaUse.h"

SaperaUse::SaperaUse()
{

}

SaperaUse::~SaperaUse()
{
    // 退出相机
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
    if (CONFIG.getRecordMode() == 1) { // 自由录制
        Buffers->SetCount(CONFIG.getBufferCount());
    }
    else if (CONFIG.getRecordMode() == 2) { // 固定时长录制
        Buffers->SetCount(CONFIG.getRecordFrame()+5);
        std::cout << "预估录制时长(s)：" << CONFIG.getRecordFrame() / CONFIG.getFrameRate() << std::endl;
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

    /*SapTransfer类实现了管理通用传输过程功能，即从一个源节点向目标节点传输数据的操作。
    所有继承自SapXferNode类的以下类都被视为传输节点：
    */
    SapTransfer* Xfer = new SapAcqToBuf(Acq, Buffers, this->XferCallback, Pro);
    
    /*实例化非流式录制器*/
    RecordFromBuffer *bufferRecorder = new RecordFromBuffer(Buffers);


    SOCKET serverSocket, clientSocket;
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    char buffer[1024];
    int recvLen;

    // 初始化连接，监听端口 12345
    serverSocket = this->initializeConnection("127.0.0.1", 12345);
    if (serverSocket == INVALID_SOCKET) {
        return 1;
    }

    // 接受客户端连接
    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!" << std::endl;



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
    if (Xfer && !Xfer->Create()) {
        this->_errorStaus = 4; // fail to creat Xfer
        return false;
    }



    // Start continous grab
    Xfer->Grab();
    
    SapXferFrameRateInfo* pFrameRateInfo = Xfer->GetFrameRateStatistics(); // 获取帧率信息
    float frameRate = 0;
    float thisframeRate = 0;
    bool isQuit = false;
    int recBeginBufferIdx = 0;
    while (1) {

        // 非流式录制模式
        if (_monitorRecording && CONFIG.getRecordMode() == 2) {
            int totalFrame = CONFIG.getRecordFrame();  // 总帧数
            int bufferCount = Buffers->GetCount() - 1; // 最后的缓冲区索引
           
            // 帧计数器
            int frameCounter = 0; 
            int thisIdx;
            int lastIdx = recBeginBufferIdx;
            while (frameCounter <= totalFrame) {
                thisIdx = Buffers->GetIndex();
                if (thisIdx >= lastIdx) {
                    frameCounter += thisIdx-lastIdx;
                } else {
                    frameCounter += (bufferCount - lastIdx) + thisIdx;
                }
                lastIdx = thisIdx;
                // std::cout << frameCounter << std::endl;
            }
            // 监控 buffer 满时暂停捕获
            Xfer->Freeze();
            if (!Xfer->Wait(5000)) {
                printf("Grab could not stop properly.\n");
            }
            _monitorRecording = false;
            std::cout << "结束录制，暂停捕获" << std::endl;
            // 构建idx数组，传入给录制器
            std::vector<int> bufferIdx(totalFrame);
            bufferIdx[0] = recBeginBufferIdx;
            for (int m = 1; m < totalFrame; m++) {
                if (bufferIdx[m - 1] == bufferCount) {
                    bufferIdx[m] = 0;
                }
                else {
                    bufferIdx[m] = bufferIdx[m - 1] + 1;
                }
            }
            // 将buffer中的帧写入到视频
            std::stringstream ss;
            // 获取当前时间
            std::time_t t = std::time(0);
            struct tm now;
            localtime_s(&now, &t);

            ss << CONFIG.getSavePath() << CONFIG.getVideoPrefix() << std::put_time(&now, "%Y%m%dT%H%M%S") << CONFIG.getVideoExt();
            std::string filePath = ss.str();

            try {
                if (!CONFIG.getSaveAsFrameSequence()) {
                    std::cout << "正在保存视频..." << std::endl;
                    bufferRecorder->SaveVideo(filePath, GetEncoder(CONFIG.getEncoder()), CONFIG.getFrameRate(),
                        Buffers->GetWidth(), Buffers->GetHeight(), false, bufferIdx);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    std::cout << "视频已保存至: " << filePath << std::endl;
                    
                }
                else {
                    std::cout << "正在保存视频帧..." << std::endl;
                    
                    std::string fileFolder = filePath.erase(filePath.length() - 4, 4);

                    if (CreateDirectory(fileFolder.c_str(), NULL)) {
                        fileFolder.append("\\\\");
                    }
                    else {
                        std::cerr << "Failed to create directory. Error: " << GetLastError() << std::endl;
                    }
                   
                    bufferRecorder->SaveFrames(fileFolder, bufferIdx);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    std::cout << "视频帧已保存至: " << fileFolder << std::endl;
                }
                Xfer->Grab();
            }
            catch (const std::exception& e) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                std::cerr << "保存失败: " << e.what() << std::endl;
                Xfer->Grab();
            }
            
        }

        if (pFrameRateInfo->IsLiveFrameRateAvailable()) {
            if (!pFrameRateInfo->IsLiveFrameRateStalled()) {
                if (CONFIG.getIsRoundFramerate()) { // 帧率四舍五入
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

        if (_kbhit() != 0) {  //监测键盘事件
            char k = _getch();
            switch (k) {
                case 'q': case 'Q':
                    isQuit = 1;
                    break;
                case 'g': case 'G':
                    if (Xfer->IsGrabbing()) {
                        break;
                    }
                    Xfer->Grab();
                    std::cout << "\n\n开始捕获\n\n" << std::endl;
                    break;
                case 'p': case 'P':
                    if (!Xfer->IsGrabbing()) {
                        break;
                    }
                    Xfer->Freeze();
                    std::cout << "\n\n暂停捕获\n\n" << std::endl;
                    break;
                case 'i': case 'I':
                    Pro->keyControler = 1; // 显示信息
                    break;
                case 'r': case 'R':
                    if (CONFIG.getRecordMode() == 1) {
                        Pro->keyControler = 2; // 开始流式录制
                        break;
                    } else if (CONFIG.getRecordMode() == 2 && !_monitorRecording) { // 开始非流式录制
                        recBeginBufferIdx = Buffers->GetIndex();
                        if (!Xfer->IsGrabbing()) {
                            std::cout << "\n\n未开始录制，请开启画面捕获\n\n" << std::endl;
                            break;
                        };
                        _monitorRecording = true;
                        // Buffers->Clear(); // 清除所有缓冲区的内容
                        
                        std::cout << "\n\n开始非流式录制\n\n" << std::endl;
                        // 重置索引
                        //
                        
                        // Xfer->Grab();
                        break;
                    }
                case 's': case 'S':
                    if (CONFIG.getRecordMode() == 1) {
                        Pro->keyControler = 3; // 停止录制
                        break;
                    }
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
    
    

    if (mPro->IsRecording() && CONFIG.getExecuteNext()) {
        mPro->ExecuteNext();
    }
    else {
        mPro->Execute();
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


SOCKET SaperaUse::initializeConnection(const std::string& ip, int port) {
    WSADATA wsaData;
    SOCKET serverSocket;
    sockaddr_in serverAddr;

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return INVALID_SOCKET;
    }

    // 创建套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // 设置服务器地址
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());  // 将IP字符串转换为网络字节顺序的地址
    serverAddr.sin_port = htons(port);                    // 设置端口号

    // 绑定套接字到指定端口
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // 监听客户端连接
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    std::cout << "Server is listening on " << ip << ":" << port << std::endl;

    return serverSocket;
}

/* PRIVATE */

