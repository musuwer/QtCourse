QT       += core gui widgets sql network
QT += widgets sql charts

CONFIG += c++17

SOURCES += \
    main.cpp \
    dbmanager.cpp \
    loginwindow.cpp \
    registerwindow.cpp \
    mainwindow.cpp \
    logrecordwindow.cpp \
    homewindow.cpp \
    achievementwindow.cpp \
    moodcalendarwindow.cpp \
    messageuserwindow.cpp \
    messageadminwindow.cpp \
    aboutwindow.cpp \
    addlogwindow.cpp \
    goalrecordwindow.cpp \
    annoucewindow.cpp \
    replywindow.cpp \
    chatclient.cpp \
    serverworker.cpp \
    chatserver.cpp \
    chatexchangewindow.cpp

HEADERS += \
    dbmanager.h \
    loginwindow.h \
    registerwindow.h \
    mainwindow.h \
    logrecordwindow.h \
    homewindow.h \
    achievementwindow.h \
    moodcalendarwindow.h \
    messageuserwindow.h \
    messageadminwindow.h \
    aboutwindow.h \
    addlogwindow.h \
    goalrecordwindow.h \
    annoucewindow.h \
    replywindow.h \
    chatclient.h \
    serverworker.h \
    chatserver.h \
    chatexchangewindow.h

FORMS += \
    main_window.ui \
    login_window.ui \
    register_window.ui \
    logrecord_window.ui \
    home_window.ui \
    achievement_window.ui \
    mood_calendar_window.ui \
    message_info_user_window.ui \
    message_info_window.ui \
    about_window.ui \
    add_log_window.ui \
    goal_record_window.ui \
    annouce_window.ui \
    reply_window.ui \
    chat_exchange_window.ui

RESOURCES += \
    res1.qrc

# Frameless resize uses Win32 hit-test helpers (GetWindowRect / HT*). Link user32 on Windows.
win32:LIBS += -luser32
