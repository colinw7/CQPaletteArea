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

unix:LIBS += \
-L../lib -L../../CQTitleBar/lib \
-lCQPaletteArea -lCQTitleBar
