#include "CameraPreviewWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>

CameraPreviewWidget::CameraPreviewWidget(QWidget* parent)
    : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(false);
}

void CameraPreviewWidget::setFrame(const QImage& frame) {
    if (frame.isNull()) {
        clearFrame();
        return;
    }

    // The producer can reuse its backing buffer after this call returns.
    m_frame = frame.copy();
    update();
}

void CameraPreviewWidget::setOwnedFrame(const QImage& frame) {
    if (frame.isNull()) {
        clearFrame();
        return;
    }

    m_frame = frame;
    update();
}

void CameraPreviewWidget::clearFrame() {
    if (!m_frame.isNull()) {
        m_frame = QImage();
        update();
    }
}

void CameraPreviewWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    if (m_frame.isNull()) {
        painter.fillRect(rect(), QColor(108, 116, 122));
        return;
    }
    painter.fillRect(rect(), QColor(41, 45, 49));

    const QSize targetSize = m_frame.size().scaled(size(), Qt::KeepAspectRatio);
    const QRect targetRect(
        (width() - targetSize.width()) / 2,
        (height() - targetSize.height()) / 2,
        targetSize.width(),
        targetSize.height());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.drawImage(targetRect, m_frame);
}
