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

    void clear();

private:
    QPointer<PdfSinglePagePreviewWidget> m_widget;
    PdfStreamDecoder m_decoder;
};
