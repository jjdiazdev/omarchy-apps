#pragma once

#include <QByteArray>
#include <QLocalServer>
#include <QString>

class Controller;

// Accepts one-line commands from `numouse <command>` clients on a socket in
// $XDG_RUNTIME_DIR and hands them to the controller.
class Server : public QObject
{
    Q_OBJECT

public:
    Server(Controller *controller, QObject *parent = nullptr);

    bool listen(const QString &path);
    QString errorString() const { return m_server.errorString(); }

    static QString socketPath();

    // Runs one command line and returns the reply; shared with the tests.
    static QByteArray handle(Controller *controller, const QByteArray &line);

private:
    Controller *m_controller;
    QLocalServer m_server;
};
