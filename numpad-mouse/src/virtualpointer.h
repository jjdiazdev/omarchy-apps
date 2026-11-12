#pragma once

#include <QObject>
#include <QString>

#include "pointer.h"

struct wl_display;
struct wl_registry;
struct wl_seat;
struct zwlr_virtual_pointer_manager_v1;
struct zwlr_virtual_pointer_v1;
class QSocketNotifier;

// A pointer device made up through the compositor's wlr virtual pointer
// protocol. Hyprland accepts it from any client of the session, so no root,
// uinput, or input group is needed.
class VirtualPointer : public QObject, public Pointer
{
    Q_OBJECT

public:
    explicit VirtualPointer(QObject *parent = nullptr);
    ~VirtualPointer() override;

    // Empty when the pointer is ready, otherwise what went wrong.
    QString error() const { return m_error; }

    void move(double dx, double dy) override;
    void click(Button button) override;
    void scroll(int steps) override;

    // Registry callbacks; public only so the C listener table can point at them.
    static void global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void globalRemove(void *data, wl_registry *registry, uint32_t name);

signals:
    void disconnected();

private:
    void flush();

    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    wl_seat *m_seat = nullptr;
    zwlr_virtual_pointer_manager_v1 *m_manager = nullptr;
    zwlr_virtual_pointer_v1 *m_pointer = nullptr;
    QSocketNotifier *m_notifier = nullptr;
    QString m_error;
};
