LIBS += -L$$PWD -lqnapi
greaterThan(QT_MAJOR_VERSION, 5): QT += core5compat
INCLUDEPATH += $$PWD/src \
    $$PWD/../deps/qt-maybe
DEPENDPATH += $$PWD/src
PRE_TARGETDEPS += $$PWD/libqnapi.a

