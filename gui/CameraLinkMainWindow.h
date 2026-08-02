#pragma once

#include <QMainWindow>
#include <QList>

class CameraPreviewWidget;
class CommandConsoleWidget;
class QCloseEvent;
class QImage;
class QSplitter;

class CameraLinkMainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit CameraLinkMainWindow(QWidget* parent = nullptr);

    void appendLogLine(const QString& text);
    void clearConsoleLog();
    void setPreviewFrame(const QImage& frame);
    void setCommandEnabled(bool enabled);
    void allowClose();
    QList<int> splitterSizes() const;

signals:
    void commandSubmitted(const QString& command);
    void closeRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void toggleConsoleExpansion();

private:
    CameraPreviewWidget* m_preview = nullptr;
    CommandConsoleWidget* m_console = nullptr;
    QSplitter* m_splitter = nullptr;
    QList<int> m_previousSplitterSizes;
    bool m_closeAllowed = false;
};
