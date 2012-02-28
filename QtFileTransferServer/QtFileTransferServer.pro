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
    dirtreemodel.cpp \
    ../uibundle.cpp

HEADERS  += mainwindow.h \
    serverobject.h \
    mytcpserver.h \
    dirtreemodel.h

FORMS    += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    resources/restart.png \
    resources/stop.png
