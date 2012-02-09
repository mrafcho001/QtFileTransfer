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
    filelistitemmodel.cpp

HEADERS  += mainwindow.h \
    filelistitemmodel.h

FORMS    += mainwindow.ui
