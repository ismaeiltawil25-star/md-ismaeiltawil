#pragma once

#include <QWidget>
#include <QImage>
#include <QString>
#include <QSize>
#include <qmath.h>

class PdfSinglePagePreviewWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double zoomFactor READ zoomFactor WRITE setZoomFactor)

public:
    static constexpr double MinZoom = 0.1;
    static constexpr double MaxZoom = 10.0;
    static QSize minimumPreviewSize();

    explicit PdfSinglePagePreviewWidget(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void setStatusText(const QString &text);
    void clear();
    QImage image() const;
    QString statusText() const;

    void setZoomFactor(double zoom);
    double zoomFactor() const;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_image;
    QString m_statusText;
    double m_zoom = 1.0;

    QSizeF effectiveLogicalSizeF() const;
    QSize scaledImageSize() const;
};
