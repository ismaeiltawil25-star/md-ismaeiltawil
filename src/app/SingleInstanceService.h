#ifndef SINGLEINSTANCESERVICE_H
#define SINGLEINSTANCESERVICE_H

#include <QObject>
#include <QStringList>

class QLocalServer;

class SingleInstanceService : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstanceService(const QString &serverName, QObject *parent = nullptr);
    ~SingleInstanceService() override;

    bool isPrimary() const { return m_isPrimary; }

    static bool tryRouteToPrimary(const QString &serverName, const QStringList &args);

signals:
    void filesRequested(const QStringList &paths);

private slots:
    void onNewConnection();

private:
    void handleMessage(const QByteArray &data);
    void activatePrimaryWindow();

    QLocalServer *m_server = nullptr;
    QString m_serverName;
    bool m_isPrimary = false;
};

#endif
