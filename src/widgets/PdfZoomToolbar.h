#pragma once

#include <QWidget>
#include <QPushButton>

class PdfZoomToolbar : public QWidget {
    Q_OBJECT
public:
    explicit PdfZoomToolbar(QWidget *parent = nullptr);
    void setZoom(double factor);

signals:
    void zoomInRequested();
    void zoomOutRequested();
    void zoomResetRequested();

private:
    QPushButton *m_zoomOutBtn  = nullptr;
    QPushButton *m_zoomResetBtn = nullptr;
    QPushButton *m_zoomInBtn   = nullptr;
};
