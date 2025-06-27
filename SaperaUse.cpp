#include "SaperaUse.h"

SaperaUse::SaperaUse()
{

}

SaperaUse::~SaperaUse()
{
    // �˳����
}


bool SaperaUse::GrabbersInit()
{
    // ��ȡϵͳ�еĲɼ�������
    int grabberCount = SapManager::GetServerCount();
    if (grabberCount == 0)
    {
        this->_errorStaus = 0; //no grabber found
        return false;
    }

    // ����ϵͳ�еĲɼ������ҵ�֧�ֲɼ��İ忨��֧�ֵ��豸
    bool serverFound = false;
    std::ostringstream oss;
    int tempCount = 0; 
    for (int serverIndex = 0; serverIndex < grabberCount; serverIndex++)
    {
        // ���ɼ����Ƿ����
        if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcq) != 0)
        {
            // ��ȡ�ɼ�������
            tempCount += 1;
            char serverName[CORSERVER_MAX_STRLEN];
            SapManager::GetServerName(serverIndex, serverName, sizeof(serverName));
            oss << serverName;

            // ��ȡ�豸����
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
    if (!serverFound) // ������һ���ɼ����������
    {
        this->_errorStaus = 1; // no grabbers is available
        return false;
    }

    return true;
}

bool SaperaUse::CreateDevice(int grabberIndex, int deviceIndex, const char* configFilePath)
{
    /* ��ʼ���ɼ����� */
    if (this->_availableGrabberCount <= 0)
    {
        this->_errorStaus = 1; // no grabber available, �˳�
        return false;
    }

    // ��ȡ�ɼ�����
    auto& deviceInfo = _devicesInfo[grabberIndex];
    std::string grabberName = std::get<0>(deviceInfo);

    SapLocation loc(grabberName.c_str(), deviceIndex);

    // Դ����ڵ�
    SapAcquisition* Acq = new SapAcquisition(loc, configFilePath);

    /*������������Ŀ�괫��ڵ㣬�����Դ�ڵ㣨��ɼ�����һ�����������������ݵ���������Դ��
     ������������Դ����ڵ㣬�������ݴ�һ����������Դת�Ƶ���һ��������
    */ 
    SapBufferWithTrash* Buffers = new SapBufferWithTrash(2, Acq);
    if (CONFIG.getRecordMode() == 1) { // ����¼��
        Buffers->SetCount(CONFIG.getBufferCount());
    }
    else if (CONFIG.getRecordMode() == 2) { // �̶�ʱ��¼��
        Buffers->SetCount(CONFIG.getRecordFrame()+5);
        std::cout << "Ԥ��¼��ʱ��(s)��" << CONFIG.getRecordFrame() / CONFIG.getFrameRate() << std::endl;
    }
    
    /*SapView������ڴ�������ʾSapBuffer������Դ�Ĺ��ܡ�����������ʾ��ǰ��������Դ��
    �ض���Դ����δ��ʾ����һ����Դ���ڲ��߳�ʵʱ�Ż���������ʾ����ʹ����Ӧ�ó����߳̿�
    ��ִ�ж����ص�����ʾ�����Զ���ջ�������SapView��SapTransfer����֮��ͬ������
    ��ʵʱ��ʾ�����������ᶪʧ�κ����ݡ� SapHwndDesktop/SapHwndAutomatic
    */
    //SapView* View = new SapView(Buffers, SapHwndAutomatic);
    SapView* View = new SapView(Buffers);

    /*ʵʱ�����*/
    RealtimeView* Pro = new RealtimeView(Buffers, ProCallback, NULL);

    /*SapTransfer��ʵ���˹���ͨ�ô�����̹��ܣ�����һ��Դ�ڵ���Ŀ��ڵ㴫�����ݵĲ�����
    ���м̳���SapXferNode��������඼����Ϊ����ڵ㣺
    */
    SapTransfer* Xfer = new SapAcqToBuf(Acq, Buffers, this->XferCallback, Pro);
    
    /*ʵ��������ʽ¼����*/
    RecordFromBuffer *bufferRecorder = new RecordFromBuffer(Buffers);


    SOCKET serverSocket, clientSocket;
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    char buffer[1024];
    int recvLen;

    // ��ʼ�����ӣ������˿� 12345
    serverSocket = this->initializeConnection("127.0.0.1", 12345);
    if (serverSocket == INVALID_SOCKET) {
        return 1;
    }

    // ���ܿͻ�������
    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!" << std::endl;



    /* �������� */
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
    
    SapXferFrameRateInfo* pFrameRateInfo = Xfer->GetFrameRateStatistics(); // ��ȡ֡����Ϣ
    float frameRate = 0;
    float thisframeRate = 0;
    bool isQuit = false;
    int recBeginBufferIdx = 0;
    while (1) {

        // ����ʽ¼��ģʽ
        if (_monitorRecording && CONFIG.getRecordMode() == 2) {
            int totalFrame = CONFIG.getRecordFrame();  // ��֡��
            int bufferCount = Buffers->GetCount() - 1; // ���Ļ���������
           
            // ֡������
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
            // ��� buffer ��ʱ��ͣ����
            Xfer->Freeze();
            if (!Xfer->Wait(5000)) {
                printf("Grab could not stop properly.\n");
            }
            _monitorRecording = false;
            std::cout << "����¼�ƣ���ͣ����" << std::endl;
            // ����idx���飬�����¼����
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
            // ��buffer�е�֡д�뵽��Ƶ
            std::stringstream ss;
            // ��ȡ��ǰʱ��
            std::time_t t = std::time(0);
            struct tm now;
            localtime_s(&now, &t);

            ss << CONFIG.getSavePath() << CONFIG.getVideoPrefix() << std::put_time(&now, "%Y%m%dT%H%M%S") << CONFIG.getVideoExt();
            std::string filePath = ss.str();

            try {
                if (!CONFIG.getSaveAsFrameSequence()) {
                    std::cout << "���ڱ�����Ƶ..." << std::endl;
                    bufferRecorder->SaveVideo(filePath, GetEncoder(CONFIG.getEncoder()), CONFIG.getFrameRate(),
                        Buffers->GetWidth(), Buffers->GetHeight(), false, bufferIdx);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    std::cout << "��Ƶ�ѱ�����: " << filePath << std::endl;
                    
                }
                else {
                    std::cout << "���ڱ�����Ƶ֡..." << std::endl;
                    
                    std::string fileFolder = filePath.erase(filePath.length() - 4, 4);

                    if (CreateDirectory(fileFolder.c_str(), NULL)) {
                        fileFolder.append("\\\\");
                    }
                    else {
                        std::cerr << "Failed to create directory. Error: " << GetLastError() << std::endl;
                    }
                   
                    bufferRecorder->SaveFrames(fileFolder, bufferIdx);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    std::cout << "��Ƶ֡�ѱ�����: " << fileFolder << std::endl;
                }
                Xfer->Grab();
            }
            catch (const std::exception& e) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                std::cerr << "����ʧ��: " << e.what() << std::endl;
                Xfer->Grab();
            }
            
        }

        if (pFrameRateInfo->IsLiveFrameRateAvailable()) {
            if (!pFrameRateInfo->IsLiveFrameRateStalled()) {
                if (CONFIG.getIsRoundFramerate()) { // ֡����������
                    thisframeRate = round(pFrameRateInfo->GetLiveFrameRate());
                }
                else {
                    thisframeRate = pFrameRateInfo->GetLiveFrameRate();
                }
                if (thisframeRate != frameRate) {
                    std::cout << "ʵʱ֡�ʣ�" << thisframeRate << std::endl;
                }
                frameRate = thisframeRate;
            }
        }

        if (_kbhit() != 0) {  //�������¼�
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
                    std::cout << "\n\n��ʼ����\n\n" << std::endl;
                    break;
                case 'p': case 'P':
                    if (!Xfer->IsGrabbing()) {
                        break;
                    }
                    Xfer->Freeze();
                    std::cout << "\n\n��ͣ����\n\n" << std::endl;
                    break;
                case 'i': case 'I':
                    Pro->keyControler = 1; // ��ʾ��Ϣ
                    break;
                case 'r': case 'R':
                    if (CONFIG.getRecordMode() == 1) {
                        Pro->keyControler = 2; // ��ʼ��ʽ¼��
                        break;
                    } else if (CONFIG.getRecordMode() == 2 && !_monitorRecording) { // ��ʼ����ʽ¼��
                        recBeginBufferIdx = Buffers->GetIndex();
                        if (!Xfer->IsGrabbing()) {
                            std::cout << "\n\nδ��ʼ¼�ƣ��뿪�����沶��\n\n" << std::endl;
                            break;
                        };
                        _monitorRecording = true;
                        // Buffers->Clear(); // ������л�����������
                        
                        std::cout << "\n\n��ʼ����ʽ¼��\n\n" << std::endl;
                        // ��������
                        //
                        
                        // Xfer->Grab();
                        break;
                    }
                case 's': case 'S':
                    if (CONFIG.getRecordMode() == 1) {
                        Pro->keyControler = 3; // ֹͣ¼��
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
    // ��ȡsapProcess����
    RealtimeView* mPro = (RealtimeView*)pInfo->GetContext();
    
    // ִ��pro���̣�run������realtimeProcess�ж��壩Execute();FileNameʵʱ��ExecuteNext����ʵʱ�����ζ�֡
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

    // ˢ����ͼ
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

    // ��ʼ�� Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return INVALID_SOCKET;
    }

    // �����׽���
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // ���÷�������ַ
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());  // ��IP�ַ���ת��Ϊ�����ֽ�˳��ĵ�ַ
    serverAddr.sin_port = htons(port);                    // ���ö˿ں�

    // ���׽��ֵ�ָ���˿�
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // �����ͻ�������
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

