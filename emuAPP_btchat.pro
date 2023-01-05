TEMPLATE = app
#TARGET = btchat
TARGET = skin_data_collector

QT = core bluetooth widgets sql xml charts
greaterThan(QT_MAJOR_VERSION, 5){
    QT += core5compat
}
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
    sqldb_remote_worker.cpp \
    sqldb_works.cpp \
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
    sqldb_remote_worker.h \
    sqldb_works.h \
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
VERSION = 4.3
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
DEFINES += QT_MJ_V_STR=\\\"$$QT_MAJOR_VERSION\\\"

target.path = $$[QT_INSTALL_EXAMPLES]/bluetooth/btchat
INSTALLS += target
