#include "notifysend.h"

#include <QProcess>
#include <QString>
#include <QStringList>
#include <cstdio>

void NotifySend::notify(const char *message, Urgency urgency)
{
    // The same tag replaces the previous numouse notification rather than
    // stacking one per toggle.
    QStringList args = {
        "-a", "numouse",
        "-h", "string:x-canonical-private-synchronous:numouse",
        "-u", urgency == Critical ? "critical" : "low",
        QString::fromUtf8(message),
    };
    if (!QProcess::startDetached("notify-send", args))
        std::fprintf(stderr, "numouse: notify-send failed: %s\n", message);
}
