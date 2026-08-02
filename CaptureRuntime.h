#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <iosfwd>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

enum class CaptureCommand {
    Start,
    Pause,
    Info,
    Record,
    Stop,
    Quit
};

enum class ParsedCommandKind {
    Command,
    Help,
    Clear,
    Empty,
    Unknown
};

struct CommandParseResult {
    ParsedCommandKind kind = ParsedCommandKind::Empty;
    CaptureCommand command = CaptureCommand::Info;
    std::string text;
};

// Parses the command text accepted by both the console and Qt terminal.
// The input is trimmed and compared case-insensitively.
CommandParseResult ParseCaptureCommand(std::string_view text);

const char* CaptureCommandName(CaptureCommand command) noexcept;

enum class CaptureState {
    Idle,
    Initializing,
    Running,
    Paused,
    Recording,
    WaitingForTtl,
    Saving,
    Stopping,
    Stopped,
    Error
};

const char* CaptureStateName(CaptureState state) noexcept;

class CommandQueue {
public:
    void Submit(CaptureCommand command);
    bool TryPop(CaptureCommand& command);
    bool WaitPop(CaptureCommand& command, std::chrono::milliseconds timeout);

    bool IsStopRequested() const noexcept;
    bool IsQuitRequested() const noexcept;

    // Quit dominates stop and cannot be cleared by ClearStop().
    std::optional<CaptureCommand> PendingCancellation() const noexcept;
    void ClearStop();
    void Clear();

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_changed;
    std::deque<CaptureCommand> m_commands;
    std::atomic<bool> m_stopRequested { false };
    std::atomic<bool> m_quitRequested { false };
};

enum class LogLevel {
    Info,
    Success,
    Warning,
    Error
};

const char* LogLevelName(LogLevel level) noexcept;

class ILogSink {
public:
    virtual ~ILogSink() = default;

    // message is an UTF-8 byte string. Implementations must not retain its view.
    // Logging can occur from a Sapera callback, so implementations must not throw.
    virtual void Write(LogLevel level, std::string_view message) noexcept = 0;
};

class ConsoleLogSink final : public ILogSink {
public:
    ConsoleLogSink();
    explicit ConsoleLogSink(std::ostream& output);

    void Write(LogLevel level, std::string_view message) noexcept override;

private:
    std::ostream& m_output;
    std::mutex m_mutex;
};

// Uses sink when present. A null sink writes to stdout/stderr without throwing.
void WriteLog(ILogSink* sink, LogLevel level, std::string_view message) noexcept;
