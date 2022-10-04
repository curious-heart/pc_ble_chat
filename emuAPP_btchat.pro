TEMPLATE = app
TARGET = btchat

QT = core bluetooth widgets
requires(qtConfig(listwidget))
android: QT += androidextras

SOURCES = \
    characteristicinfo.cpp \
    diy_common_tool.cpp \
    main.cpp \
    chat.cpp \
    remoteselector.cpp \
    chatserver.cpp \
    chatclient.cpp \
    serviceinfo.cpp

HEADERS = \
    characteristicinfo.h \
    chat.h \
    diy_common_tool.h \
    remoteselector.h \
    chatserver.h \
    chatclient.h \
    serviceinfo.h

FORMS = \
    chat.ui \
    remoteselector.ui

win32-msvc*: {
    QMAKE_CFLAGS *= /utf-8
    QMAKE_CXXFLAGS *= /utf-8
}

target.path = $$[QT_INSTALL_EXAMPLES]/bluetooth/btchat
INSTALLS += target
