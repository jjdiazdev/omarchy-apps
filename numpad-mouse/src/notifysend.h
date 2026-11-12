#pragma once

#include "pointer.h"

// Shows desktop notifications through notify-send, which Omarchy's
// notification daemon picks up like any other.
class NotifySend : public Notifier
{
public:
    void notify(const char *message, Urgency urgency = Normal) override;
};
