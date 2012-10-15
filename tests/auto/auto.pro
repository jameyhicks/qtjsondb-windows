TEMPLATE = subdirs
SUBDIRS = \
    client \
    cmake \
    partition \
    accesscontrol \
    jsondblistmodel \
    jsondbsortinglistmodel \
    jsondbcachinglistmodel \
    jsondbpartition \
    jsondbnotification \
    jsondbqueryobject \
    jsondbscriptengine \
    queries \
    qjsondbrequest \
    qjsondbwatcher \
    qjsondbflushrequest \
    qjsondbterminaterequest \
    jsonstream \
    hbtree \
    headersclean \
    storage

testcocoon: SUBDIRS -= headersclean
