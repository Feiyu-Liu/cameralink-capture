#include "SaperaUse.h"

SaperaUse::SaperaUse()
{

}

SaperaUse::~SaperaUse()
{
    // �˳������ʱ��ǵ�д
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
    if (RECORD_MODE == 1) { // ����¼��
        Buffers->SetCount(BUFFER_COUNT);
    }
    else if (RECORD_MODE == 2) { // �̶�ʱ��¼��
        Buffers->SetCount(RECORD_FRAME+5);
        std::cout << "Ԥ��¼��ʱ����" << RECORD_FRAME/FRAME_RATE << std::endl;
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
    //VideoRecorder* Rec = new VideoRecorder(Buffers, ProCallback2, this);

    /*SapTransfer��ʵ���˹���ͨ�ô�����̹��ܣ�����һ��Դ�ڵ���Ŀ��ڵ㴫�����ݵĲ�����
    ���м̳���SapXferNode��������඼����Ϊ����ڵ㣺
    */
    SapTransfer* Xfer = new SapAcqToBuf(Acq, Buffers, this->XferCallback, Pro);
    //SapTransfer* Xfer2 = new SapAcqToBuf(Acq, Buffers, this->XferCallback2, Rec);


    RecordFromBuffer *bufferRecorder = new RecordFromBuffer(Buffers);


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
        // �̶�֡��¼��ģʽ�¼�� buffer ��ʱ��ͣ������
        if (_monitorRecording && RECORD_MODE == 2) {
            //std::cout << Buffers->GetIndex() << std::endl;
            if (Buffers->GetIndex() > RECORD_FRAME) {
                Xfer->Freeze();
                _monitorRecording = false;
                if (!Xfer->Wait(5000)) {
                    printf("Grab could not stop properly.\n");
                }
                std::cout << "��ͣ����: " << Buffers->GetIndex() << std::endl;

                std::stringstream ss;
                ss << SAVE_PATH << VIDEO_FILE_NAME;
                std::string filePath = ss.str();
                bool isSaved = bufferRecorder->SaveVideo(filePath, ENCODER, FRAME_RATE, 
                    Buffers->GetWidth(), Buffers->GetHeight(), false, Buffers->GetIndex());
                if (isSaved) {
                    std::cout << "��Ƶ�ѱ�����: " << filePath << std::endl;
                }
                else {
                    std::cout << "��Ƶ����ʧ��" << std::endl;
                }
            }
        }

        
        if (pFrameRateInfo->IsLiveFrameRateAvailable()) {
            if (!pFrameRateInfo->IsLiveFrameRateStalled()) {
                // ֡����������
                if (IS_ROUND_FRAMERATE) {
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

        if (_kbhit() != 0) {  //������̱��û�
            char k = _getch();
            switch (k) {
                case 'q':
                    isQuit = 1;
                    break;
                case 'g':
                    Xfer->Grab();
                    std::cout << "\n\n��ʼ����\n\n" << std::endl;
                    break;
                case 'p':
                    Xfer->Freeze();
                    std::cout << "\n\n��ͣ����\n\n" << std::endl;
                    break;
                case 'i':
                    Pro->keyControler = 1; // ��ʾ��Ϣ
                    break;
                case 'r':

                    //Pro->keyControler = 2; // ��ʼ¼��
                    _monitorRecording = true;
                    Buffers->ResetIndex(); // ��������
                    Buffers->SetIndex(0);

                    Xfer->Grab();
                    
                    break;
                case 's':
                    Pro->keyControler = 3; // ֹͣ¼��
                    std::cout << "\n\nֹͣ¼��\n\n" << std::endl;
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


void SaperaUse::ProCallback2(SapProCallbackInfo* pInfo)
{
    VideoRecorder* mRec = (VideoRecorder*)pInfo->GetContext();
    if (_kbhit() != 0) //������̱��û�
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

