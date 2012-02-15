#-------------------------------------------------
#
# Project created by QtCreator 2012-02-06T22:39:59
#
#-------------------------------------------------

QT       += core gui network

TARGET = QtFileTransferClient
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ../fileinfo.cpp \
    filelistitemmodel.cpp \
    downloadclient.cpp

HEADERS  += mainwindow.h \
    filelistitemmodel.h \
    downloadclient.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    stop.png \
    restart.png
