QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutwindow.cpp \
    achievementwindow.cpp \
    addlogwindow.cpp \
    annoucewindow.cpp \
    chatexchangewindow.cpp \
    dbmanager.cpp \
    goalrecordwindow.cpp \
    homewindow.cpp \
    loginwindow.cpp \
    logrecordwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    messageadminwindow.cpp \
    messageuserwindow.cpp \
    moodcalendarwindow.cpp \
    registerwindow.cpp \
    replywindow.cpp

HEADERS += \
    aboutwindow.h \
    achievementwindow.h \
    addlogwindow.h \
    annoucewindow.h \
    chatclient.h \
    chatexchangewindow.h \
    chatserver.h \
    dbmanager.h \
    goalrecordwindow.h \
    homewindow.h \
    loginwindow.h \
    logrecordwindow.h \
    mainwindow.h \
    messageadminwindow.h \
    messageuserwindow.h \
    moodcalendarwindow.h \
    registerwindow.h \
    replywindow.h

FORMS += \
    about_window.ui \
    achievement_window.ui \
    add_log_window.ui \
    annouce_window.ui \
    chat_exchange_window.ui \
    goal_record_window.ui \
    home_window.ui \
    login_window.ui \
    logrecord_window.ui \
    main_window.ui \
    mainwindow.ui \
    message_info_user_window.ui \
    message_info_window.ui \
    mood_calendar_window.ui \
    register_window.ui \
    reply_window.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res1.qrc \

