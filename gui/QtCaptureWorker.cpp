#include "QtCaptureWorker.h"

#include "../CaptureRuntime.h"
#include "../ConfigManager.h"
#include "../PreviewMailbox.h"
#include "../SaperaUse.h"
#include "../config.h"

#include <functional>
#include <exception>
#include <string>
#include <string_view>

namespace {

class QtLogSink final : public ILogSink {
public:
    explicit QtLogSink(std::function<void(LogLevel, const QString&)> receiver)
        : m_receiver(std::move(receiver)) {
    }

    void Write(LogLevel level, std::string_view message) noexcept override {
        try {
            m_receiver(level, QString::fromUtf8(
                message.data(), static_cast<qsizetype>(message.size())));
        }
        catch (...) {
            // Logging cannot interrupt capture or an SDK callback.
        }
    }

private:
    std::function<void(LogLevel, const QString&)> m_receiver;
};

} // namespace

QtCaptureWorker::QtCaptureWorker(
    QString configPath,
    std::shared_ptr<CommandQueue> commands,
    std::shared_ptr<PreviewMailbox> previewMailbox)
    : m_configPath(std::move(configPath)),
      m_commands(std::move(commands)),
      m_previewMailbox(std::move(previewMailbox)) {
}

void QtCaptureWorker::run() {
    QtLogSink logSink([this](LogLevel level, const QString& message) {
        emit logReceived(static_cast<int>(level), message);
    });

    try {
        const QByteArray encodedPath = m_configPath.toUtf8();
        const std::string configPath(encodedPath.constData(), static_cast<std::size_t>(encodedPath.size()));
        WriteLog(&logSink, LogLevel::Info, std::string("Loading configuration: ") + configPath);
        if (!ConfigManager::getInstance().loadConfig(configPath)) {
            WriteLog(&logSink, LogLevel::Error, "Could not load the selected INI configuration.");
            emit completed(false);
            return;
        }

        SaperaUse camera;
        if (!camera.GrabbersInit()) {
            WriteLog(&logSink, LogLevel::Error, "No usable CameraLink grabber was found.");
            emit completed(false);
            return;
        }

        if (m_commands->IsQuitRequested()) {
            WriteLog(&logSink, LogLevel::Info, "Startup cancelled.");
            emit completed(true);
            return;
        }

        const bool runSucceeded = camera.RunDevice(
            CONFIG.getGrabberIndex(),
            CONFIG.getCameraIndex(),
            CONFIG.getGrabberConfigPath().c_str(),
            *m_commands,
            &logSink,
            m_previewMailbox.get());
        if (!runSucceeded) {
            WriteLog(&logSink, LogLevel::Error, "Capture session stopped with an error.");
        }
        emit completed(runSucceeded);
    }
    catch (const std::exception& exception) {
        WriteLog(&logSink, LogLevel::Error,
            std::string("Capture worker failed: ") + exception.what());
        emit completed(false);
    }
    catch (...) {
        WriteLog(&logSink, LogLevel::Error, "Capture worker failed with an unknown exception.");
        emit completed(false);
    }
}
