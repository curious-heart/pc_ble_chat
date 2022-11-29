TEMPLATE = app
#TARGET = btchat
TARGET = skin_data_collector

QT = core bluetooth widgets sql xml
requires(qtConfig(listwidget))
android: QT += androidextras

SOURCES = \
    ble_comm_pkt.cpp \
    characteristicinfo.cpp \
    diy_common_tool.cpp \
    logger.cpp \
    main.cpp \
    chat.cpp \
    remoteselector.cpp \
    serviceinfo.cpp \
    sw_setting_parse.cpp \
    types_and_defs.cpp

HEADERS = \
    ble_comm_pkt.h \
    characteristicinfo.h \
    chat.h \
    diy_common_tool.h \
    logger.h \
    remoteselector.h \
    serviceinfo.h \
    sw_setting_parse.h \
    types_and_defs.h

FORMS = \
    chat.ui \
    remoteselector.ui

win32-msvc*: {
    QMAKE_CFLAGS *= /utf-8
    QMAKE_CXXFLAGS *= /utf-8
}
#QMAKE_CXXFLAGS += -P
VERSION = 3.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

target.path = $$[QT_INSTALL_EXAMPLES]/bluetooth/btchat
INSTALLS += target
