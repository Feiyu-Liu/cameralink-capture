#include "CameraLinkMainWindow.h"
#include "QtCaptureController.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

#include <opencv2/core/utils/logger.hpp>

namespace {

QString SelectConfigurationFile(const QStringList& arguments) {
    if (arguments.size() > 1) {
        return arguments.at(1);
    }

    return QFileDialog::getOpenFileName(
        nullptr,
        QObject::tr("Select CameraLink configuration"),
        QString(),
        QObject::tr("INI configuration (*.ini)"));
}

bool IsValidConfigurationFile(const QString& path) {
    const QFileInfo file(path);
    return file.isFile() && file.suffix().compare(QStringLiteral("ini"), Qt::CaseInsensitive) == 0;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("CameraLink Capture"));
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    const QStringList arguments = application.arguments();
    if (arguments.size() > 2) {
        QMessageBox::critical(nullptr, QObject::tr("CameraLink Capture"),
            QObject::tr("Usage: CameraLinkCapture.exe [config.ini]"));
        return 1;
    }

    const QString configPath = SelectConfigurationFile(arguments);
    if (configPath.isEmpty()) {
        return 0;
    }
    if (!IsValidConfigurationFile(configPath)) {
        QMessageBox::critical(nullptr, QObject::tr("CameraLink Capture"),
            QObject::tr("The selected path is not a readable INI configuration file."));
        return 1;
    }

    CameraLinkMainWindow window;
    QtCaptureController controller;
    QObject::connect(&window, &CameraLinkMainWindow::commandSubmitted,
        &controller, &QtCaptureController::submitTextCommand);
    QObject::connect(&window, &CameraLinkMainWindow::closeRequested,
        &controller, &QtCaptureController::requestShutdown);
    QObject::connect(&controller, &QtCaptureController::logLineReady,
        &window, &CameraLinkMainWindow::appendLogLine);
    QObject::connect(&controller, &QtCaptureController::clearConsoleRequested,
        &window, &CameraLinkMainWindow::clearConsoleLog);
    QObject::connect(&controller, &QtCaptureController::previewFrameReady,
        &window, &CameraLinkMainWindow::setPreviewFrame);
    QObject::connect(&controller, &QtCaptureController::closingStarted,
        [&window]() { window.setCommandEnabled(false); });
    QObject::connect(&controller, &QtCaptureController::shutdownCompleted,
        [&window]() {
            window.allowClose();
            window.close();
        });

    window.show();
    window.setCommandEnabled(true);
    QTimer::singleShot(0, &controller, [&controller, configPath]() {
        if (!controller.startCapture(configPath)) {
            controller.requestShutdown();
        }
    });
    return application.exec();
}
