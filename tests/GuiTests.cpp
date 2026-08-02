#include "gui/CameraLinkMainWindow.h"
#include "gui/CameraPreviewWidget.h"

#include <QImage>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QSplitter>
#include <QTest>
#include <QTextDocument>
#include <QToolButton>

#include <algorithm>
#include <cstdint>
#include <vector>

class GuiTests final : public QObject {
    Q_OBJECT

private slots:
    void initialWindowHasUsablePreviewAndConsole();
    void commandEntryEmitsTrimmedCommand();
    void clearButtonAndHistoryLimitManageTerminalOutput();
    void expandButtonTogglesConsoleHeight();
    void previewCopiesExternallyOwnedImageData();
};

void GuiTests::initialWindowHasUsablePreviewAndConsole() {
    CameraLinkMainWindow window;
    window.show();
    QTest::qWait(20);

    QCOMPARE(window.width(), 600);
    QCOMPARE(window.height(), 870);

    const auto sizes = window.splitterSizes();
    QCOMPARE(sizes.size(), 2);
    QVERIFY(sizes.at(0) >= 240);
    QVERIFY(sizes.at(1) >= 140);
    QVERIFY(window.findChild<CameraPreviewWidget*>("preview") != nullptr);
    QVERIFY(window.findChild<QPlainTextEdit*>("terminal") != nullptr);
    auto* commandInput = window.findChild<QLineEdit*>("commandInput");
    QVERIFY(commandInput != nullptr);
    window.setCommandEnabled(true);
    QTRY_VERIFY(commandInput->hasFocus());
}

void GuiTests::commandEntryEmitsTrimmedCommand() {
    CameraLinkMainWindow window;
    window.show();
    QTest::qWait(20);

    QSignalSpy commands(&window, &CameraLinkMainWindow::commandSubmitted);
    auto* input = window.findChild<QLineEdit*>("commandInput");
    QVERIFY(input != nullptr);

    input->setText(QStringLiteral("  R  "));
    QTest::keyClick(input, Qt::Key_Return);

    QCOMPARE(commands.count(), 1);
    QCOMPARE(commands.at(0).at(0).toString(), QStringLiteral("R"));
    QVERIFY(input->text().isEmpty());
}

void GuiTests::clearButtonAndHistoryLimitManageTerminalOutput() {
    CameraLinkMainWindow window;
    window.show();
    QTest::qWait(20);

    const QString unicodeLine = QString::fromUtf8("相机预览已启动");
    window.appendLogLine(unicodeLine);
    auto* terminal = window.findChild<QPlainTextEdit*>("terminal");
    QVERIFY(terminal != nullptr);
    QCOMPARE(terminal->toPlainText(), unicodeLine);

    for (int line = 0; line < 5025; ++line) {
        window.appendLogLine(QString::number(line));
    }
    QVERIFY(terminal->document()->blockCount() <= 5000);

    auto* clearButton = window.findChild<QToolButton*>("clearAction");
    QVERIFY(clearButton != nullptr);
    QTest::mouseClick(clearButton, Qt::LeftButton);
    QVERIFY(terminal->toPlainText().isEmpty());
}

void GuiTests::expandButtonTogglesConsoleHeight() {
    CameraLinkMainWindow window;
    window.show();
    QTest::qWait(20);

    auto* splitter = window.findChild<QSplitter*>();
    auto* expandButton = window.findChild<QToolButton*>("expandAction");
    QVERIFY(splitter != nullptr);
    QVERIFY(expandButton != nullptr);

    const int initialConsoleHeight = window.splitterSizes().at(1);
    QTest::mouseClick(expandButton, Qt::LeftButton);
    QTest::qWait(20);

    const int expandedConsoleHeight = window.splitterSizes().at(1);
    QVERIFY(expandedConsoleHeight >= (splitter->height() * 55) / 100);

    QTest::mouseClick(expandButton, Qt::LeftButton);
    QTest::qWait(20);
    const int restoredConsoleHeight = window.splitterSizes().at(1);
    QVERIFY(restoredConsoleHeight < expandedConsoleHeight);
    QVERIFY(restoredConsoleHeight >= initialConsoleHeight - 2);
}

void GuiTests::previewCopiesExternallyOwnedImageData() {
    CameraPreviewWidget preview;
    preview.resize(160, 80);
    preview.show();
    QTest::qWait(20);

    std::vector<std::uint8_t> bytes(8, 0);
    const QImage borrowed(bytes.data(), 4, 2, 4, QImage::Format_Grayscale8);
    preview.setFrame(borrowed);
    std::fill(bytes.begin(), bytes.end(), 255);

    QTest::qWait(20);
    const QImage rendered = preview.grab().toImage();
    QVERIFY(!rendered.isNull());
    QVERIFY(rendered.pixelColor(80, 40).value() < 10);
}

QTEST_MAIN(GuiTests)

#include "GuiTests.moc"
