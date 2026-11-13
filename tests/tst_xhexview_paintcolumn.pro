QT += core gui widgets testlib concurrent network printsupport sql svg opengl openglwidgets qml xml uitools
CONFIG += console testcase c++17
CONFIG -= app_bundle
DEFINES += NOMINMAX _CRT_SECURE_NO_WARNINGS
XCONFIG += no_xemulator
win32: LIBS += advapi32.lib

TEMPLATE = app
TARGET = tst_xhexview_paintcolumn
SOURCES += tst_xhexview_paintcolumn.cpp

include(../xhexview.pri)
