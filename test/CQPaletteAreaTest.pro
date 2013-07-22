TEMPLATE = app

TARGET = 

DEPENDPATH += .

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
../../CQTitleBar/lib/libCQTitleBar.a

unix:LIBS += \
-L../lib -L../../CQTitleBar/lib \
-lCQPaletteArea -lCQTitleBar
