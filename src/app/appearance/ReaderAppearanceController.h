#ifndef READERAPPEARANCECONTROLLER_H
#define READERAPPEARANCECONTROLLER_H

#include <QObject>
#include "ReaderAppearanceSettings.h"

class ReaderAppearanceController : public QObject
{
    Q_OBJECT

public:
    explicit ReaderAppearanceController(QObject *parent = nullptr);

    void load();
    const ReaderAppearanceSettings &settings() const;

public slots:
    void applySettings(const ReaderAppearanceSettings &newSettings);
    void restoreDefaults();
    void clearStoredSettings();

signals:
    void appearanceChanged(const ReaderAppearanceSettings &settings);

private:
    ReaderAppearanceSettings m_settings;
};

#endif
