#-------------------------------------------------
#
# Project created by QtCreator 2012-02-06T22:40:25
#
#-------------------------------------------------

QT       += core gui network

TARGET = QtFileTransferServer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serverobject.cpp \
    mytcpserver.cpp \
    ../fileinfo.cpp \
    dirtreemodel.cpp

HEADERS  += mainwindow.h \
    serverobject.h \
    mytcpserver.h \
    dirtreemodel.h

FORMS    += \
    mainwindow.ui
