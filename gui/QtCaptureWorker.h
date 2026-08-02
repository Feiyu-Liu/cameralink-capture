#pragma once

#include <QObject>
#include <QString>

#include <memory>

class CommandQueue;
class PreviewMailbox;

class QtCaptureWorker final : public QObject {
    Q_OBJECT

public:
    QtCaptureWorker(
        QString configPath,
        std::shared_ptr<CommandQueue> commands,
        std::shared_ptr<PreviewMailbox> previewMailbox);

public slots:
    void run();

signals:
    void logReceived(int level, const QString& message);
    void completed(bool success);

private:
    QString m_configPath;
    std::shared_ptr<CommandQueue> m_commands;
    std::shared_ptr<PreviewMailbox> m_previewMailbox;
};
