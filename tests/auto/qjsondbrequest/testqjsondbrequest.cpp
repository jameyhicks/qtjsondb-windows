/****************************************************************************
 **
 ** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtAddOn.JsonDb module of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** Commercial License Usage
 ** Licensees holding valid commercial Qt licenses may use this file in
 ** accordance with the commercial license agreement provided with the
 ** Software or, alternatively, in accordance with the terms contained in
 ** a written agreement between you and Digia.  For licensing terms and
 ** conditions see http://qt.digia.com/licensing.  For further information
 ** use the contact form at http://qt.digia.com/contact-us.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Digia gives you certain additional
 ** rights.  These rights are described in the Digia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** GNU General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU
 ** General Public License version 3.0 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU General Public License version 3.0 requirements will be
 ** met: http://www.gnu.org/copyleft/gpl.html.
 **
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

#include "qjsondbconnection.h"
#include "qjsondbobject.h"
#include "qjsondbreadrequest.h"
#include "qjsondbwriterequest.h"
#include "private/qjsondbstandardpaths_p.h"
#include "private/qjsondbflushrequest_p.h"
#include "private/qjsondbreloadrequest_p.h"
#include "testhelper.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTest>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QTemporaryDir>
#include <QThread>
#include <QSignalSpy>

#include <pwd.h>
#include <signal.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

QT_USE_NAMESPACE_JSONDB

inline static const QString typeStr() { return QStringLiteral("_type"); }
inline static const QString uuidStr() { return QStringLiteral("_uuid"); }
inline static const QString versionStr() { return QStringLiteral("_version"); }

/*!
    The TestQJsonDbRequest class tests the different QJsonDbRequest objects.
    Currently QJsonDbReadRequest, QJsonDbCreateRequest, QJsonDbUpdateRequest,
    QJsonDbRemoveRequest.
*/
class TestQJsonDbRequest: public TestHelper
{
    Q_OBJECT

public slots:
    void writeAndRemove();
    void severalWrites();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void modifyPartitions();
    void closeIndexes();
    void removablePartition();
    void readRequest_data();
    void readRequest();
    void writeRequest_data();
    void writeRequest();
    void createRequest_data();
    void createRequest();
    void updateRequest_data();
    void updateRequest();
    void privatePartition();
    void privatePartition2();
    void privatePartitions();
    void invalidPrivatePartition();
    void removeRequest();
    void forced();
    void readObjectRequest();
    void bindings();
    void replaceFromNull();
    void multiplerequests();
    void defaultConnection();
    void privatePartitionFlushRequest();
    void multipleThreads_data();
    void multipleThreads();
    void defaultPartition();

private:
    bool writeTestObject(QObject* parent, const QString &type, int value, const QString &partition = QString());
};

void TestQJsonDbRequest::initTestCase()
{
    QJsonDbStandardPaths::setAutotestMode(true);
    removeDbFiles();

    QStringList arg_list = QStringList() << "-validate-schemas";
    launchJsonDbDaemon(arg_list, __FILE__);
}

void TestQJsonDbRequest::cleanupTestCase()
{
    removeDbFiles();

    QFile partitionsFile(QFileInfo(QFINDTESTDATA("partitions.json")).absoluteDir().absoluteFilePath(QLatin1String("partitions-test.json")));
    partitionsFile.remove();

    stopDaemon();
}

void TestQJsonDbRequest::init()
{
    clearHelperData();
    connectToServer();
}

void TestQJsonDbRequest::cleanup()
{
    disconnectFromServer();
}

void TestQJsonDbRequest::modifyPartitions()
{
    // create a notification on Partitions
    QJsonDbWatcher watcher;
    watcher.setPartition(QLatin1String("Ephemeral"));
    watcher.setQuery("[?_type=\"Partition\"]");
    mConnection->addWatcher(&watcher);

    // ensure that there's only one partition defined and that it's the default
    QLatin1String defaultPartition("com.qt-project.shared");

    QJsonDbReadRequest partitionQuery;
    partitionQuery.setPartition(QLatin1String("Ephemeral"));
    partitionQuery.setQuery(QLatin1String("[?_type=%type]"));
    partitionQuery.bindValue(QLatin1String("type"), QLatin1String("Partition"));

    mConnection->send(&partitionQuery);
    QVERIFY(waitForResponse(&partitionQuery));

    QList<QJsonObject> results = partitionQuery.takeResults();
    QCOMPARE(results.count(), 1);
    QCOMPARE(results[0].value(QLatin1String("name")).toString(), defaultPartition);
    QVERIFY(results[0].value(QLatin1String("default")).toBool());

    // write a new partitions file
    QJsonObject def1;
    def1.insert(QLatin1String("name"), QLatin1String("com.qt-project.test1"));
    def1.insert(QLatin1String("path"), QLatin1String("."));
    QJsonObject def2;
    def2.insert(QLatin1String("name"), QLatin1String("com.qt-project.test2"));
    def2.insert(QLatin1String("path"), QLatin1String("."));
    QJsonArray defs;
    defs.append(def1);
    defs.append(def2);

    // ensure that the new file is written to the same directory as the main partitions.json
    QFile partitionsFile(QFileInfo(QFINDTESTDATA("partitions.json")).absoluteDir().absoluteFilePath(QLatin1String("partitions-test.json")));
    partitionsFile.open(QFile::WriteOnly);
    partitionsFile.write(QJsonDocument(defs).toJson());
    partitionsFile.close();

#ifndef Q_OS_WIN32
    // test SIGHUP behavior here, but use the reload command elsewhere to reduce clutter
    // send the daemon a SIGHUP to get it to reload the partitions
    kill(mProcess->pid(), SIGHUP);
    QVERIFY(waitForResponseAndNotifications(0, &watcher, 2));
#else
    QJsonDbReloadRequest reload;
    mConnection->send(&reload);
    QVERIFY(waitForResponseAndNotifications(&reload, &watcher, 2));
#endif

    // query for the new partitions
    mConnection->send(&partitionQuery);
    QVERIFY(waitForResponse(&partitionQuery));

    results = partitionQuery.takeResults();
    QCOMPARE(results.count(), 3);

    // operate on the new partition to make sure it works
    QJsonDbWriteRequest writeRequest;
    QUuid testUuid = QJsonDbObject::createUuidFromString(QLatin1String("testobject1"));
    QJsonDbObject toWrite;
    toWrite.setUuid(testUuid);
    toWrite.insert(QLatin1String("_type"), QLatin1String("TestObject"));
    writeRequest.setObjects(QList<QJsonObject>() << toWrite);
    mConnection->send(&writeRequest);
    QVERIFY(waitForResponse(&writeRequest));
    QVERIFY(!mRequestErrors.contains(&writeRequest));

    QJsonDbReadObjectRequest readRequest(testUuid);
    mConnection->send(&readRequest);
    QVERIFY(waitForResponse(&readRequest));
    QVERIFY(!mRequestErrors.contains(&readRequest));
    results = readRequest.takeResults();
    QCOMPARE(results.count(), 1);
    QCOMPARE(results[0].value(QLatin1String("_type")).toString(), QLatin1String("TestObject"));

    // remove the new partitions file
    partitionsFile.remove();

    // send the daemon a reload request to get it to unload the partitions
    QJsonDbReloadRequest reload;
    mConnection->send(&reload);
    QVERIFY(waitForResponseAndNotifications(&reload, &watcher, 2));

    // verify that we're back to just the origin partition
    mConnection->send(&partitionQuery);
    QVERIFY(waitForResponse(&partitionQuery));

    results = partitionQuery.takeResults();
    QCOMPARE(results.count(), 1);
    QCOMPARE(results[0].value(QLatin1String("name")).toString(), defaultPartition);
    QVERIFY(results[0].value(QLatin1String("default")).toBool());

    // query one of the test partitions to ensure we get an InvalidPartition error
    QJsonDbReadRequest failingRequest;
    failingRequest.setPartition(QLatin1String("com.qt-project.test1"));
    failingRequest.setQuery(QLatin1String("[*]"));
    mConnection->send(&failingRequest);
    QVERIFY(waitForResponse(&failingRequest));

    QVERIFY(mRequestErrors.contains(&failingRequest));
    QCOMPARE(mRequestErrors[&failingRequest], QJsonDbRequest::InvalidPartition);

    mConnection->removeWatcher(&watcher);
    waitForStatus(&watcher, QJsonDbWatcher::Inactive);
}

void TestQJsonDbRequest::closeIndexes()
{
    {
        QList<QJsonObject> objects;
        QJsonDbObject object;
        object.setUuid(QJsonDbObject::createUuid());
        object.insert(QLatin1String("_type"), QLatin1String("duck"));
        object.insert(QLatin1String("foobar"), QLatin1String("bazinga"));
        objects.append(object);
        QJsonDbWriteRequest writeRequest;
        writeRequest.setObjects(objects);
        mConnection->send(&writeRequest);
        waitForResponse(&writeRequest);
        QVERIFY(!mRequestErrors.contains(&writeRequest));
    }

    {
        // create 3 indexes
        QList<QJsonObject> indexes;
        for (int i = 0; i < 3; ++i) {
            QJsonDbObject index;
            index.setUuid(QJsonDbObject::createUuid());
            index.insert(QLatin1String("_type"), QLatin1String("Index"));
            index.insert(QLatin1String("name"), QString(QLatin1String("closeIndexesTest%1")).arg(i));
            index.insert(QLatin1String("propertyName"), QLatin1String("foobar"));
            index.insert(QLatin1String("propertyType"), QLatin1String("string"));
            indexes.append(index);
        }
        QJsonDbWriteRequest writeRequest;
        writeRequest.setObjects(indexes);
        mConnection->send(&writeRequest);
        waitForResponse(&writeRequest);
        QVERIFY(!mRequestErrors.contains(&writeRequest));
    }

    // query on one of the indexes
    {
        QJsonDbReadRequest readRequest;
        readRequest.setQuery(QLatin1String("[?_type=\"duck\"][/closeIndexesTest2]"));
        mConnection->send(&readRequest);
        waitForResponse(&readRequest);
        QList<QJsonObject> results = readRequest.takeResults();
        QCOMPARE(results.count(), 1);
    }

    // close indexes
    kill(mProcess->pid(), SIGUSR1);

    // query again
    {
        QJsonDbReadRequest readRequest;
        readRequest.setQuery(QLatin1String("[?_type=\"duck\"][/closeIndexesTest2]"));
        mConnection->send(&readRequest);
        waitForResponse(&readRequest);
        QList<QJsonObject> results = readRequest.takeResults();
        QCOMPARE(results.count(), 1);
    }
}

void TestQJsonDbRequest::removablePartition()
{
#ifdef Q_OS_LINUX
    if (geteuid())
        QSKIP("This test only works as root");

    if (!(QFile::exists(QLatin1String("/sbin/mkfs.ext2")) || QFile::exists(QLatin1String("/sbin/mkfs"))))
        QSKIP("This test requires mkfs.ext2 or mkfs");

    // create a notification on Partitions
    QJsonDbWatcher watcher;
    watcher.setPartition(QLatin1String("Ephemeral"));
    watcher.setQuery("[?_type=\"Partition\"]");
    mConnection->addWatcher(&watcher);
    waitForStatus(&watcher, QJsonDbWatcher::Active);

    QJsonObject def;
    def.insert(QStringLiteral("name"), QStringLiteral("removableTest"));
    def.insert(QStringLiteral("path"), QStringLiteral("/mnt/removablePartition"));
    def.insert(QStringLiteral("removable"), true);

    QJsonArray defs;
    defs.append(def);

    // ensure that the new file is written to the same directory as the main partitions.json
    QFile partitionsFile(QFileInfo(QFINDTESTDATA("partitions.json")).absoluteDir().absoluteFilePath(QLatin1String("partitions-test.json")));
    partitionsFile.open(QFile::WriteOnly);
    partitionsFile.write(QJsonDocument(defs).toJson());
    partitionsFile.close();

    QDir mnt(def.value(QStringLiteral("path")).toString());

    // unmount just in case previous run failed
    system(QString::fromLatin1("umount -l %1 > /dev/null 2>&1").arg(mnt.absolutePath()).toLatin1());

    // send the daemon a reload request to get it to unload the partitions
    QJsonDbReloadRequest reload;
    mConnection->send(&reload);
    QVERIFY(waitForResponseAndNotifications(&reload, &watcher, 1));
    QList<QJsonDbNotification> notifications = watcher.takeNotifications();
    QCOMPARE(notifications.count(), 1);

    QJsonObject newPartition = notifications.at(0).object();
    QCOMPARE(newPartition.value(QStringLiteral("name")).toString(),
             def.value(QStringLiteral("name")).toString());
    QCOMPARE(newPartition.value(QStringLiteral("path")).toString(),
             def.value(QStringLiteral("path")).toString());
    QVERIFY(!newPartition.value(QStringLiteral("available")).toBool());

    // add a watcher on the partition
    QJsonDbWatcher watcher2;
    watcher2.setPartition(def.value(QStringLiteral("name")).toString());
    watcher2.setQuery(QStringLiteral("[?_type=\"removable-test\"]"));
    mConnection->addWatcher(&watcher2);
    waitForStatus(&watcher2, QJsonDbWatcher::Active);

    // make sure the partition doesn't work
    QJsonObject o;
    o.insert(QStringLiteral("_type"), QStringLiteral("removable-test"));
    QJsonDbCreateRequest write(o);

    write.setPartition(def.value(QStringLiteral("name")).toString());
    mConnection->send(&write);
    waitForResponse(&write);
    QVERIFY(mRequestErrors.contains(&write));
    QCOMPARE(mRequestErrors[&write], QJsonDbRequest::PartitionUnavailable);
    mRequestErrors.clear();

    // dd a file to use with our loopback device
    QString tmpFile = QStringLiteral("/tmp/removablePartition.img");
    QVERIFY(system(QString::fromLatin1("dd if=/dev/zero of=%1 bs=1024 count=512 > /dev/null 2>&1").arg(tmpFile).toLatin1()) == 0);
    if (QFile::exists(QLatin1String("/sbin/mkfs")))
        QVERIFY(system(QString::fromLatin1("mkfs -F -T ext2 %1 > /dev/null 2>&1").arg(tmpFile).toLatin1()) == 0);
    else
        QVERIFY(system(QString::fromLatin1("mkfs.ext2 -F -T ext2 %1 > /dev/null 2>&1").arg(tmpFile).toLatin1()) == 0);

    // mount
    QVERIFY(mnt.mkpath(QStringLiteral(".")));
    QVERIFY(system(QString::fromLatin1("mount -o loop %1 %2 > /dev/null 2>&1").arg(tmpFile).arg(mnt.absolutePath()).toLatin1()) == 0);

    // send the daemon a reload request to get it to unload the partitions
    mConnection->send(&reload);
    QVERIFY(waitForResponseAndNotifications(&reload, &watcher, 1));
    notifications = watcher.takeNotifications();
    QCOMPARE(notifications.count(), 1);

    QJsonObject updatedPartition = notifications.at(0).object();
    QCOMPARE(updatedPartition.value(QStringLiteral("name")).toString(),
             def.value(QStringLiteral("name")).toString());
    QCOMPARE(updatedPartition.value(QStringLiteral("path")).toString(),
             def.value(QStringLiteral("path")).toString());
    QVERIFY(updatedPartition.value(QStringLiteral("available")).toBool());

    // make sure the partition works
    mConnection->send(&write);
    waitForResponseAndNotifications(&write, &watcher2, 1);
    QVERIFY(!mRequestErrors.contains(&write));
    notifications = watcher2.takeNotifications();
    QCOMPARE(notifications.size(), 1);

    // unmount
    QVERIFY(system(QString::fromLatin1("umount -l %1 > /dev/null 2>&1").arg(mnt.absolutePath()).toLatin1()) == 0);

    // remove the mount point
    QVERIFY(mnt.rmpath(QStringLiteral(".")));

    // send the daemon a reload request to get it to unload the partitions
    mConnection->send(&reload);
    QVERIFY(waitForResponseAndNotifications(&reload, &watcher, 1));
    notifications = watcher.takeNotifications();
    QCOMPARE(notifications.count(), 1);

    updatedPartition = notifications.at(0).object();
    QCOMPARE(updatedPartition.value(QStringLiteral("name")).toString(),
             def.value(QStringLiteral("name")).toString());
    QCOMPARE(updatedPartition.value(QStringLiteral("path")).toString(),
             def.value(QStringLiteral("path")).toString());
    QVERIFY(!updatedPartition.value(QStringLiteral("available")).toBool());

    // make sure the partition doesn't work
    mConnection->send(&write);
    waitForResponse(&write);
    QVERIFY(mRequestErrors.contains(&write));
    QCOMPARE(mRequestErrors[&write], QJsonDbRequest::PartitionUnavailable);

    // remove the partition definition and reload
    partitionsFile.remove();

    // send the daemon a reload request to get it to unload the partitions
    mConnection->send(&reload);
    QVERIFY(waitForResponseAndNotifications(&reload, &watcher, 1));

    mConnection->removeWatcher(&watcher);
    waitForStatus(&watcher, QJsonDbWatcher::Inactive);
    mConnection->removeWatcher(&watcher2);
    waitForStatus(&watcher2, QJsonDbWatcher::Inactive);

    // remove the tmp file
    QFile::remove(tmpFile);
#endif
}

/*!
    Write one object with the _type "test" and the value \a value into the database.
*/
bool TestQJsonDbRequest::writeTestObject(QObject* parent, const QString &type, int value, const QString &partition)
{
    // -- write one object
    QJsonDbObject obj;
    obj.insert(typeStr(), QJsonValue(type));
    obj.insert(QStringLiteral("val"), QJsonValue(value));

    QJsonDbCreateRequest *req1 = new QJsonDbCreateRequest(obj, parent);
    req1->setPartition(partition);
    mConnection->send(req1);
    return waitForResponse(req1);
}

/*!
    Simple test, just writing and reading from the database.
    This is the most basic test.
*/
void TestQJsonDbRequest::writeAndRemove()
{
    QVERIFY(mConnection);

    // -- write one object
    QObject parent;
    QVERIFY(writeTestObject(&parent, QStringLiteral("writeRemoveTest"), 0));

    // -- read one object
    QJsonDbReadRequest req2(QString("[?_type=\"writeRemoveTest\"]"));
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));
    QVERIFY(!mRequestErrors.contains(&req2));

    QList<QJsonObject> results = req2.takeResults();
    QCOMPARE(results.count(), 1);
    QJsonObject obj = results.takeFirst();
    QCOMPARE(obj.value(typeStr()).toString(), QStringLiteral("writeRemoveTest"));
    QCOMPARE(obj.value(QStringLiteral("val")), QJsonValue(0));
}

/*!
    Checking that queue handling with several writes at the same time works.
*/
void TestQJsonDbRequest::severalWrites()
{
    QVERIFY(mConnection);

    QObject parent;
    const int numRequests = 300;
    QList<QJsonDbRequest *> pendingRequests;

    // -- Create and send the write requests
    for (int i = 0; i < numRequests; i++) {
        QJsonObject obj;
        obj.insert(typeStr(), QJsonValue(QStringLiteral("test")));
        obj.insert(QStringLiteral("_uuid"), QJsonDbObject::createUuid().toString());
        obj.insert(QStringLiteral("val"), QJsonValue(QString::number(i)));

        QJsonDbCreateRequest *req1 = new QJsonDbCreateRequest(obj, &parent);
        mConnection->send(req1);
        pendingRequests << req1;
    }

    QVERIFY(waitForResponse(pendingRequests));
    QVERIFY(mRequestErrors.isEmpty());

    // -- read one object
    QJsonDbReadRequest req2(QString("[?_type=\"test\"]"));
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));
    QVERIFY(!mRequestErrors.contains(&req2));

    QList<QJsonObject> results = req2.takeResults();
    QCOMPARE(results.count(), numRequests);
    QJsonObject obj = results.takeFirst();
    QCOMPARE(obj.value(typeStr()).toString(), QStringLiteral("test"));
}

void TestQJsonDbRequest::readRequest_data()
{
    QTest::addColumn<QString>("partition");

    QTest::newRow("default") << "";
    QTest::newRow("private") << "Private";
}

/*!
    In depth checking of read request.
*/
void TestQJsonDbRequest::readRequest()
{
    QVERIFY(mConnection);
    QFETCH(QString, partition);

    QJsonDbReadRequest req2(QString("[?_type=\"hallo\"]"));

    // -- check query property
    QCOMPARE(req2.query(), QString("[?_type=\"hallo\"]"));
    req2.setQuery(QString("[?_type=\"%1\"]"));
    QCOMPARE(req2.query(), QString("[?_type=\"%1\"]"));

    // -- try to send a query with no query field
    req2.setQuery(QString());
    req2.setPartition(partition);
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));

    QCOMPARE(req2.status(), QJsonDbRequest::Error);
    QVERIFY(mRequestErrors.contains(&req2));
    QCOMPARE(mRequestErrors[&req2], QJsonDbRequest::MissingQuery);

    // -- check bind values
    QJsonDbReadRequest req3;
    req3.setQuery(QStringLiteral("[?_type=%1]"));

    QJsonValue value = req3.boundValue("1");
    QVERIFY(value.isUndefined());

    req3.bindValue("1", QJsonValue(QStringLiteral("readTest")));
    value = req3.boundValue("1");
    QVERIFY(value.isString());
    QCOMPARE(value.toString(), QStringLiteral("readTest"));

    req3.clearBoundValues();
    value = req3.boundValue("1");
    QVERIFY(value.isUndefined());

    // - rebind
    req3.bindValue("1", QJsonValue(QStringLiteral("readTest2")));
    value = req3.boundValue("1");
    QVERIFY(value.isString());
    QCOMPARE(value.toString(), QString("readTest2"));

    // -- check queryLimit
    QCOMPARE(req3.queryLimit(), -1);
    req3.setQueryLimit(42);
    QCOMPARE(req3.queryLimit(), 42);
    req3.setQueryLimit(2);
    QCOMPARE(req3.queryLimit(), 2);

    // -- check status
    QCOMPARE(req3.status(), QJsonDbRequest::Inactive);
    req3.setPartition(partition);
    mConnection->send(&req3);
    QVERIFY(req3.status() != QJsonDbRequest::Inactive);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));

    // -- no object returned
    QList<QJsonObject> results = req3.takeResults();
    QCOMPARE(results.count(), 0);

    // -- write one object and re-read
    QObject parent;
    QVERIFY(writeTestObject(&parent, QStringLiteral("readTest2"), 0, partition));
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));

    results = req3.takeResults();
    QCOMPARE(results.count(), 1);
    QJsonObject obj = results.takeFirst();
    QCOMPARE(obj.value(typeStr()).toString(), QStringLiteral("readTest2"));
    QCOMPARE(obj.value(QStringLiteral("val")), QJsonValue(0));

    // -- write another object and re-read
    QVERIFY(writeTestObject(&parent, QStringLiteral("readTest2"), 1, partition));
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));

    results = req3.takeResults();
    QCOMPARE(results.count(), 2);

    // -- check sortKey
    QCOMPARE(req3.sortKey(), typeStr());

    req3.setQuery(QStringLiteral("[?_type=%1][/_uuid]"));
    req3.bindValue(QStringLiteral("1"), QStringLiteral("readTest2"));
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));
    QCOMPARE(req3.sortKey(), uuidStr());
}

void TestQJsonDbRequest::writeRequest_data()
{
    QTest::addColumn<QString>("partition");

    QTest::newRow("default") << "";
    QTest::newRow("private") << "Private";
}

/*!
    In depth checking of write request.
*/
void TestQJsonDbRequest::writeRequest()
{
    QVERIFY(mConnection);
    QFETCH(QString, partition);

    QJsonObject obj;
    obj.insert(typeStr(), QJsonValue(QStringLiteral("writeTest")));
    obj.insert(QStringLiteral("val"), QJsonValue(0));
    obj.insert(QStringLiteral("_deleted"), true);

    QJsonDbWriteRequest req1;
    QCOMPARE(req1.status(), QJsonDbRequest::Inactive);

    // try to send an object without uuid and since it's a remove
    // it'll fail
    req1.setObjects(QList<QJsonObject>() << obj);
    req1.setPartition(partition);
    mConnection->send(&req1);
    QVERIFY(waitForResponse(&req1));

    QCOMPARE(req1.status(), QJsonDbRequest::Error);
    QVERIFY(mRequestErrors.contains(&req1));
    QCOMPARE(mRequestErrors[&req1], QJsonDbRequest::MissingUUID);
    mRequestErrors.clear();

    // try to send no objects
    req1.setObjects(QList<QJsonObject>());
    mConnection->send(&req1);
    QVERIFY(waitForResponse(&req1));

    QCOMPARE(req1.status(), QJsonDbRequest::Error);
    QVERIFY(mRequestErrors.contains(&req1));
    QCOMPARE(mRequestErrors[&req1], QJsonDbRequest::MissingObject);
    mRequestErrors.clear();

    // try to send an object without type
    obj.remove(typeStr());
    obj.remove(QStringLiteral("_deleted"));
    req1.setObjects(QList<QJsonObject>() << obj);
    mConnection->send(&req1);
    QVERIFY(waitForResponse(&req1));

    QCOMPARE(req1.status(), QJsonDbRequest::Error);
    QVERIFY(mRequestErrors.contains(&req1));
    QCOMPARE(mRequestErrors[&req1], QJsonDbRequest::MissingType);
    mRequestErrors.clear();
}

void TestQJsonDbRequest::createRequest_data()
{
    QTest::addColumn<QString>("partition");

    QTest::newRow("default") << "";
    QTest::newRow("private") << "Private";
}

void TestQJsonDbRequest::createRequest()
{
    QVERIFY(mConnection);
    QFETCH(QString, partition);

    // -- write one object
    QJsonObject obj;
    obj.insert(typeStr(), QJsonValue(QStringLiteral("createTest")));
    obj.insert(QStringLiteral("val"), QJsonValue(QStringLiteral("test2")));

    QJsonDbCreateRequest req1(obj);
    req1.setPartition(partition);
    mConnection->send(&req1);
    QVERIFY(waitForResponse(&req1));
    QVERIFY(!mRequestErrors.contains(&req1));

    // - check that we have results of the create. The results just have a uuid and a version.
    QList<QJsonObject> results = req1.takeResults();
    QCOMPARE(results.count(), 1);
    obj = results.takeFirst();
    QVERIFY(!obj.value(uuidStr()).toString().isEmpty());
    QVERIFY(!obj.value(versionStr()).toString().isEmpty());

    // -- reuse the old request to write a couple more objects
    const int numObjects = 300;

    QList<QJsonObject> list;
    for (int i = 0; i < numObjects; i++) {
        QJsonObject obj;
        obj.insert(typeStr(), QJsonValue(QStringLiteral("createTest")));
        obj.insert(QStringLiteral("val"), QJsonValue(i));

        list << obj;
    }
    req1.setObjects(list);
    mConnection->send(&req1);
    QVERIFY(waitForResponse(&req1));
    QVERIFY(!mRequestErrors.contains(&req1));

    // -- read the objects
    QJsonDbReadRequest req2(QString("[?_type=\"createTest\"]"));
    req2.setPartition(partition);
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));
    QVERIFY(!mRequestErrors.contains(&req2));

    results = req2.takeResults();
    QCOMPARE(results.count(), numObjects + 1);
    obj = results.takeFirst();
    QCOMPARE(obj.value(typeStr()).toString(), QStringLiteral("createTest"));
}

void TestQJsonDbRequest::updateRequest_data()
{
    QTest::addColumn<QString>("partition");

    QTest::newRow("default") << "";
    QTest::newRow("private") << "Private";
}

void TestQJsonDbRequest::updateRequest()
{
    QVERIFY(mConnection);
    QFETCH(QString, partition);

    // -- write one object
    QObject parent;
    QVERIFY(writeTestObject(&parent, QStringLiteral("updateTest"), 0, partition));

    // -- read one object
    QJsonDbReadRequest req2(QString("[?_type=\"updateTest\"]"));
    req2.setPartition(partition);
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));
    QVERIFY(!mRequestErrors.contains(&req2));

    QList<QJsonObject> results = req2.takeResults();
    QCOMPARE(results.count(), 1);

    // -- update the object
    QJsonObject obj1(results.takeFirst());
    obj1.insert(QStringLiteral("val"), QJsonValue(1));
    results.append(obj1);

    QJsonDbUpdateRequest req3(results);
    req3.setPartition(partition);
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));

    // - check that we have results of the update. The results just have a uuid and a version.
    results = req3.takeResults();
    QCOMPARE(results.count(), 1);
    QJsonObject obj = results.takeFirst();
    QVERIFY(!obj.value(uuidStr()).toString().isEmpty());
    QVERIFY(!obj.value(versionStr()).toString().isEmpty());

    // - re-read and check if really updated also in the database
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));
    QVERIFY(!mRequestErrors.contains(&req2));

    results = req2.takeResults();
    QCOMPARE(results.count(), 1);
    obj = results.takeFirst();
    QCOMPARE(obj.value(QStringLiteral("val")), QJsonValue(1)); // this is the updated value

    // -- update from null
    obj.insert(typeStr(), QJsonValue(QStringLiteral("new")));
    obj.insert(QStringLiteral("_uuid"), QJsonDbObject::createUuid().toString());
    req3.setObjects(QList<QJsonObject>() << obj);
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));
}

void TestQJsonDbRequest::privatePartition()
{
    QJsonObject contact1;
    contact1.insert(QStringLiteral("_type"), QStringLiteral("test-contact"));
    contact1.insert(QStringLiteral("_uuid"), QUuid::createUuid().toString());
    contact1.insert(QStringLiteral("name"), QStringLiteral("Joe"));
    QJsonObject contact2;
    contact2.insert(QStringLiteral("_type"), QStringLiteral("test-contact"));
    contact2.insert(QStringLiteral("_uuid"), QUuid::createUuid().toString());
    contact2.insert(QStringLiteral("name"), QStringLiteral("Alice"));

    QJsonDbWriteRequest write;
    write.setObjects(QList<QJsonObject>() << contact1 << contact2);
    // use the explicit form of the private partition (with full username)
    write.setPartition(QString::fromLatin1("%1.Private").arg(QJsonDbStandardPaths::currentUser()));
    mConnection->send(&write);
    QVERIFY(waitForResponse(&write));
    QVERIFY(!mRequestErrors.contains(&write));
    QList<QJsonObject> results = write.takeResults();
    QCOMPARE(results.count(), 2);
    contact1.insert(QStringLiteral("_version"), results[0].value(QStringLiteral("_version")));
    contact2.insert(QStringLiteral("_version"), results[1].value(QStringLiteral("_version")));

    QJsonDbReadRequest query;
    query.setQuery(QStringLiteral("[?_type=%type][/_type]"));
    query.bindValue(QStringLiteral("type"), QStringLiteral("test-contact"));

    // use the default form of private partition (no username specified, default to current user)
    query.setPartition(QStringLiteral("Private"));
    mConnection->send(&query);
    QVERIFY(waitForResponse(&query));
    QVERIFY(!mRequestErrors.contains(&query));
    QCOMPARE(query.sortKey(), QStringLiteral("_type"));

    quint32 state = query.stateNumber();
    QVERIFY(state != 0);

    results = query.takeResults();
    QCOMPARE(results.count(), 2);
    foreach (const QJsonObject &object, results) {
        QVERIFY(object.value(QStringLiteral("_uuid")).toString() == contact1.value(QStringLiteral("_uuid")).toString() ||
                object.value(QStringLiteral("_uuid")).toString() == contact2.value(QStringLiteral("_uuid")).toString());
    }

    contact1.insert(QStringLiteral("_deleted"), true);
    contact2.insert(QStringLiteral("name"), QStringLiteral("Jane"));
    write.setObjects(QList<QJsonObject>() << contact1 << contact2);
    mConnection->send(&write);
    QVERIFY(waitForResponse(&write));
    QVERIFY(!mRequestErrors.contains(&write));
    results = write.takeResults();
    QCOMPARE(results.count(), 2);
    contact1.insert(QStringLiteral("_version"), results[0].value(QStringLiteral("_version")));
    contact2.insert(QStringLiteral("_version"), results[1].value(QStringLiteral("_version")));

    mConnection->send(&query);
    QVERIFY(waitForResponse(&query));
    QVERIFY(!mRequestErrors.contains(&query));
    QVERIFY(query.stateNumber() > state);
    state = query.stateNumber();

    results = query.takeResults();
    QCOMPARE(results.count(), 1);
    QCOMPARE(results[0].value(QStringLiteral("_uuid")).toString(), contact2.value(QStringLiteral("_uuid")).toString());
    QCOMPARE(results[0].value(QStringLiteral("name")).toString(), contact2.value(QStringLiteral("name")).toString());

    // switch
    query.setPartition(QString::fromLatin1("%1.Private").arg(QJsonDbStandardPaths::currentUser()));
    write.setPartition(QStringLiteral("Private"));

    contact2.insert(QStringLiteral("_deleted"), true);
    write.setObjects(QList<QJsonObject>() << contact2);
    mConnection->send(&write);
    QVERIFY(waitForResponse(&write));
    QVERIFY(!mRequestErrors.contains(&write));

    mConnection->send(&query);
    QVERIFY(waitForResponse(&query));
    QVERIFY(!mRequestErrors.contains(&query));
    QVERIFY(query.stateNumber() > state);

    results = query.takeResults();
    QVERIFY(results.isEmpty());
}

void TestQJsonDbRequest::privatePartition2()
{
    QJsonDbObject contact1;
    contact1.insert(QStringLiteral("_uuid"), QStringLiteral("{ed3d2c73-27f6-4443-9945-3b18b11d7c56}"));
    contact1.insert(QStringLiteral("_type"), QStringLiteral("privatePartition2"));
    contact1.insert(QStringLiteral("name"), QStringLiteral("Joe"));
    QJsonDbObject contact2;
    contact2.insert(QStringLiteral("_type"), QStringLiteral("privatePartition2"));
    contact2.insert(QStringLiteral("_uuid"), QStringLiteral("{6ec6277b-0020-4fa6-a40f-02e574b3af6a}"));
    contact2.insert(QStringLiteral("name"), QStringLiteral("Alice"));

    QJsonDbWriteRequest write1;
    write1.setObjects(QList<QJsonObject>() << contact1);
    write1.setPartition(QStringLiteral("Private"));
    QJsonDbWriteRequest write2;
    write2.setObjects(QList<QJsonObject>() << contact2);
    write2.setPartition(QStringLiteral("Private"));
    QJsonDbReadRequest read(QStringLiteral("[?_type=\"privatePartition2\"]"));
    read.setPartition(QStringLiteral("Private"));
    mConnection->send(&write1);
    mConnection->send(&write2);
    mConnection->send(&read);

    QVERIFY(waitForResponse(&write2));
    QVERIFY(!mRequestErrors.contains(&write1));
    QVERIFY(!mRequestErrors.contains(&write2));
    QList<QJsonObject> results = write2.takeResults();
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.at(0).value(QStringLiteral("_uuid")).toString(), contact2.value(QStringLiteral("_uuid")).toString());

    QVERIFY(waitForResponse(&read));
    results = read.takeResults();
    QCOMPARE(results.size(), 2);
    QList<QJsonDbRequest::Status> statuses = mRequestStatuses.value(&write2);
    QCOMPARE(statuses.size(), 3);
    QCOMPARE((int)statuses.at(0), (int)QJsonDbRequest::Sent);
    QCOMPARE((int)statuses.at(1), (int)QJsonDbRequest::Receiving);
    QCOMPARE((int)statuses.at(2), (int)QJsonDbRequest::Finished);
}

void TestQJsonDbRequest::privatePartitions()
{
    QJsonDbObject contact1;
    contact1.insert(QStringLiteral("_uuid"), QStringLiteral("{9a57c619-fa16-4259-acf5-7d670f59674f}"));
    contact1.insert(QStringLiteral("_type"), QStringLiteral("privatePartitions"));
    contact1.insert(QStringLiteral("name"), QStringLiteral("Joe"));
    QJsonDbObject contact2;
    contact2.insert(QStringLiteral("_type"), QStringLiteral("privatePartitions"));
    contact2.insert(QStringLiteral("_uuid"), QStringLiteral("{d87707c5-71ae-4524-813e-4dd9dda55a87}"));
    contact2.insert(QStringLiteral("name"), QStringLiteral("Alice"));

    QJsonDbWriteRequest write1;
    write1.setObjects(QList<QJsonObject>() << contact1);
    write1.setPartition(QStringLiteral("joe.Private"));
    mConnection->send(&write1);
    QVERIFY(waitForResponse(&write1));

    QJsonDbWriteRequest write2;
    write2.setObjects(QList<QJsonObject>() << contact2);
    write2.setPartition(QStringLiteral("alice.Private"));
    mConnection->send(&write2);
    QVERIFY(waitForResponse(&write2));

    {
        QJsonDbReadRequest read(QStringLiteral("[?_type=\"privatePartitions\"]"));
        read.setPartition(QStringLiteral("Private"));
        mConnection->send(&read);
        QVERIFY(waitForResponse(&read));
        QList<QJsonObject> results = read.takeResults();
        QCOMPARE(results.size(), 0);
    }
    {
        QJsonDbReadRequest read(QStringLiteral("[?_type=\"privatePartitions\"]"));
        read.setPartition(QStringLiteral("alice.Private"));
        mConnection->send(&read);
        QVERIFY(waitForResponse(&read));
        QList<QJsonObject> results = read.takeResults();
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.at(0).value(QStringLiteral("name")).toString(), QLatin1String("Alice"));
    }
    {
        QJsonDbReadRequest read(QStringLiteral("[?_type=\"privatePartitions\"]"));
        read.setPartition(QStringLiteral("joe.Private"));
        mConnection->send(&read);
        QVERIFY(waitForResponse(&read));
        QList<QJsonObject> results = read.takeResults();
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.at(0).value(QStringLiteral("name")).toString(), QLatin1String("Joe"));
    }
}

void TestQJsonDbRequest::invalidPrivatePartition()
{
    QJsonObject contact1;
    contact1.insert(QStringLiteral("_type"), QStringLiteral("test-contact"));
    contact1.insert(QStringLiteral("_uuid"), QUuid::createUuid().toString());
    contact1.insert(QStringLiteral("name"), QStringLiteral("Joe"));

    QJsonDbWriteRequest write;
    write.setObjects(QList<QJsonObject>() << contact1);
    write.setPartition(QStringLiteral("userthatdoesnotexist.Private"));
    mConnection->send(&write);
    waitForResponse(&write);
    QVERIFY(mRequestErrors.contains(&write));
    QCOMPARE(mRequestErrors[&write], QJsonDbRequest::InvalidPartition);
}

/*!
    Check if removing an object works and throws errors if it doesn't work.
*/
void TestQJsonDbRequest::removeRequest()
{
    QVERIFY(mConnection);

    // -- write some objects
    QObject parent;
    writeTestObject(&parent, QStringLiteral("removeTest"), 0);
    writeTestObject(&parent, QStringLiteral("removeTest"), 1);
    writeTestObject(&parent, QStringLiteral("removeTest"), 2);
    writeTestObject(&parent, QStringLiteral("removeTest"), 3);

    // -- read the objects
    QJsonDbReadRequest req2(QString("[?_type=\"removeTest\"]"));
    mConnection->send(&req2);
    QVERIFY(waitForResponse(&req2));

    QList<QJsonObject> results = req2.takeResults();
    QCOMPARE(results.count(), 4);

    QJsonDbRemoveRequest req3(results);

    // try to delete one object
    QJsonObject toDelete = results[0];
    req3.setObjects(QList<QJsonObject>() << toDelete );
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QList<QJsonObject> writeResults = req3.takeResults();
    QCOMPARE(writeResults.size(), 1);
    QCOMPARE(req3.status(), QJsonDbRequest::Finished);
    toDelete.insert(QStringLiteral("_version"), writeResults.at(0).value(QStringLiteral("_version")).toString());

    // try to delete an object again. It's a replay, not an error?
    req3.setObjects(QList<QJsonObject>() << toDelete );
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(!mRequestErrors.contains(&req3));
    QCOMPARE(req3.status(), QJsonDbRequest::Finished);

    QJsonObject obj;
    obj.insert(typeStr(), QJsonValue(QStringLiteral("removeTest")));

    // try to delete an object without uuid
    QJsonDbRemoveRequest req4(QList<QJsonObject>() << obj);
    mConnection->send(&req4);
    QVERIFY(waitForResponse(&req4));
    QVERIFY(mRequestErrors.contains(&req4));
    QCOMPARE(req4.status(), QJsonDbRequest::Error);
    QCOMPARE(mRequestErrors.value(&req4), QJsonDbRequest::MissingUUID);

    // try to delete no objects
    req3.setObjects(QList<QJsonObject>());
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(mRequestErrors.contains(&req3));
    QCOMPARE(req3.status(), QJsonDbRequest::Error);
    QCOMPARE(mRequestErrors.value(&req3), QJsonDbRequest::MissingObject);

    // try to delete an object without type
    obj.remove(typeStr());
    obj.insert(uuidStr(), results[0].value(uuidStr()));
    req3.setObjects(QList<QJsonObject>() << obj);
    mConnection->send(&req3);
    QVERIFY(waitForResponse(&req3));
    QVERIFY(mRequestErrors.contains(&req3));
    QCOMPARE(req3.status(), QJsonDbRequest::Error);
    QCOMPARE(mRequestErrors.value(&req3), QJsonDbRequest::MissingType);

}

void TestQJsonDbRequest::forced()
{
    QJsonDbObject item;
    item.setUuid(QJsonDbObject::createUuid());
    item.insert(QStringLiteral("_type"), QStringLiteral("forcedTest"));
    item.insert(QStringLiteral("name"), QStringLiteral("Alice"));
    QJsonDbCreateRequest create(item);
    mConnection->send(&create);
    QVERIFY(waitForResponse(&create));
    QVERIFY(!mRequestErrors.contains(&create));

    // this should get rejected...no _version specified
    item.insert(QStringLiteral("name"), QStringLiteral("Joe"));
    QJsonDbWriteRequest update;
    update.setObjects(QList<QJsonObject>() << item);
    mConnection->send(&update);
    QVERIFY(waitForResponse(&update));
    QVERIFY(mRequestErrors.contains(&update));
    QCOMPARE(mRequestErrors[&update], QJsonDbRequest::UpdatingStaleVersion);
    mRequestErrors.clear();

    // force the write
    update.setConflictResolutionMode(QJsonDbWriteRequest::Replace);
    mConnection->send(&update);
    QVERIFY(waitForResponse(&update));
    QVERIFY(!mRequestErrors.contains(&update));

    // fail with a stale remove
    QJsonDbRemoveRequest remove(item);
    mConnection->send(&remove);
    QVERIFY(waitForResponse(&remove));
    QVERIFY(mRequestErrors.contains(&remove));
    QCOMPARE(mRequestErrors[&remove], QJsonDbRequest::UpdatingStaleVersion);
    mRequestErrors.clear();

    // force the remove
    remove.setConflictResolutionMode(QJsonDbWriteRequest::Replace);
    mConnection->send(&remove);
    QVERIFY(waitForResponse(&remove));
    QVERIFY(!mRequestErrors.contains(&remove));

    // make sure it's really gone
    QJsonDbReadObjectRequest read(item.uuid());
    mConnection->send(&read);
    QVERIFY(waitForResponse(&read));
    QVERIFY(read.takeResults().isEmpty());
}

void TestQJsonDbRequest::replaceFromNull()
{
    QJsonDbObject object;
    object.setUuid(QUuid::createUuid());
    object.insert(QLatin1String("_type"), QLatin1String("replaceFromNull"));
    object.insert(QLatin1String("bar"), QLatin1String("Julia"));

    QJsonDbUpdateRequest replace(object);
    replace.setConflictResolutionMode(QJsonDbWriteRequest::Replace);
    mConnection->send(&replace);
    QVERIFY(waitForResponse(&replace));

    // now read the object, it should be there
    QJsonDbReadRequest read;
    read.setQuery(QStringLiteral("[?_type=\"replaceFromNull\"]"));
    mConnection->send(&read);
    QVERIFY(waitForResponse(&read));
    QCOMPARE((int)read.status(), (int)QJsonDbReadRequest::Finished);
    QList<QJsonObject> results = read.takeResults();
    QCOMPARE(results.size(), 1);
}

void TestQJsonDbRequest::multiplerequests()
{
    QJsonDbObject object;
    object.setUuid(QUuid::createUuid());
    object.insert(QLatin1String("_type"), QLatin1String("multiplerequests"));
    object.insert(QLatin1String("bar"), QLatin1String("Julia"));

    {
        QJsonDbCreateRequest request(object);
        mConnection->send(&request);
        QVERIFY(waitForResponse(&request));

        // now read the object, it should be there
        QJsonDbReadRequest read;
        read.setQuery(QStringLiteral("[?_type=\"multiplerequests\"]"));
        mConnection->send(&read);
        QVERIFY(waitForResponse(&read));
        QCOMPARE((int)read.status(), (int)QJsonDbReadRequest::Finished);
        QList<QJsonObject> results = read.takeResults();
        QCOMPARE(results.size(), 1);
    }

    QJsonDbRemoveRequest remove1(object);
    remove1.setConflictResolutionMode(QJsonDbWriteRequest::Replace);
    mConnection->send(&remove1);

    QJsonDbUpdateRequest update(object);
    update.setConflictResolutionMode(QJsonDbWriteRequest::Replace);
    mConnection->send(&update);

    QJsonDbRemoveRequest remove2(object);
    remove2.setConflictResolutionMode(QJsonDbWriteRequest::Replace);
    mConnection->send(&remove2);

    QList<QJsonDbRequest *> requests;
    requests << &remove1 << &update << &remove2;
    QVERIFY(waitForResponse(requests));

    // now read the object, it shouldn't exist
    QJsonDbReadRequest read;
    read.setQuery(QStringLiteral("[?_type=\"multiplerequests\"]"));
    mConnection->send(&read);
    QVERIFY(waitForResponse(&read));
    QCOMPARE((int)read.status(), (int)QJsonDbReadRequest::Finished);
    QList<QJsonObject> results = read.takeResults();
    QCOMPARE(results.size(), 0);
}

void TestQJsonDbRequest::defaultConnection()
{
    // make sure that the default connection connects automatically
    QJsonDbConnection *connection = QJsonDbConnection::defaultConnection();
    QJsonDbReadRequest request(QStringLiteral("[?_type=\"Foo\"]"));
    QEventLoop ev;
    QObject::connect(&request, SIGNAL(finished()), &ev, SLOT(quit()));
    QTimer::singleShot(10000, &ev, SLOT(quit()));
    QVERIFY(connection->send(&request));
    ev.exec();
    QCOMPARE((int)request.status(), (int)QJsonDbRequest::Finished);
}

void TestQJsonDbRequest::privatePartitionFlushRequest()
{
    QJsonDbFlushRequest request;
    request.setPartition(QStringLiteral("Private"));
    QVERIFY(mConnection->send(&request));
    QVERIFY(waitForResponse(&request));
    QVERIFY(mRequestStatuses.contains(&request));
    QList<QtJsonDb::QJsonDbRequest::Status> statuses = mRequestStatuses.value(&request);
    QCOMPARE(statuses.size(), 2);
    QCOMPARE((int)statuses.at(0), (int)QJsonDbRequest::Receiving);
    QCOMPARE((int)statuses.at(1), (int)QJsonDbRequest::Finished);
}

class Worker : public QObject
{
    Q_OBJECT
public:
    QString partitionName;
    int requestsCount;
    QTimer *t;
    Worker() : requestsCount(0) { }

public Q_SLOTS:
    void start()
    {
        send();
    }

    void send()
    {
        QJsonDbObject item;
        item.setUuid(QUuid::createUuid());
        item.insert(QStringLiteral("_type"), QStringLiteral("multipleThreads"));

        QThread *thread = QThread::currentThread();

        QJsonDbWriteRequest *write = new QJsonDbWriteRequest;
        write->setPartition(partitionName);
        write->setObject(item);
        QObject::connect(write, SIGNAL(error(QtJsonDb::QJsonDbRequest::ErrorCode,QString)),
                         this, SLOT(requestError(QtJsonDb::QJsonDbRequest::ErrorCode,QString)));
        QObject::connect(write, SIGNAL(finished()), this, SLOT(writeRequestFinished()));
        QObject::connect(write, SIGNAL(finished()), write, SLOT(deleteLater()));
        QObject::connect(thread, SIGNAL(finished()), write, SLOT(deleteLater()));
        QJsonDbConnection::defaultConnection()->send(write);
        requestsCount++;

        QJsonDbReadRequest *read = new QJsonDbReadRequest;
        read->setPartition(partitionName);
        read->setQuery(QStringLiteral("[?_type=\"multipleThreads\"]"));
        QObject::connect(read, SIGNAL(error(QtJsonDb::QJsonDbRequest::ErrorCode,QString)),
                         this, SLOT(requestError(QtJsonDb::QJsonDbRequest::ErrorCode,QString)));
        QObject::connect(read, SIGNAL(finished()), this, SLOT(readRequestFinished()));
        QObject::connect(read, SIGNAL(finished()), this, SLOT(send()));
        QObject::connect(read, SIGNAL(finished()), read, SLOT(deleteLater()));
        QObject::connect(thread, SIGNAL(finished()), read, SLOT(deleteLater()));
        QJsonDbConnection::defaultConnection()->send(read);
        requestsCount++;
    }

    void writeRequestFinished()
    {
        QJsonDbWriteRequest *r = qobject_cast<QJsonDbWriteRequest *>(sender());
        QVERIFY(r != 0);
        QList<QJsonObject> results = r->takeResults();
        QCOMPARE(results.size(), 1);
    }

    void readRequestFinished()
    {
        QJsonDbReadRequest *r = qobject_cast<QJsonDbReadRequest *>(sender());
        QVERIFY(r != 0);
        QList<QJsonObject> results = r->takeResults();
        QVERIFY(results.size() >= 1);
    }

    void requestError(QtJsonDb::QJsonDbRequest::ErrorCode code, const QString &message)
    {
        if (partitionName != QLatin1String("userthatdoesnotexist.Private")) {
            QJsonDbRequest *r = qobject_cast<QJsonDbRequest *>(sender());
            qDebug() << "request failed" << partitionName << r << code << message;
            QVERIFY(false);
        }
    }

    void stop()
    {
        // uncomment me if you want to see how many requests each thread managed to push through
        //qDebug() << QThread::currentThread() << "is stopping. Requests count" << requestsCount;
    }

    void quit()
    {
        QThread::currentThread()->quit();
    }
};

void TestQJsonDbRequest::multipleThreads_data()
{
    QTest::addColumn<QStringList>("partitions");

    QTest::newRow("serverpartition") << (QStringList());
    QTest::newRow("private") << (QStringList() << QStringLiteral("Private"));
    QTest::newRow("private-multi")
            << (QStringList() << QStringLiteral("Private") << QStringLiteral("alice.Private"));
    QTest::newRow("private-multi-nonexistent")
            << (QStringList() << QStringLiteral("Private") << QStringLiteral("alice.Private") << QStringLiteral("userthatdoesnotexist.Private"));
}

void TestQJsonDbRequest::multipleThreads()
{
    QFETCH(QStringList, partitions);

    QList<QThread *> threads;
    static const int NumThreads = 3;
    for (int i = 0; i < NumThreads; ++i) {
        QThread *thread = new QThread;

        Worker *w = new Worker;
        QTimer *timer = new QTimer;
        timer->setInterval(1000 + qrand() % 2000);
        timer->moveToThread(thread);
        QObject::connect(thread, SIGNAL(started()), timer, SLOT(start()));
        QObject::connect(timer, SIGNAL(timeout()), w, SLOT(quit()));
        QObject::connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));

        if (!partitions.isEmpty())
            w->partitionName = partitions.at(i % partitions.size());
        w->moveToThread(thread);
        QObject::connect(thread, SIGNAL(started()), w, SLOT(start()));
        QObject::connect(thread, SIGNAL(finished()), w, SLOT(stop()));
        QObject::connect(thread, SIGNAL(finished()), w, SLOT(deleteLater()));

        w->t = timer;
        threads.append(thread);
    }

    foreach (QThread *t, threads)
        t->start();
    foreach (QThread *t, threads)
        t->wait();

    qDeleteAll(threads);
}

void TestQJsonDbRequest::readObjectRequest()
{
    QUuid uuid = QUuid::createUuid();
    QJsonDbObject item;
    item.setUuid(uuid);
    item.insert(QStringLiteral("_type"), QStringLiteral("readObjectRequest"));

    QJsonDbReadObjectRequest request;
    QSignalSpy availableSpy(&request, SIGNAL(objectAvailable(QJsonObject)));
    QSignalSpy unavailableSpy(&request, SIGNAL(objectUnavailable(QUuid)));

    // test non-existent object
    request.setUuid(uuid);
    QVERIFY(mConnection->send(&request));
    QVERIFY(waitForResponse(&request));
    QCOMPARE(availableSpy.count(), 0);
    QCOMPARE(unavailableSpy.count(), 1);
    QList<QVariant> args = unavailableSpy.takeFirst();
    QCOMPARE(args.size(), 1);
    QCOMPARE(args.at(0).value<QUuid>(), uuid);

    // create the object and try again
    {
        QJsonDbCreateRequest request(item);
        QVERIFY(mConnection->send(&request));
        QVERIFY(waitForResponse(&request));
    }
    QVERIFY(mConnection->send(&request));
    QVERIFY(waitForResponse(&request));
    QCOMPARE(availableSpy.count(), 1);
    QCOMPARE(unavailableSpy.count(), 0);
    args = availableSpy.takeFirst();
    QCOMPARE(args.size(), 1);
    QJsonObject result = args.at(0).value<QJsonObject>();
    QCOMPARE(result.value(QStringLiteral("_uuid")).toString(), item.uuid().toString());
    QCOMPARE(result.value(QStringLiteral("_type")).toString(), item.value(QStringLiteral("_type")).toString());
}

void TestQJsonDbRequest::bindings()
{
    {
        QJsonDbObject object;
        object.setUuid(QUuid::createUuid());
        object.insert(QLatin1String("_type"), QLatin1String("bindingstest"));
        object.insert(QLatin1String("foo"), QLatin1String("[?_type=\"bunny\"]"));
        object.insert(QLatin1String("bar"), QLatin1String("Julia"));
        QJsonDbCreateRequest request(object);
        mConnection->send(&request);
        waitForResponse(&request);
    }

    {
        QJsonDbReadRequest request;
        request.setQuery(QStringLiteral("[?_type=\"bindingstest\"][?foo=%blah]"));
        request.bindValue(QLatin1String("blah"), QLatin1String("[?_type=\"bunny\"]"));
        mConnection->send(&request);
        waitForResponse(&request);
        QList<QJsonObject> results = request.takeResults();
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.at(0).value(QLatin1String("_type")).toString(), QLatin1String("bindingstest"));
        QCOMPARE(results.at(0).value(QLatin1String("foo")).toString(), QLatin1String("[?_type=\"bunny\"]"));
    }

    {
        // first check that the real regexp works:
        QJsonDbReadRequest request;
        request.setQuery(QStringLiteral("[?_type=\"bindingstest\"][?bar =~ \"/J*/wi\"]"));
        mConnection->send(&request);
        waitForResponse(&request);
        QList<QJsonObject> results = request.takeResults();
        QCOMPARE(results.size(), 1);

        request.setQuery(QStringLiteral("[?_type=\"bindingstest\"][?bar=%blah]"));
        request.bindValue(QLatin1String("blah"), QLatin1String("~/J*/wi"));
        mConnection->send(&request);
        waitForResponse(&request);
        results = request.takeResults();
        QCOMPARE(results.size(), 0);

        request.setQuery(QStringLiteral("[?_type=\"bindingstest\"][?bar=%blah]"));
        request.bindValue(QLatin1String("blah"), QLatin1String("~\"/J*/wi\""));
        mConnection->send(&request);
        waitForResponse(&request);
        results = request.takeResults();
        QCOMPARE(results.size(), 0);
    }

    {
        QJsonDbReadRequest request;
        request.setQuery(QStringLiteral("[?_type=\"bindingstest\"][?bar =~ %blah]"));
        request.bindValue(QLatin1String("blah"), QLatin1String("/J*/wi"));
        mConnection->send(&request);
        waitForResponse(&request);
        QList<QJsonObject> results = request.takeResults();
        QCOMPARE(results.size(), 1);

        request.setQuery(QStringLiteral("[?_type=\"bindingstest\"][?bar =~ %blah]"));
        request.bindValue(QLatin1String("blah"), QLatin1String("/Z*/wi"));
        mConnection->send(&request);
        waitForResponse(&request);
        results = request.takeResults();
        QCOMPARE(results.size(), 0);
    }
}

class defaultPartition_helper
{
public:
    defaultPartition_helper(TestHelper *testHelper)
        : mTestHelper(testHelper)
    {
        // temporarily rename config files
        renameFiles(false);

        // add a Partitions watcher
        mWatcher.setPartition(QLatin1String("Ephemeral"));
        mWatcher.setQuery("[?_type=\"Partition\"]");
        mTestHelper->connection()->addWatcher(&mWatcher);
        mTestHelper->waitForStatus(&mWatcher, QtJsonDb::QJsonDbWatcher::Active);
    }

    ~defaultPartition_helper()
    {
        // restore original config file names
        renameFiles(true);

        // remove the Partitions watcher
        mTestHelper->connection()->removeWatcher(&mWatcher);
        mTestHelper->waitForStatus(&mWatcher, QtJsonDb::QJsonDbWatcher::Inactive);
    }

    // Replaces the current partition defs in the daemon with new ones. Returns the final partition defs as seen by the daemon.
    QList<QJsonObject> loadAndQueryPartitionDefs(const QJsonArray &defs)
    {
        // ensure that the new file is written to the same directory as the original partitions.json (which is now renamed!)
        QFile partitionsFile(QFileInfo(QFINDTESTDATA("partitions.json.bak")).absoluteDir().absoluteFilePath(QLatin1String("partitions-test.json")));
        partitionsFile.open(QFile::WriteOnly);
        partitionsFile.write(QJsonDocument(defs).toJson());
        partitionsFile.close();

        // send the daemon a reload request to get it to unload the partitions
        QJsonDbReloadRequest reload;
        mTestHelper->connection()->send(&reload);
        if (!mTestHelper->waitForResponseAndNotifications(&reload, &mWatcher, defs.size())) {
            QWARN("reloading partition defs failed");
            return QList<QJsonObject>();
        }
        partitionsFile.remove();

        // retrieve current partition defs
        QJsonDbReadRequest partitionQuery;
        partitionQuery.setPartition(QLatin1String("Ephemeral"));
        partitionQuery.setQuery(QLatin1String("[?_type=%type]"));
        partitionQuery.bindValue(QLatin1String("type"), QLatin1String("Partition"));
        mTestHelper->connection()->send(&partitionQuery);
        if (!mTestHelper->waitForResponse(&partitionQuery)) {
            QWARN("retrieving partition defs failed");
            return QList<QJsonObject>();
        }

        return partitionQuery.takeResults();
    }

    QtJsonDb::QJsonDbWatcher mWatcher;

private:
    void renameFiles(bool restoring)
    {
        const QString refFile = restoring ? QStringLiteral("partitions.json.bak") : QStringLiteral("partitions.json");
        QFileInfoList fileInfos1 = QFileInfo(QFINDTESTDATA(refFile)).absoluteDir().entryInfoList(
                    QStringList() << (restoring ? QStringLiteral("partitions*.json.bak") : QStringLiteral("partitions*json")),
                    QDir::Files | QDir::Readable);
        foreach (const QFileInfo &fileInfo1, fileInfos1) {
            const QString fileName1 = fileInfo1.absoluteFilePath();
            const QString fileName2 = restoring
                    ? fileName1.left(fileName1.lastIndexOf(QStringLiteral(".bak")))
                    : QString("%1.bak").arg(fileName1);
            if (!QFile::rename(fileName1, fileName2))
                QFAIL(qPrintable(QString("failed to rename %1 to %2 (restoring = %3)").arg(fileName1).arg(fileName2).arg(restoring)));
        }
    }

    TestHelper *mTestHelper;
};


void TestQJsonDbRequest::defaultPartition()
{
    defaultPartition_helper helper(this);

    int index = 0; // to avoid duplicate partition names

    // *** Case 1: two partitions, neither of which is marked as default or removable
    // *** Expected result: exactly one of the partitions gets set as default
    {
        QJsonArray defs;

        QJsonObject def1;
        def1.insert(QLatin1String("name"), QString("com.qt-project.test1_%1").arg(++index));
        def1.insert(QLatin1String("path"), QLatin1String("."));
        defs.append(def1);

        QJsonObject def2;
        def2.insert(QLatin1String("name"), QString("com.qt-project.test2_%1").arg(index));
        def2.insert(QLatin1String("path"), QLatin1String("."));
        defs.append(def2);

        QList<QJsonObject> results = helper.loadAndQueryPartitionDefs(defs);
        QCOMPARE(results.count(), 2);

        int defaultCount = 0;
        foreach (const QJsonObject &object, results)
            defaultCount += (object.value(QLatin1String("default")).toBool() ? 1 : 0);
        QCOMPARE(defaultCount, 1);
    }

    // *** Case 2: two non-removable partitions, exactly one of which is marked as default
    // *** Expected result: the original default partition is still the only one set as default
    {
        QJsonArray defs;

        QJsonObject def1;
        def1.insert(QLatin1String("name"), QString("com.qt-project.test1_%1").arg(++index));
        def1.insert(QLatin1String("path"), QLatin1String("."));
        defs.append(def1);

        QJsonObject def2;
        const QString expectedDefaultName = QString("com.qt-project.test2_%1").arg(index);
        def2.insert(QLatin1String("name"), expectedDefaultName);
        def2.insert(QLatin1String("path"), QLatin1String("."));
        def2.insert(QLatin1String("default"), true);
        defs.append(def2);

        QList<QJsonObject> results = helper.loadAndQueryPartitionDefs(defs);
        QCOMPARE(results.count(), 2);

        int defaultCount = 0;
        foreach (const QJsonObject &object, results) {
            if (object.value(QLatin1String("default")).toBool()) {
                QCOMPARE(object.value(QLatin1String("removable")).toBool(), false);
                QCOMPARE(object.value(QLatin1String("name")).toString(), expectedDefaultName);
                defaultCount++;
            }
        }
        QCOMPARE(defaultCount, 1);
    }

    // *** Case 3: exactly one non-removable partition, no default ones
    // *** Expected result: the non-removable partition is automatically set as default
    {
        QJsonArray defs;

        QJsonObject def1;
        def1.insert(QLatin1String("name"), QString("com.qt-project.test1_%1").arg(++index));
        def1.insert(QLatin1String("path"), QLatin1String("."));
        def1.insert(QLatin1String("removable"), true);
        defs.append(def1);

        QJsonObject def2;
        const QString expectedDefaultName = QString("com.qt-project.test2_%1").arg(index);
        def2.insert(QLatin1String("name"), expectedDefaultName);
        def2.insert(QLatin1String("path"), QLatin1String("."));
        // def2.insert(QLatin1String("removable"), false); // false by default
        defs.append(def2);

        QJsonObject def3;
        def3.insert(QLatin1String("name"), QString("com.qt-project.test3_%1").arg(index));
        def3.insert(QLatin1String("path"), QLatin1String("."));
        def3.insert(QLatin1String("removable"), true);
        defs.append(def3);

        QList<QJsonObject> results = helper.loadAndQueryPartitionDefs(defs);
        QCOMPARE(results.count(), 3);

        int defaultCount = 0;
        foreach (const QJsonObject &object, results) {
            if (object.value(QLatin1String("default")).toBool()) {
                QCOMPARE(object.value(QLatin1String("removable")).toBool(), false);
                QCOMPARE(object.value(QLatin1String("name")).toString(), expectedDefaultName);
                defaultCount++;
            }
        }
        QCOMPARE(defaultCount, 1);
    }

    // *** Case 4: only removable partitions
    // *** Expected response: an extra partition is created (in CWD) to act as the default one
    {
        QJsonArray defs;

        QJsonObject def1;
        def1.insert(QLatin1String("name"), QString("com.qt-project.test1_%1").arg(++index));
        def1.insert(QLatin1String("path"), QLatin1String("."));
        def1.insert(QLatin1String("removable"), true);
        defs.append(def1);

        QJsonObject def2;
        const QString expectedDefaultName = QString("com.qt-project.test2_%1").arg(index);
        def2.insert(QLatin1String("name"), expectedDefaultName);
        def2.insert(QLatin1String("path"), QLatin1String("."));
        def2.insert(QLatin1String("removable"), true);
        defs.append(def2);

        QList<QJsonObject> results = helper.loadAndQueryPartitionDefs(defs);
        QCOMPARE(results.count(), 3); // expect an extra partition

        int defaultCount = 0;
        foreach (const QJsonObject &object, results) {
            if (object.value(QLatin1String("default")).toBool()) {
                QCOMPARE(object.value(QLatin1String("removable")).toBool(), false);
                defaultCount++;
            }
        }
        QCOMPARE(defaultCount, 1);
    }
}

QTEST_MAIN(TestQJsonDbRequest)

#include "testqjsondbrequest.moc"
