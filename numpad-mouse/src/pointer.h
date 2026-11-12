#pragma once

// The controller only needs to move and click, so it talks to this interface.
// The daemon backs it with a Wayland virtual pointer and the tests with a fake
// that records what it was asked to do.
class Pointer
{
public:
    enum Button { Left, Right };

    virtual ~Pointer() = default;
    virtual void move(double dx, double dy) = 0;
    virtual void click(Button button) = 0;
    // One wheel notch per step; positive steps scroll down, like a wheel.
    virtual void scroll(int steps) = 0;
};

// Notifications go through this interface for the same reason.
class Notifier
{
public:
    enum Urgency { Normal, Critical };

    virtual ~Notifier() = default;
    virtual void notify(const char *message, Urgency urgency = Normal) = 0;
};
