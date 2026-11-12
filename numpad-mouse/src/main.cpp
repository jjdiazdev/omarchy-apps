#include <QCoreApplication>
#include <QLocalSocket>
#include <cstdio>
#include <cstring>

#include "controller.h"
#include "notifysend.h"
#include "server.h"
#include "virtualpointer.h"

namespace {

const char *usage =
    "Usage: numouse <command>\n"
    "\n"
    "  daemon           Run the background service (started by Hyprland)\n"
    "  toggle           Turn numpad mouse mode on or off\n"
    "  enable | disable Turn it on or off explicitly\n"
    "  press <key>      A keypad key went down (1-9, add, enter)\n"
    "  release <key>    A keypad key went up\n"
    "  status           Print active or inactive\n";

int runDaemon(QCoreApplication &app)
{
    NotifySend notifier;
    VirtualPointer pointer;
    if (!pointer.error().isEmpty()) {
        const QByteArray message = "numouse: " + pointer.error().toUtf8();
        std::fprintf(stderr, "%s\n", message.constData());
        notifier.notify(message.constData(), Notifier::Critical);
        return 1;
    }
    QObject::connect(&pointer, &VirtualPointer::disconnected, &app, [] {
        std::fprintf(stderr, "numouse: lost the Wayland connection\n");
        QCoreApplication::exit(1);
    });

    Controller controller(&pointer, &notifier);
    Server server(&controller);
    const QString path = Server::socketPath();
    if (!server.listen(path)) {
        std::fprintf(stderr, "numouse: cannot listen on %s (%s). Is another daemon running?\n",
                     qPrintable(path), qPrintable(server.errorString()));
        return 1;
    }
    return app.exec();
}

int runClient(const QByteArray &line)
{
    QLocalSocket socket;
    socket.connectToServer(Server::socketPath());
    if (!socket.waitForConnected(500)) {
        const char *message = "numouse: the daemon is not running. Start it with `numouse daemon`.";
        std::fprintf(stderr, "%s\n", message);
        // Keypad keys spawn this client too, so only the toggle reports it on
        // screen rather than flooding notifications on every key press.
        if (line == "toggle" || line == "enable")
            NotifySend().notify(message, Notifier::Critical);
        return 1;
    }
    socket.write(line + '\n');
    socket.flush();
    if (!socket.waitForReadyRead(500) && !socket.canReadLine()) {
        std::fprintf(stderr, "numouse: no reply from the daemon\n");
        return 1;
    }
    const QByteArray reply = socket.readLine();
    if (reply.startsWith("error")) {
        std::fprintf(stderr, "numouse: %s", reply.constData());
        return 1;
    }
    if (line == "status")
        std::fputs(reply.constData(), stdout);
    return 0;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc < 2 || !std::strcmp(argv[1], "-h") || !std::strcmp(argv[1], "--help")) {
        std::fputs(usage, argc < 2 ? stderr : stdout);
        return argc < 2 ? 1 : 0;
    }

    QCoreApplication app(argc, argv);
    const QByteArray command = argv[1];
    if (command == "daemon")
        return runDaemon(app);

    QByteArray line = command;
    for (int i = 2; i < argc; ++i)
        line += ' ' + QByteArray(argv[i]);
    return runClient(line);
}
