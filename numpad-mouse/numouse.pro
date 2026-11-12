QT += core network
QT -= gui

CONFIG += c++17 release link_pkgconfig
PKGCONFIG += wayland-client
TARGET = numouse
TEMPLATE = app

HEADERS += \
    src/controller.h \
    src/notifysend.h \
    src/pointer.h \
    src/server.h \
    src/virtualpointer.h

SOURCES += \
    src/main.cpp \
    src/controller.cpp \
    src/notifysend.cpp \
    src/server.cpp \
    src/virtualpointer.cpp

# The protocol bindings are generated at build time, never committed.
WAYLAND_PROTOCOLS = $$PWD/protocol/wlr-virtual-pointer-unstable-v1.xml

wayland_header.input = WAYLAND_PROTOCOLS
wayland_header.output = ${QMAKE_FILE_BASE}-client-protocol.h
wayland_header.commands = wayland-scanner client-header ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
wayland_header.variable_out = HEADERS
wayland_header.CONFIG += no_link target_predeps

wayland_code.input = WAYLAND_PROTOCOLS
wayland_code.output = ${QMAKE_FILE_BASE}-protocol.c
wayland_code.commands = wayland-scanner private-code ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
wayland_code.variable_out = SOURCES
wayland_code.depends = ${QMAKE_FILE_BASE}-client-protocol.h

QMAKE_EXTRA_COMPILERS += wayland_header wayland_code
INCLUDEPATH += $$OUT_PWD
