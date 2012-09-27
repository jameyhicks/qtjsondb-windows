INCLUDEPATH += $$PWD
LIBS_PRIVATE += -L$$shadowed($$PWD)/release -lQtJsonDbJsonStream
POST_TARGETDEPS += $$shadowed($$PWD)/release/libQtJsonDbJsonStream.a
