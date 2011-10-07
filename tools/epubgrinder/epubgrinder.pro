#-------------------------------------------------
#
# Project created by QtCreator 2011-06-08T13:23:53
#
#-------------------------------------------------

QT       += core gui xml

TARGET = epubgrinder
TEMPLATE = app

INCLUDEPATH += ../../src/hardware \
../../src/platform \
zlib

SOURCES += main.cpp\
    mainwindow.cpp \
    epubgrinder.cpp \
    zipfile.cpp \
    applyXml.cpp \
    qtsim.cpp \
    simwidget.cpp \
    blobfile.cpp \
    builder.cpp \
    layout.cpp \
    fonteditor.cpp \
    zlib/zutil.c \
    zlib/inflate.c \
    zlib/inftrees.c \
    zlib/inffast.c \
    ../../src/platform/Shell.cpp \
    ../../src/platform/Scroller.cpp \
    ../../src/platform/Platform.cpp \
    ../../src/platform/Graphics.cpp \
    ../../src/platform/File.cpp \
    ../../src/platform/Fat32.cpp \
    ../../src/apps/3D/math3D.cpp \
    ../../src/apps/3D/3DApp.cpp \
    ../../src/apps/core/ViewApp.cpp \
    ../../src/apps/core/ShellApp.cpp \
    ../../src/apps/demos/MinesApp.cpp \
    ../../src/apps/demos/LatticeApp.cpp \
    ../../src/apps/demos/FlipApp.cpp \
    ../../src/apps/demos/Doomed.cpp \
    ../../src/apps/ebook/ebookApp.cpp \
    ../../src/apps/hardware/PaintApp.cpp \
    ../../src/apps/hardware/BatteryApp.cpp \
    ../../src/apps/hardware/AccelerateApp.cpp \
    ../../src/apps/pacman/PacmanApp.cpp \
    ../../src/apps/wiki/WikiApp.cpp

HEADERS  += mainwindow.h \
    epubgrinder.h \
    zipfile.h \
    simwidget.h \
    blobfile.h \
    common.h \
    builder.h \
    layout.h \
    Tokens.h \
    fonteditor.h \
    zlib/zlib.h \
    zlib/inflate.h \
    zlib/inftrees.h \
    zlib/inftrees.h \
    zlib/inflate.h \
    zlib/inffast.h \
    ../../src/platform/Scroller.h \
    ../../src/platform/Platform.h \
    ../../src/platform/LCD.h \
    ../../src/platform/Graphics.h \
    ../../src/platform/File.h \
    ../../src/platform/Fat32.h \
    ../../src/apps/3D/math3D.h \
    ../../src/apps/3D/3DModels.h \
    ../../src/apps/ebook/Sans8.h \
    ../../src/apps/frotz/Verdana6.h \
    ../../src/apps/frotz/Sans8.h \
    ../../src/apps/frotz/FrotzFonts.h \
    ../../src/apps/frotz/dumb-frotz-2.32r1/frotz.h \
    ../../src/apps/pacman/PacmanAppTiles.h \
    ../../src/apps/wiki/WikiFonts.h

FORMS    += mainwindow.ui \
    fontdialog.ui \
    fonteditor.ui

DEFINES += _QT

RESOURCES += \
    res.qrc
