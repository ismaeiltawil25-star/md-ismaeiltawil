#include "PdfZoomToolbar.h"
#include <QHBoxLayout>
#include <cmath>

PdfZoomToolbar::PdfZoomToolbar(QWidget *parent) : QWidget(parent) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    m_zoomOutBtn = new QPushButton(QStringLiteral("-"));
    m_zoomOutBtn->setObjectName("pdfZoomOutButton");
    layout->addWidget(m_zoomOutBtn);

    m_zoomResetBtn = new QPushButton(QStringLiteral("100 %"));
    m_zoomResetBtn->setObjectName("pdfZoomResetButton");
    layout->addWidget(m_zoomResetBtn);

    m_zoomInBtn = new QPushButton(QStringLiteral("+"));
    m_zoomInBtn->setObjectName("pdfZoomInButton");
    layout->addWidget(m_zoomInBtn);

    layout->addStretch();

    connect(m_zoomInBtn, &QPushButton::clicked,
            this, &PdfZoomToolbar::zoomInRequested);
    connect(m_zoomOutBtn, &QPushButton::clicked,
            this, &PdfZoomToolbar::zoomOutRequested);
    connect(m_zoomResetBtn, &QPushButton::clicked,
            this, &PdfZoomToolbar::zoomResetRequested);
}

void PdfZoomToolbar::setZoom(double factor) {
    int pct = std::max(1, static_cast<int>(std::round(factor * 100.0)));
    m_zoomResetBtn->setText(QStringLiteral("%1 %").arg(pct));
}
