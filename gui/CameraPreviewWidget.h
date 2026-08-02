#pragma once

#include <QImage>
#include <QWidget>

class CameraPreviewWidget final : public QWidget {
    Q_OBJECT

public:
    explicit CameraPreviewWidget(QWidget* parent = nullptr);

public slots:
    void setFrame(const QImage& frame);
    void setOwnedFrame(const QImage& frame);
    void clearFrame();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_frame;
};
