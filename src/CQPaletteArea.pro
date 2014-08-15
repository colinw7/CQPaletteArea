TEMPLATE = lib

TARGET = CQPaletteArea

DEPENDPATH += .

QT += widgets

CONFIG += staticlib

# Input
HEADERS += \
../include/CQDockArea.h \
../include/CQPaletteArea.h \
../include/CQPaletteGroup.h \
../include/CQPalettePreview.h \
../include/CQRubberBand.h \
../include/CQTabBar.h \
../include/CQWidgetResizer.h \
CQSplitterArea.h \
CQTitleBar.h \

SOURCES += \
CQPaletteArea.cpp \
CQDockArea.cpp \
CQPalettePreview.cpp \
CQPaletteGroup.cpp \
CQRubberBand.cpp \
CQSplitterArea.cpp \
CQTabBar.cpp \
CQTitleBar.cpp \
CQWidgetResizer.cpp \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += \
. \
../include \
../../CQUtil/include \
../../CQTitleBar/include \
