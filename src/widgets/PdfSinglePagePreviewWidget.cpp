#include "PdfSinglePagePreviewWidget.h"
#include <QPainter>
#include <QtMath>

QSize PdfSinglePagePreviewWidget::minimumPreviewSize() { return QSize(200, 200); }

PdfSinglePagePreviewWidget::PdfSinglePagePreviewWidget(QWidget *parent)
    : QWidget(parent) {}

void PdfSinglePagePreviewWidget::setImage(const QImage &image) {
    m_image = image;
    updateGeometry();
    update();
}

void PdfSinglePagePreviewWidget::setStatusText(const QString &text) {
    m_statusText = text;
    update();
}

void PdfSinglePagePreviewWidget::clear() {
    m_image = QImage{};
    m_statusText.clear();
    updateGeometry();
    update();
}

QImage PdfSinglePagePreviewWidget::image() const { return m_image; }
QString PdfSinglePagePreviewWidget::statusText() const { return m_statusText; }

void PdfSinglePagePreviewWidget::setZoomFactor(double zoom) {
    if (!std::isfinite(zoom)) zoom = MinZoom;
    zoom = qBound(MinZoom, zoom, MaxZoom);
    if (qFuzzyCompare(m_zoom, zoom)) return;
    m_zoom = zoom;
    updateGeometry();
    update();
}

double PdfSinglePagePreviewWidget::zoomFactor() const { return m_zoom; }

QSizeF PdfSinglePagePreviewWidget::effectiveLogicalSizeF() const {
    if (m_image.isNull())
        return QSizeF(minimumPreviewSize());
    qreal dpr = m_image.devicePixelRatio();
    if (dpr <= 0.0 || !std::isfinite(dpr))
        dpr = 1.0;
    return QSizeF(m_image.width() / dpr, m_image.height() / dpr);
}

QSize PdfSinglePagePreviewWidget::scaledImageSize() const {
    const QSizeF logical = effectiveLogicalSizeF();
    const int w = std::max(1, static_cast<int>(std::ceil(logical.width() * m_zoom)));
    const int h = std::max(1, static_cast<int>(std::ceil(logical.height() * m_zoom)));
    return QSize(w, h);
}

QSize PdfSinglePagePreviewWidget::sizeHint() const {
    if (m_image.isNull())
        return minimumPreviewSize();
    return scaledImageSize();
}

void PdfSinglePagePreviewWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());

    if (!m_image.isNull()) {
        QSize drawSize = scaledImageSize();
        int x = (width() - drawSize.width()) / 2;
        int y = (height() - drawSize.height()) / 2;
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(QRect(x, y, drawSize.width(), drawSize.height()), m_image);
        return;
    }

    if (!m_statusText.isEmpty()) {
        painter.setPen(palette().windowText().color());
        painter.drawText(rect(), Qt::AlignCenter | Qt::TextWordWrap, m_statusText);
    }
}
