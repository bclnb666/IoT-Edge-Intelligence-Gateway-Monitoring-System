QT += widgets network mqtt charts sql gui core

CONFIG += c++17

# Use the compiler bundled with Qt (GCC 13.1.0), not the old GCC 8.1.0 in PATH
QMAKE_CC = D:/Qt/Tools/mingw1310_64/bin/gcc
QMAKE_CXX = D:/Qt/Tools/mingw1310_64/bin/g++

# Qt6EntryPoint needs __stack_chk_* symbols; GCC 13 dropped libssp,
# so pull libssp from the old MinGW 8.1 (it's just a static stub)
QMAKE_LIBDIR += D:/MinGW/mingw64/lib/gcc/x86_64-w64-mingw32/8.1.0
QMAKE_LIBS += -lssp -lssp_nonshared

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwidget.cpp \
    overviewpage.cpp \
    trendpage.cpp \
    alarmpage.cpp \
    settingspage.cpp \
    registerwidget.cpp \
    widget.cpp

HEADERS += \
    mainwidget.h \
    overviewpage.h \
    trendpage.h \
    alarmpage.h \
    settingspage.h \
    protocol.h \
    registerwidget.h \
    widget.h

FORMS += \
    mainwidget.ui \
    registerwidget.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    src.qrc
