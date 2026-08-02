#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

enum class CaptureWindowStatus {
    Idle,
    Armed,
    Complete,
    Cancelled,
    TimedOut,
    TrashFrame,
    IndexDiscontinuity,
    InvalidFrame
};

struct CaptureWindow {
    CaptureWindowStatus status = CaptureWindowStatus::Idle;
    int firstIndex = -1;
    std::size_t frameCount = 0;
    std::uint64_t firstSequence = 0;
    std::uint64_t lastSequence = 0;
    std::string message;
};

class FrameArrivalTracker {
public:
    void Reset(int bufferCount, int lastIndex);
    bool Arm(std::size_t frameCount, std::size_t skipCount, std::string& error);
    void OnFrame(int bufferIndex, int eventCount);
    void OnTrashFrame(int eventCount);
    void Cancel(const std::string& reason);

    bool WaitForFirstEvent(std::chrono::milliseconds timeout);
    CaptureWindow WaitForCompletion(std::chrono::milliseconds timeout);
    CaptureWindow Snapshot() const;
    std::uint64_t TotalNormalFrames() const;

private:
    bool IsTerminal() const noexcept;
    void Fail(CaptureWindowStatus status, const std::string& message);

    mutable std::mutex m_mutex;
    std::condition_variable m_changed;
    int m_bufferCount = 0;
    int m_lastIndex = -1;
    bool m_hasLastIndex = false;
    std::uint64_t m_totalNormalFrames = 0;
    std::uint64_t m_eventsAtArm = 0;
    std::uint64_t m_totalEvents = 0;
    std::size_t m_skipRemaining = 0;
    std::size_t m_targetFrameCount = 0;
    std::size_t m_capturedFrameCount = 0;
    CaptureWindow m_window;
};

bool BuildCircularIndices(
    int firstIndex,
    std::size_t frameCount,
    int bufferCount,
    std::vector<int>& indices,
    std::string& error);

bool IsCaptureWindowIntact(
    const CaptureWindow& window,
    std::uint64_t totalNormalFrames,
    int bufferCount,
    std::string& error);

const char* CaptureWindowStatusName(CaptureWindowStatus status) noexcept;
