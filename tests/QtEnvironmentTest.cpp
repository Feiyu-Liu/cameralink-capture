#include <QApplication>
#include <QImage>
#include <QWidget>
#include <QtGlobal>

#include <iostream>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);

    QImage mono8Image(32, 16, QImage::Format_Grayscale8);
    QWidget widget;
    widget.resize(320, 240);

    if (mono8Image.isNull() || mono8Image.depth() != 8 || widget.size().isEmpty()) {
        std::cerr << "Qt Widgets environment validation failed.\n";
        return 1;
    }

    std::cout << "Qt " << qVersion() << " Widgets environment ready.\n";
    return 0;
}
