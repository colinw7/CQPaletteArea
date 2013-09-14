TEMPLATE = lib

TARGET = 

DEPENDPATH += .

CONFIG += staticlib

# Input
HEADERS += \
../include/CQPaletteArea.h \
../include/CQDockArea.h \
../include/CQPalettePreview.h \
../include/CQPaletteGroup.h \
../include/CQRubberBand.h \
../include/CQTabBar.h \
../include/CQWidgetResizer.h \

SOURCES += \
CQPaletteArea.cpp \
CQDockArea.cpp \
CQPalettePreview.cpp \
CQPaletteGroup.cpp \
CQRubberBand.cpp \
CQTabBar.cpp \
CQWidgetResizer.cpp \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += \
. \
../include \
../../CQUtil/include \
../../CQTitleBar/include \
