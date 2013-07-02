TEMPLATE = lib

TARGET = 

DEPENDPATH += .

CONFIG += staticlib

# Input
HEADERS += \
CQPaletteArea.h \
CQPaletteGroup.h \
CQDockArea.h \

SOURCES += \
CQPaletteArea.cpp \
CQPaletteGroup.cpp \
CQDockArea.cpp \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += . ../../CQTitleBar/src
