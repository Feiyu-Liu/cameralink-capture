#pragma once

#include <QWidget>

class QLineEdit;
class QPlainTextEdit;
class QToolButton;

class CommandConsoleWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CommandConsoleWidget(QWidget* parent = nullptr);

    void appendLine(const QString& text);
    void clearLog();
    int historyBlockCount() const;
    void setCommandEnabled(bool enabled);
    void focusCommandInput();

signals:
    void commandEntered(const QString& command);
    void expandRequested();

private slots:
    void submitCurrentCommand();
    void clearOutput();

private:
    QPlainTextEdit* m_output = nullptr;
    QLineEdit* m_input = nullptr;
    QToolButton* m_expandButton = nullptr;
};
