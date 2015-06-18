#-------------------------------------------------
#
# Project created by QtCreator 2015-06-16T08:39:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tcpserver.cpp

HEADERS  += mainwindow.h \
    tcpserver.h \
    datastruct.h

FORMS    += mainwindow.ui
