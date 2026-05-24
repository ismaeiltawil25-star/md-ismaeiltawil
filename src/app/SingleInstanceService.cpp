#include "SingleInstanceService.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QDataStream>
#include <QWidget>

SingleInstanceService::SingleInstanceService(const QString &serverName, QObject *parent)
    : QObject(parent)
    , m_serverName(serverName)
{
    QLocalServer::removeServer(m_serverName);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstanceService::onNewConnection);

    if (m_server->listen(m_serverName)) {
        m_isPrimary = true;
    }
}

SingleInstanceService::~SingleInstanceService()
{
    if (m_server) {
        m_server->close();
        QLocalServer::removeServer(m_serverName);
    }
}

bool SingleInstanceService::tryRouteToPrimary(const QString &serverName, const QStringList &args)
{
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (!socket.waitForConnected(3000))
        return false;

    QJsonObject msg;
    msg["type"] = args.isEmpty() ? QStringLiteral("activate") : QStringLiteral("open");
    QJsonArray arr;
    for (const auto &a : args)
        arr.append(a);
    msg["paths"] = arr;

    QByteArray json = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint32>(json.size());
    packet.append(json);

    socket.write(packet);
    socket.flush();
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();

    return true;
}

void SingleInstanceService::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
            handleMessage(socket->readAll());
            socket->deleteLater();
        });
        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void SingleInstanceService::handleMessage(const QByteArray &data)
{
    if (data.size() < 4)
        return;

    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint32 msgSize;
    stream >> msgSize;

    if (data.size() < 4 + static_cast<int>(msgSize))
        return;

    QByteArray jsonData = data.mid(4, msgSize);
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QString type = obj.value("type").toString();

    if (type == QStringLiteral("open")) {
        QJsonArray arr = obj.value("paths").toArray();
        QStringList paths;
        for (const auto &v : arr)
            paths.append(v.toString());
        if (!paths.isEmpty())
            emit filesRequested(paths);
    }

    activatePrimaryWindow();
}

void SingleInstanceService::activatePrimaryWindow()
{
    auto *mainWin = qobject_cast<QWidget *>(parent());
    if (mainWin) {
        mainWin->showNormal();
        mainWin->raise();
        mainWin->activateWindow();
    }
}
