#include "CaptureWindow.h"

#include <sstream>

void FrameArrivalTracker::Reset(int bufferCount, int lastIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bufferCount = bufferCount;
    m_lastIndex = lastIndex;
    m_hasLastIndex = bufferCount > 0 && lastIndex >= 0 && lastIndex < bufferCount;
    m_totalNormalFrames = 0;
    m_eventsAtArm = 0;
    m_totalEvents = 0;
    m_skipRemaining = 0;
    m_targetFrameCount = 0;
    m_capturedFrameCount = 0;
    m_window = {};
    m_changed.notify_all();
}

bool FrameArrivalTracker::Arm(
    std::size_t frameCount,
    std::size_t skipCount,
    std::string& error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_bufferCount <= 0) {
        error = "Cannot arm capture without a valid buffer count.";
        return false;
    }
    if (frameCount == 0 || frameCount > static_cast<std::size_t>(m_bufferCount)) {
        error = "Capture frame count must be between 1 and the buffer count.";
        return false;
    }
    if (skipCount >= static_cast<std::size_t>(m_bufferCount)) {
        error = "Capture skip count must be smaller than the buffer count.";
        return false;
    }
    if (m_window.status == CaptureWindowStatus::Armed) {
        error = "A capture window is already armed.";
        return false;
    }

    m_skipRemaining = skipCount;
    m_targetFrameCount = frameCount;
    m_capturedFrameCount = 0;
    m_eventsAtArm = m_totalEvents;
    m_window = {};
    m_window.status = CaptureWindowStatus::Armed;
    m_window.frameCount = frameCount;
    error.clear();
    return true;
}

void FrameArrivalTracker::OnFrame(int bufferIndex, int eventCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ++m_totalEvents;

    if (bufferIndex < 0 || bufferIndex >= m_bufferCount) {
        if (m_window.status == CaptureWindowStatus::Armed) {
            std::ostringstream stream;
            stream << "Callback reported invalid buffer index " << bufferIndex << '.';
            Fail(CaptureWindowStatus::InvalidFrame, stream.str());
        }
        m_changed.notify_all();
        return;
    }

    if (m_hasLastIndex) {
        const int expected = (m_lastIndex + 1) % m_bufferCount;
        if (bufferIndex != expected && m_window.status == CaptureWindowStatus::Armed) {
            std::ostringstream stream;
            stream << "Buffer index discontinuity: expected " << expected
                   << ", received " << bufferIndex << ", event " << eventCount << '.';
            Fail(CaptureWindowStatus::IndexDiscontinuity, stream.str());
        }
    }

    m_lastIndex = bufferIndex;
    m_hasLastIndex = true;
    ++m_totalNormalFrames;

    if (m_window.status == CaptureWindowStatus::Armed) {
        if (m_skipRemaining > 0) {
            --m_skipRemaining;
        }
        else {
            if (m_capturedFrameCount == 0) {
                m_window.firstIndex = bufferIndex;
                m_window.firstSequence = m_totalNormalFrames;
            }
            ++m_capturedFrameCount;
            m_window.lastSequence = m_totalNormalFrames;
            if (m_capturedFrameCount == m_targetFrameCount) {
                m_window.status = CaptureWindowStatus::Complete;
                m_window.message.clear();
            }
        }
    }

    m_changed.notify_all();
}

void FrameArrivalTracker::OnTrashFrame(int eventCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ++m_totalEvents;
    if (m_window.status == CaptureWindowStatus::Armed) {
        std::ostringstream stream;
        stream << "Sapera routed frame event " << eventCount << " to the trash buffer.";
        Fail(CaptureWindowStatus::TrashFrame, stream.str());
    }
    m_changed.notify_all();
}

void FrameArrivalTracker::Cancel(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_window.status == CaptureWindowStatus::Armed) {
        Fail(CaptureWindowStatus::Cancelled, reason);
        m_changed.notify_all();
    }
}

void FrameArrivalTracker::Timeout(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_window.status == CaptureWindowStatus::Armed) {
        Fail(CaptureWindowStatus::TimedOut, reason);
        m_changed.notify_all();
    }
}

bool FrameArrivalTracker::WaitForFirstEvent(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_changed.wait_for(lock, timeout, [this]() {
        return m_totalEvents > m_eventsAtArm || IsTerminal();
    });
    return m_totalEvents > m_eventsAtArm;
}

bool FrameArrivalTracker::WaitForTerminal(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_changed.wait_for(lock, timeout, [this]() { return IsTerminal(); });
}

CaptureWindow FrameArrivalTracker::WaitForCompletion(std::chrono::milliseconds timeout) {
    if (!WaitForTerminal(timeout)) {
        Timeout("Timed out while waiting for the capture window.");
    }
    return Snapshot();
}

CaptureWindow FrameArrivalTracker::Snapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_window;
}

std::uint64_t FrameArrivalTracker::TotalNormalFrames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalNormalFrames;
}

bool FrameArrivalTracker::IsTerminal() const noexcept {
    return m_window.status != CaptureWindowStatus::Idle &&
           m_window.status != CaptureWindowStatus::Armed;
}

void FrameArrivalTracker::Fail(CaptureWindowStatus status, const std::string& message) {
    m_window.status = status;
    m_window.message = message;
}

bool BuildCircularIndices(
    int firstIndex,
    std::size_t frameCount,
    int bufferCount,
    std::vector<int>& indices,
    std::string& error) {
    if (bufferCount <= 0) {
        error = "Buffer count must be positive.";
        return false;
    }
    if (firstIndex < 0 || firstIndex >= bufferCount) {
        error = "First buffer index is out of range.";
        return false;
    }
    if (frameCount == 0 || frameCount > static_cast<std::size_t>(bufferCount)) {
        error = "Frame count must be between 1 and the buffer count.";
        return false;
    }

    indices.clear();
    indices.reserve(frameCount);
    for (std::size_t offset = 0; offset < frameCount; ++offset) {
        const auto index = (static_cast<std::int64_t>(firstIndex) +
            static_cast<std::int64_t>(offset)) % bufferCount;
        indices.push_back(static_cast<int>(index));
    }
    error.clear();
    return true;
}

bool IsCaptureWindowIntact(
    const CaptureWindow& window,
    std::uint64_t totalNormalFrames,
    int bufferCount,
    std::string& error) {
    if (window.status != CaptureWindowStatus::Complete || window.firstSequence == 0) {
        error = "Capture window is not complete.";
        return false;
    }
    if (bufferCount <= 0 || totalNormalFrames < window.lastSequence) {
        error = "Capture window counters are invalid.";
        return false;
    }
    if (totalNormalFrames - window.firstSequence >= static_cast<std::uint64_t>(bufferCount)) {
        error = "The capture window was overwritten before the transfer stopped.";
        return false;
    }
    error.clear();
    return true;
}

const char* CaptureWindowStatusName(CaptureWindowStatus status) noexcept {
    switch (status) {
    case CaptureWindowStatus::Idle: return "Idle";
    case CaptureWindowStatus::Armed: return "Armed";
    case CaptureWindowStatus::Complete: return "Complete";
    case CaptureWindowStatus::Cancelled: return "Cancelled";
    case CaptureWindowStatus::TimedOut: return "TimedOut";
    case CaptureWindowStatus::TrashFrame: return "TrashFrame";
    case CaptureWindowStatus::IndexDiscontinuity: return "IndexDiscontinuity";
    case CaptureWindowStatus::InvalidFrame: return "InvalidFrame";
    default: return "Unknown";
    }
}
