LIBS += -L$$PWD -lqnapi
INCLUDEPATH += $$PWD/src \
    $$PWD/../deps/qt-maybe
DEPENDPATH += $$PWD/src
PRE_TARGETDEPS += $$PWD/libqnapi.a

