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
        Buffers->SetCount(CONFIG.getRecordFrame()+CONFIG.getBufferOverflow());
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

    bool isQuit = false;
    int recBeginBufferIdx = 0;
    while (1) {
        // ֡�ʼ�� 
        _FrameRateDisp(pFrameRateInfo);
        
        // trigger����ʽ¼��
        if (_isTriggerToRecording) {
			bool res = _TriggerToBufferRecord(Buffers, Xfer, Acq);
            if (!res) {
                if (Xfer->IsGrabbing()) {
                    Xfer->Freeze();
                };
                if (!Xfer->Wait(5000)) {
                    printf("Grab could not stop properly.\n");
                }
                // �رմ���
                Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_ENABLE, 1, 1);


                // ��������
                Xfer->Grab();

                _isTriggerToRecording = false; // ����¼��

                std::cout << "ֹͣtrigger¼��" << std::endl;
            }
            else {
                continue; // ��������ѭ��
            }
        }

        // �������¼�
        if (_kbhit() != 0) {  
            char k = _getch();
            switch (k) {
                case 'q': case 'Q':
                    isQuit = 1;
                    break;
                case 'g': case 'G':
                    if (_isTriggerToRecording) {
                        break;
                    }
                    if (Xfer->IsGrabbing()) {
                        break;
                    }
                    Xfer->Grab();
                    std::cout << "\n\n��ʼ����\n\n" << std::endl;
                    break;
                case 'p': case 'P':
                    if (_isTriggerToRecording) {
                        break;
                    }
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
                    if (CONFIG.getRecordMode() == 1) {  // ��ʼ��ʽ¼��
                        Pro->keyControler = 2; 
                        break;
                    } else if (CONFIG.getRecordMode() == 2 && !_isKeyToRecording) { // ��ʼ����ʽ¼��
                        int triggerMode = CONFIG.getTriigerMode();
                        if (triggerMode == 0) { // ���̴���
                            recBeginBufferIdx = Buffers->GetIndex(); // ��ȡ��ǰ֡����������
                            if (!Xfer->IsGrabbing()) {
                                std::cout << "\n\nδ��ʼ¼�ƣ��뿪�����沶��\n\n" << std::endl;
                                break;
                            };
                            _isKeyToRecording = true;

                            // ��ʾ��Ϣ
                            std::cout << "\n\n��ʼ����ʽ¼��\n\n" << std::endl;
                            std::cout << "ʵʱ֡�ʣ�" << _SteadyFrameRate << "\n��Ƶ֡�ʣ�" << CONFIG.getFrameRate() << std::endl;

                            // ��ʼ¼��
                            _KeyToBufferRecord(Buffers, Xfer, recBeginBufferIdx); 

                        } else if (triggerMode == 1 && !_isTriggerToRecording) { // TTL����
                            if (Xfer->IsGrabbing()) {
                               Xfer->Freeze();
                            };
                            if (!Xfer->Wait(5000)) {
                                printf("Grab could not stop properly.\n");
                            }
                            // ���ô���
                            Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_ENABLE, CORACQ_VAL_EXT_TRIGGER_ON,1);
                            //Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_DETECTION, CORACQ_VAL_RISING_EDGE, 1);
                            //Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_LEVEL, CORACQ_VAL_LEVEL_TTL, 1);
                            // Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_SOURCE, CORACQ_VAL_FRAME_COUNT_1, 1);
							//Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_FRAME_COUNT, CONFIG.getRecordFrame()+CONFIG.getBufferOverflow(), 1);

                            Xfer->Grab(); // ��ʼ�ɼ�

                            _isTriggerToRecording = true;
                            std::cout << "\n\n��ʼ����ʽ¼��\n\n" << std::endl;
                            std::cout << "��Ƶ֡�ʣ�" << CONFIG.getFrameRate() << std::endl;

                        }
                        
                        break;
                    }
                case 's': case 'S':
                    if (CONFIG.getRecordMode() == 1) { // ��ʽ¼��
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


/* PRIVATE */

float SaperaUse::_FrameRateDisp(SapXferFrameRateInfo* FrameRateInfo) {
    float thisframeRate;
    if (FrameRateInfo->IsLiveFrameRateAvailable()) {
        if (!FrameRateInfo->IsLiveFrameRateStalled()) {
            if (CONFIG.getIsRoundFramerate()) { // ֡����������
                thisframeRate = round(FrameRateInfo->GetLiveFrameRate());
            }
            else {
                thisframeRate = FrameRateInfo->GetLiveFrameRate();
            }
            if (thisframeRate != _SteadyFrameRate) {
                std::cout << "ʵʱ֡�ʣ�" << thisframeRate << std::endl;
                _SteadyFrameRate = thisframeRate; // �����ȶ�֡��
            }
            return thisframeRate;
        }
    }
    return _SteadyFrameRate;
}

void SaperaUse::_KeyToBufferRecord(SapBufferWithTrash* mBuffer, SapTransfer* Xfer, int beginBufferIdx) {
	// ʵ����¼����
    RecordFromBuffer* bufferRecorder = new RecordFromBuffer(mBuffer);

    int totalFrame = CONFIG.getRecordFrame();  // ��֡��
    int bufferCount = mBuffer->GetCount() - 1; // ���һ������������

    // ֡������
    int frameCounter = 0;
    int thisIdx;
    int lastIdx = beginBufferIdx;
    while (frameCounter <= totalFrame) {
        thisIdx = mBuffer->GetIndex();
        if (thisIdx >= lastIdx) {
            frameCounter += thisIdx - lastIdx;
        }
        else {
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
    _isKeyToRecording = false;
    std::cout << "����¼�ƣ���ͣ����" << std::endl;

    // ����idx���飬�����¼����
    std::vector<int> bufferIdx(totalFrame);
    bufferIdx[0] = beginBufferIdx;
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
        if (!CONFIG.getSaveAsFrameSequence()) { // ����Ϊ��Ƶ
            std::cout << "���ڱ�����Ƶ..." << std::endl;
            bufferRecorder->SaveVideo(filePath, GetEncoder(CONFIG.getEncoder()), CONFIG.getFrameRate(),
                mBuffer->GetWidth(), mBuffer->GetHeight(), false, bufferIdx);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "��Ƶ�ѱ�����: " << filePath << std::endl;

        }
        else { // ����Ϊ֡����
            std::cout << "���ڱ�����Ƶ֡..." << std::endl;

            std::string fileFolder = filePath.erase(filePath.length() - 4, 4);

            if (CreateDirectory(fileFolder.c_str(), NULL)) {
                fileFolder.append("\\\\");
            }
            else {
                std::cerr << "�����ļ���ʧ��: " << GetLastError() << std::endl;
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


bool SaperaUse::_TriggerToBufferRecord(SapBufferWithTrash* mBuffer, SapTransfer* Xfer, SapAcquisition* Acq) {
    int beginBufferIdx = mBuffer->GetIndex(); // ��ȡ��ǰ����������
    int totalFrame = CONFIG.getRecordFrame();  // ��֡��
    int bufferCount = mBuffer->GetCount() - 1; // ���һ������������

    // ���û��grab��grab
    if (!Xfer->IsGrabbing()) {
        // Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_ENABLE, CORACQ_VAL_EXT_TRIGGER_ON, 1); // �򿪴���
        Xfer->Grab(); // ��ʼ�ɼ�
    };

    // �ȴ�����
    std::cout << "\n�ȴ�Trigger\n����'s'���˳���" << std::endl;
    while (beginBufferIdx == mBuffer->GetIndex()) {
        // std::cout << mBuffer->GetIndex() << std::endl;
        if (_kbhit() != 0) {
            char k = _getch();
            if (k == 's' || k == 'S') {

                return false;
            }
        }
    }
    std::cout << "\n��ʼ¼��\n\n" << std::endl;

	beginBufferIdx = beginBufferIdx + 1; // ��������֡
    // ֡������
    int frameCounter = 0;
    int thisIdx;
    int lastIdx = beginBufferIdx;
    while (frameCounter <= totalFrame) {
        thisIdx = mBuffer->GetIndex();
        if (thisIdx >= lastIdx) {
            frameCounter += thisIdx - lastIdx;
        }
        else {
            frameCounter += (bufferCount - lastIdx) + thisIdx;
        }
        lastIdx = thisIdx;
        
        std::cout << frameCounter << std::endl;
    }

    // ��� buffer ��ʱ��ͣ����
    Xfer->Freeze();
    if (!Xfer->Wait(5000)) {
        printf("Grab could not stop properly.\n");
    }
    // Acq->SetParameter(CORACQ_PRM_EXT_TRIGGER_ENABLE, CORACQ_VAL_EXT_TRIGGER_ON, 0); // �رմ���
    std::cout << "����¼��" << std::endl;

    //// ¼��
    // ʵ����¼����
    RecordFromBuffer* bufferRecorder = new RecordFromBuffer(mBuffer);

    // ����idx���飬�����¼����
    std::vector<int> bufferIdx(totalFrame);
    bufferIdx[0] = beginBufferIdx;
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
        if (!CONFIG.getSaveAsFrameSequence()) { // ����Ϊ��Ƶ
            std::cout << "���ڱ�����Ƶ..." << std::endl;
            bufferRecorder->SaveVideo(filePath, GetEncoder(CONFIG.getEncoder()), CONFIG.getFrameRate(),
                mBuffer->GetWidth(), mBuffer->GetHeight(), false, bufferIdx);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "��Ƶ�ѱ�����: " << filePath << std::endl;

        }
        else { // ����Ϊ֡����
            std::cout << "���ڱ�����Ƶ֡..." << std::endl;

            std::string fileFolder = filePath.erase(filePath.length() - 4, 4);

            if (CreateDirectory(fileFolder.c_str(), NULL)) {
                fileFolder.append("\\\\");
            }
            else {
                std::cerr << "�����ļ���ʧ��: " << GetLastError() << std::endl;
            }

            bufferRecorder->SaveFrames(fileFolder, bufferIdx);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "��Ƶ֡�ѱ�����: " << fileFolder << std::endl;

        }

        return true;
    }
    catch (const std::exception& e) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cerr << "����ʧ��: " << e.what() << std::endl;
        return false;
    }
}