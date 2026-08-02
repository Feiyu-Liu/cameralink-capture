#include "CaptureRuntime.h"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace {

std::string NormalizeCommand(std::string_view input) {
    std::size_t first = 0;
    while (first < input.size() && std::isspace(static_cast<unsigned char>(input[first])) != 0) {
        ++first;
    }

    std::size_t last = input.size();
    while (last > first && std::isspace(static_cast<unsigned char>(input[last - 1])) != 0) {
        --last;
    }

    std::string normalized(input.substr(first, last - first));
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return normalized;
}

} // namespace

CommandParseResult ParseCaptureCommand(std::string_view text) {
    CommandParseResult result;
    result.text = NormalizeCommand(text);

    if (result.text.empty()) {
        return result;
    }
    if (result.text == "help") {
        result.kind = ParsedCommandKind::Help;
        return result;
    }
    if (result.text == "clear") {
        result.kind = ParsedCommandKind::Clear;
        return result;
    }

    const struct {
        const char* text;
        CaptureCommand command;
    } commands[] {
        { "g", CaptureCommand::Start },
        { "p", CaptureCommand::Pause },
        { "i", CaptureCommand::Info },
        { "r", CaptureCommand::Record },
        { "s", CaptureCommand::Stop },
        { "q", CaptureCommand::Quit }
    };

    for (const auto& candidate : commands) {
        if (result.text == candidate.text) {
            result.kind = ParsedCommandKind::Command;
            result.command = candidate.command;
            return result;
        }
    }

    result.kind = ParsedCommandKind::Unknown;
    return result;
}

const char* CaptureCommandName(CaptureCommand command) noexcept {
    switch (command) {
    case CaptureCommand::Start: return "Start";
    case CaptureCommand::Pause: return "Pause";
    case CaptureCommand::Info: return "Info";
    case CaptureCommand::Record: return "Record";
    case CaptureCommand::Stop: return "Stop";
    case CaptureCommand::Quit: return "Quit";
    }
    return "Unknown";
}

const char* CaptureStateName(CaptureState state) noexcept {
    switch (state) {
    case CaptureState::Idle: return "Idle";
    case CaptureState::Initializing: return "Initializing";
    case CaptureState::Running: return "Running";
    case CaptureState::Paused: return "Paused";
    case CaptureState::Recording: return "Recording";
    case CaptureState::WaitingForTtl: return "WaitingForTtl";
    case CaptureState::Saving: return "Saving";
    case CaptureState::Stopping: return "Stopping";
    case CaptureState::Stopped: return "Stopped";
    case CaptureState::Error: return "Error";
    }
    return "Unknown";
}

void CommandQueue::Submit(CaptureCommand command) {
    if (command == CaptureCommand::Quit) {
        m_quitRequested.store(true, std::memory_order_release);
        m_stopRequested.store(true, std::memory_order_release);
    }
    else if (command == CaptureCommand::Stop) {
        m_stopRequested.store(true, std::memory_order_release);
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_commands.push_back(command);
    }
    m_changed.notify_one();
}

bool CommandQueue::TryPop(CaptureCommand& command) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_commands.empty()) {
        return false;
    }
    command = m_commands.front();
    m_commands.pop_front();
    return true;
}

bool CommandQueue::WaitPop(CaptureCommand& command, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_changed.wait_for(lock, timeout, [this]() { return !m_commands.empty(); })) {
        return false;
    }
    command = m_commands.front();
    m_commands.pop_front();
    return true;
}

bool CommandQueue::IsStopRequested() const noexcept {
    return m_stopRequested.load(std::memory_order_acquire);
}

bool CommandQueue::IsQuitRequested() const noexcept {
    return m_quitRequested.load(std::memory_order_acquire);
}

std::optional<CaptureCommand> CommandQueue::PendingCancellation() const noexcept {
    if (IsQuitRequested()) {
        return CaptureCommand::Quit;
    }
    if (IsStopRequested()) {
        return CaptureCommand::Stop;
    }
    return std::nullopt;
}

void CommandQueue::ClearStop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_quitRequested.load(std::memory_order_acquire)) {
            return;
        }
        m_commands.erase(
            std::remove(m_commands.begin(), m_commands.end(), CaptureCommand::Stop),
            m_commands.end());
        m_stopRequested.store(false, std::memory_order_release);
        // A concurrent Quit may have been submitted after the first check.
        if (m_quitRequested.load(std::memory_order_acquire)) {
            m_stopRequested.store(true, std::memory_order_release);
        }
    }
}

void CommandQueue::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands.clear();
    m_stopRequested.store(false, std::memory_order_release);
    m_quitRequested.store(false, std::memory_order_release);
}

const char* LogLevelName(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Info: return "INFO";
    case LogLevel::Success: return "SUCCESS";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

ConsoleLogSink::ConsoleLogSink(std::ostream& output)
    : m_output(output) {
}

ConsoleLogSink::ConsoleLogSink()
    : ConsoleLogSink(std::cout) {
}

void ConsoleLogSink::Write(LogLevel level, std::string_view message) noexcept {
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_output << '[' << LogLevelName(level) << "] " << message << std::endl;
    }
    catch (...) {
        // Logging must never terminate the acquisition callback.
    }
}

void WriteLog(ILogSink* sink, LogLevel level, std::string_view message) noexcept {
    if (sink != nullptr) {
        sink->Write(level, message);
        return;
    }

    try {
        static std::mutex outputMutex;
        std::lock_guard<std::mutex> lock(outputMutex);
        std::ostream& output = level == LogLevel::Error ? std::cerr : std::cout;
        output << '[' << LogLevelName(level) << "] " << message << std::endl;
    }
    catch (...) {
        // Best-effort fallback for code paths without an installed log sink.
    }
}
