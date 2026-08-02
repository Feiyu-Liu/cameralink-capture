#include "CameraLinkMainWindow.h"

#include "CameraPreviewWidget.h"
#include "CommandConsoleWidget.h"

#include <QCloseEvent>
#include <QImage>
#include <QSplitter>

namespace {

constexpr int kInitialContentWidth = 600;
constexpr int kInitialContentHeight = 870;
constexpr int kPreviewInitialHeight = 600;
constexpr int kConsoleInitialHeight = 270;

} // namespace

CameraLinkMainWindow::CameraLinkMainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("CameraLink Capture"));
    resize(kInitialContentWidth, kInitialContentHeight);
    setMinimumSize(360, 400);

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(5);

    m_preview = new CameraPreviewWidget(m_splitter);
    m_preview->setObjectName("preview");
    m_preview->setMinimumHeight(240);
    m_console = new CommandConsoleWidget(m_splitter);
    m_splitter->addWidget(m_preview);
    m_splitter->addWidget(m_console);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 0);
    m_splitter->setSizes({kPreviewInitialHeight, kConsoleInitialHeight});
    setCentralWidget(m_splitter);

    connect(m_console, &CommandConsoleWidget::commandEntered,
            this, &CameraLinkMainWindow::commandSubmitted);
    connect(m_console, &CommandConsoleWidget::expandRequested,
            this, &CameraLinkMainWindow::toggleConsoleExpansion);
}

void CameraLinkMainWindow::appendLogLine(const QString& text) {
    m_console->appendLine(text);
}

void CameraLinkMainWindow::clearConsoleLog() {
    m_console->clearLog();
}

void CameraLinkMainWindow::setPreviewFrame(const QImage& frame) {
    m_preview->setOwnedFrame(frame);
}

void CameraLinkMainWindow::setCommandEnabled(bool enabled) {
    m_console->setCommandEnabled(enabled);
}

void CameraLinkMainWindow::allowClose() {
    m_closeAllowed = true;
}

QList<int> CameraLinkMainWindow::splitterSizes() const {
    return m_splitter->sizes();
}

void CameraLinkMainWindow::closeEvent(QCloseEvent* event) {
    if (m_closeAllowed) {
        event->accept();
        return;
    }

    setCommandEnabled(false);
    emit closeRequested();
    event->ignore();
}

void CameraLinkMainWindow::toggleConsoleExpansion() {
    const int windowHeight = m_splitter->height();
    const QList<int> currentSizes = m_splitter->sizes();
    const bool consoleIsExpanded = currentSizes.size() == 2 &&
        currentSizes.at(1) >= (windowHeight * 55) / 100;

    if (!consoleIsExpanded) {
        m_previousSplitterSizes = currentSizes;
        const int consoleHeight = (windowHeight * 60) / 100;
        m_splitter->setSizes({windowHeight - consoleHeight, consoleHeight});
        return;
    }

    if (m_previousSplitterSizes.size() == 2) {
        m_splitter->setSizes(m_previousSplitterSizes);
    }
    else {
        m_splitter->setSizes({kPreviewInitialHeight, kConsoleInitialHeight});
    }
}
