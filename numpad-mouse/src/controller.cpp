#include "controller.h"

#include <QtMath>

Controller::Controller(Pointer *pointer, Notifier *notifier, QObject *parent)
    : QObject(parent), m_pointer(pointer), m_notifier(notifier)
{
    m_motion.setInterval(TickMs);
    m_motion.setTimerType(Qt::PreciseTimer);
    connect(&m_motion, &QTimer::timeout, this, &Controller::tick);

    // A single 5 can only be told apart from the first half of a double 5 by
    // waiting, so the left click fires when this timer runs out unanswered.
    m_click.setSingleShot(true);
    m_click.setInterval(DoubleClickMs);
    connect(&m_click, &QTimer::timeout, this, [this] { m_pointer->click(Pointer::Left); });

    // Holding a scroll key repeats like a held key in a text field: one notch
    // at once, a pause, then a steady stream.
    m_scroll.setTimerType(Qt::PreciseTimer);
    connect(&m_scroll, &QTimer::timeout, this, [this] {
        m_pointer->scroll(m_scrollStep);
        m_scroll.setInterval(ScrollRepeatMs);
    });
}

void Controller::enable()
{
    if (m_active)
        return;
    m_active = true;
    m_notifier->notify("Numpad Mouse Actived");
}

void Controller::disable()
{
    if (!m_active)
        return;
    m_active = false;
    // Hyprland stops sending releases once the keypad binds are disabled, so a
    // key still held at that moment would otherwise keep the pointer drifting.
    releaseAll();
    m_click.stop();
    m_scroll.stop();
    m_scrollStep = 0;
    m_notifier->notify("Numpad Mouse Deactivated");
}

void Controller::toggle()
{
    m_active ? disable() : enable();
}

void Controller::press(int key)
{
    if (!m_active)
        return;

    if (key == 5) {
        if (m_click.isActive()) {
            m_click.stop();
            m_pointer->click(Pointer::Right);
        } else {
            m_click.start();
        }
        return;
    }

    if (key == ScrollUpKey || key == ScrollDownKey) {
        const int step = key == ScrollUpKey ? -1 : 1;
        if (m_scrollStep == step && m_scroll.isActive())
            return; // A repeated press while already held.
        m_scrollStep = step;
        m_pointer->scroll(step);
        m_scroll.setInterval(ScrollDelayMs);
        m_scroll.start();
        return;
    }

    if (keyVector(key).isNull() || m_held.contains(key))
        return;

    if (m_held.isEmpty())
        m_heldFor.start();
    m_held.insert(key);
    if (!m_motion.isActive()) {
        tick(); // Move at once, so a quick tap still nudges the pointer.
        m_motion.start();
    }
}

void Controller::release(int key)
{
    if ((key == ScrollUpKey && m_scrollStep < 0) || (key == ScrollDownKey && m_scrollStep > 0)) {
        m_scroll.stop();
        m_scrollStep = 0;
        return;
    }
    m_held.remove(key);
    if (m_held.isEmpty())
        m_motion.stop();
}

void Controller::releaseAll()
{
    m_held.clear();
    m_motion.stop();
}

QPointF Controller::keyVector(int key)
{
    switch (key) {
    case 1: return {-1, 1};
    case 2: return {0, 1};
    case 3: return {1, 1};
    case 4: return {-1, 0};
    case 6: return {1, 0};
    case 7: return {-1, -1};
    case 8: return {0, -1};
    case 9: return {1, -1};
    default: return {};
    }
}

QPointF Controller::direction() const
{
    QPointF sum;
    for (int key : m_held)
        sum += keyVector(key);
    const double length = qSqrt(sum.x() * sum.x() + sum.y() * sum.y());
    if (length < 1e-9)
        return {};
    return sum / length;
}

double Controller::speedAt(qint64 elapsedMs)
{
    const double t = qBound(0.0, double(elapsedMs) / RampMs, 1.0);
    // Ease in, so short holds stay precise and long ones cross the screen fast.
    return MinSpeed + (MaxSpeed - MinSpeed) * t * t;
}

void Controller::tick()
{
    const QPointF dir = direction();
    if (dir.isNull())
        return;
    const double speed = speedAt(m_heldFor.isValid() ? m_heldFor.elapsed() : 0);
    m_pointer->move(dir.x() * speed, dir.y() * speed);
}
