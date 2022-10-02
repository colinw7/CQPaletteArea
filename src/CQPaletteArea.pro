TEMPLATE = lib

TARGET = CQPaletteArea

DEPENDPATH += .

MOC_DIR = .moc

QT += widgets

CONFIG += staticlib

QMAKE_CXXFLAGS += \
-std=c++17

# Input
HEADERS += \
../include/CQDockArea.h \
../include/CQPaletteArea.h \
../include/CQPaletteGroup.h \
../include/CQPalettePreview.h \
../include/CQRubberBand.h \
../include/CQTabBar.h \
../include/CQWidgetResizer.h \
../include/CQSplitterArea.h \

SOURCES += \
CQDockArea.cpp \
CQPaletteArea.cpp \
CQPaletteGroup.cpp \
CQPalettePreview.cpp \
CQRubberBand.cpp \
CQSplitterArea.cpp \
CQTabBar.cpp \
CQWidgetResizer.cpp \

OBJECTS_DIR = ../obj

DESTDIR = ../lib

INCLUDEPATH += \
. \
../include \
../../CQUtil/include \
../../CQTitleBar/include \
