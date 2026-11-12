#include "server.h"

#include <QDir>
#include <QLocalSocket>
#include <QStandardPaths>

#include "controller.h"

Server::Server(Controller *controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QLocalSocket::readyRead, socket, [this, socket] {
                while (socket->canReadLine()) {
                    socket->write(handle(m_controller, socket->readLine().trimmed()));
                    socket->flush();
                }
            });
        }
    });
}

QString Server::socketPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (dir.isEmpty())
        dir = QDir::tempPath();
    return dir + QStringLiteral("/numouse.sock");
}

bool Server::listen(const QString &path)
{
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    if (m_server.listen(path))
        return true;
    // A crashed daemon leaves its socket behind. Only remove it once nothing
    // answers on it, so a second daemon never steals the first one's socket.
    QLocalSocket probe;
    probe.connectToServer(path);
    if (probe.waitForConnected(200))
        return false;
    QLocalServer::removeServer(path);
    return m_server.listen(path);
}

QByteArray Server::handle(Controller *controller, const QByteArray &line)
{
    const QList<QByteArray> parts = line.split(' ');
    const QByteArray command = parts.value(0);

    if (command == "enable") {
        controller->enable();
    } else if (command == "disable") {
        controller->disable();
    } else if (command == "toggle") {
        controller->toggle();
    } else if (command == "press" || command == "release") {
        const QByteArray name = parts.value(1);
        bool ok = false;
        int key = name.toInt(&ok);
        if (name == "add") {
            key = Controller::ScrollUpKey;
        } else if (name == "enter") {
            key = Controller::ScrollDownKey;
        } else if (!ok || key < 1 || key > 9) {
            return "error: expected a key from 1 to 9, add, or enter\n";
        }
        command == "press" ? controller->press(key) : controller->release(key);
    } else if (command == "status") {
        // Falls through to the reply below.
    } else {
        return "error: unknown command\n";
    }
    return controller->isActive() ? "active\n" : "inactive\n";
}
