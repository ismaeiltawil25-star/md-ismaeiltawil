#pragma once

#include "PdfSinglePagePreviewWidget.h"
#include "PdfPageImageRenderer.h"
#include <QObject>
#include <QPointer>
#include <QString>

class PdfSinglePagePreviewController : public QObject {
    Q_OBJECT

public:
    explicit PdfSinglePagePreviewController(PdfSinglePagePreviewWidget *widget,
                                            QObject *parent = nullptr);

    PdfPageImageRenderResult renderDocumentPage(const PdfDocument &document,
                                                int pageIndex,
                                                const PdfPathRenderSettings &settings = {});

    void zoomIn();
    void zoomOut();
    void zoomReset();
    void zoomTo(double factor);
    double zoomFactor() const;

    void clear();

signals:
    void zoomChanged(double factor);

private:
    void applyZoom(double factor);
    QPointer<PdfSinglePagePreviewWidget> m_widget;
    PdfStreamDecoder m_decoder;
};
