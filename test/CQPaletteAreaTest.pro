TEMPLATE = app

TARGET = CQPaletteAreaTest

DEPENDPATH += .

QT += widgets

#CONFIG += debug

# Input
SOURCES += \
CQPaletteAreaTest.cpp \

HEADERS += \
CQPaletteAreaTest.h \

DESTDIR     = .
OBJECTS_DIR = .

INCLUDEPATH += \
../include \
../../CQTitleBar/include \
.

PRE_TARGETDEPS = \
../lib/libCQPaletteArea.a \

unix:LIBS += \
-L../lib \
-lCQPaletteArea
