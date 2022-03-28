QT += core
QT += serialport

QT -= gui
QT -= widgets

CONFIG += c++17
CONFIG += console

CONFIG -= app_bundle

TARGET = eeprom-programmer
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# Disable debug messages for release builds
# evaluate only when "release" is defined of the two options "debug" and "release"
CONFIG(release, debug|release): {
    DEFINES += QT_NO_DEBUG_OUTPUT NDEBUG
}

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


VPATH += ./src

SOURCES += \
	app_.cpp \
	main.cpp \
	app.cpp \
	crc16.cpp \
	eeprom.cpp \
	serialportreader.cpp \
	serialportwriter.cpp \
        memorycomm.cpp

HEADERS += \
	app.h \
	crc16.h \
	eeprom.h \
	serialportreader.h \
	serialportwriter.h \
        memorycomm.h

unix {
    SOURCES += sigwatch.cpp
    HEADERS += sigwatch.h
    CONFIG(release, debug|release): \
        CONFIG += staticlib
}

win32 {
    SOURCES += signalhandler.cpp
    HEADERS += signalhandler.h
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


