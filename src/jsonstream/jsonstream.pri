INCLUDEPATH += $$PWD

HEADERS += $$PWD/jsonstream.h
SOURCES += $$PWD/jsonstream.cpp
#LIBS_PRIVATE += -L$$shadowed($$PWD)/release -lQtJsonDbJsonStream
#POST_TARGETDEPS += $$shadowed($$PWD)/release/libQtJsonDbJsonStream.a
