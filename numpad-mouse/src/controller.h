#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointF>
#include <QSet>
#include <QTimer>

#include "pointer.h"

// Turns numpad key presses into pointer motion and clicks. Keys are the digits
// printed on the keypad: 8 is up, 2 down, 4 left, 6 right, the corners move
// diagonally, and 5 clicks.
class Controller : public QObject
{
    Q_OBJECT

public:
    static constexpr int TickMs = 8;
    static constexpr int DoubleClickMs = 250;
    static constexpr double MinSpeed = 3.0;  // pixels per tick when a key goes down
    static constexpr double MaxSpeed = 22.0; // pixels per tick after the ramp
    static constexpr int RampMs = 700;
    static constexpr int ScrollDelayMs = 300;  // hold this long before scrolling repeats
    static constexpr int ScrollRepeatMs = 60;

    // The keypad keys beside the digits: + scrolls up, Enter scrolls down.
    static constexpr int ScrollUpKey = 10;
    static constexpr int ScrollDownKey = 11;

    Controller(Pointer *pointer, Notifier *notifier, QObject *parent = nullptr);

    bool isActive() const { return m_active; }

    void enable();
    void disable();
    void toggle();
    void press(int key);
    void release(int key);

    // The unit-length direction the held keys add up to, or zero when they
    // cancel out or none are held.
    QPointF direction() const;

    // How far one tick moves after the keys have been held for elapsedMs.
    static double speedAt(qint64 elapsedMs);

    static QPointF keyVector(int key);

private:
    void tick();
    void releaseAll();

    Pointer *m_pointer;
    Notifier *m_notifier;
    bool m_active = false;
    QSet<int> m_held;
    QTimer m_motion;
    QTimer m_click;
    QTimer m_scroll;
    int m_scrollStep = 0;
    QElapsedTimer m_heldFor;
};
