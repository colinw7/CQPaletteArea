TEMPLATE = lib

TARGET = 

DEPENDPATH += .

CONFIG += staticlib

# Input
HEADERS += \
../include/CQPaletteArea.h \
../include/CQDockArea.h \
../include/CQPaletteGroup.h \

SOURCES += \
CQPaletteArea.cpp \
CQPaletteGroup.cpp \
CQDockArea.cpp \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += . ../include ../../CQTitleBar/include
