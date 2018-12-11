# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

QT += core network xml widgets gui

TARGET = AchainWallet
TEMPLATE = app

DEFINES += QT_WIDGETS_LIB QT_XML_LIB QT_NETWORK_LIB

LIBS += /usr/local/lib/libqrencode.a
ICON = achain.icns

UI_DIR += ./GeneratedFiles
include(AchainWallet.pri)


QMAKE_LFLAGS    += -framework CoreGraphics


#TEMPLATE = app
#TARGET = AchainWallet
#DESTDIR = ./release
#QT += core network xml widgets gui
#CONFIG += release
#DEFINES += _WINDOWS QT_WIDGETS_LIB QT_XML_LIB QT_NETWORK_LIB
#INCLUDEPATH += ./release \
#    . \
#    $(QTDIR)/mkspecs/win32-msvc2013 \
#    ./GeneratedFiles
#LIBS += -lshell32 \
#    -lDbgHelp \
#    -lUser32.Lib \
#    -lD:/git/wallet/GOPWallet4.2.2(64bit)/qrencode \
#    -limm32
#DEPENDPATH += .
#MOC_DIR += release
#OBJECTS_DIR += release
#UI_DIR += ./GeneratedFiles
#RCC_DIR += ./GeneratedFiles
#include(AchainWallet.pri)
#TRANSLATIONS += gop_English.ts \
#    gop_simplified_Chinese.ts
