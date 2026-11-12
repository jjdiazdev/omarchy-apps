QT += core network testlib
QT -= gui

CONFIG += c++17 testcase
TARGET = numouse_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/controller.h \
    ../src/pointer.h \
    ../src/server.h

SOURCES += \
    numouse_tests.cpp \
    ../src/controller.cpp \
    ../src/server.cpp
