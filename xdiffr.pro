#-------------------------------------------------
#
# Project created by QtCreator 2013-08-16T23:32:17
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = xdiffr
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    diffdoc.cpp \
    qdifftextedit.cpp \
    foldersdlg.cpp \
    aboutdlg.cpp \
    settingsdlg.cpp

HEADERS  += mainwindow.h \
    diffdoc.h \
    qdifftextedit.h \
    foldersdlg.h \
    aboutdlg.h \
    settingsdlg.h

FORMS    += mainwindow.ui \
    aboutdlg.ui

RESOURCES += \
    xdiffr.qrc

RC_FILE = xdiffr.rc

#CONFIG += static
