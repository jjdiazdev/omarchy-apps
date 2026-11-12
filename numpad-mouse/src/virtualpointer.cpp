#include "virtualpointer.h"

#include <QSocketNotifier>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <linux/input-event-codes.h>
#include <wayland-client.h>

#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"

namespace {

const wl_registry_listener registryListener = {
    &VirtualPointer::global,
    &VirtualPointer::globalRemove,
};

uint32_t nowMs()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return uint32_t(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

} // namespace

VirtualPointer::VirtualPointer(QObject *parent)
    : QObject(parent)
{
    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        m_error = QStringLiteral("Cannot connect to the Wayland display. Run numouse inside the Hyprland session.");
        return;
    }

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registryListener, this);
    wl_display_roundtrip(m_display);

    if (!m_manager) {
        m_error = QStringLiteral("The compositor does not offer zwlr_virtual_pointer_manager_v1.");
        return;
    }

    m_pointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(m_manager, m_seat);
    wl_display_roundtrip(m_display);

    // Reading the socket keeps the queue drained and tells us when Hyprland
    // goes away, so the daemon can exit instead of writing into a dead pipe.
    m_notifier = new QSocketNotifier(wl_display_get_fd(m_display), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this] {
        if (wl_display_dispatch(m_display) < 0) {
            m_notifier->setEnabled(false);
            emit disconnected();
        }
    });
}

VirtualPointer::~VirtualPointer()
{
    if (m_pointer)
        zwlr_virtual_pointer_v1_destroy(m_pointer);
    if (m_manager)
        zwlr_virtual_pointer_manager_v1_destroy(m_manager);
    if (m_seat)
        wl_seat_destroy(m_seat);
    if (m_registry)
        wl_registry_destroy(m_registry);
    if (m_display) {
        wl_display_flush(m_display);
        wl_display_disconnect(m_display);
    }
}

void VirtualPointer::global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto *self = static_cast<VirtualPointer *>(data);
    if (std::strcmp(interface, zwlr_virtual_pointer_manager_v1_interface.name) == 0) {
        self->m_manager = static_cast<zwlr_virtual_pointer_manager_v1 *>(
            wl_registry_bind(registry, name, &zwlr_virtual_pointer_manager_v1_interface, 1));
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0 && !self->m_seat) {
        self->m_seat = static_cast<wl_seat *>(
            wl_registry_bind(registry, name, &wl_seat_interface, qMin(version, 1u)));
    }
}

void VirtualPointer::globalRemove(void *, wl_registry *, uint32_t)
{
}

void VirtualPointer::move(double dx, double dy)
{
    if (!m_pointer)
        return;
    zwlr_virtual_pointer_v1_motion(m_pointer, nowMs(), wl_fixed_from_double(dx), wl_fixed_from_double(dy));
    zwlr_virtual_pointer_v1_frame(m_pointer);
    flush();
}

void VirtualPointer::click(Button button)
{
    if (!m_pointer)
        return;
    const uint32_t code = button == Left ? BTN_LEFT : BTN_RIGHT;
    zwlr_virtual_pointer_v1_button(m_pointer, nowMs(), code, WL_POINTER_BUTTON_STATE_PRESSED);
    zwlr_virtual_pointer_v1_frame(m_pointer);
    zwlr_virtual_pointer_v1_button(m_pointer, nowMs(), code, WL_POINTER_BUTTON_STATE_RELEASED);
    zwlr_virtual_pointer_v1_frame(m_pointer);
    flush();
}

void VirtualPointer::scroll(int steps)
{
    if (!m_pointer || steps == 0)
        return;
    // Report it as a wheel with discrete notches, so apps scroll by their
    // usual line amount instead of treating it as smooth touchpad motion.
    const uint32_t time = nowMs();
    zwlr_virtual_pointer_v1_axis_source(m_pointer, WL_POINTER_AXIS_SOURCE_WHEEL);
    zwlr_virtual_pointer_v1_axis_discrete(m_pointer, time, WL_POINTER_AXIS_VERTICAL_SCROLL,
                                          wl_fixed_from_int(15 * steps), steps);
    zwlr_virtual_pointer_v1_frame(m_pointer);
    flush();
}

void VirtualPointer::flush()
{
    if (wl_display_flush(m_display) < 0 && errno != EAGAIN)
        emit disconnected();
}
