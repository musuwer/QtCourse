QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutwindow.cpp \
    achievementwindow.cpp \
    annoucewindow.cpp \
    dbmanager.cpp \
    goalrecordwindow.cpp \
    homewindow.cpp \
    loginwindow.cpp \
    logrecordwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    registerwindow.cpp

HEADERS += \
    aboutwindow.h \
    achievementwindow.h \
    annoucewindow.h \
    dbmanager.h \
    goalrecordwindow.h \
    homewindow.h \
    loginwindow.h \
    logrecordwindow.h \
    mainwindow.h \
    registerwindow.h

FORMS += \
    about_window.ui \
    achievement_window.ui \
    annouce_window.ui \
    goal_record_window.ui \
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

