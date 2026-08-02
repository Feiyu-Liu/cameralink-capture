#include "SaperaUse.h"

#include "RealtimeView.h"
#include "RecordFromBuffer.h"
#include "SaperaResource.h"
#include "config.h"

#include <conio.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>

namespace {

std::chrono::milliseconds CaptureTimeout(int frameCount, int frameRate) {
    const auto expectedMilliseconds =
        static_cast<std::int64_t>(frameCount) * 1000 / std::max(frameRate, 1);
    return std::chrono::milliseconds(std::max<std::int64_t>(5000, expectedMilliseconds * 3 + 2000));
}

std::string BuildOutputPath() {
    std::stringstream stream;
    const std::time_t timestamp = std::time(nullptr);
    struct tm localTime {};
    localtime_s(&localTime, &timestamp);
    stream << CONFIG.getSavePath()
           << CONFIG.getVideoPrefix()
           << std::put_time(&localTime, "%Y%m%dT%H%M%S")
           << CONFIG.getVideoExt();
    return stream.str();
}

void PrintCleanupError(const char* operation) noexcept {
    try {
        std::cerr << "Cleanup failed: " << operation << std::endl;
    }
    catch (...) {
    }
}

bool ValidateCcfStructure(const char* path, std::string& error) {
    struct RequiredEntry {
        const char* section;
        const char* key;
    };
    constexpr std::array<RequiredEntry, 7> requiredEntries {{
        { "Board", "Server Name" },
        { "Board", "Device Name" },
        { "General", "Version" },
        { "Output", "Output Format" },
        { "Signal Description", "Pixel Depth" },
        { "Stream Conditioning", "Crop Width" },
        { "Stream Conditioning", "Crop Height" }
    }};

    std::array<char, 256> value {};
    for (const RequiredEntry& entry : requiredEntries) {
        value.fill('\0');
        if (GetPrivateProfileStringA(
                entry.section,
                entry.key,
                "",
                value.data(),
                static_cast<DWORD>(value.size()),
                path) == 0) {
            error = std::string("CCF is missing [") + entry.section + "] " + entry.key + '.';
            return false;
        }
    }

    const int width = GetPrivateProfileIntA("Stream Conditioning", "Crop Width", 0, path);
    const int height = GetPrivateProfileIntA("Stream Conditioning", "Crop Height", 0, path);
    if (width <= 0 || height <= 0) {
        error = "CCF crop width and height must be positive.";
        return false;
    }
    error.clear();
    return true;
}

} // namespace

struct SaperaUse::CameraSession {
    SapOwned<SapAcquisition> acquisition;
    SapOwned<SapBufferWithTrash> buffers;
    std::unique_ptr<RealtimeView> processor;
    SapOwned<SapTransfer> transfer;
    FrameLayout frameLayout;
    FrameArrivalTracker tracker;

    ~CameraSession() noexcept {
        Close();
    }

    bool Close() noexcept {
        bool success = true;

        if (transfer && static_cast<bool>(*transfer)) {
            if (transfer->IsGrabbing()) {
                if (!transfer->Freeze()) {
                    PrintCleanupError("SapTransfer::Freeze");
                    success = false;
                }
                if (!transfer->Wait(5000)) {
                    PrintCleanupError("SapTransfer::Wait");
                    success = false;
                    if (!transfer->Abort() || !transfer->Wait(5000)) {
                        PrintCleanupError("SapTransfer::Abort/Wait");
                    }
                }
            }
        }

        // Constructor callbacks are unregistered by Destroy(); UnregisterCallback()
        // only applies to extended events added through RegisterCallback().
        if (!transfer.Destroy()) {
            PrintCleanupError("SapTransfer::Destroy");
            success = false;
        }
        transfer.Reset();

        if (processor) {
            if (!processor->Shutdown()) {
                PrintCleanupError("RealtimeView::Shutdown");
                success = false;
            }
            processor.reset();
        }

        if (!buffers.Destroy()) {
            PrintCleanupError("SapBuffer::Destroy");
            success = false;
        }
        buffers.Reset();

        if (!acquisition.Destroy()) {
            PrintCleanupError("SapAcquisition::Destroy");
            success = false;
        }
        acquisition.Reset();
        return success;
    }
};

SaperaUse::SaperaUse() = default;

SaperaUse::~SaperaUse() {
    Shutdown();
}

bool SaperaUse::Shutdown() noexcept {
    _isTriggerToRecording = false;
    if (!_session) {
        return true;
    }
    const bool success = _session->Close();
    _session.reset();
    return success;
}

bool SaperaUse::GrabbersInit() {
    _devicesInfo.clear();
    _availableGrabberCount = 0;

    const int grabberCount = SapManager::GetServerCount();
    if (grabberCount == 0) {
        _errorStaus = 0;
        return false;
    }

    for (int serverIndex = 0; serverIndex < grabberCount; ++serverIndex) {
        if (SapManager::GetResourceCount(serverIndex, SapManager::ResourceAcq) == 0) {
            continue;
        }

        char serverName[CORSERVER_MAX_STRLEN] {};
        if (!SapManager::GetServerName(serverIndex, serverName, sizeof(serverName))) {
            continue;
        }

        const int deviceCount = SapManager::GetResourceCount(serverName, SapManager::ResourceAcq);
        std::vector<std::string> deviceNames;
        deviceNames.reserve(deviceCount);
        for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex) {
            char deviceName[CORPRM_GETSIZE(CORACQ_PRM_LABEL)] {};
            if (SapManager::GetResourceName(
                    serverName,
                    SapManager::ResourceAcq,
                    deviceIndex,
                    deviceName,
                    sizeof(deviceName))) {
                deviceNames.emplace_back(deviceName);
            }
            else {
                deviceNames.emplace_back("<unknown>");
            }
        }
        _devicesInfo.emplace_back(serverName, std::move(deviceNames));
    }

    _availableGrabberCount = static_cast<int>(_devicesInfo.size());
    if (_availableGrabberCount == 0) {
        _errorStaus = 1;
        return false;
    }
    return true;
}

bool SaperaUse::_ValidateRuntimeConfig(std::string& error) const {
    if (CONFIG.getRecordMode() != 1 && CONFIG.getRecordMode() != 2) {
        error = "RecordMode must be 1 or 2.";
        return false;
    }
    if (CONFIG.getFrameRate() <= 0) {
        error = "FrameRate must be positive.";
        return false;
    }
    if (CONFIG.getRecordMode() == 1) {
        if (CONFIG.getBufferCount() <= 0) {
            error = "BufferCount must be positive in streaming mode.";
            return false;
        }
        return true;
    }

    if (CONFIG.getRecordFrame() <= 0) {
        error = "RecordFrame must be positive.";
        return false;
    }
    if (CONFIG.getBufferOverflow() < 1) {
        error = "BufferOverflow must be at least 1.";
        return false;
    }
    if (CONFIG.getTriigerMode() != 0 && CONFIG.getTriigerMode() != 1) {
        error = "Only keyboard and TTL trigger modes are implemented.";
        return false;
    }

    const std::int64_t skipCount = CONFIG.getTriigerMode() == 1 ? 1 : 0;
    const std::int64_t bufferCount = static_cast<std::int64_t>(CONFIG.getRecordFrame()) +
        CONFIG.getBufferOverflow() + skipCount;
    if (bufferCount > std::numeric_limits<int>::max()) {
        error = "Requested buffer count exceeds the supported integer range.";
        return false;
    }
    return true;
}

bool SaperaUse::CreateDevice(int grabberIndex, int deviceIndex, const char* configFilePath) {
    Shutdown();

    std::string error;
    if (!_ValidateRuntimeConfig(error)) {
        std::cerr << "Invalid configuration: " << error << std::endl;
        return false;
    }
    if (grabberIndex < 0 || grabberIndex >= static_cast<int>(_devicesInfo.size())) {
        std::cerr << "GrabberIndex is out of range: " << grabberIndex << std::endl;
        return false;
    }

    const auto& deviceInfo = _devicesInfo.at(grabberIndex);
    const std::string& grabberName = std::get<0>(deviceInfo);
    const auto& deviceNames = std::get<1>(deviceInfo);
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(deviceNames.size())) {
        std::cerr << "CameraIndex is out of range: " << deviceIndex << std::endl;
        return false;
    }
    if (configFilePath == nullptr || configFilePath[0] == '\0' ||
        !std::filesystem::exists(std::filesystem::path(configFilePath))) {
        std::cerr << "Grabber configuration file does not exist: "
                  << (configFilePath == nullptr ? "<null>" : configFilePath) << std::endl;
        return false;
    }
    if (!ValidateCcfStructure(configFilePath, error)) {
        std::cerr << "Invalid grabber configuration file: " << error << std::endl;
        return false;
    }

    auto session = std::make_unique<CameraSession>();
    const SapLocation location(grabberName.c_str(), deviceIndex);
    session->acquisition.Reset(std::make_unique<SapAcquisition>(location, configFilePath));
    if (!session->acquisition->Create()) {
        _errorStaus = 2;
        std::cerr << "Failed to create SapAcquisition." << std::endl;
        return false;
    }

    session->buffers.Reset(std::make_unique<SapBufferWithTrash>(2, session->acquisition.Get()));
    int bufferCount = CONFIG.getBufferCount();
    if (CONFIG.getRecordMode() == 2) {
        const int skipCount = CONFIG.getTriigerMode() == 1 ? 1 : 0;
        bufferCount = CONFIG.getRecordFrame() + CONFIG.getBufferOverflow() + skipCount;
        std::cout << "Estimated recording duration (s): "
                  << static_cast<double>(CONFIG.getRecordFrame()) / CONFIG.getFrameRate()
                  << std::endl;
    }
    if (!session->buffers->SetCount(bufferCount) || !session->buffers->Create()) {
        _errorStaus = 3;
        std::cerr << "Failed to create " << bufferCount << " Sapera buffers." << std::endl;
        return false;
    }

    if (!FrameLayout::FromSapBuffer(*session->buffers, session->frameLayout, error)) {
        std::cerr << "Unsupported camera buffer: " << error << std::endl;
        return false;
    }

    session->processor = std::make_unique<RealtimeView>(
        session->buffers.Get(),
        session->frameLayout,
        nullptr,
        nullptr);
    if (!session->processor->Create()) {
        _errorStaus = 6;
        std::cerr << "Failed to create RealtimeView." << std::endl;
        return false;
    }

    session->transfer.Reset(std::make_unique<SapAcqToBuf>(
        session->acquisition.Get(),
        session->buffers.Get(),
        XferCallback,
        session.get()));
    SapXferPair* pair = session->transfer->GetPair(0);
    if (pair == nullptr || !pair->SetTrashCallbackInfo(XferCallback, session.get())) {
        _errorStaus = 4;
        std::cerr << "Failed to register the Sapera trash callback." << std::endl;
        return false;
    }
    if (!session->transfer->Create()) {
        _errorStaus = 4;
        std::cerr << "Failed to create SapTransfer." << std::endl;
        return false;
    }

    session->tracker.Reset(session->buffers->GetCount(), session->buffers->GetIndex());
    std::cout << "Selected grabber: " << grabberName
              << ", device: " << deviceNames.at(deviceIndex) << std::endl;
    std::cout << "Buffer layout: " << session->frameLayout.width << 'x'
              << session->frameLayout.height << ", Mono8, pitch="
              << session->frameLayout.pitch << ", buffers="
              << session->buffers->GetCount() << std::endl;

    _session = std::move(session);
    CameraSession& active = *_session;
    if (!active.transfer->Grab()) {
        std::cerr << "Failed to start continuous acquisition." << std::endl;
        Shutdown();
        return false;
    }

    SapXferFrameRateInfo* frameRateInfo = active.transfer->GetFrameRateStatistics();
    bool quit = false;
    while (!quit) {
        _FrameRateDisp(frameRateInfo);

        if (_isTriggerToRecording) {
            if (!_TriggerToBufferRecord(active)) {
                std::string stopError;
                _StopTransfer(active, stopError);
                active.acquisition->SetParameter(
                    CORACQ_PRM_EXT_TRIGGER_ENABLE,
                    CORACQ_VAL_EXT_TRIGGER_OFF,
                    TRUE);
                if (!active.transfer->Grab()) {
                    std::cerr << "Failed to resume free-running acquisition." << std::endl;
                    quit = true;
                }
                _isTriggerToRecording = false;
                std::cout << "TTL recording stopped." << std::endl;
            }
            continue;
        }

        if (_kbhit() == 0) {
            continue;
        }

        const char key = static_cast<char>(_getch());
        switch (key) {
        case 'q': case 'Q':
            quit = true;
            break;
        case 'g': case 'G':
            if (!active.transfer->IsGrabbing() && active.transfer->Grab()) {
                std::cout << "Acquisition started." << std::endl;
            }
            break;
        case 'p': case 'P':
            if (active.transfer->IsGrabbing()) {
                std::string stopError;
                if (_StopTransfer(active, stopError)) {
                    std::cout << "Acquisition paused." << std::endl;
                }
                else {
                    std::cerr << stopError << std::endl;
                }
            }
            break;
        case 'i': case 'I':
            active.processor->keyControler = 1;
            break;
        case 'r': case 'R':
            if (CONFIG.getRecordMode() == 1) {
                active.processor->keyControler = 2;
            }
            else if (CONFIG.getTriigerMode() == 0) {
                if (!active.transfer->IsGrabbing()) {
                    std::cerr << "Start acquisition before recording." << std::endl;
                }
                else if (!_KeyToBufferRecord(active)) {
                    std::cerr << "Keyboard-triggered recording failed." << std::endl;
                }
            }
            else {
                _isTriggerToRecording = true;
            }
            break;
        case 's': case 'S':
            if (CONFIG.getRecordMode() == 1) {
                active.processor->keyControler = 3;
            }
            break;
        default:
            break;
        }
    }

    return Shutdown();
}

void SaperaUse::XferCallback(SapXferCallbackInfo* info) {
    if (info == nullptr) {
        return;
    }
    auto* session = static_cast<CameraSession*>(info->GetContext());
    if (session == nullptr || !session->buffers || !session->processor) {
        return;
    }

    if (info->IsTrash()) {
        session->tracker.OnTrashFrame(info->GetEventCount());
        return;
    }

    session->tracker.OnFrame(session->buffers->GetIndex(), info->GetEventCount());
    if (session->processor->IsRecording() && CONFIG.getExecuteNext()) {
        session->processor->ExecuteNext();
    }
    else {
        session->processor->Execute();
    }
}

float SaperaUse::_FrameRateDisp(SapXferFrameRateInfo* frameRateInfo) {
    if (frameRateInfo == nullptr || !frameRateInfo->IsLiveFrameRateAvailable() ||
        frameRateInfo->IsLiveFrameRateStalled()) {
        return _SteadyFrameRate;
    }

    const float currentFrameRate = CONFIG.getIsRoundFramerate()
        ? std::round(frameRateInfo->GetLiveFrameRate())
        : frameRateInfo->GetLiveFrameRate();
    if (currentFrameRate != _SteadyFrameRate) {
        std::cout << "Live frame rate: " << currentFrameRate << std::endl;
        _SteadyFrameRate = currentFrameRate;
    }
    return currentFrameRate;
}

bool SaperaUse::_StopTransfer(CameraSession& session, std::string& error) const {
    if (!session.transfer || !static_cast<bool>(*session.transfer) ||
        !session.transfer->IsGrabbing()) {
        error.clear();
        return true;
    }

    if (!session.transfer->Freeze()) {
        error = "SapTransfer::Freeze failed.";
        session.transfer->Abort();
        session.transfer->Wait(5000);
        return false;
    }
    if (!session.transfer->Wait(5000)) {
        session.transfer->Abort();
        if (!session.transfer->Wait(5000)) {
            error = "SapTransfer did not stop after Freeze and Abort.";
            return false;
        }
    }
    error.clear();
    return true;
}

bool SaperaUse::_KeyToBufferRecord(CameraSession& session) {
    std::string error;
    const int frameCount = CONFIG.getRecordFrame();
    if (!session.tracker.Arm(static_cast<std::size_t>(frameCount), 0, error)) {
        std::cerr << "Could not arm keyboard capture: " << error << std::endl;
        return false;
    }

    std::cout << "Buffered recording started." << std::endl;
    const CaptureWindow window = session.tracker.WaitForCompletion(
        CaptureTimeout(frameCount, CONFIG.getFrameRate()));
    const bool stopped = _StopTransfer(session, error);
    if (!stopped) {
        std::cerr << error << std::endl;
    }

    bool saved = false;
    if (stopped && window.status == CaptureWindowStatus::Complete) {
        saved = _SaveCaptureWindow(session, window);
    }
    else if (window.status != CaptureWindowStatus::Complete) {
        std::cerr << "Capture failed [" << CaptureWindowStatusName(window.status)
                  << "]: " << window.message << std::endl;
    }

    if (!session.transfer->Grab()) {
        std::cerr << "Failed to resume acquisition after buffered recording." << std::endl;
        return false;
    }
    return saved;
}

bool SaperaUse::_TriggerToBufferRecord(CameraSession& session) {
    std::string error;
    if (!_StopTransfer(session, error)) {
        std::cerr << error << std::endl;
        return false;
    }

    const int frameCount = CONFIG.getRecordFrame();
    const int triggerFrameCount = frameCount + 1 + CONFIG.getBufferOverflow();
    if (!session.acquisition->SetParameter(
            CORACQ_PRM_EXT_TRIGGER_ENABLE,
            CORACQ_VAL_EXT_TRIGGER_ON,
            TRUE) ||
        !session.acquisition->SetParameter(
            CORACQ_PRM_EXT_TRIGGER_DETECTION,
            CORACQ_VAL_RISING_EDGE,
            TRUE) ||
        !session.acquisition->SetParameter(
            CORACQ_PRM_EXT_TRIGGER_LEVEL,
            CORACQ_VAL_LEVEL_TTL,
            TRUE) ||
        !session.acquisition->SetParameter(
            CORACQ_PRM_EXT_TRIGGER_FRAME_COUNT,
            triggerFrameCount,
            TRUE)) {
        std::cerr << "Failed to configure TTL trigger parameters." << std::endl;
        return false;
    }

    if (!session.tracker.Arm(static_cast<std::size_t>(frameCount), 1, error)) {
        std::cerr << "Could not arm TTL capture: " << error << std::endl;
        return false;
    }
    if (!session.transfer->Grab()) {
        session.tracker.Cancel("SapTransfer::Grab failed.");
        return false;
    }

    std::cout << "Waiting for TTL trigger (press S to cancel)." << std::endl;
    while (!session.tracker.WaitForFirstEvent(std::chrono::milliseconds(50))) {
        const CaptureWindow snapshot = session.tracker.Snapshot();
        if (snapshot.status != CaptureWindowStatus::Armed) {
            break;
        }
        if (_kbhit() != 0) {
            const char key = static_cast<char>(_getch());
            if (key == 's' || key == 'S') {
                session.tracker.Cancel("TTL capture was cancelled by the user.");
                _StopTransfer(session, error);
                return false;
            }
        }
    }

    std::cout << "TTL trigger received; recording started." << std::endl;
    const CaptureWindow window = session.tracker.WaitForCompletion(
        CaptureTimeout(frameCount + 1, CONFIG.getFrameRate()));
    const bool stopped = _StopTransfer(session, error);
    if (!stopped) {
        std::cerr << error << std::endl;
        return false;
    }
    if (window.status != CaptureWindowStatus::Complete) {
        std::cerr << "TTL capture failed [" << CaptureWindowStatusName(window.status)
                  << "]: " << window.message << std::endl;
        return false;
    }
    return _SaveCaptureWindow(session, window);
}

bool SaperaUse::_SaveCaptureWindow(CameraSession& session, const CaptureWindow& window) {
    std::string error;
    if (!IsCaptureWindowIntact(
            window,
            session.tracker.TotalNormalFrames(),
            session.buffers->GetCount(),
            error)) {
        std::cerr << "Capture window is not safe to save: " << error << std::endl;
        return false;
    }

    std::vector<int> indices;
    if (!BuildCircularIndices(
            window.firstIndex,
            window.frameCount,
            session.buffers->GetCount(),
            indices,
            error)) {
        std::cerr << "Could not build capture indices: " << error << std::endl;
        return false;
    }

    const std::string outputPath = BuildOutputPath();
    RecordFromBuffer recorder(*session.buffers, session.frameLayout);
    try {
        if (!CONFIG.getSaveAsFrameSequence()) {
            std::cout << "Saving video..." << std::endl;
            if (!recorder.SaveVideo(
                    outputPath,
                    GetEncoder(CONFIG.getEncoder()),
                    CONFIG.getFrameRate(),
                    session.frameLayout.width,
                    session.frameLayout.height,
                    false,
                    indices)) {
                std::cerr << "Video save failed: " << outputPath << std::endl;
                return false;
            }
            std::cout << "Video saved to: " << outputPath << std::endl;
            return true;
        }

        std::filesystem::path folder(outputPath);
        folder.replace_extension();
        std::error_code fileError;
        std::filesystem::create_directories(folder, fileError);
        if (fileError) {
            std::cerr << "Could not create frame folder: " << fileError.message() << std::endl;
            return false;
        }
        if (!recorder.SaveFrames(folder.string(), indices)) {
            std::cerr << "Frame sequence save failed: " << folder.string() << std::endl;
            return false;
        }
        std::cout << "Frames saved to: " << folder.string() << std::endl;
        return true;
    }
    catch (const std::exception& exception) {
        std::cerr << "Capture save failed: " << exception.what() << std::endl;
        return false;
    }
}
