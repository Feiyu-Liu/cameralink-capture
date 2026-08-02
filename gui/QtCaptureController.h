#pragma once

#include <QObject>
#include <QImage>
#include <QString>
#include <QThread>

#include <memory>

class CommandQueue;
class PreviewMailbox;
class QtCaptureWorker;
class QTimer;

class QtCaptureController final : public QObject {
    Q_OBJECT

public:
    explicit QtCaptureController(QObject* parent = nullptr);
    ~QtCaptureController() override;

    bool startCapture(const QString& configPath);
    void submitTextCommand(const QString& text);
    void requestShutdown();

signals:
    void logLineReady(const QString& text);
    void clearConsoleRequested();
    void previewFrameReady(const QImage& frame);
    void closingStarted();
    void shutdownCompleted();

private slots:
    void drainPreviewMailbox();
    void handleWorkerLog(int level, const QString& message);
    void handleWorkerCompleted(bool success);
    void handleWorkerThreadFinished();

private:
    void submitCommand(int command);
    void finishShutdownIfIdle();

    QThread m_workerThread;
    QtCaptureWorker* m_worker = nullptr;
    QTimer* m_previewTimer = nullptr;
    std::shared_ptr<CommandQueue> m_commands;
    std::shared_ptr<PreviewMailbox> m_previewMailbox;
    bool m_captureStarted = false;
    bool m_closeWhenStopped = false;
};
