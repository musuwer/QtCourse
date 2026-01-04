QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dbmanager.cpp \
    homewindow.cpp \
    loginwindow.cpp \
    logrecordwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    registerwindow.cpp

HEADERS += \
    dbmanager.h \
    homewindow.h \
    loginwindow.h \
    logrecordwindow.h \
    mainwindow.h \
    registerwindow.h

FORMS += \
    login_window.ui \
    main_window.ui \
    mainwindow.ui \
    register_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res1.qrc \

