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
#include <utility>

namespace {

template <typename... Values>
void Log(ILogSink* sink, LogLevel level, Values&&... values) noexcept {
    try {
        std::ostringstream stream;
        (stream << ... << std::forward<Values>(values));
        WriteLog(sink, level, stream.str());
    }
    catch (...) {
    }
}

std::chrono::milliseconds CaptureTimeout(int frameCount, int frameRate) {
    const auto expectedMilliseconds =
        static_cast<std::int64_t>(frameCount) * 1000 / std::max(frameRate, 1);
    return std::chrono::milliseconds(std::max<std::int64_t>(5000, expectedMilliseconds * 3 + 2000));
}

CaptureWindow WaitForCaptureCompletion(
    FrameArrivalTracker& tracker,
    std::chrono::milliseconds timeout,
    CommandQueue* commands,
    bool stopCancels,
    const char* cancellationMessage) {
    if (commands == nullptr) {
        return tracker.WaitForCompletion(timeout);
    }

    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (tracker.Snapshot().status == CaptureWindowStatus::Armed) {
        const std::optional<CaptureCommand> cancellation = commands->PendingCancellation();
        if (cancellation == CaptureCommand::Quit ||
            (stopCancels && cancellation == CaptureCommand::Stop)) {
            tracker.Cancel(cancellationMessage);
            if (cancellation == CaptureCommand::Stop) {
                commands->ClearStop();
            }
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            tracker.Timeout("Timed out while waiting for the capture window.");
            break;
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        tracker.WaitForTerminal(std::min(remaining, std::chrono::milliseconds(50)));
    }
    return tracker.Snapshot();
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

void PrintCleanupError(ILogSink* sink, const char* operation) noexcept {
    Log(sink, LogLevel::Error, "Cleanup failed: ", operation);
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
    ILogSink* logSink = nullptr;

    ~CameraSession() noexcept {
        Close();
    }

    bool Close() noexcept {
        bool success = true;

        if (transfer && static_cast<bool>(*transfer)) {
            if (transfer->IsGrabbing()) {
                if (!transfer->Freeze()) {
                    PrintCleanupError(logSink, "SapTransfer::Freeze");
                    success = false;
                }
                if (!transfer->Wait(5000)) {
                    PrintCleanupError(logSink, "SapTransfer::Wait");
                    success = false;
                    if (!transfer->Abort() || !transfer->Wait(5000)) {
                        PrintCleanupError(logSink, "SapTransfer::Abort/Wait");
                    }
                }
            }
        }

        // Constructor callbacks are unregistered by Destroy(); UnregisterCallback()
        // only applies to extended events added through RegisterCallback().
        if (!transfer.Destroy()) {
            PrintCleanupError(logSink, "SapTransfer::Destroy");
            success = false;
        }
        transfer.Reset();

        if (processor) {
            if (!processor->Shutdown()) {
                PrintCleanupError(logSink, "RealtimeView::Shutdown");
                success = false;
            }
            processor.reset();
        }

        if (!buffers.Destroy()) {
            PrintCleanupError(logSink, "SapBuffer::Destroy");
            success = false;
        }
        buffers.Reset();

        if (!acquisition.Destroy()) {
            PrintCleanupError(logSink, "SapAcquisition::Destroy");
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
    return _RunDevice(grabberIndex, deviceIndex, configFilePath, nullptr, nullptr, nullptr);
}

bool SaperaUse::RunDevice(
    int grabberIndex,
    int deviceIndex,
    const char* configFilePath,
    CommandQueue& commands,
    ILogSink* logSink,
    IPreviewSink* previewSink) {
    return _RunDevice(
        grabberIndex,
        deviceIndex,
        configFilePath,
        &commands,
        logSink,
        previewSink);
}

bool SaperaUse::_RunDevice(
    int grabberIndex,
    int deviceIndex,
    const char* configFilePath,
    CommandQueue* commands,
    ILogSink* logSink,
    IPreviewSink* previewSink) {
    Shutdown();
    const auto quitRequested = [commands]() noexcept {
        return commands != nullptr && commands->IsQuitRequested();
    };

    std::string error;
    if (!_ValidateRuntimeConfig(error)) {
        Log(logSink, LogLevel::Error, "Invalid configuration: ", error);
        return false;
    }
    if (grabberIndex < 0 || grabberIndex >= static_cast<int>(_devicesInfo.size())) {
        Log(logSink, LogLevel::Error, "GrabberIndex is out of range: ", grabberIndex);
        return false;
    }

    const auto& deviceInfo = _devicesInfo.at(grabberIndex);
    const std::string& grabberName = std::get<0>(deviceInfo);
    const auto& deviceNames = std::get<1>(deviceInfo);
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(deviceNames.size())) {
        Log(logSink, LogLevel::Error, "CameraIndex is out of range: ", deviceIndex);
        return false;
    }
    if (configFilePath == nullptr || configFilePath[0] == '\0' ||
        !std::filesystem::exists(std::filesystem::path(configFilePath))) {
        Log(logSink, LogLevel::Error, "Grabber configuration file does not exist: ",
            configFilePath == nullptr ? "<null>" : configFilePath);
        return false;
    }
    if (!ValidateCcfStructure(configFilePath, error)) {
        Log(logSink, LogLevel::Error, "Invalid grabber configuration file: ", error);
        return false;
    }

    auto session = std::make_unique<CameraSession>();
    session->logSink = logSink;
    const SapLocation location(grabberName.c_str(), deviceIndex);
    session->acquisition.Reset(std::make_unique<SapAcquisition>(location, configFilePath));
    if (!session->acquisition->Create()) {
        _errorStaus = 2;
        Log(logSink, LogLevel::Error, "Failed to create SapAcquisition.");
        return false;
    }
    if (quitRequested()) {
        Log(logSink, LogLevel::Info, "Initialization cancelled.");
        return true;
    }

    session->buffers.Reset(std::make_unique<SapBufferWithTrash>(2, session->acquisition.Get()));
    int bufferCount = CONFIG.getBufferCount();
    if (CONFIG.getRecordMode() == 2) {
        const int skipCount = CONFIG.getTriigerMode() == 1 ? 1 : 0;
        bufferCount = CONFIG.getRecordFrame() + CONFIG.getBufferOverflow() + skipCount;
        Log(logSink, LogLevel::Info, "Estimated recording duration (s): ",
            static_cast<double>(CONFIG.getRecordFrame()) / CONFIG.getFrameRate());
    }
    if (!session->buffers->SetCount(bufferCount) || !session->buffers->Create()) {
        _errorStaus = 3;
        Log(logSink, LogLevel::Error, "Failed to create ", bufferCount, " Sapera buffers.");
        return false;
    }
    if (quitRequested()) {
        Log(logSink, LogLevel::Info, "Initialization cancelled.");
        return true;
    }

    if (!FrameLayout::FromSapBuffer(*session->buffers, session->frameLayout, error)) {
        Log(logSink, LogLevel::Error, "Unsupported camera buffer: ", error);
        return false;
    }

    session->processor = std::make_unique<RealtimeView>(
        session->buffers.Get(),
        session->frameLayout,
        nullptr,
        nullptr,
        logSink,
        previewSink);
    if (!session->processor->Create()) {
        _errorStaus = 6;
        Log(logSink, LogLevel::Error, "Failed to create RealtimeView.");
        return false;
    }
    if (quitRequested()) {
        Log(logSink, LogLevel::Info, "Initialization cancelled.");
        return true;
    }

    session->transfer.Reset(std::make_unique<SapAcqToBuf>(
        session->acquisition.Get(),
        session->buffers.Get(),
        XferCallback,
        session.get()));
    SapXferPair* pair = session->transfer->GetPair(0);
    if (pair == nullptr || !pair->SetTrashCallbackInfo(XferCallback, session.get())) {
        _errorStaus = 4;
        Log(logSink, LogLevel::Error, "Failed to register the Sapera trash callback.");
        return false;
    }
    if (!session->transfer->Create()) {
        _errorStaus = 4;
        Log(logSink, LogLevel::Error, "Failed to create SapTransfer.");
        return false;
    }
    if (quitRequested()) {
        Log(logSink, LogLevel::Info, "Initialization cancelled.");
        return true;
    }

    session->tracker.Reset(session->buffers->GetCount(), session->buffers->GetIndex());
    Log(logSink, LogLevel::Success, "Selected grabber: ", grabberName,
        ", device: ", deviceNames.at(deviceIndex));
    Log(logSink, LogLevel::Info, "Buffer layout: ", session->frameLayout.width, 'x',
        session->frameLayout.height, ", Mono8, pitch=", session->frameLayout.pitch,
        ", buffers=", session->buffers->GetCount());

    _session = std::move(session);
    CameraSession& active = *_session;
    if (!active.transfer->Grab()) {
        Log(logSink, LogLevel::Error, "Failed to start continuous acquisition.");
        Shutdown();
        return false;
    }

    SapXferFrameRateInfo* frameRateInfo = active.transfer->GetFrameRateStatistics();
    bool quit = false;
    while (!quit) {
        _FrameRateDisp(frameRateInfo, logSink);

        if (_isTriggerToRecording) {
            const bool recorded = _TriggerToBufferRecord(active, commands);
            const bool quitRequested = commands != nullptr && commands->IsQuitRequested();
            std::string stopError;
            _StopTransfer(active, stopError);
            active.acquisition->SetParameter(
                CORACQ_PRM_EXT_TRIGGER_ENABLE,
                CORACQ_VAL_EXT_TRIGGER_OFF,
                TRUE);
            _isTriggerToRecording = false;
            if (quitRequested) {
                quit = true;
                continue;
            }
            if (!active.transfer->Grab()) {
                Log(logSink, LogLevel::Error, "Failed to resume free-running acquisition.");
                quit = true;
            }
            else {
                Log(logSink,
                    recorded ? LogLevel::Success : LogLevel::Warning,
                    recorded ? "TTL recording completed." : "TTL recording stopped.");
            }
            continue;
        }

        CaptureCommand command = CaptureCommand::Info;
        bool hasCommand = false;
        if (commands != nullptr) {
            if (commands->IsQuitRequested()) {
                command = CaptureCommand::Quit;
                hasCommand = true;
            }
            else {
                hasCommand = commands->WaitPop(command, std::chrono::milliseconds(20));
            }
        }
        else if (_kbhit() != 0) {
            const char key = static_cast<char>(_getch());
            const CommandParseResult parsed = ParseCaptureCommand(std::string(1, key));
            if (parsed.kind == ParsedCommandKind::Command) {
                command = parsed.command;
                hasCommand = true;
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        if (!hasCommand) {
            continue;
        }

        switch (command) {
        case CaptureCommand::Quit:
            quit = true;
            break;
        case CaptureCommand::Start:
            if (!active.transfer->IsGrabbing() && active.transfer->Grab()) {
                Log(logSink, LogLevel::Success, "Acquisition started.");
            }
            break;
        case CaptureCommand::Pause:
            if (active.transfer->IsGrabbing()) {
                std::string stopError;
                if (_StopTransfer(active, stopError)) {
                    Log(logSink, LogLevel::Info, "Acquisition paused.");
                }
                else {
                    Log(logSink, LogLevel::Error, stopError);
                }
            }
            break;
        case CaptureCommand::Info:
            active.processor->SubmitControl(RealtimeView::ControlCommand::Info);
            break;
        case CaptureCommand::Record:
            if (CONFIG.getRecordMode() == 1) {
                active.processor->SubmitControl(RealtimeView::ControlCommand::StartRecording);
            }
            else if (CONFIG.getTriigerMode() == 0) {
                if (!active.transfer->IsGrabbing()) {
                    Log(logSink, LogLevel::Warning, "Start acquisition before recording.");
                }
                else if (!_KeyToBufferRecord(active, commands) &&
                    !(commands != nullptr && commands->IsQuitRequested())) {
                    Log(logSink, LogLevel::Error, "Keyboard-triggered recording failed.");
                }
            }
            else {
                _isTriggerToRecording = true;
            }
            break;
        case CaptureCommand::Stop:
            if (CONFIG.getRecordMode() == 1) {
                active.processor->SubmitControl(RealtimeView::ControlCommand::StopRecording);
            }
            if (commands != nullptr) {
                commands->ClearStop();
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

float SaperaUse::_FrameRateDisp(SapXferFrameRateInfo* frameRateInfo, ILogSink* logSink) {
    if (frameRateInfo == nullptr || !frameRateInfo->IsLiveFrameRateAvailable() ||
        frameRateInfo->IsLiveFrameRateStalled()) {
        return _SteadyFrameRate;
    }

    const float currentFrameRate = CONFIG.getIsRoundFramerate()
        ? std::round(frameRateInfo->GetLiveFrameRate())
        : frameRateInfo->GetLiveFrameRate();
    if (currentFrameRate != _SteadyFrameRate) {
        Log(logSink, LogLevel::Info, "Live frame rate: ", currentFrameRate);
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

bool SaperaUse::_KeyToBufferRecord(CameraSession& session, CommandQueue* commands) {
    std::string error;
    const int frameCount = CONFIG.getRecordFrame();
    if (!session.tracker.Arm(static_cast<std::size_t>(frameCount), 0, error)) {
        Log(session.logSink, LogLevel::Error, "Could not arm keyboard capture: ", error);
        return false;
    }

    Log(session.logSink, LogLevel::Info, "Buffered recording started.");
    const CaptureWindow window = WaitForCaptureCompletion(
        session.tracker,
        CaptureTimeout(frameCount, CONFIG.getFrameRate()),
        commands,
        false,
        "Buffered recording was cancelled because the application is closing.");
    const bool stopped = _StopTransfer(session, error);
    if (!stopped) {
        Log(session.logSink, LogLevel::Error, error);
    }

    bool saved = false;
    if (stopped && window.status == CaptureWindowStatus::Complete) {
        saved = _SaveCaptureWindow(session, window);
    }
    else if (window.status != CaptureWindowStatus::Complete) {
        Log(session.logSink, LogLevel::Error, "Capture failed [",
            CaptureWindowStatusName(window.status), "]: ", window.message);
    }

    if (commands != nullptr && commands->IsQuitRequested()) {
        return false;
    }
    if (!session.transfer->Grab()) {
        Log(session.logSink, LogLevel::Error, "Failed to resume acquisition after buffered recording.");
        return false;
    }
    return saved;
}

bool SaperaUse::_TriggerToBufferRecord(CameraSession& session, CommandQueue* commands) {
    std::string error;
    if (!_StopTransfer(session, error)) {
        Log(session.logSink, LogLevel::Error, error);
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
        Log(session.logSink, LogLevel::Error, "Failed to configure TTL trigger parameters.");
        return false;
    }

    if (!session.tracker.Arm(static_cast<std::size_t>(frameCount), 1, error)) {
        Log(session.logSink, LogLevel::Error, "Could not arm TTL capture: ", error);
        return false;
    }
    if (!session.transfer->Grab()) {
        session.tracker.Cancel("SapTransfer::Grab failed.");
        return false;
    }

    Log(session.logSink, LogLevel::Info, "Waiting for TTL trigger (enter S to cancel).");
    while (!session.tracker.WaitForFirstEvent(std::chrono::milliseconds(50))) {
        const CaptureWindow snapshot = session.tracker.Snapshot();
        if (snapshot.status != CaptureWindowStatus::Armed) {
            break;
        }
        if (commands != nullptr) {
            const std::optional<CaptureCommand> cancellation = commands->PendingCancellation();
            if (cancellation == CaptureCommand::Quit || cancellation == CaptureCommand::Stop) {
                session.tracker.Cancel(cancellation == CaptureCommand::Quit
                    ? "TTL capture was cancelled because the application is closing."
                    : "TTL capture was cancelled by the user.");
                if (cancellation == CaptureCommand::Stop) {
                    commands->ClearStop();
                }
                _StopTransfer(session, error);
                return false;
            }
        }
        else if (_kbhit() != 0) {
            const char key = static_cast<char>(_getch());
            if (key == 's' || key == 'S') {
                session.tracker.Cancel("TTL capture was cancelled by the user.");
                _StopTransfer(session, error);
                return false;
            }
        }
    }

    const CaptureWindow firstEvent = session.tracker.Snapshot();
    if (firstEvent.status != CaptureWindowStatus::Armed &&
        firstEvent.status != CaptureWindowStatus::Complete) {
        return false;
    }

    Log(session.logSink, LogLevel::Info, "TTL trigger received; recording started.");
    const CaptureWindow window = WaitForCaptureCompletion(
        session.tracker,
        CaptureTimeout(frameCount + 1, CONFIG.getFrameRate()),
        commands,
        true,
        "TTL capture was cancelled by the user.");
    const bool stopped = _StopTransfer(session, error);
    if (!stopped) {
        Log(session.logSink, LogLevel::Error, error);
        return false;
    }
    if (window.status != CaptureWindowStatus::Complete) {
        Log(session.logSink, LogLevel::Error, "TTL capture failed [",
            CaptureWindowStatusName(window.status), "]: ", window.message);
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
        Log(session.logSink, LogLevel::Error, "Capture window is not safe to save: ", error);
        return false;
    }

    std::vector<int> indices;
    if (!BuildCircularIndices(
            window.firstIndex,
            window.frameCount,
            session.buffers->GetCount(),
            indices,
            error)) {
        Log(session.logSink, LogLevel::Error, "Could not build capture indices: ", error);
        return false;
    }

    const std::string outputPath = BuildOutputPath();
    RecordFromBuffer recorder(*session.buffers, session.frameLayout);
    try {
        if (!CONFIG.getSaveAsFrameSequence()) {
            Log(session.logSink, LogLevel::Info, "Saving video...");
            if (!recorder.SaveVideo(
                    outputPath,
                    GetEncoder(CONFIG.getEncoder()),
                    CONFIG.getFrameRate(),
                    session.frameLayout.width,
                    session.frameLayout.height,
                    false,
                    indices)) {
                Log(session.logSink, LogLevel::Error, "Video save failed: ", outputPath);
                return false;
            }
            Log(session.logSink, LogLevel::Success, "Video saved to: ", outputPath);
            return true;
        }

        std::filesystem::path folder(outputPath);
        folder.replace_extension();
        std::error_code fileError;
        std::filesystem::create_directories(folder, fileError);
        if (fileError) {
            Log(session.logSink, LogLevel::Error, "Could not create frame folder: ", fileError.message());
            return false;
        }
        if (!recorder.SaveFrames(folder.string(), indices)) {
            Log(session.logSink, LogLevel::Error, "Frame sequence save failed: ", folder.string());
            return false;
        }
        Log(session.logSink, LogLevel::Success, "Frames saved to: ", folder.string());
        return true;
    }
    catch (const std::exception& exception) {
        Log(session.logSink, LogLevel::Error, "Capture save failed: ", exception.what());
        return false;
    }
}
