#include "QtCaptureController.h"

#include "QtCaptureWorker.h"

#include "../CaptureRuntime.h"
#include "../PreviewMailbox.h"

#include <QImage>
#include <QTimer>

#include <limits>
#include <string>

namespace {

constexpr int kPreviewPollIntervalMilliseconds = 16;

QString FormatLogLine(LogLevel level, const QString& message) {
    return QStringLiteral("[%1] %2")
        .arg(QString::fromLatin1(LogLevelName(level)), message);
}

bool ToImage(const PreviewFrame& frame, QImage& image) {
    if (!frame.IsValid() || frame.Stride() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    QImage::Format format = QImage::Format_Invalid;
    switch (frame.Format()) {
    case PreviewPixelFormat::Mono8:
        format = QImage::Format_Grayscale8;
        break;
    case PreviewPixelFormat::Bgr8:
        format = QImage::Format_BGR888;
        break;
    }
    if (format == QImage::Format_Invalid) {
        return false;
    }

    const QImage borrowed(
        frame.Data(),
        frame.Width(),
        frame.Height(),
        static_cast<int>(frame.Stride()),
        format);
    if (borrowed.isNull()) {
        return false;
    }
    image = borrowed.copy();
    return !image.isNull();
}

} // namespace

QtCaptureController::QtCaptureController(QObject* parent)
    : QObject(parent),
      m_commands(std::make_shared<CommandQueue>()),
      m_previewMailbox(std::make_shared<PreviewMailbox>(30.0)) {
    m_previewTimer = new QTimer(this);
    m_previewTimer->setInterval(kPreviewPollIntervalMilliseconds);
    connect(m_previewTimer, &QTimer::timeout,
            this, &QtCaptureController::drainPreviewMailbox);
    connect(&m_workerThread, &QThread::finished,
            this, &QtCaptureController::handleWorkerThreadFinished);
}

QtCaptureController::~QtCaptureController() {
    m_previewMailbox->SetEnabled(false);
    if (m_workerThread.isRunning()) {
        m_commands->Submit(CaptureCommand::Quit);
        m_workerThread.quit();
        m_workerThread.wait();
    }
}

bool QtCaptureController::startCapture(const QString& configPath) {
    if (m_captureStarted || m_workerThread.isRunning() || configPath.isEmpty()) {
        return false;
    }

    m_commands->Clear();
    m_previewMailbox->Clear();
    m_previewMailbox->SetEnabled(true);
    m_closeWhenStopped = false;
    m_captureStarted = true;
    m_previewTimer->start();

    m_worker = new QtCaptureWorker(configPath, m_commands, m_previewMailbox);
    m_worker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::started,
            m_worker, &QtCaptureWorker::run);
    connect(m_worker, &QtCaptureWorker::logReceived,
            this, &QtCaptureController::handleWorkerLog);
    connect(m_worker, &QtCaptureWorker::completed,
            this, &QtCaptureController::handleWorkerCompleted);
    connect(m_worker, &QtCaptureWorker::completed,
            m_worker, &QObject::deleteLater);
    connect(m_worker, &QtCaptureWorker::completed,
            &m_workerThread, &QThread::quit);
    m_workerThread.start();
    return true;
}

void QtCaptureController::submitTextCommand(const QString& text) {
    const QByteArray utf8 = text.toUtf8();
    const CommandParseResult parsed = ParseCaptureCommand(
        std::string_view(utf8.constData(), static_cast<std::size_t>(utf8.size())));

    switch (parsed.kind) {
    case ParsedCommandKind::Command:
        submitCommand(static_cast<int>(parsed.command));
        return;
    case ParsedCommandKind::Help:
        emit logLineReady(QStringLiteral("Commands: g start, p pause, i info, r record, s stop, q quit."));
        return;
    case ParsedCommandKind::Clear:
        emit clearConsoleRequested();
        return;
    case ParsedCommandKind::Unknown:
        emit logLineReady(QStringLiteral("[ERROR] Unknown command: %1").arg(text.trimmed()));
        return;
    case ParsedCommandKind::Empty:
        return;
    }
}

void QtCaptureController::requestShutdown() {
    if (!m_closeWhenStopped) {
        m_closeWhenStopped = true;
        m_previewMailbox->SetEnabled(false);
        m_previewTimer->stop();
        emit closingStarted();
    }

    if (m_workerThread.isRunning()) {
        m_commands->Submit(CaptureCommand::Quit);
        return;
    }
    finishShutdownIfIdle();
}

void QtCaptureController::drainPreviewMailbox() {
    PreviewFrame frame;
    if (!m_previewMailbox->TryTake(frame)) {
        return;
    }

    QImage image;
    if (!ToImage(frame, image)) {
        emit logLineReady(QStringLiteral("[ERROR] Discarded an invalid preview frame."));
        return;
    }
    emit previewFrameReady(image);
}

void QtCaptureController::handleWorkerLog(int rawLevel, const QString& message) {
    emit logLineReady(FormatLogLine(static_cast<LogLevel>(rawLevel), message));
}

void QtCaptureController::handleWorkerCompleted(bool success) {
    Q_UNUSED(success);
}

void QtCaptureController::handleWorkerThreadFinished() {
    m_worker = nullptr;
    m_captureStarted = false;
    m_previewTimer->stop();
    m_previewMailbox->SetEnabled(false);
    if (m_closeWhenStopped) {
        emit shutdownCompleted();
    }
}

void QtCaptureController::submitCommand(int rawCommand) {
    const CaptureCommand command = static_cast<CaptureCommand>(rawCommand);
    if (command == CaptureCommand::Quit) {
        requestShutdown();
        return;
    }

    if (!m_captureStarted) {
        emit logLineReady(QStringLiteral("[ERROR] No active capture session."));
        return;
    }
    m_commands->Submit(command);
}

void QtCaptureController::finishShutdownIfIdle() {
    if (m_closeWhenStopped && !m_workerThread.isRunning()) {
        emit shutdownCompleted();
    }
}
