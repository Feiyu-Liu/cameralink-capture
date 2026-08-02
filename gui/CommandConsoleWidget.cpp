#include "CommandConsoleWidget.h"

#include <QColor>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStyle>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextOption>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kMaximumConsoleBlocks = 5000;

} // namespace

CommandConsoleWidget::CommandConsoleWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(140);
    setObjectName("commandConsole");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("consoleToolbar");
    toolbar->setFixedHeight(38);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(14, 0, 14, 0);
    toolbarLayout->setSpacing(8);

    auto* terminalIcon = new QToolButton(toolbar);
    terminalIcon->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    terminalIcon->setToolTip(tr("Command console"));
    terminalIcon->setEnabled(false);
    terminalIcon->setAutoRaise(true);
    toolbarLayout->addWidget(terminalIcon);
    toolbarLayout->addStretch();

    auto* clearButton = new QToolButton(toolbar);
    clearButton->setObjectName("clearAction");
    clearButton->setIcon(style()->standardIcon(QStyle::SP_DialogDiscardButton));
    clearButton->setToolTip(tr("Clear console"));
    clearButton->setAutoRaise(true);
    connect(clearButton, &QToolButton::clicked, this, &CommandConsoleWidget::clearOutput);
    toolbarLayout->addWidget(clearButton);

    m_expandButton = new QToolButton(toolbar);
    m_expandButton->setObjectName("expandAction");
    m_expandButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    m_expandButton->setToolTip(tr("Expand console"));
    m_expandButton->setAutoRaise(true);
    connect(m_expandButton, &QToolButton::clicked, this, &CommandConsoleWidget::expandRequested);
    toolbarLayout->addWidget(m_expandButton);
    layout->addWidget(toolbar);

    m_output = new QPlainTextEdit(this);
    m_output->setObjectName("terminal");
    m_output->setReadOnly(true);
    m_output->setUndoRedoEnabled(false);
    m_output->setMaximumBlockCount(kMaximumConsoleBlocks);
    m_output->setWordWrapMode(QTextOption::NoWrap);
    m_output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_output, 1);

    auto* inputRow = new QWidget(this);
    inputRow->setObjectName("consoleInputRow");
    auto* inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(16, 0, 16, 10);
    inputLayout->setSpacing(8);

    auto* prompt = new QLabel(QStringLiteral(">"), inputRow);
    prompt->setObjectName("consolePrompt");
    prompt->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    inputLayout->addWidget(prompt);

    m_input = new QLineEdit(inputRow);
    m_input->setObjectName("commandInput");
    m_input->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_input->setClearButtonEnabled(true);
    connect(m_input, &QLineEdit::returnPressed, this, &CommandConsoleWidget::submitCurrentCommand);
    inputLayout->addWidget(m_input, 1);
    layout->addWidget(inputRow);

    setStyleSheet(
        "#commandConsole { background: #111417; border-top: 1px solid #2b3137; }"
        "#consoleToolbar { background: #181c20; border-bottom: 1px solid #2b3137; }"
        "#terminal { background: #111417; color: #d8dee5; border: 0; padding: 8px 16px; }"
        "#consoleInputRow { background: #111417; }"
        "#consolePrompt { color: #71b9e8; }"
        "#commandInput { background: #111417; color: #ffffff; border: 0; }"
        "#commandInput:disabled { color: #7e8a96; }");
}

void CommandConsoleWidget::appendLine(const QString& text) {
    QTextCharFormat format;
    if (text.startsWith(QStringLiteral("[ERROR]"))) {
        format.setForeground(QColor(QStringLiteral("#f08078")));
    }
    else if (text.startsWith(QStringLiteral("[WARNING]"))) {
        format.setForeground(QColor(QStringLiteral("#e7c36b")));
    }
    else if (text.startsWith(QStringLiteral("[SUCCESS]"))) {
        format.setForeground(QColor(QStringLiteral("#57c995")));
    }
    else if (text.startsWith(QLatin1Char('>'))) {
        format.setForeground(QColor(QStringLiteral("#71b9e8")));
    }
    else {
        format.setForeground(QColor(QStringLiteral("#d8dee5")));
    }

    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);
    if (!m_output->document()->isEmpty()) {
        cursor.insertBlock();
    }
    cursor.insertText(text, format);
    m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum());
}

void CommandConsoleWidget::clearLog() {
    m_output->clear();
}

int CommandConsoleWidget::historyBlockCount() const {
    return m_output->document()->blockCount();
}

void CommandConsoleWidget::setCommandEnabled(bool enabled) {
    m_input->setEnabled(enabled);
    if (enabled) {
        focusCommandInput();
    }
}

void CommandConsoleWidget::focusCommandInput() {
    if (m_input->isEnabled()) {
        m_input->setFocus(Qt::OtherFocusReason);
    }
}

void CommandConsoleWidget::submitCurrentCommand() {
    const QString command = m_input->text().trimmed();
    m_input->clear();
    if (command.isEmpty()) {
        return;
    }

    appendLine(QStringLiteral("> %1").arg(command));
    emit commandEntered(command);
}

void CommandConsoleWidget::clearOutput() {
    clearLog();
}
