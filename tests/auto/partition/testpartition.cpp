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

#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QtMessageHandler>
#include <QTime>
#include <QUuid>

#include "jsondbbtree.h"
#include "jsondbobjecttable.h"
#include "jsondbpartition.h"
#include "private/jsondbpartition_p.h"
#include "jsondbindex.h"
#include "jsondbsettings.h"
#include "jsondbstrings.h"
#include "jsondberrors.h"
#include "jsondbqueryparser.h"

#include <qjsonobject.h>

#include "../../shared/util.h"

#ifndef QT_NO_DEBUG_OUTPUT
#define DBG() if (gDebug) qDebug()
#else
#define DBG() if (0) qDebug()
#endif

QT_USE_NAMESPACE_JSONDB_PARTITION

static QString kContactStr = "com.example.unittest.contact";

/*
  Ensure that a good result object contains the correct fields
 */
#define verifyGoodResult(result) \
{ \
    JsonDbWriteResult __result = result; \
    QVERIFY2(__result.message.isEmpty(), __result.message.toLatin1()); \
    QVERIFY(__result.code == JsonDbError::NoError); \
}

/*
  Ensure that a error result contains the correct fields
 */
#define verifyErrorResult(result) \
{\
    JsonDbWriteResult __result = result; \
    QVERIFY(!__result.message.isEmpty()); \
    QVERIFY(__result.code != JsonDbError::NoError); \
}

#define verifyGoodQueryResult(result) \
{ \
    JsonDbQueryResult __result = result; \
    QVERIFY2(__result.code == JsonDbError::NoError,  \
         __result.message.toLocal8Bit()); \
}

template<class T>
class ScopedAssignment
{
public:
    ScopedAssignment(T &p, T b) : mP(&p) {
        mValue = *mP;
        *mP = b;
    };
    ~ScopedAssignment() {
        *mP = mValue;
    }

private:
    T *mP;
    T mValue;
};

class TestPartition: public QObject
{
    Q_OBJECT
public:
    TestPartition();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void reopen();
    void openTwice();

    void computeVersion();
    void updateVersionOptimistic();
    void updateVersionReplicating();

    void create();

    void update();
    void update2();
    void update3();
    void update4();

    void remove();
    void remove2();
    void remove3();
    void remove4();
    void remove5();
    void tombstoneResurrection();

    void schemaValidation_data();
    void schemaValidation();
    void schemaValidationExtends_data();
    void schemaValidationExtends();
    void schemaValidationExtendsArray_data();
    void schemaValidationExtendsArray();
    void schemaValidationLazyInit();

    void createList();
    void updateList();

    void mapDefinition();
    void mapDefinitionInvalid();
    void reduceDefinition();
    void reduceDefinitionInvalid();
    void mapInvalidMapFunc();
    void reduceInvalidAddSubtractFuncs();

    void map();
    void mapDuplicateSourceAndTarget();
    void mapRemoval();
    void mapUpdate();
    void mapJoin();
    void mapSelfJoinSourceUuids();
    void mapMapFunctionError();
    void mapSchemaViolation();
    void mapMultipleEmitNoTargetKeyName();
    void mapArrayConversion();
    void mapConsole();
    void reduce();
    void reduceFlattened();
    void reduceSourceKeyFunction();
    void reduceRemoval();
    void reduceUpdate();
    void reduceDuplicate();
    void reduceFunctionError();
    void reduceSchemaViolation();
    void reduceSubObjectProp();
    void reduceArray();
    void changesSinceCreate();

    void addIndex();
    void addSchema();
    void duplicateSchema();
    void removeSchema();
    void removeViewSchema();
    void updateSchema();
    void orQuery_data();
    void orQuery();
    void find1();
    void find2();
    void findFields();

    void findLikeRegexp_data();
    void findLikeRegexp();
    void findInContains();

    void wildcardIndex();
    void uuidJoin();

    void orderedFind1_data();
    void orderedFind1();
    void orderedFind2_data();
    void orderedFind2();

    void findByName();
    void findEQ();
    void find10();

    void startsWith();
    void comparison();

    void removedObjects();
    void arrayIndexQuery();
    void deindexError();
    void expectedOrder();
    void indexQueryOnCommonValues();

    void removeIndexes();
    void setOwner();
    void indexPropertyFunction();
    void indexCollation();
    void indexCaseSensitive();
    void indexCasePreference();

    void settings();

    void typeChangeIndex();
    void typeChangeMap();
    void typeChangeReduce();
    void typeChangeMapSource();
    void typeChangeReduceSource();
    void typeChangeSchema();

    void updateListWithIndex();
    void addBigIndex();
    void ensureBadPartitionFunctionCalls_data();
    void ensureBadPartitionFunctionCalls();

private:
    void createContacts();
    JsonDbQueryResult find(JsonDbOwner *owner, const QString &query, const QJsonObject bindings = QJsonObject());
    JsonDbWriteResult create(JsonDbOwner *owner, JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode = JsonDbPartition::RejectStale);
    JsonDbWriteResult update(JsonDbOwner *owner, JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode = JsonDbPartition::RejectStale);
    JsonDbWriteResult remove(JsonDbOwner *owner, JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode = JsonDbPartition::RejectStale);
    JsonDbWriteResult remove(JsonDbOwner *owner, const JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode = JsonDbPartition::RejectStale);

    void addSchema(const QString &schemaName, JsonDbObject &schemaObject);
    void addIndex(const QString &propertyName, const QString &propertyType=QString(), const QString &objectType=QString());

    void removeDbFiles();

private:
    JsonDbPartition *mJsonDbPartition;
    QList<JsonDbObject> mContactList;
    JsonDbOwner *mOwner;
};

const char *kFilename = "testdatabase";
//const char *kFilename = ":memory:";
const QString kReplica1Name = QString("replica1");
const QString kReplica2Name = QString("replica2");
const QStringList kReplicaNames = (QStringList() << kReplica1Name << kReplica2Name);

TestPartition::TestPartition() :
    mJsonDbPartition(0)
  , mOwner(0)
{
}

void TestPartition::removeDbFiles()
{
    QStringList filters;
    filters << QLatin1String("*.db")
            << "objectFile.bin" << "objectFile2.bin";
    QStringList lst = QDir().entryList(filters);
    foreach (const QString &fileName, lst)
        QFile::remove(fileName);
}

void TestPartition::initTestCase()
{
    qsrand(QDateTime::currentDateTime().toTime_t());
    QCoreApplication::setOrganizationName("Example");
    QCoreApplication::setOrganizationDomain("example.com");
    QCoreApplication::setApplicationName("testjsondb");
    QCoreApplication::setApplicationVersion("1.0");

    removeDbFiles();

    mOwner = new JsonDbOwner(this);
    mOwner->setOwnerId(QStringLiteral("com.example.JsonDbTest"));

    JsonDbPartitionSpec spec;
    spec.name = QStringLiteral("com.example.JsonDbTest");
    spec.path = QDir::currentPath();

    mJsonDbPartition = new JsonDbPartition(this);
    mJsonDbPartition->setPartitionSpec(spec);
    mJsonDbPartition->setDefaultOwner(mOwner);
    if (!mJsonDbPartition->open())
        qCritical() << "Failed to open the partition";
}

void TestPartition::cleanupTestCase()
{
    delete mJsonDbPartition;
    delete mOwner;
    removeDbFiles();
}

void TestPartition::cleanup()
{
    QCOMPARE(mJsonDbPartition->d_func()->mTransactionDepth, 0);
}

void TestPartition::reopen()
{
#ifdef Q_OS_WIN32
  QSKIP("test fails");
#endif
    JsonDbQueryParser parser;
    parser.setQuery(QStringLiteral("[?_type=\"reopentest\"]"));
    parser.parse();
    JsonDbQuery parsedQuery = parser.result();

    int counter = 1;
    for (int i = 0; i < 10; ++i, ++counter) {
        JsonDbObject item;
        item.insert(QLatin1String("_type"), QLatin1String("reopentest"));
        item.insert("create-string", QString("string"));
        Q_UNUSED(mJsonDbPartition->updateObject(mOwner, item));

        JsonDbPartitionSpec spec = mJsonDbPartition->partitionSpec();
        mJsonDbPartition->close();
        delete mJsonDbPartition;

        mJsonDbPartition = new JsonDbPartition;
        mJsonDbPartition->setPartitionSpec(spec);
        mJsonDbPartition->setDefaultOwner(mOwner);
        mJsonDbPartition->open();

        JsonDbQueryResult queryResult = mJsonDbPartition->queryObjects(mOwner, parsedQuery);
        QCOMPARE(queryResult.data.size(), counter);
    }

    // repeat only reuse the same JsonDbPartition object
    for (int i = 0; i < 10; ++i, ++counter) {
        JsonDbObject item;
        item.insert(QLatin1String("_type"), QLatin1String("reopentest"));
        item.insert("create-string", QString("string"));
        Q_UNUSED(mJsonDbPartition->updateObject(mOwner, item));

        mJsonDbPartition->close();
        mJsonDbPartition->open();

        JsonDbQueryResult queryResult = mJsonDbPartition->queryObjects(mOwner, parsedQuery);
        QCOMPARE(queryResult.data.size(), counter);
    }

    // make sure a closed partition doesn't answer requests
    mJsonDbPartition->close();

    JsonDbObject item1;
    JsonDbWriteResult writeRes = mJsonDbPartition->updateObject(mOwner, item1);
    QCOMPARE(writeRes.code, JsonDbError::PartitionUnavailable);
    JsonDbQueryResult queryRes = mJsonDbPartition->queryObjects(mOwner, parsedQuery);
    QCOMPARE(queryRes.code, JsonDbError::PartitionUnavailable);

    mJsonDbPartition->open();
}

void TestPartition::openTwice()
{
#ifdef Q_OS_WIN32
    QSKIP("flock unimplemented on windows");
#endif
    JsonDbPartitionSpec spec = mJsonDbPartition->partitionSpec();

    JsonDbPartition partition;
    partition.setPartitionSpec(spec);
    partition.setDefaultOwner(mOwner);
    QVERIFY(!partition.open());
}

void TestPartition::createContacts()
{
    if (!mContactList.isEmpty())
        return;

    QVariantList contactList = readJsonFile(":/partition/json/largeContactsTest.json").toArray().toVariantList();
    QList<JsonDbObject> newContactList;
    foreach (QVariant v, contactList) {
        JsonDbObject contact(JsonDbObject::fromVariantMap(v.toMap()));
        QString name = contact.value("name").toString();
        QStringList names = name.split(" ");
        QJsonObject nameObject;
        nameObject.insert("first", names[0]);
        nameObject.insert("last", names[names.size()-1]);
        contact.insert("name", nameObject);
        contact.insert(JsonDbString::kTypeStr, QString("contact"));
        verifyGoodResult(create(mOwner, contact));
        newContactList.append(contact);
    }
    mContactList = newContactList;

    addIndex(QLatin1String("name"));
    addIndex(QLatin1String("name.first"));
    addIndex(QLatin1String("name.last"));
    addIndex(QLatin1String("_type"));

}

JsonDbQueryResult TestPartition::find(JsonDbOwner *owner, const QString &query, const QJsonObject bindings)
{
    JsonDbQueryParser parser;
    parser.setQuery(query);
    parser.setBindings(bindings);
    parser.parse();
    JsonDbQuery parsedQuery = parser.result();
    JsonDbQueryResult result = mJsonDbPartition->queryObjects(owner, parsedQuery);
    return result;
}

JsonDbWriteResult TestPartition::create(JsonDbOwner *owner, JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode)
{
    JsonDbWriteResult result =  mJsonDbPartition->updateObject(owner, object, mode);
    if (result.code == JsonDbError::NoError) {
        object.insert(JsonDbString::kUuidStr, result.objectsWritten[0].uuid().toString());
        object.insert(JsonDbString::kVersionStr, result.objectsWritten[0].version());
    }
    return result;
}

JsonDbWriteResult TestPartition::update(JsonDbOwner *owner, JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode)
{
    JsonDbWriteResult result =  mJsonDbPartition->updateObject(owner, object, mode);
    if (result.code == JsonDbError::NoError)
        object.insert(JsonDbString::kVersionStr, result.objectsWritten[0].version());
    return result;
}

JsonDbWriteResult TestPartition::remove(JsonDbOwner *owner, JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode)
{
    JsonDbObject toDelete = object;
    toDelete.insert(JsonDbString::kDeletedStr, true);
    JsonDbWriteResult result = mJsonDbPartition->updateObject(owner, toDelete, mode);
    if (result.code == JsonDbError::NoError)
        object = result.objectsWritten[0];
    return result;
}

/*
 * const version for all the cleanup code that doesn't care about updated properties
 */
JsonDbWriteResult TestPartition::remove(JsonDbOwner *owner, const JsonDbObject &object, JsonDbPartition::ConflictResolutionMode mode)
{
    JsonDbObject toDelete = object;
    toDelete.insert(JsonDbString::kDeletedStr, true);
    return mJsonDbPartition->updateObject(owner, toDelete, mode);
}

void TestPartition::addSchema(const QString &schemaName, JsonDbObject &schemaObject)
{
    QJsonValue schema = readJsonFile(QString(":/partition/schemas/%1.json").arg(schemaName));
    schemaObject = JsonDbObject();
    schemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    schemaObject.insert("name", schemaName);
    schemaObject.insert("schema", schema);

    verifyGoodResult(create(mOwner, schemaObject));
}

void TestPartition::addIndex(const QString &propertyName, const QString &propertyType, const QString &objectType)
{
    JsonDbObject index;
    index.insert(JsonDbString::kTypeStr, JsonDbString::kIndexTypeStr);
    index.insert(JsonDbString::kNameStr, propertyName);
    index.insert(JsonDbString::kPropertyNameStr, propertyName);
    if (!propertyType.isEmpty())
        index.insert(JsonDbString::kPropertyTypeStr, propertyType);
    if (!objectType.isEmpty())
        index.insert(JsonDbString::kObjectTypeStr, objectType);
    verifyGoodResult(create(mOwner, index));
}

void TestPartition::computeVersion()
{
    JsonDbObject item0;
    QCOMPARE(item0.version(), QString());
    item0.computeVersion();
    QVERIFY(!item0.version().isEmpty());
    QVERIFY(item0.version().startsWith("1-"));
    QCOMPARE(item0.version().size(), 12);

    // check that _uuid and _meta are ignored
    JsonDbObject item1;
    QVERIFY(item1.version().isEmpty());
    item1.insert(JsonDbString::kUuidStr, QString("123"));
    item1.insert(JsonDbString::kMetaStr, QString("789"));
    item1.computeVersion();
    QVERIFY(!item0.isAncestorOf(item1));
    QVERIFY(!item1.isAncestorOf(item0));
    QCOMPARE(item1.version(), item0.version());

    JsonDbObject replay(item1);
    replay.computeVersion();
    QCOMPARE(replay.isAncestorOf(item1), false);
    QCOMPARE(item1.isAncestorOf(replay), false);
    QCOMPARE(replay.version(), item1.version());

    JsonDbObject item2(item1);
    item2.insert(QStringLiteral("key"), QJsonValue::Null);
    item2.computeVersion();
    QVERIFY(item2.version().startsWith("2-"));
    QCOMPARE(item2.version().size(), 12);
    QVERIFY(item2.version().mid(2) != item1.version().mid(2));
    QVERIFY(item1.isAncestorOf(item2));
    QVERIFY(!item2.isAncestorOf(item1));

    JsonDbObject item3(item2);
    item3.insert(QStringLiteral("key"), false);
    item3.computeVersion();
    QVERIFY(item3.version().startsWith("3-"));
    QCOMPARE(item3.version().size(), 12);
    QVERIFY(item3.version().mid(2) != item2.version().mid(2));
    QVERIFY(item1.isAncestorOf(item3));
    QVERIFY(item2.isAncestorOf(item3));
    QVERIFY(!item3.isAncestorOf(item1));
    QVERIFY(!item3.isAncestorOf(item2));

    JsonDbObject item4(item3);
    item4.insert(QStringLiteral("key"), true);
    item4.computeVersion();
    QVERIFY(item4.version().startsWith("4-"));
    QCOMPARE(item4.version().size(), 12);
    QVERIFY(item4.version().mid(2) != item3.version().mid(2));
    QVERIFY(item1.isAncestorOf(item4));
    QVERIFY(item2.isAncestorOf(item4));
    QVERIFY(item3.isAncestorOf(item4));
    QVERIFY(!item4.isAncestorOf(item1));
    QVERIFY(!item4.isAncestorOf(item2));
    QVERIFY(!item4.isAncestorOf(item3));

    JsonDbObject item5(item4);
    item5.insert(QStringLiteral("key"), double(0));
    item5.computeVersion();
    QVERIFY(item5.version().startsWith("5-"));
    QCOMPARE(item5.version().size(), 12);
    QVERIFY(item5.version().mid(2) != item4.version().mid(2));
    QVERIFY(item1.isAncestorOf(item5));
    QVERIFY(item2.isAncestorOf(item5));
    QVERIFY(item3.isAncestorOf(item5));
    QVERIFY(item4.isAncestorOf(item5));
    QVERIFY(!item5.isAncestorOf(item1));
    QVERIFY(!item5.isAncestorOf(item2));
    QVERIFY(!item5.isAncestorOf(item3));
    QVERIFY(!item5.isAncestorOf(item4));

    JsonDbObject item6(item5);
    item6.insert(QStringLiteral("key"), QString());
    item6.computeVersion();
    QVERIFY(item6.version().startsWith("6-"));
    QCOMPARE(item6.version().size(), 12);
    QVERIFY(item6.version().mid(2) != item5.version().mid(2));
    QVERIFY(item1.isAncestorOf(item6));
    QVERIFY(item2.isAncestorOf(item6));
    QVERIFY(item3.isAncestorOf(item6));
    QVERIFY(item4.isAncestorOf(item6));
    QVERIFY(item5.isAncestorOf(item6));
    QVERIFY(!item6.isAncestorOf(item1));
    QVERIFY(!item6.isAncestorOf(item2));
    QVERIFY(!item6.isAncestorOf(item3));
    QVERIFY(!item6.isAncestorOf(item4));
    QVERIFY(!item6.isAncestorOf(item5));

    JsonDbObject item7(item6);
    item7.insert(QStringLiteral("key"), QJsonArray());
    item7.computeVersion();
    QVERIFY(item7.version().startsWith("7-"));
    QCOMPARE(item7.version().size(), 12);
    QVERIFY(item7.version().mid(2) != item6.version().mid(2));
    QVERIFY(item1.isAncestorOf(item7));
    QVERIFY(item2.isAncestorOf(item7));
    QVERIFY(item3.isAncestorOf(item7));
    QVERIFY(item4.isAncestorOf(item7));
    QVERIFY(item5.isAncestorOf(item7));
    QVERIFY(item6.isAncestorOf(item7));
    QVERIFY(!item7.isAncestorOf(item1));
    QVERIFY(!item7.isAncestorOf(item2));
    QVERIFY(!item7.isAncestorOf(item3));
    QVERIFY(!item7.isAncestorOf(item4));
    QVERIFY(!item7.isAncestorOf(item5));
    QVERIFY(!item7.isAncestorOf(item6));

    JsonDbObject item8(item7);
    item8.insert(QStringLiteral("key"), QJsonObject());
    item8.computeVersion();
    QVERIFY(item8.version().startsWith("8-"));
    QCOMPARE(item8.version().size(), 12);
    QVERIFY(item8.version().mid(2) != item7.version().mid(2));
    QVERIFY(item1.isAncestorOf(item8));
    QVERIFY(item2.isAncestorOf(item8));
    QVERIFY(item3.isAncestorOf(item8));
    QVERIFY(item4.isAncestorOf(item8));
    QVERIFY(item5.isAncestorOf(item8));
    QVERIFY(item6.isAncestorOf(item8));
    QVERIFY(item7.isAncestorOf(item8));
    QVERIFY(!item8.isAncestorOf(item1));
    QVERIFY(!item8.isAncestorOf(item2));
    QVERIFY(!item8.isAncestorOf(item3));
    QVERIFY(!item8.isAncestorOf(item4));
    QVERIFY(!item8.isAncestorOf(item5));
    QVERIFY(!item8.isAncestorOf(item6));
    QVERIFY(!item8.isAncestorOf(item7));

    JsonDbObject item9(item8);
    item9.insert(QStringLiteral("key"), QJsonValue::Undefined);
    item9.computeVersion();
    QVERIFY(item9.version().startsWith("9-"));
    QCOMPARE(item9.version().size(), 12);
    QVERIFY(item9.version().mid(2) != item8.version().mid(2));
    QVERIFY(item1.isAncestorOf(item9));
    QVERIFY(item2.isAncestorOf(item9));
    QVERIFY(item3.isAncestorOf(item9));
    QVERIFY(item4.isAncestorOf(item9));
    QVERIFY(item5.isAncestorOf(item9));
    QVERIFY(item6.isAncestorOf(item9));
    QVERIFY(item7.isAncestorOf(item9));
    QVERIFY(item8.isAncestorOf(item9));
    QVERIFY(!item9.isAncestorOf(item1));
    QVERIFY(!item9.isAncestorOf(item2));
    QVERIFY(!item9.isAncestorOf(item3));
    QVERIFY(!item9.isAncestorOf(item4));
    QVERIFY(!item9.isAncestorOf(item5));
    QVERIFY(!item9.isAncestorOf(item6));
    QVERIFY(!item9.isAncestorOf(item7));
    QVERIFY(!item9.isAncestorOf(item8));

    QCOMPARE(item9.version().mid(1), item1.version().mid(1));

    JsonDbObject item10(item9);
    item10.insert(QStringLiteral("key"), QString());
    item10.computeVersion(false);
    QVERIFY(item10.version().startsWith("10-"));
    QVERIFY(!item10.contains(JsonDbString::kMetaStr));

    JsonDbObject localItem(item9);
    localItem.insert(JsonDbString::kLocalStr, true);
    localItem.computeVersion();
    QVERIFY(localItem.version().startsWith("10-"));
    QVERIFY(!localItem.contains(JsonDbString::kMetaStr));
}

void TestPartition::updateVersionOptimistic()
{
    JsonDbObject master;
    master.generateUuid();
    master.computeVersion();

    JsonDbObject target(master);
    JsonDbObject update;

    QString versionWritten;

    // non-matching uuid should fail
    QVERIFY(!target.updateVersionOptimistic(update, &versionWritten));
    QCOMPARE(target, master);

    // replay should succeed but not do anything
    update = master;
    QCOMPARE(target.updateVersionOptimistic(update, &versionWritten), true);
    QCOMPARE(target, master);
    QCOMPARE(versionWritten, target.version());
    update.computeVersion();
    QCOMPARE(versionWritten, update.version());

    update.remove("_meta");
    update.insert(QStringLiteral("update"), true);
    JsonDbObject lateUpdate(master);
    lateUpdate.remove("_meta");
    lateUpdate.insert(QStringLiteral("update"), false);

    QVERIFY(target.updateVersionOptimistic(update, &versionWritten));
    update.computeVersion();
    QCOMPARE(versionWritten, update.version());
    QCOMPARE(target.value(QStringLiteral("update")).toBool(), true);

    QVERIFY(master.isAncestorOf(target));
    QVERIFY(!target.isAncestorOf(master));

    QVERIFY(!target.updateVersionOptimistic(lateUpdate, &versionWritten));
    QCOMPARE(target.version(), update.version());
    QCOMPARE(target.value(QStringLiteral("update")).toBool(), true);
    QVERIFY(master.isAncestorOf(target));
    QVERIFY(!target.isAncestorOf(master));

    lateUpdate.computeVersion();
    QVERIFY(!lateUpdate.isAncestorOf(target));

}

void TestPartition::updateVersionReplicating()
{
    JsonDbObject master;
    master.generateUuid();
    master.computeVersion();

    JsonDbObject target(master);
    JsonDbObject source;

    // add something that doesn't have matching uuid
    QCOMPARE(target.updateVersionReplicating(source), false);
    QCOMPARE(target, master);

    // merge with replay
    source = master;
    QCOMPARE(target.updateVersionReplicating(source), true);
    QCOMPARE(target, master);

    // merge from empty
    {
        JsonDbObject target;
        JsonDbObject source(master);
        QCOMPARE(target.updateVersionReplicating(source), true);
        QCOMPARE(target, master);
    }

    // create a conflict
    JsonDbObject conflict(master);
    source.insert(QStringLiteral("key"), false);
    source.computeVersion();
    conflict.insert(QStringLiteral("key"), true);
    conflict.computeVersion();

    // sanity check which version will win
    // deterministic assumption: source
    // Note: if this ever fails, you modified the test or the version algorithm
    // Changing the algorithm is very bad!
    // If just changing the test, you need to reverse source and conflict in below QCOMPAREs
    QVERIFY(conflict < source);

    // adding order does NOT matter
    QCOMPARE(target.updateVersionReplicating(conflict), true);
    QCOMPARE(target.updateVersionReplicating(source), true);

    // let's proof that
    {
        JsonDbObject reversedMergeOrder(master);
        QCOMPARE(reversedMergeOrder.updateVersionReplicating(source), true);
        QCOMPARE(reversedMergeOrder.updateVersionReplicating(conflict), true);
        QCOMPARE(reversedMergeOrder, target);
    }

    // let's look at target more closely
    QVERIFY(master.isAncestorOf(target));
    QCOMPARE(target.version(), source.version());
    {
        // the reported conflict should equal the original sans meta

        QJsonArray conflicts = target.value(JsonDbString::kMetaStr).toObject().value(JsonDbString::kConflictsStr).toArray();
        JsonDbObject reportedConflict(conflicts.at(0).toObject());
        QCOMPARE(reportedConflict.contains(JsonDbString::kMetaStr), false);

        JsonDbObject checkConflict(conflict);
        checkConflict.remove(JsonDbString::kMetaStr);

        QCOMPARE(reportedConflict, checkConflict);
    }

    master = target;

    // lets progress
    source.insert(QStringLiteral("update"), true);
    source.computeVersion();
    QCOMPARE(target.updateVersionReplicating(source), true);

    QVERIFY(master.isAncestorOf(target));
    QCOMPARE(target.version(), source.version());
    QCOMPARE(target.value(QStringLiteral("update")).toBool(), true);
    {
        // the reported conflict should equal the original sans meta

        QJsonArray conflicts = target.value(JsonDbString::kMetaStr).toObject().value(JsonDbString::kConflictsStr).toArray();
        JsonDbObject reportedConflict(conflicts.at(0).toObject());
        QCOMPARE(reportedConflict.contains(JsonDbString::kMetaStr), false);

        JsonDbObject checkConflict(conflict);
        checkConflict.remove(JsonDbString::kMetaStr);

        QCOMPARE(reportedConflict, checkConflict);
    }

    master = target;

    // lets kill the conflict
    conflict.insert(JsonDbString::kDeletedStr, true);
    JsonDbObject conflictReplay(conflict);

    conflict.computeVersion();
    QCOMPARE(target.updateVersionReplicating(conflict), true);
    {
        QJsonObject meta = target.value(JsonDbString::kMetaStr).toObject();
        QCOMPARE(meta.contains(JsonDbString::kConflictsStr), false);
    }
    QVERIFY(conflict.isAncestorOf(target));
    QVERIFY(!target.isAncestorOf(conflict));
    QCOMPARE(target.value(QStringLiteral("update")).toBool(), true);
    QCOMPARE(target.version(), source.version());

    // conflict removal should be possible by updateVersionOptimistic()
    JsonDbObject update(master);
    QString versionWritten;
    QVERIFY(update.updateVersionOptimistic(conflictReplay, &versionWritten));
    QCOMPARE(update, target);
    QCOMPARE(versionWritten, conflict.version());
}

/*
 * Create an item
 */
void TestPartition::create()
{
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QString("create-test-type"));
    item.insert("create-test", 22);
    item.insert("create-string", QString("string"));

    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    QJsonObject map = result.objectsWritten[0];
    QVERIFY(map.contains(JsonDbString::kUuidStr));
    QVERIFY(!map.value(JsonDbString::kUuidStr).toString().isEmpty());
    QCOMPARE(map.value(JsonDbString::kUuidStr).type(), QJsonValue::String);

    QVERIFY(map.contains(JsonDbString::kVersionStr));
    QVERIFY(!map.value(JsonDbString::kVersionStr).toString().isEmpty());
    QCOMPARE(map.value(JsonDbString::kVersionStr).type(), QJsonValue::String);

    QString querystr(QString("[?_uuid=\"%1\"]").arg(map.value(JsonDbString::kUuidStr).toString()));
    JsonDbQueryResult findResult = find(mOwner, querystr);
    QCOMPARE(findResult.data.size(), 1);
    QCOMPARE(findResult.data.at(0).value(JsonDbString::kUuidStr).toString(), map.value(JsonDbString::kUuidStr).toString());
    QCOMPARE(findResult.data.at(0).value(JsonDbString::kVersionStr).toString(), map.value(JsonDbString::kVersionStr).toString());
}


/*
 * Insert an item and then update it.
 */

void TestPartition::update()
{
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("update-test-type"));
    item.insert("update-test", 100);
    item.insert("update-string", QLatin1String("update-test-100"));

    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);
    QString uuid = result.objectsWritten[0].uuid().toString();

    item.insert(JsonDbString::kUuidStr, uuid);
    item.insert("update-test", 101);

    result = update(mOwner, item);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), 1);
}

/*
 * Update an item which doesn't exist
 */

void TestPartition::update2()
{
    JsonDbObject item;
    item.insert("update2-test", 100);
    item.insert("_type", QString("update-from-null"));
    item.generateUuid();

    JsonDbWriteResult result = update(mOwner, item);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), 1);
}

/*
 * Update an item which doesn't have a "uuid" field
 */

void TestPartition::update3()
{
    JsonDbObject item;
    item.insert("update2-test", 100);

    JsonDbWriteResult result = update(mOwner, item);
    verifyErrorResult(result);
}

/*
 * Update a stale copy of an item
 */

void TestPartition::update4()
{
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("update-test-type"));
    item.insert("update-test", 100);
    item.insert("update-string", QLatin1String("update-test-100"));

    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);
    QString uuid = result.objectsWritten[0].uuid().toString();

    QString version1 = result.objectsWritten[0].version();

    item.insert(JsonDbString::kUuidStr, uuid);
    JsonDbObject replay(item);

    item.insert(JsonDbString::kVersionStr, version1);


    item.insert("update-test", 101);

    result = update(mOwner, item);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), 1);
    QCOMPARE(result.objectsWritten[0].uuid().toString(), uuid);

    QString version2 = result.objectsWritten[0].version();
    QVERIFY(version1 != version2);

    JsonDbQueryResult queryResult = find(mOwner, QString("[?_uuid=\"%1\"]").arg(uuid));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value(JsonDbString::kUuidStr).toString(), uuid);
    QCOMPARE(queryResult.data.at(0).value(JsonDbString::kVersionStr).toString(), version2);

    item.insert("update-test", 202);
    item.insert("_version", version2);
    result = update(mOwner, item);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), 1);
    QCOMPARE(result.objectsWritten[0].uuid().toString(), uuid);
    QString version3 = result.objectsWritten[0].version();
    QVERIFY(version3 != version2);

    // replay
    result = update(mOwner, replay);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), 1);
    QCOMPARE(result.objectsWritten[0].uuid().toString(), uuid);
    QCOMPARE(result.objectsWritten[0].version(), version1);

    // conflict
    item.insert(JsonDbString::kVersionStr, version2);
    item.insert("update-test", 102);
    result = update(mOwner, item);
    verifyErrorResult(result);
}

/*
 * Create an item and immediately remove it
 */

void TestPartition::remove()
{
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("remove-test-type"));
    item.insert("remove-test", 100);

    JsonDbWriteResult response = create(mOwner, item);
    verifyGoodResult(response);

    QString uuid = response.objectsWritten[0].uuid().toString();
    QString version = response.objectsWritten[0].version();

    item.insert(JsonDbString::kUuidStr, uuid);
    //item.insert(JsonDbString::kVersionStr, version);

    response = remove(mOwner, item);
    verifyGoodResult(response);
    QCOMPARE(response.objectsWritten.count(), 1);

    JsonDbQueryResult queryResult = find(mOwner, QString("[?_uuid=\"%1\"]").arg(uuid));
    QCOMPARE(queryResult.data.size(), 0);
}

/*
 * Try to remove an item which doesn't exist
 */

void TestPartition::remove2()
{
    JsonDbObject item;
    item.insert("remove2-test", 100);
    item.insert(JsonDbString::kTypeStr, QLatin1String("Remove2Type"));
    item.insert(JsonDbString::kUuidStr, QUuid::createUuid().toString());

    JsonDbWriteResult result = remove(mOwner, item);
    verifyErrorResult(result);
}

/*
 * Don't include a 'uuid' field
 */

void TestPartition::remove3()
{
    JsonDbObject item;
    item.insert("remove3-test", 100);

    JsonDbWriteResult result = remove(mOwner, item);
    verifyErrorResult(result);
}

/*
 * Try to remove an item which existed before but was removed
 */
void TestPartition::remove4()
{
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("remove-test-type"));
    item.insert("remove-test", 100);

    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);
    QString uuid = result.objectsWritten[0].uuid().toString();
    item.insert(JsonDbString::kUuidStr, uuid);

    result = remove(mOwner, item);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), 1);

    result = remove(mOwner, item);
    verifyErrorResult(result);
    result = remove(mOwner, item);
    verifyErrorResult(result);
}

/*
 * Remove a stale version of the object
 */
void TestPartition::remove5()
{
    jsondbSettings->setRejectStaleUpdates(true);

    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("update-test-type"));

    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    QString version = item.take(JsonDbString::kVersionStr).toString();
    result = remove(mOwner, item);
    verifyErrorResult(result);

    item.insert(JsonDbString::kVersionStr, version);
    result = remove(mOwner, item);
    verifyGoodResult(result);

    jsondbSettings->setRejectStaleUpdates(false);
}

void TestPartition::tombstoneResurrection()
{
    jsondbSettings->setRejectStaleUpdates(true);

    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("zombie"));

    // first we create an object;
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);
    QVERIFY(item.version().startsWith(QLatin1String("1-")));

    // remove it again
    result = remove(mOwner, item);
    verifyGoodResult(result);
    QVERIFY(item.version().startsWith(QLatin1String("2-")));
    QVERIFY(item.isDeleted());

    // resurrect it
    JsonDbObject newItem;
    newItem.insert(JsonDbString::kTypeStr, QLatin1String("reborn"));
    newItem.insert(JsonDbString::kUuidStr, item.value(JsonDbString::kUuidStr));

    result = update(mOwner, newItem);
    verifyGoodResult(result);
    QVERIFY(newItem.version().startsWith(QLatin1String("3-")));

    JsonDbQueryResult queryResult = find(mOwner, QString("[?_uuid=\"%1\"]").arg(newItem.uuid().toString()));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    JsonDbObject reread = queryResult.data.at(0);
    QVERIFY(item.isAncestorOf(reread));
}

void TestPartition::schemaValidation_data()
{
    QTest::addColumn<QByteArray>("schema");
    QTest::addColumn<QByteArray>("object");
    QTest::addColumn<bool>("result"); // is an object valid against a schema?

    QByteArray schema, object;
    bool result;

    const QString dataDir = QString::fromLatin1(":/json-validation/");
    QDir dir(dataDir);
    QStringList schemaNames = dir.entryList(QStringList() << "*-schema.json");

    Q_ASSERT_X(schemaNames.count(), Q_FUNC_INFO, "Tests not found");

    foreach (const QString &schemaFileName, schemaNames) {
        QFile schemeFile(dataDir + schemaFileName);
        schemeFile.open(QIODevice::ReadOnly);
        schema = schemeFile.readAll();
        const QString testName = schemaFileName.mid(0, schemaFileName.length() - 12); // chop "-schema.json"
        QStringList tests = dir.entryList(QStringList() << (testName + "*"));
        foreach (const QString &test, tests) {
            if (test.endsWith("-schema.json"))
                continue;

            QFile objectFile(dataDir + test);
            objectFile.open(QIODevice::ReadOnly);
            object = objectFile.readAll();
            result = !test.endsWith("invalid.json");
            QTest::newRow(test.toLatin1().data()) << schema << object << result;
        }
    }
}

void TestPartition::schemaValidation()
{
    bool isValidated = jsondbSettings->validateSchemas();
    jsondbSettings->setValidateSchemas(true);

    QFETCH(QByteArray, schema);
    QFETCH(QByteArray, object);
    QFETCH(bool, result);

    static uint id = 0;
    id++;
    QString schemaName = QLatin1String("schemaValidationSchema") + QString::number(id);

    QJsonObject schemaBody = QJsonDocument::fromJson(schema).object();
    JsonDbObject schemaObject;
    schemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    schemaObject.insert("name", schemaName);
    schemaObject.insert("schema", schemaBody);

    JsonDbWriteResult qResult = create(mOwner, schemaObject);
    verifyGoodResult(qResult);

    JsonDbObject item = QJsonDocument::fromJson(object).object();
    item.insert(JsonDbString::kTypeStr, schemaName);

    // Create an item that matches the schema
    qResult = create(mOwner, item);
    if (result) {
        verifyGoodResult(qResult);

    } else {
        verifyErrorResult(qResult);
    }
    if (result) {
        qResult = remove(mOwner, item);
        verifyGoodResult(qResult);
    }
    qResult = remove(mOwner, schemaObject);
    verifyGoodResult(qResult);

    jsondbSettings->setValidateSchemas(isValidated);
}

void TestPartition::schemaValidationExtends_data()
{
    QTest::addColumn<QByteArray>("item");
    QTest::addColumn<bool>("isPerson");
    QTest::addColumn<bool>("isAdult");

    QTest::newRow("empty")
            << QByteArray("{}") << true << true;
    QTest::newRow("40 and 4")
            << QByteArray("{ \"name\":\"40 and 4\" }") << true << true;
    QTest::newRow("Alice")
            << QByteArray("{ \"name\":\"Alice\", \"age\": 5}")  << true << false;
    QTest::newRow("Alice's mother")
            << QByteArray("{ \"name\":\"Alice's mother\", \"age\": 32}") << true << true;
    QTest::newRow("Alice's grandmother")
            << QByteArray("{ \"name\":\"Alice's grandmother\", \"age\": 100}") << true << true;
    QTest::newRow("Alice's great-grandmother")
            << QByteArray("{ \"name\":\"Alice's great-grandmother\", \"age\": 130}") << false << false;
}

void TestPartition::schemaValidationExtends()
{
    jsondbSettings->setValidateSchemas(true);

    QFETCH(QByteArray, item);
    QFETCH(bool, isPerson);
    QFETCH(bool, isAdult);

    static int id = 0; // schema name id
    ++id;
    const QString personSchemaName = QString::fromLatin1("person") + QString::number(id);
    const QString adultSchemaName = QString::fromLatin1("adult") + QString::number(id);

    // init schemas
    JsonDbWriteResult qResult;
    {
        const QByteArray person =
                "{"
                "    \"description\": \"A person\","
                "    \"type\": \"object\","
                "    \"properties\": {"
                "        \"name\": {\"type\": \"string\"},"
                "        \"age\" : {\"type\": \"integer\", \"maximum\": 125 }"
                "    }"
                "}";
        const QByteArray adult = QString::fromLatin1(
                "{"
                "    \"description\": \"An adult\","
                "    \"properties\": {\"age\": {\"minimum\": 18}},"
                "    \"extends\": {\"$ref\":\"person%1\"}"
                "}").arg(QString::number(id)).toLatin1();
        QJsonObject personSchemaBody = QJsonDocument::fromJson(person).object();
        QJsonObject adultSchemaBody = QJsonDocument::fromJson(adult).object();

        JsonDbObject personSchemaObject;
        personSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
        personSchemaObject.insert("name", personSchemaName);
        personSchemaObject.insert("schema", personSchemaBody);

        JsonDbObject adultSchemaObject;
        adultSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
        adultSchemaObject.insert("name", adultSchemaName);
        adultSchemaObject.insert("schema", adultSchemaBody);

        qResult = create(mOwner, personSchemaObject);
        verifyGoodResult(qResult);
        qResult = create(mOwner, adultSchemaObject);
        verifyGoodResult(qResult);
    }

    {
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert("testingForPerson", isPerson);
        object.insert(JsonDbString::kTypeStr, personSchemaName);
        qResult = create(mOwner, object);
        if (isPerson) {
            verifyGoodResult(qResult);
        } else {
            verifyErrorResult(qResult);
        }
    }

    {
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert("testingForAdult", isAdult);
        object.insert(JsonDbString::kTypeStr, adultSchemaName);
        qResult = create(mOwner, object);
        if (isAdult) {
            verifyGoodResult(qResult);
        } else {
            verifyErrorResult(qResult);
        }
    }

    jsondbSettings->setValidateSchemas(false);
}


void TestPartition::schemaValidationExtendsArray_data()
{
    QTest::addColumn<QByteArray>("item");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("empty")
            << QByteArray("{}") << false;
    QTest::newRow("fiat 125p")
            << QByteArray("{ \"landSpeed\": 5}")  << false;
    QTest::newRow("bavaria 44")
            << QByteArray("{ \"waterSpeed\": 6}") << false;
    QTest::newRow("orukter amphibolos")
            << QByteArray("{ \"waterSpeed\":10, \"landSpeed\": 10}") << true;
    QTest::newRow("batmans car")
            << QByteArray("{ \"waterSpeed\":100, \"landSpeed\": 100, \"airSpeed\": 100}") << true;
}

void TestPartition::schemaValidationExtendsArray()
{
    jsondbSettings->setValidateSchemas(true);

    QFETCH(QByteArray, item);
    QFETCH(bool, isValid);

    static int id = 0; // schema name id
    ++id;
    const QString amphibiousSchemaName = QString::fromLatin1("amphibious") + QString::number(id);
    const QString carSchemaName = QString::fromLatin1("car") + QString::number(id);
    const QString boatSchemaName = QString::fromLatin1("boat") + QString::number(id);

    // init schemas
    JsonDbWriteResult qResult;
    {
        const QByteArray car =
                "{"
                "    \"description\": \"A car\","
                "    \"properties\": {\"landSpeed\": {\"required\": true}}"
                "}";

        const QByteArray boat =
                "{"
                "    \"description\": \"A boat\","
                "    \"properties\": {\"waterSpeed\": {\"required\": true}}"
                "}";

        const QByteArray amphibious = QString::fromLatin1(
                "{"
                "    \"description\": \"A amphibious\","
                "    \"extends\": [{\"$ref\":\"car%1\"}, {\"$ref\":\"boat%1\"}]"
                "}").arg(QString::number(id)).toLatin1();

        QJsonObject amphibiousSchemaBody = QJsonDocument::fromJson(amphibious).object();
        QJsonObject carSchemaBody = QJsonDocument::fromJson(car).object();
        QJsonObject boatSchemaBody = QJsonDocument::fromJson(boat).object();


        JsonDbObject carSchemaObject;
        carSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
        carSchemaObject.insert("name", carSchemaName);
        carSchemaObject.insert("schema", carSchemaBody);

        JsonDbObject boatSchemaObject;
        boatSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
        boatSchemaObject.insert("name", boatSchemaName);
        boatSchemaObject.insert("schema", boatSchemaBody);

        JsonDbObject amphibiousSchemaObject;
        amphibiousSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
        amphibiousSchemaObject.insert("name", amphibiousSchemaName);
        amphibiousSchemaObject.insert("schema", amphibiousSchemaBody);

        qResult = create(mOwner, carSchemaObject);
        verifyGoodResult(qResult);
        qResult = create(mOwner, boatSchemaObject);
        verifyGoodResult(qResult);
        qResult = create(mOwner, amphibiousSchemaObject);
        verifyGoodResult(qResult);
    }

    {
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert("testingForAmphibious", isValid);
        object.insert(JsonDbString::kTypeStr, amphibiousSchemaName);
        qResult = create(mOwner, object);
        if (isValid) {
            verifyGoodResult(qResult);
        } else {
            verifyErrorResult(qResult);
        }
    }

    jsondbSettings->setValidateSchemas(false);
}

void TestPartition::schemaValidationLazyInit()
{
    jsondbSettings->setValidateSchemas(true);

    const QByteArray person =
            "{"
            "    \"description\": \"A person\","
            "    \"type\": \"object\","
            "    \"properties\": {"
            "        \"name\": {\"type\": \"string\", \"required\": true},"
            "        \"age\" : {\"type\": \"integer\", \"maximum\": 125 }"
            "    }"
            "}";
    const QByteArray adult =
            "{"
            "    \"description\": \"An adult\","
            "    \"properties\": {\"age\": {\"minimum\": 18}},"
            "    \"extends\": {\"$ref\":\"personLazyInit\"}"
            "}";

    const QString personSchemaName = QString::fromLatin1("personLazyInit");
    const QString adultSchemaName = QString::fromLatin1("adultLazyInit");

    QJsonObject personSchemaBody = QJsonDocument::fromJson(person).object();
    QJsonObject adultSchemaBody = QJsonDocument::fromJson(adult).object();

    JsonDbObject personSchemaObject;
    personSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    personSchemaObject.insert("name", personSchemaName);
    personSchemaObject.insert("schema", personSchemaBody);

    JsonDbObject adultSchemaObject;
    adultSchemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    adultSchemaObject.insert("name", adultSchemaName);
    adultSchemaObject.insert("schema", adultSchemaBody);

    // Without lazy compilation this operation fails, because adult schema referece unexisting yet
    // person schema
    JsonDbWriteResult qResult;
    qResult = create(mOwner, adultSchemaObject);
    verifyGoodResult(qResult);
    qResult = create(mOwner, personSchemaObject);
    verifyGoodResult(qResult);

    // Insert some objects to force full schema compilation
    {
        const QByteArray item = "{ \"name\":\"Nierob\", \"age\":99 }";
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert(JsonDbString::kTypeStr, adultSchemaName);
        qResult = create(mOwner, object);
        verifyGoodResult(qResult);
    }
    {
        const QByteArray item = "{ \"name\":\"Nierob\", \"age\":12 }";
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert(JsonDbString::kTypeStr, adultSchemaName);
        qResult = create(mOwner, object);
        verifyErrorResult(qResult);
    }
    {
        const QByteArray item = "{ \"age\":19 }";
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert(JsonDbString::kTypeStr, adultSchemaName);
        qResult = create(mOwner, object);
        verifyErrorResult(qResult);
    }
    {
        const QByteArray item = "{ \"name\":\"Nierob\", \"age\":12 }";
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert(JsonDbString::kTypeStr, personSchemaName);
        qResult = create(mOwner, object);
        verifyGoodResult(qResult);
    }
    {
        const QByteArray item = "{ \"age\":12 }";
        JsonDbObject object = QJsonDocument::fromJson(item).object();
        object.insert(JsonDbString::kTypeStr, personSchemaName);
        qResult = create(mOwner, object);
        verifyErrorResult(qResult);
    }

    jsondbSettings->setValidateSchemas(false);
}


/*
 * Create a list of items
 */

#define LIST_TEST_ITEMS 6

void TestPartition::createList()
{
    JsonDbObjectList list;
    for (int i = 0 ; i < LIST_TEST_ITEMS ; i++ ) {
        JsonDbObject map;
        map.insert(JsonDbString::kTypeStr, QLatin1String("create-list-type"));
        map.insert("create-list-test", i + 100);
        list.append(map);
    }

    JsonDbWriteResult result = mJsonDbPartition->updateObjects(mOwner, list);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), LIST_TEST_ITEMS);
}
/*
 * Create a list of items and then update them
 */

void TestPartition::updateList()
{
    JsonDbObjectList list;
    for (int i = 0 ; i < LIST_TEST_ITEMS ; i++ ) {
        JsonDbObject map;
        map.insert(JsonDbString::kTypeStr, QLatin1String("update-list-type"));
        map.insert("update-list-test", i + 100);
        JsonDbWriteResult result = create(mOwner, map);
        verifyGoodResult(result);
        QString uuid = result.objectsWritten[0].uuid().toString();
        map.insert(JsonDbString::kUuidStr, uuid);
        map.insert("fuzzyduck", QLatin1String("Duck test"));
        list.append(map);
    }

    JsonDbWriteResult result = mJsonDbPartition->updateObjects(mOwner, list);
    verifyGoodResult(result);
    QCOMPARE(result.objectsWritten.count(), LIST_TEST_ITEMS);
}

void TestPartition::mapDefinition()
{
    // we need a schema that extends View for our targetType
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QLatin1String("MyViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QLatin1String("object"));
    schemaSub.insert("extends", QLatin1String("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject mapDefinition;
    mapDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Map"));
    mapDefinition.insert("targetType", QLatin1String("MyViewType"));
    JsonDbObject sourceToMapFunctions;
    sourceToMapFunctions.insert("Contact", QLatin1String("function map (c) { }"));
    mapDefinition.insert("map", sourceToMapFunctions);
    JsonDbWriteResult res = create(mOwner, mapDefinition);
    verifyGoodResult(res);

    verifyGoodResult(remove(mOwner, mapDefinition));
    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::mapDefinitionInvalid()
{
    // we need a schema that extends View for our targetType
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QLatin1String("MyViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QLatin1String("object"));
    schemaSub.insert("extends", QLatin1String("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject mapDefinition, map;
    mapDefinition.insert(JsonDbString::kTypeStr, JsonDbString::kMapTypeStr);
    mapDefinition.insert("sourceType", QLatin1String("Contact"));
    mapDefinition.insert("map", QLatin1String("function map (c) { }"));
    JsonDbWriteResult res = create(mOwner, mapDefinition);
    verifyErrorResult(res);

    mapDefinition = JsonDbObject();
    mapDefinition.insert(JsonDbString::kTypeStr, JsonDbString::kMapTypeStr);
    mapDefinition.insert("targetType", QLatin1String("MyViewType"));
    mapDefinition.insert("map", QLatin1String("function map (c) { }"));
    res = create(mOwner, mapDefinition);
    verifyErrorResult(res);

    mapDefinition = JsonDbObject();
    mapDefinition.insert(JsonDbString::kTypeStr, JsonDbString::kMapTypeStr);
    mapDefinition.insert("targetType", QLatin1String("MyViewType"));
    mapDefinition.insert("sourceType", QLatin1String("Contact"));
    res = create(mOwner, mapDefinition);
    verifyErrorResult(res);

    // fail because targetType doesn't extend View
    mapDefinition = JsonDbObject();
    mapDefinition.insert(JsonDbString::kTypeStr, JsonDbString::kMapTypeStr);
    mapDefinition.insert("targetType", QLatin1String("MyViewType2"));
    map.insert(QLatin1String("Contact"), QLatin1String("function map (c) { }"));
    mapDefinition.insert("map", map);
    res = create(mOwner, mapDefinition);
    verifyErrorResult(res);
    QVERIFY(res.message.contains("View"));

    res = remove(mOwner, schema);
    verifyGoodResult(res);
}

void TestPartition::reduceDefinition()
{
    // we need a schema that extends View for our targetType
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QLatin1String("MyViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QLatin1String("object"));
    schemaSub.insert("extends", QLatin1String("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject reduceDefinition;
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    JsonDbWriteResult res = create(mOwner, reduceDefinition);
    verifyGoodResult(res);

    verifyGoodResult(remove(mOwner, reduceDefinition));
    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::reduceDefinitionInvalid()
{
    // we need a schema that extends View for our targetType
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QLatin1String("MyViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QLatin1String("object"));
    schemaSub.insert("extends", QLatin1String("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject reduceDefinition;
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    JsonDbWriteResult res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    reduceDefinition = JsonDbObject();
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    reduceDefinition = JsonDbObject();
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    reduceDefinition = JsonDbObject();
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    reduceDefinition = JsonDbObject();
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    // fail because targetType doesn't extend View
    reduceDefinition = JsonDbObject();
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType2"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);
    QVERIFY(res.message.contains("View"));

    // fail because targetValue name is not a string or null
    reduceDefinition = JsonDbObject();
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { }"));
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    reduceDefinition.insert("targetValueName", true);
    res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    //schemaRes.value("result").toObject()
    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::mapInvalidMapFunc()
{
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QLatin1String("InvalidMapViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QLatin1String("object"));
    schemaSub.insert("extends", QLatin1String("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject mapDefinition;
    mapDefinition.insert(JsonDbString::kTypeStr, JsonDbString::kMapTypeStr);
    mapDefinition.insert("targetType", QLatin1String("InvalidMapViewType"));
    JsonDbObject sourceToMapFunctions;
    sourceToMapFunctions.insert("Contact", QLatin1String("function map (c) { ;")); // non-parsable map function
    mapDefinition.insert("map", sourceToMapFunctions);

    JsonDbWriteResult defRes = create(mOwner, mapDefinition);
    verifyErrorResult(defRes);
    QCOMPARE(defRes.objectsWritten.size(), 0);

    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::reduceInvalidAddSubtractFuncs()
{
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QLatin1String("MyViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QLatin1String("object"));
    schemaSub.insert("extends", QLatin1String("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject reduceDefinition;
    reduceDefinition.insert(JsonDbString::kTypeStr, QLatin1String("Reduce"));
    reduceDefinition.insert("targetType", QLatin1String("MyViewType"));
    reduceDefinition.insert("sourceType", QLatin1String("Contact"));
    reduceDefinition.insert("sourceKeyName", QLatin1String("phoneNumber"));
    reduceDefinition.insert("add", QLatin1String("function add (k, z, c) { ;")); // non-parsable add function
    reduceDefinition.insert("subtract", QLatin1String("function subtract (k, z, c) { }"));
    JsonDbWriteResult res = create(mOwner, reduceDefinition);
    verifyErrorResult(res);

    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::map()
{
    addIndex(QLatin1String("phoneNumber"));
    QJsonArray objects(readJsonFile(":/partition/json/map-reduce.json").toArray());

    JsonDbObjectList mapsReduces;
    JsonDbObjectList schemas;
    QMap<QString, JsonDbObject> toDelete;
    for (int i = 0; i < objects.size(); ++i) {
        QJsonObject object(objects.at(i).toObject());
        JsonDbObject doc(object);
        JsonDbWriteResult result = create(mOwner, doc);
        verifyGoodResult(result);

        if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr ||
            object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kReduceTypeStr)
            mapsReduces.append(doc);
        else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kSchemaTypeStr)
            schemas.append(doc);
        else
            toDelete.insert(doc.value("_uuid").toString(), doc);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"PhoneCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 5);

    queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 5);

    // now remove one of the source items
    queryResult = find(mOwner, QLatin1String("[?_type=\"Contact\"][?displayName=\"Nancy Doe\"]"));
    JsonDbObject firstItem = queryResult.data.at(0);
    QVERIFY(!firstItem.value("_uuid").toString().isEmpty());
    toDelete.remove(firstItem.value("_uuid").toString());
    JsonDbWriteResult result = remove(mOwner, firstItem);
    verifyGoodResult(result);

    // get results with getObjects()
    GetObjectsResult getObjectsResult = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Phone"));
    if (jsondbSettings->verbose()) {
        JsonDbObjectList vs = getObjectsResult.data;
        for (int i = 0; i < vs.size(); i++)
            qDebug() << "    " << vs[i];
    }
    QCOMPARE(getObjectsResult.data.size(), 3);

    // query for results
    queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 3);

    queryResult = find(mOwner, QLatin1String("[?_type=\"PhoneCount\"][/key]"));
    verifyGoodQueryResult(queryResult);
    if (jsondbSettings->verbose()) {
        JsonDbObjectList vs = queryResult.data;
        for (int i = 0; i < vs.size(); i++)
            qDebug() << "    " << vs[i];
    }
    QCOMPARE(queryResult.data.size(), 3);

    QSet<QString> versions;
    GetObjectsResult gor = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("PhoneCount"));
    QCOMPARE(gor.data.size(), 3);
    foreach (const JsonDbObject &o, gor.data)
        versions.insert(o.value(JsonDbString::kVersionStr).toString());

    // now add a phone number to one of the source items
    QString query2 = QLatin1String("[?_type=\"Contact\"][?displayName=\"Joe Smith\"]");
    queryResult = find(mOwner, query2);
    verifyGoodQueryResult(queryResult);
    firstItem = queryResult.data.at(0);
    QJsonObject newInfo;
    newInfo.insert(QLatin1String("type"), QLatin1String("satellite"));
    newInfo.insert(QLatin1String("number"), QLatin1String("+22-555222222"));
    QJsonArray phoneNumbers = firstItem.value("phoneNumbers").toArray();
    phoneNumbers.append(newInfo);
    firstItem.insert("phoneNumbers", phoneNumbers);

    result = update(mOwner, firstItem);
    verifyGoodResult(result);
    toDelete[firstItem.uuid().toString()] = firstItem;

    gor = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("PhoneCount"));
    QCOMPARE(gor.data.size(), 4);
    // verify only one object got updated
    int numChanges = 0;
    foreach (const JsonDbObject &o, gor.data)
        if (!versions.contains(o.value(JsonDbString::kVersionStr).toString()))
            numChanges++;
    QCOMPARE(numChanges, 1);

    for (int i = 0; i < mapsReduces.size(); ++i) {
        JsonDbObject object = mapsReduces.at(i);
        verifyGoodResult(remove(mOwner, object));
    }
    for (int i = 0; i < schemas.size(); ++i) {
        JsonDbObject object = schemas.at(i);
        verifyGoodResult(remove(mOwner, object));
    }
    foreach (JsonDbObject del, toDelete.values())
        verifyGoodResult(remove(mOwner, del));
    //mJsonDbPartition->removeIndex(QLatin1String("phoneNumber"));
}


void TestPartition::mapDuplicateSourceAndTarget()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-sametarget.json").toArray());
    JsonDbObjectList toDelete;
    JsonDbObjectList maps;

    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Map")
            maps.append(object);
        else
            toDelete.append(object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"ContactView\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 4);

    int firstNameCount = 0;
    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++)
        if (!data.at(ii).value("firstName").toString().isEmpty())
            firstNameCount++;
    QCOMPARE(firstNameCount, 2);

    for (int ii = 0; ii < maps.size(); ii++)
        verifyGoodResult(remove(mOwner, maps.at(ii)));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
    mJsonDbPartition->d_func()->removeIndex("ContactView");
}

void TestPartition::mapRemoval()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-sametarget.json").toArray());

    QList<JsonDbObject> maps;
    QList<JsonDbObject> toDelete;

    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Map")
            maps.append(object);
        else
            toDelete.append(object);
    }

    QLatin1String query("[?_type=\"ContactView\"]");
    JsonDbQueryResult queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 4);

    // remove a map
    JsonDbWriteResult result = remove(mOwner, maps.takeAt(0));
    verifyGoodResult(result);
    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    // an now the other
    result = remove(mOwner, maps.takeAt(0));
    verifyGoodResult(result);
    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    for (int ii = 0; ii < maps.size(); ii++)
        verifyGoodResult(remove(mOwner, maps.at(ii)));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
}

void TestPartition::mapUpdate()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-sametarget.json").toArray());

    JsonDbObjectList maps;
    JsonDbObjectList toDelete;

    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Map")
            maps.append(object);
        else
            toDelete.append(object);
    }

    QLatin1String query("[?_type=\"ContactView\"]");
    JsonDbQueryResult queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 4);

    // remove a map
    JsonDbObject map1 = maps.takeAt(1);
    JsonDbWriteResult result1 = remove(mOwner, map1);
    verifyGoodResult(result1);

    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    // update a map
    JsonDbObject map0 = maps.takeAt(0);
    QJsonObject sourceMap = map0.value("map").toObject();
    sourceMap.insert("Contact3", sourceMap.value("Contact2"));;
    map0.insert("map", sourceMap);
    JsonDbWriteResult result0 = update(mOwner, map0);
    verifyGoodResult(result0);

    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 4);

    // update the targetType
    map0.insert("targetType", QLatin1String("ContactView2"));
    result0 = update(mOwner, map0);
    verifyGoodResult(result0);
    maps.prepend(map0);

    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactView2\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 4);

    for (int ii = 0; ii < maps.size(); ii++)
        verifyGoodResult(remove(mOwner, maps.at(ii)));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));

}

void TestPartition::mapJoin()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-join.json").toArray());

    JsonDbObject join;
    JsonDbObject schema;
    JsonDbObjectList people;

    for (int i = 0; i < objects.size(); ++i) {
        JsonDbObject object(objects.at(i).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        object.insert(JsonDbString::kUuidStr, result.objectsWritten[0].uuid().toString());

        if (object.value(JsonDbString::kTypeStr).toString() == "Map")
            join = object;
        else if (object.value(JsonDbString::kTypeStr).toString() == "_schemaType")
            schema = object;
        else
            people.append(object);
    }
    addIndex("friend", "string", "FoafPerson");
    addIndex("foaf", "string", "FoafPerson");
    addIndex("friend", "string", "Person");

    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("FoafPerson"));
    QCOMPARE(getObjects.data.size(), 0);

    // set some friends
    QString previous;
    for (int i = 0; i < people.size(); ++i) {
        JsonDbObject person = people.at(i);
        if (!previous.isEmpty())
            person.insert("friend", previous);

        previous = person.value(JsonDbString::kUuidStr).toString();
        QCOMPARE(person.value(JsonDbString::kTypeStr).toString(), QLatin1String("Person"));
        verifyGoodResult(update(mOwner, person));
    }

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Person"));
    JsonDbObjectList peopleWithFriends = getObjects.data;

    // sort the list by key to make ordering deterministic
    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"FoafPerson\"][?foaf exists][/friend]"));
    QCOMPARE(queryResult.data.size(), people.size()-2); // one has no friend and one is friends with the one with no friends

    JsonDbObjectList resultList = queryResult.data;
    for (int i = 0; i < resultList.size(); ++i) {
        JsonDbObject person = resultList.at(i);
        QJsonArray sources = person.value("_sourceUuids").toArray();
        QVERIFY(sources.size() == 2 || sources.contains(person.value("friend")));
    }

    // take the last person, find his friend, and remove that friend's friend property
    // then make sure the foaf is updated
    JsonDbObject p = resultList.at(resultList.size()-1);
    QVERIFY(p.contains("foaf"));

    JsonDbObject fr;
    for (int i = 0; i < peopleWithFriends.size(); ++i) {
        JsonDbObject f = peopleWithFriends.at(i);
        if (f.value(JsonDbString::kUuidStr).toString() == p.value("friend").toString()) {
            fr = f;
            break;
        }
    }

    QVERIFY(fr.value(JsonDbString::kUuidStr).toString() == p.value("friend").toString());
    fr.insert("friend", QJsonValue(QJsonValue::Undefined));
    verifyGoodResult(update(mOwner, fr));

    GetObjectsResult foafRes = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("FoafPerson"));
    JsonDbObjectList foafs = foafRes.data;
    QString query = QString("[?_type=\"FoafPerson\"][?name=\"%1\"]").arg(p.value("name").toString());

    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    p = queryResult.data.at(0);
    QVERIFY(!p.value("friend").toString().isEmpty());
    QVERIFY(!p.contains("foaf"));

    queryResult = find(mOwner, QLatin1String("[?_type=\"FoafPerson\"][/key]"));
    verifyGoodQueryResult(queryResult);

    queryResult = find(mOwner, QLatin1String("[?_type=\"FoafPerson\"][/friend]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 8); // two have no friends

    verifyGoodResult(remove(mOwner, join));
    verifyGoodResult(remove(mOwner, schema));
    for (int i = 0; i < people.size(); ++i) {
        JsonDbObject object = people.at(i);
        if (object == join || object == schema)
            continue;
        remove(mOwner, object);
    }
    mJsonDbPartition->d_func()->removeIndex("value.friend", "FoafPerson");
    mJsonDbPartition->d_func()->removeIndex("value.foaf", "FoafPerson");
}

void TestPartition::mapSelfJoinSourceUuids()
{
    addIndex("magic", "string");

    QJsonArray objects(readJsonFile(":/partition/json/map-join-sourceuuids.json").toArray());
    JsonDbObjectList toDelete;
    JsonDbObject toUpdate;

    for (int i = 0; i < objects.size(); ++i) {
        JsonDbObject object(objects.at(i).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        object.insert(JsonDbString::kUuidStr, result.objectsWritten[0].uuid().toString());
        object.insert(JsonDbString::kVersionStr, result.objectsWritten[0].version());

        if (object.value(JsonDbString::kTypeStr).toString() == "Bar")
            toUpdate = object;
        else
            toDelete.append(object);
    }

    QLatin1String query("[?_type=\"MagicView\"]");
    JsonDbQueryResult queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);

    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("_sourceUuids").toArray().size(), 3);

    toUpdate.insert("extra", QString("123123"));
    verifyGoodResult(update(mOwner, toUpdate));

    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);

    QCOMPARE(queryResult.data.at(0).value("_sourceUuids").toArray().size(), 3);
    verifyGoodResult(remove(mOwner, toUpdate));
    for (int i = toDelete.size() - 1; i >= 0; i--)
        verifyGoodResult(remove(mOwner, toDelete.at(i)));
    mJsonDbPartition->d_func()->removeIndex("magic", "string");
}

void TestPartition::mapMapFunctionError()
{
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QString("_schemaType"));
    schema.insert(JsonDbString::kNameStr, QString("MapFunctionErrorViewType"));
    QJsonObject schemaSub;
    schemaSub.insert("type", QString("object"));
    schemaSub.insert("extends", QString("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject mapDefinition;
    mapDefinition.insert(JsonDbString::kTypeStr, QString("Map"));
    mapDefinition.insert("targetType", QString("MapFunctionErrorViewType"));
    JsonDbObject sourceToMapFunctions;
    sourceToMapFunctions.insert("Contact", QString("function map (c) { invalidobject.fail(); }")); // error in map function
    mapDefinition.insert("map", sourceToMapFunctions);

    JsonDbWriteResult defRes = create(mOwner, mapDefinition);
    verifyGoodResult(defRes);

    JsonDbObject contact;
    contact.insert(JsonDbString::kTypeStr, QString("Contact"));
    JsonDbWriteResult res = create(mOwner, contact);

    // trigger the view update
    mJsonDbPartition->updateView("MapFunctionErrorViewType");

    // see if the map definition is still active
    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", defRes.objectsWritten[0].uuid().toString());
    mapDefinition = getObjects.data.at(0);
    QVERIFY(mapDefinition.contains(JsonDbString::kActiveStr) && !mapDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(mapDefinition.value(JsonDbString::kErrorStr).toString().contains("invalidobject"));

    verifyGoodResult(remove(mOwner, mapDefinition));
    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::mapSchemaViolation()
{
    jsondbSettings->setValidateSchemas(true);

    GetObjectsResult contactsRes = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Contact"));
    if (contactsRes.data.size() > 0) {
        foreach (const JsonDbObject &toRemove, contactsRes.data)
            remove(mOwner, toRemove);
    }

    QJsonArray objects(readJsonFile(":/partition/json/map-reduce-schema.json").toArray());
    JsonDbObjectList toDelete;
    QJsonValue workingMap;
    JsonDbObject map;

    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        if (object.value(JsonDbString::kTypeStr).toString() != JsonDbString::kReduceTypeStr) {

            // use the broken Map function that creates schema violations
            if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr) {
                workingMap = object.value("map");
                object.insert("map", object.value("brokenMap"));
            }

            JsonDbWriteResult result = create(mOwner, object);
            if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr) {
                map = object;
            }

            verifyGoodResult(result);
            if (object.value(JsonDbString::kTypeStr).toString() != JsonDbString::kMapTypeStr)
                toDelete.append(object);
        }
    }

    mJsonDbPartition->updateView(map.value("targetType").toString());

    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", map.value(JsonDbString::kUuidStr).toString());
    QCOMPARE(getObjects.data.size(), 1);
    JsonDbObject mapDefinition = getObjects.data.at(0);
    QVERIFY(mapDefinition.contains(JsonDbString::kActiveStr));
    QVERIFY(!mapDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(mapDefinition.value(JsonDbString::kErrorStr).toString().contains("Schema"));

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Phone"));
    QCOMPARE(getObjects.data.size(), 0);

    // get the version out of the database since the Map erroring out causes a
    //_version increment
    JsonDbObject updatedMap;
    QVERIFY(mJsonDbPartition->d_func()->getObject(map.uuid().toString(), updatedMap));

    // fix the map function
    map.insert("map", workingMap);
    map.insert(JsonDbString::kActiveStr, true);
    map.insert(JsonDbString::kErrorStr, QJsonValue(QJsonValue::Null));
    map.insert(JsonDbString::kVersionStr, updatedMap.value(JsonDbString::kVersionStr).toString());

    verifyGoodResult(update(mOwner, map));

    mJsonDbPartition->updateView(map.value("targetType").toString());

    getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", map.value(JsonDbString::kUuidStr).toString());
    mapDefinition = getObjects.data.at(0);
    QVERIFY(!mapDefinition.contains(JsonDbString::kActiveStr)|| mapDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(!mapDefinition.contains(JsonDbString::kErrorStr) || mapDefinition.value(JsonDbString::kErrorStr).toString().isEmpty());

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Phone"));
    QCOMPARE(getObjects.data.size(), 5);

    verifyGoodResult(remove(mOwner, map));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));

    jsondbSettings->setValidateSchemas(false);
}

// verify that only one target object per source object is allowed without targetKeyName
void TestPartition::mapMultipleEmitNoTargetKeyName()
{
    jsondbSettings->setValidateSchemas(true);

    GetObjectsResult contactsRes = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Contact"));
    if (contactsRes.data.size() > 0) {
        foreach (const JsonDbObject &toRemove, contactsRes.data)
            remove(mOwner, toRemove);
    }

    QJsonArray objects(readJsonFile(":/partition/json/map-reduce-schema.json").toArray());
    JsonDbObjectList toDelete;
    JsonDbObject map;

    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        if (object.value(JsonDbString::kTypeStr).toString() != JsonDbString::kReduceTypeStr) {

            if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr)
                object.remove("targetKeyName");

            JsonDbWriteResult result = create(mOwner, object);
            if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr)
                map = object;

            verifyGoodResult(result);
            if (object.value(JsonDbString::kTypeStr).toString() != JsonDbString::kMapTypeStr)
                toDelete.append(object);
        }
    }

    mJsonDbPartition->updateView(map.value("targetType").toString());

    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", map.value(JsonDbString::kUuidStr).toString());
    QJsonObject mapDefinition = getObjects.data.at(0);
    QVERIFY(!mapDefinition.contains(JsonDbString::kActiveStr)|| mapDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(!mapDefinition.contains(JsonDbString::kErrorStr) || mapDefinition.value(JsonDbString::kErrorStr).toString().isEmpty());

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("Phone"));
    QCOMPARE(getObjects.data.size(), 2);

    verifyGoodResult(remove(mOwner, map));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));

    jsondbSettings->setValidateSchemas(false);
}

void TestPartition::mapArrayConversion()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-array-conversion.json").toArray());
    JsonDbObjectList toDelete;
    JsonDbObject mapDefinition;
    for (int i = 0; i < objects.size(); i++) {
        JsonDbObject object(objects.at(i).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr) == JsonDbString::kMapTypeStr)
            mapDefinition = object;
        else
            toDelete.append(object);
    }

    JsonDbObject testObject;
    testObject.insert(JsonDbString::kTypeStr, QLatin1String("com.test.Test"));
    JsonDbWriteResult result = create(mOwner, testObject);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"com.test.TestView\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);
    JsonDbObject o = queryResult.data.at(0);
    QVERIFY(o.value("result").isArray());

    verifyGoodResult(remove(mOwner, mapDefinition));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
}

static QHash<QString, QList<QString> > sDebugMessages;
void logMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    switch (type) {
    case QtDebugMsg:
        sDebugMessages[QLatin1Literal("Debug")].append(msg);
        break;
    case QtWarningMsg:
        sDebugMessages[QLatin1Literal("Warning")].append(msg);
        break;
    case QtCriticalMsg:
        sDebugMessages[QLatin1Literal("Critical")].append(msg);
        break;
    case QtFatalMsg:
        sDebugMessages[QLatin1Literal("Fatal")].append(msg);
        break;
    }
}

void TestPartition::mapConsole()
{
    bool wasVerbose = jsondbSettings->verbose();
    bool wasDebug = jsondbSettings->debug();
    sDebugMessages.clear();
    QtMessageHandler oldMessageHandler = qInstallMessageHandler(logMessageOutput);
    jsondbSettings->setVerbose(true);
    jsondbSettings->setDebug(true);

    QJsonArray objects(readJsonFile(":/partition/json/map-array-conversion.json").toArray());
    JsonDbObjectList toDelete;
    JsonDbObject mapDefinition;
    QLatin1Literal sourceType("com.test.Test");
    for (int i = 0; i < objects.size(); i++) {
        JsonDbObject object(objects.at(i).toObject());
        if (object.value(JsonDbString::kTypeStr) == JsonDbString::kMapTypeStr) {
            QJsonObject map = object.value(QLatin1Literal("map")).toObject();
            // replace with a map that just calls console functions
            map.insert(sourceType, QLatin1String("function(test) { console.warn('testing'); for (var i in test) { if (i.indexOf('_') < 0) console[i](test[i]); } }"));
            object.insert(QLatin1Literal("map"), map);
        }
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr) == JsonDbString::kMapTypeStr)
            mapDefinition = object;
        else
            toDelete.append(object);
    }

    QMap<QString,QString> levelMapping;
    levelMapping["log"] = "Debug";
    levelMapping["debug"] = "Debug";
    levelMapping["info"] = "Debug";
    levelMapping["warn"] = "Warning";
    levelMapping["error"] = "Critical";

    QStringList consoleMethods = levelMapping.keys();
    JsonDbObject testObject;
    testObject.insert(JsonDbString::kTypeStr, QLatin1String("com.test.Test"));
    foreach (const QString &method, consoleMethods)
        testObject.insert(method, QString("%1 message").arg(method));
    JsonDbWriteResult result = create(mOwner, testObject);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"com.test.TestView\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    // revert to original settings
    jsondbSettings->setVerbose(wasVerbose);
    jsondbSettings->setDebug(wasDebug);
    qInstallMessageHandler(oldMessageHandler);

    // verify each method was invoked
    foreach (const QString &method, consoleMethods) {
        QVERIFY(sDebugMessages.contains(levelMapping[method]));
        QVERIFY(sDebugMessages[levelMapping[method]].contains(QString("\"%1 message\" ").arg(method)));
    }

    verifyGoodResult(remove(mOwner, mapDefinition));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
}

void TestPartition::reduce()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-data.json").toArray());

    JsonDbObjectList toDelete;
    JsonDbObjectList reduces;

    QHash<QString, int> firstNameCount;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        firstNameCount[object.value("firstName").toString()]++;
        toDelete.append(object);
    }

    objects = readJsonFile(":/partition/json/reduce.json").toArray();
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduces.append(object);
        else
            toDelete.append(object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        QCOMPARE((int)data.at(ii).value("count").toDouble(),
                 firstNameCount[data.at(ii).value("firstName").toString()]);
    }
    for (int ii = 0; ii < reduces.size(); ii++)
        verifyGoodResult(remove(mOwner, reduces.at(ii)));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
    mJsonDbPartition->d_func()->removeIndex("MyContactCount");
}

void TestPartition::reduceFlattened()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-data.json").toArray());

    JsonDbObjectList toDelete;
    JsonDbObjectList reduces;

    QHash<QString, int> firstNameCount;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        firstNameCount[object.value("firstName").toString()]++;
        toDelete.append(object);
    }

    objects = readJsonFile(":/partition/json/reduce.json").toArray();
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kReduceTypeStr) {
            object.insert(QLatin1String("add"), object.value(QLatin1String("addFlattened")));
            object.insert(QLatin1String("subtract"), object.value(QLatin1String("subtractFlattened")));
            // transitional behavior: null targetValueName indicates whole object is value of the Reduce
            object.insert(QLatin1String("targetValueName"), QJsonValue(QJsonValue::Null));
        }
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduces.append(object);
        else
            toDelete.append(object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        QCOMPARE((int)data.at(ii).value("count").toDouble(),
                 firstNameCount[data.at(ii).value("firstName").toString()]);
    }
    for (int ii = 0; ii < reduces.size(); ii++)
        verifyGoodResult(remove(mOwner, reduces.at(ii)));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
    mJsonDbPartition->d_func()->removeIndex("MyContactCount");
}

void TestPartition::reduceSourceKeyFunction()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-data.json").toArray());

    JsonDbObjectList toDelete;
    JsonDbObjectList reduces;

    QHash<QString, int> firstNameCount;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        firstNameCount[object.value("firstName").toString()]++;
        toDelete.append(object);
    }

    objects = readJsonFile(":/partition/json/reduce.json").toArray();
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kReduceTypeStr) {
            QString sourceKeyName = object.value(QLatin1String("sourceKeyName")).toString();
            object.remove(QLatin1String("sourceKeyName"));
            object.insert(QLatin1String("sourceKeyFunction"),
                          QString("function (o) { return o.%1; }").arg(sourceKeyName));
        }
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduces.append(object);
        else
            toDelete.append(object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        QCOMPARE((int)data.at(ii).value("count").toDouble(),
                 firstNameCount[data.at(ii).value("firstName").toString()]);
    }
    for (int ii = 0; ii < reduces.size(); ii++)
        verifyGoodResult(remove(mOwner, reduces.at(ii)));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
    mJsonDbPartition->d_func()->removeIndex("MyContactCount");
}

void TestPartition::reduceRemoval()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-data.json").toArray());

    QJsonArray toDelete;
    QHash<QString, int> firstNameCount;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        firstNameCount[object.value("firstName").toString()]++;
        toDelete.append(object);
    }

    objects = readJsonFile(":/partition/json/reduce.json").toArray();
    JsonDbObject reduce;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduce = object;
        else
            toDelete.append(object);
    }

    QLatin1String query("[?_type=\"MyContactCount\"]");
    JsonDbQueryResult queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    JsonDbWriteResult result = remove(mOwner, reduce);
    verifyGoodResult(result);
    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii).toObject()));
    mJsonDbPartition->d_func()->removeIndex("MyContactCount");
}

void TestPartition::reduceUpdate()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-data.json").toArray());

    QJsonArray toDelete;
    QHash<QString, int> firstNameCount;
    QHash<QString, int> lastNameCount;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        firstNameCount[object.value("firstName").toString()]++;
        lastNameCount[object.value("lastName").toString()]++;
        toDelete.append(object);
    }

    objects = readJsonFile(":/partition/json/reduce.json").toArray();
    JsonDbObject reduce;
    JsonDbObject schema;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduce = object;
        else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kSchemaTypeStr)
            schema = object;
        else
            toDelete.append(object);
    }

    QLatin1String query("[?_type=\"MyContactCount\"]");
    JsonDbQueryResult queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    JsonDbObjectList data = queryResult.data;
    QCOMPARE(data.size(), firstNameCount.keys().count());

    for (int ii = 0; ii < data.size(); ii++)
        QCOMPARE((int)data.at(ii).value("value").toObject().value("count").toDouble(),
                 firstNameCount[data.at(ii).value("key").toString()]);

    reduce.insert("sourceKeyName", QString("lastName"));
    JsonDbWriteResult result = update(mOwner, reduce);

    query = QLatin1String("[?_type=\"MyContactCount\"]");
    queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);
    data = queryResult.data;

    QCOMPARE(queryResult.data.size(), lastNameCount.keys().count());

    data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++)
        QCOMPARE((int)data.at(ii).value("value").toObject().value("count").toDouble(),
                 lastNameCount[data.at(ii).value("key").toString()]);

    verifyGoodResult(remove(mOwner, reduce));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii).toObject()));
    verifyGoodResult(remove(mOwner, schema));
    mJsonDbPartition->d_func()->removeIndex("MyContactCount");
}

void TestPartition::reduceDuplicate()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-data.json").toArray());

    JsonDbObjectList toDelete;
    QHash<QString, int> firstNameCount;
    QHash<QString, int> lastNameCount;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        firstNameCount[object.value("firstName").toString()]++;
        lastNameCount[object.value("lastName").toString()]++;
        toDelete.append(object);
    }

    objects = readJsonFile(":/partition/json/reduce.json").toArray();
    JsonDbObject reduce;
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            object.insert("targetKeyName", QString("key"));
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduce = object;
        else
            toDelete.append(object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        JsonDbObject object(data.at(ii));
        QCOMPARE((int)object.value("count").toDouble(),
                 firstNameCount[object.value("key").toString()]);
    }

    JsonDbObject reduce2;
    reduce2.insert(JsonDbString::kTypeStr, reduce.value(JsonDbString::kTypeStr).toString());
    reduce2.insert("targetType", reduce.value("targetType").toString());
    reduce2.insert("sourceType", reduce.value("sourceType").toString());
    reduce2.insert("sourceKeyName", QString("lastName"));
    reduce2.insert("targetKeyName", QString("key"));
    reduce2.insert("targetValueName", QString("count"));
    reduce2.insert("add", reduce.value("add").toString());
    reduce2.insert("subtract", reduce.value("subtract").toString());

    JsonDbWriteResult result = create(mOwner, reduce2);
    verifyErrorResult(result);

    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        JsonDbObject object = data.at(ii);
        QVERIFY(object.value("count").toDouble() == firstNameCount[object.value("key").toString()]
                || object.value("count").toDouble() == lastNameCount[object.value("key").toString()]);
    }

    verifyGoodResult(remove(mOwner, reduce));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii)));
    mJsonDbPartition->d_func()->removeIndex("MyContactCount");
}

void TestPartition::reduceFunctionError()
{
    JsonDbObject schema;
    QString viewTypeStr("ReduceFunctionErrorView");
    schema.insert(JsonDbString::kTypeStr, QString("_schemaType"));
    schema.insert(JsonDbString::kNameStr, viewTypeStr);
    QJsonObject schemaSub;
    schemaSub.insert("type", QString("object"));
    schemaSub.insert("extends", QString("View"));
    schema.insert("schema", schemaSub);
    JsonDbWriteResult schemaRes = create(mOwner, schema);
    verifyGoodResult(schemaRes);

    JsonDbObject reduceDefinition;
    reduceDefinition.insert(JsonDbString::kTypeStr, QString("Reduce"));
    reduceDefinition.insert("targetType", viewTypeStr);
    reduceDefinition.insert("sourceType", QString("Contact"));
    reduceDefinition.insert("sourceKeyName", QString("phoneNumber"));
    reduceDefinition.insert("add", QString("function add (k, z, c) { invalidobject.test(); }")); // invalid add function
    reduceDefinition.insert("subtract", QString("function subtract (k, z, c) { }"));
    JsonDbWriteResult defRes = create(mOwner, reduceDefinition);
    verifyGoodResult(defRes);

    JsonDbObject contact;
    contact.insert(JsonDbString::kTypeStr, QString("Contact"));
    contact.insert("phoneNumber", QString("+1234567890"));
    JsonDbWriteResult res = create(mOwner, contact);
    verifyGoodResult(res);

    mJsonDbPartition->updateView(viewTypeStr);
    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", defRes.objectsWritten[0].uuid().toString(), JsonDbString::kReduceTypeStr);
    reduceDefinition = getObjects.data.at(0);
    QVERIFY(!reduceDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(reduceDefinition.value(JsonDbString::kErrorStr).toString().contains("invalidobject"));

    verifyGoodResult(remove(mOwner, reduceDefinition));
    verifyGoodResult(remove(mOwner, schema));
}

void TestPartition::reduceSchemaViolation()
{
    jsondbSettings->setValidateSchemas(true);

    QJsonArray objects(readJsonFile(":/partition/json/map-reduce-schema.json").toArray());

    QJsonArray toDelete;
    JsonDbObject map;
    JsonDbObject reduce;
    QString workingAdd;

    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());

        if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kReduceTypeStr) {
            // use the broken add function that creates schema violations
            workingAdd = object.value("add").toString();
            object.insert("add", object.value("brokenAdd").toString());
        }

        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);

        if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr) {
            map = object;
        } else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kReduceTypeStr) {
            reduce = object;
        } else if (object.value(JsonDbString::kTypeStr).toString() != JsonDbString::kMapTypeStr &&
                   object.value(JsonDbString::kTypeStr).toString() != JsonDbString::kMapTypeStr)
            toDelete.append(object);

    }

    mJsonDbPartition->updateView(reduce.value("targetType").toString());

    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", reduce.value(JsonDbString::kUuidStr).toString());
    QCOMPARE(getObjects.data.size(), 1);
    JsonDbObject reduceDefinition = getObjects.data.at(0);
    QVERIFY(reduceDefinition.contains(JsonDbString::kActiveStr) && !reduceDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(reduceDefinition.value(JsonDbString::kErrorStr).toString().contains("Schema"));

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("PhoneCount"));
    QCOMPARE(getObjects.data.size(), 0);

    // get the version out of the database since the Reduce erroring out causes a
    //_version increment
    JsonDbObject updatedReduce;
    QVERIFY(mJsonDbPartition->d_func()->getObject(reduce.uuid().toString(), updatedReduce ));

    // fix the add function
    reduce.insert("add", workingAdd);
    reduce.insert(JsonDbString::kActiveStr, true);
    reduce.insert(JsonDbString::kErrorStr, QJsonValue(QJsonValue::Null));
    reduce.insert(JsonDbString::kVersionStr, updatedReduce.value(JsonDbString::kVersionStr).toString());

    verifyGoodResult(update(mOwner, reduce));

    mJsonDbPartition->updateView(reduce.value("targetType").toString());

    getObjects = mJsonDbPartition->d_func()->getObjects("_uuid", reduce.value(JsonDbString::kUuidStr).toString());
    reduceDefinition = getObjects.data.at(0);
    QVERIFY(!reduceDefinition.contains(JsonDbString::kActiveStr)|| reduceDefinition.value(JsonDbString::kActiveStr).toBool());
    QVERIFY(!reduceDefinition.contains(JsonDbString::kErrorStr) || reduceDefinition.value(JsonDbString::kErrorStr).toString().isEmpty());

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("PhoneCount"));
    QCOMPARE(getObjects.data.size(), 4);

    verifyGoodResult(remove(mOwner, reduce));
    verifyGoodResult(remove(mOwner, map));
    for (int ii = 0; ii < toDelete.size(); ii++)
        verifyGoodResult(remove(mOwner, toDelete.at(ii).toObject()));

    jsondbSettings->setValidateSchemas(false);
}

void TestPartition::reduceSubObjectProp()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-subprop.json").toArray());

    QJsonArray toDelete;
    JsonDbObject reduce;

    QHash<QString, int> firstNameCount;
    for (int i = 0; i < objects.size(); ++i) {
        JsonDbObject object(objects.at(i).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);

        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce") {
            reduce = object;
        } else {
            if (object.value(JsonDbString::kTypeStr).toString() == "Contact")
                firstNameCount[object.value("name").toObject().value("firstName").toString()]++;
            toDelete.append(object);
        }
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"NameCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), firstNameCount.keys().count());

    JsonDbObjectList data = queryResult.data;
    for (int i = 0; i < data.size(); ++i) {
        JsonDbObject object(data.at(i));
        QCOMPARE((int)object.value("value").toObject().value("count").toDouble(), firstNameCount[object.value("key").toString()]);
    }

    verifyGoodResult(remove(mOwner, reduce));
    for (int i = 0; i < toDelete.size(); ++i) {
        JsonDbObject object(toDelete.at(i).toObject());
        verifyGoodResult(remove(mOwner, object));
    }
    mJsonDbPartition->d_func()->removeIndex("NameCount");
}

void TestPartition::reduceArray()
{
    QJsonArray objects(readJsonFile(":/partition/json/reduce-array.json").toArray());
    QJsonArray toDelete;

    JsonDbObject human;

    for (int i = 0; i < objects.size(); i++) {
        JsonDbObject object(objects.at(i).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        object.insert(JsonDbString::kUuidStr, result.objectsWritten[0].uuid().toString());

        if (object.value("firstName").toString() == "Julio")
            human = object;
        else
            toDelete.append(object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"ArrayView\"]"));
    verifyGoodQueryResult(queryResult);

    JsonDbObjectList results = queryResult.data;
    QCOMPARE(results.size(), 2);

    for (int i = 0; i < results.size(); i++) {
        QJsonArray firstNames = results.at(i).value("value").toObject().value("firstNames").toArray();
        QCOMPARE(firstNames.size(), 2);

        for (int j = 0; j < firstNames.size(); j++)
            QVERIFY(!firstNames.at(j).toString().isEmpty());
    }

    human.insert("lastName", QString("Johnson"));
    verifyGoodResult(update(mOwner, human));

    queryResult = find(mOwner, QLatin1String("[?_type=\"ArrayView\"][?key=\"Jones\"]"));
    verifyGoodQueryResult(queryResult);

    results = queryResult.data;
    QCOMPARE(results.at(0).value("value").toObject().value("firstNames").toArray().size(), 1);

    verifyGoodResult(remove(mOwner, human));
    for (int i = toDelete.size() - 1; i >= 0; i--)
        verifyGoodResult(remove(mOwner, toDelete.at(i).toObject()));
    mJsonDbPartition->d_func()->removeIndex("ArrayView");
}

void TestPartition::changesSinceCreate()
{
    JsonDbChangesSinceResult csRes = mJsonDbPartition->changesSince(0);
    QCOMPARE(csRes.code, JsonDbError::NoError);

    quint32 state = csRes.currentStateNumber;

    JsonDbObject toCreate;
    toCreate.insert("_type", QString("TestContact"));
    toCreate.insert("firstName", QString("John"));
    toCreate.insert("lastName", QString("Doe"));

    JsonDbWriteResult crRes = create(mOwner, toCreate);
    verifyGoodResult(crRes);

    csRes = mJsonDbPartition->changesSince(state);
    QCOMPARE(csRes.code, JsonDbError::NoError);

    QVERIFY(csRes.currentStateNumber > state);
    QCOMPARE(csRes.changes.count(), 1);

    QJsonObject after = csRes.changes.at(0).newObject;
    QCOMPARE(after.value("_type").toString(), toCreate.value("_type").toString());
    QCOMPARE(after.value("firstName").toString(), toCreate.value("firstName").toString());
    QCOMPARE(after.value("lastName").toString(), toCreate.value("lastName").toString());
}

void TestPartition::addIndex()
{
    addIndex(QLatin1String("subject"));

    JsonDbObject indexObject;
    indexObject.insert(JsonDbString::kTypeStr, QLatin1String("Index"));
    indexObject.insert("propertyName", QLatin1String("predicate"));
    indexObject.insert("propertyType", QLatin1String("string"));

    JsonDbWriteResult result = create(mOwner, indexObject);
    verifyGoodResult(result);
    QVERIFY(mJsonDbPartition->findObjectTable(JsonDbString::kSchemaTypeStr)->index("predicate") != 0);
    remove(mOwner, indexObject);
}

void TestPartition::addSchema()
{
    JsonDbObject s;
    addSchema("contact", s);
    verifyGoodResult(remove(mOwner, s));
}

void TestPartition::duplicateSchema()
{
    QJsonValue schema = readJsonFile(":/partition/schemas/address.json");
    JsonDbObject schemaObject;
    schemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    schemaObject.insert("name", QLatin1String("Address"));
    schemaObject.insert("schema", schema);

    JsonDbWriteResult result = create(mOwner, schemaObject);
    verifyGoodResult(result);

    JsonDbObject schemaObject2;
    schemaObject2.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    schemaObject2.insert("name", QLatin1String("Address"));
    schemaObject2.insert("schema", schema);
    result = create(mOwner, schemaObject2);
    verifyErrorResult(result);

    result = remove(mOwner, schemaObject);
    verifyGoodResult(result);
}

void TestPartition::removeSchema()
{
    QJsonValue schema = readJsonFile(":/partition/schemas/address.json");
    JsonDbObject schemaObject;
    schemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    schemaObject.insert("name", QLatin1String("Address"));
    schemaObject.insert("schema", schema);

    JsonDbWriteResult result = create(mOwner, schemaObject);
    verifyGoodResult(result);

    JsonDbObject address;
    address.insert(JsonDbString::kTypeStr, QLatin1String("Address"));
    address.insert("street", QLatin1String("Main Street"));
    address.insert("number", 1);

    result = create(mOwner, address);
    verifyGoodResult(result);

    result = remove(mOwner, schemaObject);
    verifyErrorResult(result);

    result = remove(mOwner, address);
    verifyGoodResult(result);

    result = remove(mOwner, schemaObject);
    verifyGoodResult(result);
}

void TestPartition::removeViewSchema()
{
    QJsonArray objects = readJsonFile(":/partition/json/reduce.json").toArray();
    JsonDbObject schema;
    JsonDbObject reduce;
    for (int i = 0; i < objects.size(); ++i) {
        JsonDbObject object(objects.at(i).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        if (object.value(JsonDbString::kTypeStr).toString() == "Reduce")
            reduce = object;
        else
            schema = object;
    }

    mJsonDbPartition->updateView(QLatin1String("MyContactCount"));

    JsonDbWriteResult result = remove(mOwner, schema);
    verifyErrorResult(result);

    result = remove(mOwner, reduce);
    verifyGoodResult(result);

    result = remove(mOwner, schema);
    verifyGoodResult(result);
}

void TestPartition::updateSchema()
{
    QJsonObject schema = readJsonFile(":/partition/schemas/address.json").toObject();
    QVERIFY(!schema.isEmpty());
    JsonDbObject schemaObject;
    schemaObject.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    schemaObject.insert("name", QLatin1String("Address"));
    schemaObject.insert("schema", schema);

    JsonDbWriteResult schemaResult = create(mOwner, schemaObject);
    verifyGoodResult(schemaResult);

    JsonDbObject address;
    address.insert(JsonDbString::kTypeStr, QLatin1String("Address"));
    address.insert("street", QLatin1String("Main Street"));
    address.insert("number", 1);

    JsonDbWriteResult result = create(mOwner, address);
    verifyGoodResult(result);

    schema.insert("streetNumber", schema.value("number").toObject());
    schemaObject.insert("schema", schema);
    result = update(mOwner, schemaObject);
    verifyErrorResult(result);

    result = remove(mOwner, address);
    verifyGoodResult(result);

    result = update(mOwner, schemaObject);
    verifyGoodResult(result);

    result = remove(mOwner, schemaObject);
    verifyGoodResult(result);
}

void TestPartition::find1()
{
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QString("Find1Type"));
    item.insert("find1", QString("Foobar!"));
    create(mOwner, item);

    JsonDbQueryResult queryResult= find(mOwner, QLatin1String("[*]"));

    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() >= 1);
}

void TestPartition::find2()
{
    addIndex(QLatin1String("name"));
    addIndex(QLatin1String("_type"));

    JsonDbObjectList toDelete;

    JsonDbObject item;
    item.insert("name", QString("Wilma"));
    item.insert(JsonDbString::kTypeStr, QString(__FUNCTION__));
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);
    toDelete.append(item);

    item = JsonDbObject();
    item.insert("name", QString("Betty"));
    item.insert(JsonDbString::kTypeStr, QString(__FUNCTION__));
    result = create(mOwner, item);
    verifyGoodResult(result);
    toDelete.append(item);

    int extraneous;

    QLatin1String query("[?name=\"Wilma\"][?_type=%type]");
    QJsonObject bindings;
    bindings.insert("type", QString(__FUNCTION__));
    JsonDbQueryResult queryResult = find(mOwner, query, bindings);
    QCOMPARE(queryResult.data.size(), qint32(1));

    extraneous = 0;
    foreach (JsonDbObject item, queryResult.data) {
        if (!item.contains("name") || (item.value("name").toString() != QLatin1String("Wilma")))
            extraneous++;
    }
    verifyGoodQueryResult(queryResult);
    QVERIFY(!extraneous);

    queryResult = find(mOwner, QLatin1String("[?_type=%type]"), bindings);

    extraneous = 0;
    foreach (const JsonDbObject &map, result.objectsWritten) {
        if (!map.contains(JsonDbString::kTypeStr) || (map.value(JsonDbString::kTypeStr).toString() != QString(__FUNCTION__)))
            extraneous++;
    }
    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() >= 1);
    QVERIFY(!extraneous);

    queryResult = find(mOwner,  QString("[?name=\"Wilma\"][?%1=\"%2\"]").arg(JsonDbString::kTypeStr).arg(__FUNCTION__));

    extraneous = 0;
    for (int i = 0; i < queryResult.data.size(); i++) {
        QJsonObject map = queryResult.data.at(i);
        if (!map.contains("name")
                || (map.value("name").toString() != QString("Wilma"))
                || !map.contains(JsonDbString::kTypeStr)
                || (map.value(JsonDbString::kTypeStr).toString() != QString(__FUNCTION__))
                )
            extraneous++;
    }

    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() >= 1);
    QVERIFY(!extraneous);

    for (int ii = 0; ii < toDelete.size(); ii++) {
        remove(mOwner, toDelete.at(ii));
    }
}

QStringList strings = (QStringList()
                       << "abc"
                       << "def"
                       << "deaf"
                       << "leaf"
                       << "DEAF"
                       << "LEAF"
                       << "ghi"
                       << "foo/bar");

QStringList patterns = (QStringList()
        );

void TestPartition::findLikeRegexp_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("modifiers");
    QTest::newRow("/de.*/") << "de.*" << "";
    QTest::newRow("/.*a.*/") << ".*a.*" << "";
    QTest::newRow("/de*/wi") << "de*" << "wi";
    QTest::newRow("/*a*/w") << "*a*" << "w";
    QTest::newRow("/de*/wi") << "de*" << "wi";
    QTest::newRow("/*a*/wi") << "*a*" << "wi";
    QTest::newRow("/*a*/wi") << "*a*" << "wi";
    QTest::newRow("/*a/wi") << "*a" << "wi";
    QTest::newRow("/a*/wi") << "a*" << "wi";
    QTest::newRow("/c*/wi") << "c*" << "wi";
    QTest::newRow("/c*/wi") << "c*" << "wi";
    QTest::newRow("/*a*/w") << "*a*" << "w";
    QTest::newRow("/*a/w") << "*a" << "w";
    QTest::newRow("/a*/w") << "a*" << "w";
    QTest::newRow("/a+/w") << "a+" << "w";
    QTest::newRow("/c*/w") << "c*" << "w";
    QTest::newRow("/c*/w") << "c*" << "w";
    QTest::newRow("/.*foo.*\\/.*/i") << ".*foo.*\\/.*" << "i";
    QTest::newRow("|.*foo.*/.*|i") << ".*foo.*\\/.*" << "i";

    foreach (QString s, strings) {
        JsonDbObject item;
        item.insert(JsonDbString::kTypeStr, QString("FindLikeRegexpData"));
        item.insert(__FUNCTION__, QString("Find Me!"));
        item.insert("key", s);
        create(mOwner, item);
    }
}

void TestPartition::findLikeRegexp()
{
    QFETCH(QString, pattern);
    QFETCH(QString, modifiers);
    QString q = QString("[?_type=\"%1\"][?key =~ \"/%2/%3\"]").arg("FindLikeRegexpData").arg(pattern).arg(modifiers);
    Qt::CaseSensitivity cs = (modifiers.contains("i") ? Qt::CaseInsensitive : Qt::CaseSensitive);
    QRegExp::PatternSyntax ps = (modifiers.contains("w") ? QRegExp::Wildcard : QRegExp::RegExp);
    QRegExp re(pattern, cs, ps);
    QStringList expectedMatches;
    foreach (QString s, strings) {
        if (re.exactMatch(s))
            expectedMatches << s;
    }
    expectedMatches.sort();

    JsonDbQueryResult queryResult = find(mOwner, q);
    int length = queryResult.data.size();
    verifyGoodQueryResult(queryResult);

    QStringList matches;
    foreach (JsonDbObject v, queryResult.data) {
        QVariantMap m = v.toVariantMap();
        matches << m.value("key").toString();
    }
    matches.sort();
    if (matches != expectedMatches) {
        qDebug() << "query" << q;
        qDebug() << "expectedMatches" << expectedMatches;
        qDebug() << "matches" << matches;
    }
    QCOMPARE(matches, expectedMatches);
    QCOMPARE(length, expectedMatches.size());
}

void TestPartition::findInContains()
{
    QList<QStringList> stringLists;
    stringLists << (QStringList() << "fred" << "barney");
    stringLists << (QStringList() << "wilma" << "betty");
    QVariantList intLists;
    intLists << QVariant(QVariantList() << 1 << 22);
    intLists << QVariant(QVariantList() << 42 << 17);

    for (int i = 0; i < qMin(stringLists.size(), intLists.size()); i++) {
        JsonDbObject item;
        item.insert(JsonDbString::kTypeStr, QString(__FUNCTION__));
        item.insert(__FUNCTION__, QString("Find Me!"));
        item.insert("stringlist", QJsonValue::fromVariant(stringLists[i]));
        item.insert("intlist", QJsonValue::fromVariant(intLists[i]));
        item.insert("str", QJsonValue::fromVariant(stringLists[i][0]));
        item.insert("i", QJsonValue::fromVariant(intLists[i].toList().at(0)));
        create(mOwner, item);
    }

    QStringList queries = (QStringList()
                           << "[?stringlist contains \"fred\"]"
                           << "[?intlist contains 22]"
                           << "[?str in [\"fred\", \"barney\"]]"
                           << "[?i in [\"1\", \"22\"]]"
        );

    foreach (QString q, queries) {
        QString strOutput = QString(QLatin1String("[- compileIndexQuery ] searching all objects \"%1\" "))
                                    .arg(q);
        QTest::ignoreMessage(QtSystemMsg, strOutput.toLatin1().constData());
        JsonDbQueryResult queryResult = find(mOwner, q);
        verifyGoodQueryResult(queryResult);
    }

    queries = (QStringList()
                           << "[?stringlist notContains \"fredware\"]"
                           << "[?intlist notContains 24]"
                           << "[?str notIn [\"fredware\", \"barneyfriend\"]]"
                           << "[?i notIn [\"11\", \"24\"]]"
        );

    foreach (QString q, queries) {
        QString strOutput = QString(QLatin1String("[- compileIndexQuery ] searching all objects \"%1\" "))
                                    .arg(q);
        QTest::ignoreMessage(QtSystemMsg, strOutput.toLatin1().constData());
        JsonDbQueryResult queryResult = find(mOwner, q);
        verifyGoodQueryResult(queryResult);
    }

    mJsonDbPartition->d_func()->removeIndex("stringlist");
    mJsonDbPartition->d_func()->removeIndex("intlist");
    mJsonDbPartition->d_func()->removeIndex("str");
    mJsonDbPartition->d_func()->removeIndex("i");
}

void TestPartition::findFields()
{
    addIndex(QLatin1String("name"));
    addIndex(QLatin1String("_type"));

    JsonDbObject item;
    item.insert("firstName", QString("Wilma"));
    item.insert("lastName", QString("Flintstone"));
    item.insert(JsonDbString::kTypeStr, kContactStr);
    create(mOwner, item);

    item = JsonDbObject();
    item.insert("firstName", QString("Betty"));
    item.insert("lastName", QString("Rubble"));
    item.insert(JsonDbString::kTypeStr, kContactStr);
    create(mOwner, item);

    QJsonObject query, result, map;

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?firstName=\"Wilma\"][= {firstName:firstName,lastName:lastName}]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);
    QJsonObject data = queryResult.data.at(0);
    QCOMPARE(data.value(QLatin1String("firstName")).toString(), QString("Wilma"));
    QCOMPARE(data.value(QLatin1String("lastName")).toString(), QString("Flintstone"));
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("firstName"));
}

void TestPartition::orderedFind1_data()
{
    QTest::addColumn<QString>("order");
    QTest::newRow("asc") << "/";
    QTest::newRow("desc") << "\\";

    addIndex(QLatin1String("orderedFindName"));
    addIndex(QLatin1String("_type"));

    JsonDbObject item1;
    item1.insert("orderedFindName", QString("Wilma"));
    item1.insert(JsonDbString::kTypeStr, QLatin1String("orderedFind1"));
    create(mOwner, item1);

    JsonDbObject item2;
    item2.insert("orderedFindName", QString("BamBam"));
    item2.insert(JsonDbString::kTypeStr, QLatin1String("orderedFind1"));
    create(mOwner, item2);

    JsonDbObject item3;
    item3.insert("orderedFindName", QString("Betty"));
    item3.insert(JsonDbString::kTypeStr, QLatin1String("orderedFind1"));
    create(mOwner, item3);
}

void TestPartition::orderedFind1()
{
    QFETCH(QString, order);

    JsonDbQueryResult queryResult = find(mOwner, QString("[?_type=\"orderedFind1\"][%3orderedFindName]").arg(order));
    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() > 0);

    QStringList names;
    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        names.append(data.at(ii).value("orderedFindName").toString());
    }
    QStringList orderedNames = names;
    qSort(orderedNames.begin(), orderedNames.end());
    QStringList disorderedNames = names;
    qSort(disorderedNames.begin(), disorderedNames.end(), qGreater<QString>());
    if (order == "/") {
        QVERIFY(names == orderedNames);
        QVERIFY(names != disorderedNames);
    } else {
        QVERIFY(names != orderedNames);
        QVERIFY(names == disorderedNames);
    }
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("orderedFind1"));
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("orderedFindName"));
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("_type"));
}

void TestPartition::orderedFind2_data()
{
    QTest::addColumn<QString>("order");
    QTest::addColumn<QString>("field");
    QTest::newRow("asc uuid")   << "/"  << "_uuid";
    QTest::newRow("asc foobar")   << "/"  << "foobar";
    QTest::newRow("desc uuid")  << "\\" << "_uuid";
    QTest::newRow("desc foobar")  << "\\" << "foobar";

    for (char prefix = 'z'; prefix >= 'a'; prefix--) {
        JsonDbObject item;
        item.insert(JsonDbString::kTypeStr, QLatin1String("orderedFind2"));
        item.insert(QLatin1String("foobar"), QString("%1_orderedFind2").arg(prefix));
        JsonDbWriteResult r = create(mOwner, item);
    }
}

void TestPartition::orderedFind2()
{
    QFETCH(QString, order);
    QFETCH(QString, field);

    JsonDbQueryResult queryResult = find(mOwner, QString("[?_type=\"orderedFind2\"][%1%2]").arg(order).arg(field));
    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() > 0);

    QStringList names;
    JsonDbObjectList data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        names.append(data.at(ii).value(field).toString());
    }
    QStringList orderedNames = names;
    qSort(orderedNames.begin(), orderedNames.end());
    QStringList disorderedNames = names;
    qSort(disorderedNames.begin(), disorderedNames.end(), qGreater<QString>());
    if (order == "/") {
        QVERIFY(names == orderedNames);
        QVERIFY(names != disorderedNames);
    } else {
        QVERIFY(names != orderedNames);
        QVERIFY(names == disorderedNames);
    }
}

void TestPartition::wildcardIndex()
{
    addIndex("telephoneNumbers.*.number");
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, kContactStr);
    item.insert("name", QString("BamBam"));

    QJsonObject mobileNumber;
    QString mobileNumberString = "+15515512323";
    mobileNumber.insert("type", QString("mobile"));
    mobileNumber.insert("number", mobileNumberString);
    QJsonArray telephoneNumbers;
    telephoneNumbers.append(mobileNumber);
    item.insert("telephoneNumbers", telephoneNumbers);

    create(mOwner, item);

    JsonDbQueryResult queryResult = find(mOwner, QString("[?telephoneNumbers.*.number=\"%1\"]").arg(mobileNumberString));
    verifyGoodQueryResult(queryResult);

    queryResult = find(mOwner, QString("[?%1=\"%2\"][? telephoneNumbers[*].number exists ]").arg(JsonDbString::kTypeStr).arg(kContactStr));
    verifyGoodQueryResult(queryResult);
    mJsonDbPartition->d_func()->removeIndex("telephoneNumbers.*.number");
}

void TestPartition::uuidJoin()
{
    addIndex("name");
    addIndex("thumbnailUuid");
    addIndex("url");
    QString thumbnailUrl = "file:thumbnail.png";
    JsonDbObject thumbnail;
    thumbnail.insert(JsonDbString::kTypeStr, QString("com.example.thumbnail"));
    thumbnail.insert("url", thumbnailUrl);
    create(mOwner, thumbnail);
    QString thumbnailUuid = thumbnail.value("_uuid").toString();

    JsonDbObject item;
    item.insert("_type", QString(__FUNCTION__));
    item.insert("name", QString("Pebbles"));
    item.insert(JsonDbString::kTypeStr, kContactStr);
    create(mOwner, item);

    JsonDbObject item2;
    item2.insert("_type", QString(__FUNCTION__));
    item2.insert("name", QString("Wilma"));
    item2.insert("thumbnailUuid", thumbnailUuid);
    item2.insert(JsonDbString::kTypeStr, kContactStr);
    create(mOwner, item2);

    JsonDbObject betty;
    betty.insert("_type", QString(__FUNCTION__));
    betty.insert("name", QString("Betty"));
    betty.insert("thumbnailUuid", thumbnailUuid);
    betty.insert(JsonDbString::kTypeStr, kContactStr);
    JsonDbWriteResult r = create(mOwner, betty);
    QString bettyUuid = r.objectsWritten[0].uuid().toString();

    JsonDbObject bettyRef;
    bettyRef.insert("_type", QString(__FUNCTION__));
    bettyRef.insert("bettyUuid", bettyUuid);
    bettyRef.insert("thumbnailUuid", thumbnailUuid);
    r = create(mOwner, bettyRef);

    QTest::ignoreMessage(QtCriticalMsg, "[- compileIndexQuery ] searching all objects \"[?bettyUuid exists][=  { thumbnailUuid : bettyUuid->thumbnailUuid }]\" ");
    QTest::ignoreMessage(QtCriticalMsg, "[- compileIndexQuery ] searching all objects \"[?bettyUuid exists][= { thumbnailUuid : bettyUuid->thumbnailUuid->url }]\" ");
    JsonDbQueryResult queryResult = find(mOwner, QString("[?thumbnailUuid->url=\"%1\"]").arg(thumbnailUrl));
    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() > 0);
    QCOMPARE(queryResult.data.at(0).value("thumbnailUuid").toString(), thumbnailUuid);

    queryResult = find(mOwner, QString("[?_type=\"%1\"][?thumbnailUuid->url=\"%2\"]").arg(__FUNCTION__).arg(thumbnailUrl));
    verifyGoodQueryResult(queryResult);
    QVERIFY(queryResult.data.size() > 0);
    QCOMPARE(queryResult.data.at(0).value("thumbnailUuid").toString(), thumbnailUuid);

    queryResult = find(mOwner, QLatin1String("[?name=\"Betty\"][= { name:name, url:thumbnailUuid->url }]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.at(0).value(QLatin1String("url")).toString(), thumbnailUrl);

    queryResult = find(mOwner, QString("[?_type=\"%1\"][= { name: name, url: thumbnailUuid->url } ]").arg(__FUNCTION__));
    verifyGoodQueryResult(queryResult);

    QList<JsonDbObject> data = queryResult.data;
    for (int ii = 0; ii < data.size(); ii++) {
        QJsonObject item = data.at(ii);
        QString name = item.value("name").toString();
        QString url = item.value("url").toString();
        if (name == "Pebbles")
            QVERIFY(url.isEmpty());
        else
            QCOMPARE(url, thumbnailUrl);
    }

    queryResult = find(mOwner, QLatin1String("[?bettyUuid exists][=  { thumbnailUuid : bettyUuid->thumbnailUuid }]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.at(0).value(QLatin1String("thumbnailUuid")).toString(),
             thumbnailUuid);

    queryResult = find(mOwner, QLatin1String("[?bettyUuid exists][= { thumbnailUuid : bettyUuid->thumbnailUuid->url }]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.at(0).value(QLatin1String("thumbnailUuid")).toString(),
             thumbnail.value("url").toString());
    mJsonDbPartition->d_func()->removeIndex("name");
    mJsonDbPartition->d_func()->removeIndex("thumbnailUuid");
    mJsonDbPartition->d_func()->removeIndex("url");
    mJsonDbPartition->d_func()->removeIndex("bettyUuid");
}

void TestPartition::orQuery_data()
{
    QTest::addColumn<QString>("field1");
    QTest::addColumn<QString>("value1");
    QTest::addColumn<QString>("field2");
    QTest::addColumn<QString>("value2");
    QTest::addColumn<QString>("ordering");
    QTest::newRow("[? key1 = \"red\" | key1 = \"green\" ]")
        << "key1" << "red" << "key1" << "green" << "";
    QTest::newRow("[? key1 = \"red\" | key2 = \"bar\" ]")
        << "key1" << "red" << "key2" << "bar" << "";
    QTest::newRow("[? key1 = \"red\" | key1 = \"green\"][/key1]")
        << "key1" << "red" << "key1" << "green" << "[/key1]";
    QTest::newRow("[? key1 = \"red\" | key2 = \"bar\" ][/key1]")
        << "key1" << "red" << "key2" << "bar" << "[/key1]";
    QTest::newRow("[? key1 = \"red\" | key1 = \"green\"][/key2]")
        << "key1" << "red" << "key1" << "green" << "[/key2]";
    QTest::newRow("[? key1 = \"red\" | key2 = \"bar\" ][/key2]")
        << "key1" << "red" << "key2" << "bar" << "[/key2]";
    QTest::newRow("[? _type = \"RedType\" | _type = \"BarType\" ]")
        << "_type" << "RedType" << "_type" << "BarType" << "";

    addIndex(QLatin1String("key1"));

    QStringList keys1 = QStringList() << "red" << "green" << "blue";
    QStringList keys2 = QStringList() << "foo" << "bar" << "baz";
    foreach (QString key1, keys1) {
        foreach (QString key2, keys2) {
            JsonDbObject item;
            item.insert(JsonDbString::kTypeStr, QString("OrQueryTestType"));
            item.insert("key1", key1);
            item.insert("key2", key2);
            create(mOwner, item);

            key1[0] = key1[0].toUpper();
            key2[0] = key2[0].toUpper();
            item = JsonDbObject();
            item.insert(JsonDbString::kTypeStr, QString("%1Type").arg(key1));
            item.insert("notUsed1", key1);
            item.insert("notUsed2", key2);
            create(mOwner, item);

            item = JsonDbObject();
            item.insert(JsonDbString::kTypeStr, QString("%1Type").arg(key2));
            item.insert("notUsed1", key1);
            item.insert("notUsed2", key2);
            create(mOwner, item);
        }
    }
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("key1"));
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("key2"));
}

void TestPartition::orQuery()
{
    QFETCH(QString, field1);
    QFETCH(QString, value1);
    QFETCH(QString, field2);
    QFETCH(QString, value2);
    QFETCH(QString, ordering);
    QString typeQuery = "[?_type=\"OrQueryTestType\"]";
    QString queryString = (QString("%6[? %1 = \"%2\" | %3 = \"%4\" ]%5")
                           .arg(field1).arg(value1)
                           .arg(field2).arg(value2)
                           .arg(ordering).arg(((field1 != "_type") && (field2 != "_type")) ? typeQuery : ""));
    JsonDbQueryResult queryResult = find(mOwner, queryString);
    verifyGoodQueryResult(queryResult);
    QList<JsonDbObject> objects = queryResult.data;
    int count = 0;
    for (int ii = 0; ii < objects.size(); ii++) {
        QJsonObject o = objects.at(ii);
        QVERIFY((o.value(field1).toString() == value1) || (o.value(field2).toString() == value2));
        count++;
    }
    QVERIFY(count > 0);
    mJsonDbPartition->d_func()->removeIndex("key1");
    mJsonDbPartition->d_func()->removeIndex("key2");
}

void TestPartition::findByName()
{
    createContacts();
    int count = mContactList.size();
    if (!count)
        return;

    QJsonObject request;

    //int itemNumber = (int)((double)qrand() * count / RAND_MAX);
    int itemNumber = 0;
    JsonDbObject item(mContactList.at(itemNumber));;

    if (!item.contains("name"))
        qDebug() << "no name in item" << item;
    JsonDbQueryResult queryResult = find(mOwner, QString("[?name=\"%1\"]")
                                         .arg(item.value("name").toString()));
    verifyGoodQueryResult(queryResult);
}

void TestPartition::findEQ()
{
    createContacts();
    int count = mContactList.size();
    if (!count)
        return;

    int itemNumber = (int)((double)qrand() * count / RAND_MAX);
    JsonDbObject item(mContactList.at(itemNumber));

    JsonDbQueryResult queryResult = find(mOwner, QString("[?name.first=\"%1\"][?name.last=\"%2\"][?_type=\"contact\"]")
                                         .arg(item.valueByPath("name.first").toString())
                                         .arg(item.valueByPath("name.last").toString()));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);
    mJsonDbPartition->d_func()->removeIndex("name.first");
    mJsonDbPartition->d_func()->removeIndex("name.last");
}

void TestPartition::find10()
{
    createContacts();
    int count = mContactList.size();
    if (!count)
        return;

    QJsonObject result;

    int itemNumber = count / 2;
    JsonDbObject item(mContactList.at(itemNumber));

    QString query = QString("[?name.first<=\"%1\"][?_type=\"contact\"]")
            .arg(item.valueByPath("name.first").toString());

    JsonDbQueryParser parser;
    parser.setQuery(query);
    parser.parse();
    JsonDbQuery parsedQuery = parser.result();

    JsonDbQueryResult queryResult = mJsonDbPartition->queryObjects(mOwner, parsedQuery, 10);
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 10);
    mJsonDbPartition->d_func()->removeIndex("name.first");
    mJsonDbPartition->d_func()->removeIndex("contact");
}

void TestPartition::startsWith()
{
    QTest::ignoreMessage(QtSystemMsg, "[- compileIndexQuery ] searching all objects \"[?_type startsWith \"startsWith\"][= { _type: _type } ]\" ");
    addIndex(QLatin1String("name"));

    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("startsWithTest"));
    item.insert("name", QLatin1String("Wilma"));
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("startsWithTest"));
    item.insert("name", QLatin1String("Betty"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("startsWithTest"));
    item.insert("name", QLatin1String("Bettina"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("startsWithTest"));
    item.insert("name", QLatin1String("Benny"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"startsWithTest\"][?name startsWith \"Zelda\"]"));
    QCOMPARE(queryResult.data.size(), 0);

    queryResult = find(mOwner, QLatin1String("[?_type=\"startsWithTest\"][?name startsWith \"Wilma\"]"));
    QCOMPARE(queryResult.data.size(), 1);

    queryResult = find(mOwner, QLatin1String("[?_type=\"startsWithTest\"][?name startsWith \"B\"]"));
    QCOMPARE(queryResult.data.size(), 3);

    queryResult = find(mOwner, QLatin1String("[?_type=\"startsWithTest\"][?name startsWith \"Bett\"]"));
    QCOMPARE(queryResult.data.size(), 2);

    queryResult = find(mOwner, QLatin1String("[?_type startsWith \"startsWith\"][/name]"));
    QCOMPARE(queryResult.data.size(), 4);

    queryResult = find(mOwner, QLatin1String("[?_type startsWith \"startsWith\"][= { _type: _type } ]"));
    QCOMPARE(queryResult.data.size(), 4);
}

void TestPartition::comparison()
{
    addIndex(QLatin1String("latitude"), QLatin1String("number"));

    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("comparison"));
    item.insert("latitude", qint32(10));
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("comparison"));
    item.insert("latitude", qint32(42));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("comparison"));
    item.insert("latitude", qint32(0));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("comparison"));
    item.insert("latitude", qint32(-64));
    result = create(mOwner, item);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"comparison\"][?latitude > 10]"));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("latitude").toDouble(), (double)(42));

    queryResult = find(mOwner,  QLatin1String("[?_type=\"comparison\"][?latitude >= 10]"));
    QCOMPARE(queryResult.data.size(), 2);
    QCOMPARE(queryResult.data.at(0).value("latitude").toDouble(), (double)(10));
    QCOMPARE(queryResult.data.at(1).value("latitude").toDouble(), (double)(42));

    queryResult = find(mOwner, QLatin1String("[?_type=\"comparison\"][?latitude < 0 | latitude > 10][/_type]"));
    QCOMPARE(queryResult.data.size(), 2);

    queryResult = find(mOwner, QLatin1String("[?_type=\"comparison\"][?latitude < 0 | latitude > 10][/latitude]"));
    QCOMPARE(queryResult.data.size(), 2);

    queryResult = find(mOwner, QLatin1String("[?_type=\"comparison\"][?latitude < 0]"));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("latitude").toDouble(), (double)(-64));
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("latitude"));
}

void TestPartition::removedObjects()
{
    addIndex(QLatin1String("foo"));
    addIndex(QLatin1String("name"));
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("removedObjects"));
    item.insert("foo", QLatin1String("bar"));
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"removedObjects\"]"));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("foo").toString(), QLatin1String("bar"));

    // update the object
    item.remove(QLatin1String("foo"));
    item.insert("name", QLatin1String("anna"));
    result = update(mOwner, item);
    verifyGoodResult(result);

    queryResult = find(mOwner, QLatin1String("[?_type=\"removedObjects\"]"));
    QCOMPARE(queryResult.data.size(), 1);
    QVERIFY(!queryResult.data.at(0).contains("foo"));
    QCOMPARE(queryResult.data.at(0).value("name").toString(), QLatin1String("anna"));

    queryResult = find(mOwner, QLatin1String("[?_type=\"removedObjects\"][/name]"));
    QCOMPARE(queryResult.data.size(), 1);
    QVERIFY(!queryResult.data.at(0).contains("foo"));
    QCOMPARE(queryResult.data.at(0).value("name").toString(), QLatin1String("anna"));

    queryResult = find(mOwner, QLatin1String("[?_type=\"removedObjects\"][/foo]"));
    QCOMPARE(queryResult.data.size(), 0);

    // remove the object
    result = remove(mOwner, item);
    verifyGoodResult(result);

    queryResult = find(mOwner, QLatin1String("[?_type=\"removedObjects\"]"));
    QCOMPARE(queryResult.data.size(), 0);

    queryResult = find(mOwner, QLatin1String("[?_type=\"removedObjects\"][/foo]"));
    QCOMPARE(queryResult.data.size(), 0);
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("foo"));
    mJsonDbPartition->d_func()->removeIndex(QLatin1String("name"));
}

void TestPartition::arrayIndexQuery()
{
    addIndex(QLatin1String("phoneNumber"));

    QJsonArray objects(readJsonFile(":/partition/json/array.json").toArray());
    QMap<QString, JsonDbObject> toDelete;
    for (int i = 0; i < objects.size(); ++i) {
        JsonDbObject object = objects.at(i).toObject();
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
        toDelete.insert(object.value("_uuid").toString(), object);
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    queryResult = find(mOwner,  QLatin1String("[?_type=\"ContactInArray\"][?phoneNumbers.0.number =~\"/*789*/wi\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"][?phoneNumbers.0.validTime.0.timeFrom =\"09:00\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"][?phoneNumbers.0.validTime.0.timeFrom =\"10:00\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"][?phoneNumbers.0.validTime.0.timeTo =\"13:00\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"][?phoneNumbers.0.validTime.10.timeTo =\"13:00\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    queryResult = find(mOwner,  QLatin1String("[?_type=\"ContactInArray\"][?phoneNumbers.10.validTime.10.timeTo =\"13:00\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"][?foo.0.0.bar =\"val00\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    queryResult = find(mOwner, QLatin1String("[?_type=\"ContactInArray\"][?test.45 =\"joe\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    queryResult = find(mOwner,  QLatin1String("[?_type=\"ContactInArray\"][?test2.56.firstName =\"joe\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 1);

    foreach (JsonDbObject object, toDelete.values())
        verifyGoodResult(remove(mOwner, object));
}

void TestPartition::deindexError()
{
    JsonDbWriteResult result;

    // create document with a property "name"
    JsonDbObject foo;
    foo.insert("_type", QLatin1String("Foo"));
    foo.insert("name", QLatin1String("fooo"));
    result = create(mOwner, foo);
    verifyGoodResult(result);

    // create a schema object (has 'name' property)
    JsonDbObject schema;
    schema.insert("_type", QLatin1String("_schemaType"));
    schema.insert("name", QLatin1String("ArrayObject"));
    QJsonObject s;
    s.insert("type", QLatin1String("object"));
    //s.insert("extends", QLatin1String("View"));
    schema.insert("schema", s);
    result = create(mOwner, schema);
    verifyGoodResult(result);

    // create instance of ArrayView (defined by the schema)
    JsonDbObject arrayView;
    arrayView.insert("_type", QLatin1String("ArrayView"));
    arrayView.insert("name", QLatin1String("fooo"));
    result = create(mOwner, arrayView);
    verifyGoodResult(result);

    addIndex(QLatin1String("name"));

    // now remove some objects that have "name" property
    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"Foo\"]"));
    verifyGoodQueryResult(queryResult);
    JsonDbObjectList objs = queryResult.data;
    QVERIFY(!objs.isEmpty());
    foreach (const JsonDbObject &obj, objs) {
        result = remove(mOwner, obj);
        verifyGoodResult(result);
    }
}

void TestPartition::expectedOrder()
{
    QStringList list;
    QStringList uuids;

    // Bug occurs when there is a string A and multiple other strings that are longer
    // than A but the first A.length characters are exactly the same as A's.
    const int count = 100;
    for (int i = 0; i < count; ++i) {
        int r = qrand();
        QString str0 = QString::number(r);
        QString str1 = str0 + QString(str0[0]);
        QString str2 = str0 + QString(str0[0]) + QString(str0[0]);
        list.append(str0);
        list.append(str1);
        list.append(str2);
    }

    foreach (const QString &str, list) {
        JsonDbObject item;
        item.insert("_type", QLatin1String(__FUNCTION__));
        item.insert("order", str);
        JsonDbWriteResult result = create(mOwner, item);
        verifyGoodResult(result);
        uuids << result.objectsWritten[0].uuid().toString();
    }

    // This is the order we expect from the query
    list.sort();

    JsonDbQueryResult queryResult = find(mOwner, QString("[?_type=\"%1\"][/order]").arg(__FUNCTION__));
    verifyGoodQueryResult(queryResult);
    JsonDbObjectList dataList = queryResult.data;

    QCOMPARE(dataList.size(), list.size());

    for (int i = 0; i < dataList.size(); ++i) {
        JsonDbObject obj = dataList.at(i);
        QVERIFY(obj.contains("order"));
        QCOMPARE(obj.value("order").toString(), list.at(i));
    }
}

void TestPartition::indexQueryOnCommonValues()
{
    // Specific indexing bug when you have records inserted that only differ
    // by their _type
    createContacts();

    for (int ii = 0; ii < mContactList.size(); ii++) {
        JsonDbObject data(mContactList.at(ii));
        JsonDbObject chaff;
        chaff.insert(JsonDbString::kTypeStr, QString("com.example.ContactChaff"));
        QStringList skipKeys = (QStringList() << JsonDbString::kUuidStr << JsonDbString::kVersionStr << JsonDbString::kTypeStr);
        foreach (QString key, data.keys()) {
            if (!skipKeys.contains(key))
                chaff.insert(key, data.value(key));
        }

        JsonDbWriteResult result = create(mOwner, chaff);
        verifyGoodResult(result);
    }

    int count = mContactList.size();

    QCOMPARE(count > 0, true);

    QJsonObject request;

    int itemNumber = (int)((double)qrand() * count / RAND_MAX);

    JsonDbObject item(mContactList.at(itemNumber));
    QString first = item.valueByPath("name.first").toString();
    QString last = item.valueByPath("name.last").toString();

    JsonDbQueryResult queryResult= find(mOwner, QString("[?name.first=\"%1\"][?name.last=\"%2\"][?_type=\"contact\"]")
                                        .arg(first)
                                        .arg(last));
    verifyGoodQueryResult(queryResult);

    QCOMPARE(queryResult.data.size(), 1);
}

void TestPartition::removeIndexes()
{
    addIndex("wacky_index");
    QVERIFY(mJsonDbPartition->findObjectTable(JsonDbString::kSchemaTypeStr)->index("wacky_index") != 0);

    QVERIFY(mJsonDbPartition->d_func()->removeIndex("wacky_index"));
    QVERIFY(mJsonDbPartition->findObjectTable(JsonDbString::kSchemaTypeStr)->index("wacky_index") == 0);

    JsonDbObject indexObject;
    indexObject.insert(JsonDbString::kTypeStr, QLatin1String("Index"));
    indexObject.insert("propertyName", QLatin1String("predicate"));
    indexObject.insert("propertyType", QLatin1String("string"));

    JsonDbWriteResult result = create(mOwner, indexObject);
    verifyGoodResult(result);
    QVERIFY(mJsonDbPartition->findObjectTable("Index")->index("predicate") != 0);

    indexObject.insert("propertyType", QLatin1String("integer"));
    result = update(mOwner, indexObject);
    verifyErrorResult(result);

    result = remove(mOwner, indexObject);
    verifyGoodResult(result);
    QVERIFY(mJsonDbPartition->findObjectTable("Index")->index("predicate") == 0);
}

void TestPartition::setOwner()
{
    jsondbSettings->setEnforceAccessControl(true);

    mOwner->setAllowAll(true);
    QLatin1String fooOwnerStr("com.foo.owner");

    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("SetOwnerType"));
    item.insert(JsonDbString::kOwnerStr, fooOwnerStr);
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    GetObjectsResult getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("SetOwnerType"));
    QCOMPARE(getObjects.data.at(0).value(JsonDbString::kOwnerStr).toString(), fooOwnerStr);

    result = remove(mOwner, item);
    verifyGoodResult(result);

    QScopedPointer<JsonDbOwner> unauthOwner(new JsonDbOwner(this));
    unauthOwner->setOwnerId("com.example.OtherOwner");
    unauthOwner->setAllowAll(false);
    unauthOwner->setAllowedObjects(QLatin1String("all"), "write", (QStringList() << QLatin1String("[*]")));
    unauthOwner->setAllowedObjects(QLatin1String("all"), "read", (QStringList() << QLatin1String("[*]")));

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("SetOwnerType2"));
    item.insert(JsonDbString::kOwnerStr, fooOwnerStr);
    result = create(unauthOwner.data(), item);
    verifyGoodResult(result);

    getObjects = mJsonDbPartition->d_func()->getObjects(JsonDbString::kTypeStr, QLatin1String("SetOwnerType2"));
    QVERIFY(getObjects.data.at(0).value(JsonDbString::kOwnerStr).toString()
            != fooOwnerStr);
    result = remove(unauthOwner.data(), item);
    verifyGoodResult(result);

    jsondbSettings->setEnforceAccessControl(false);
}

void TestPartition::indexPropertyFunction()
{
    JsonDbObject index;
    index.insert(JsonDbString::kTypeStr, QLatin1String("Index"));
    index.insert(QLatin1String("name"), QLatin1String("propertyFunctionIndex"));
    index.insert(QLatin1String("propertyType"), QLatin1String("number"));
    index.insert(QLatin1String("propertyFunction"), QLatin1String("function (o) { if (o.from !== undefined) jsondb.emit(o.from); else jsondb.emit(o.to); }"));
    JsonDbWriteResult result = create(mOwner, index);
    verifyGoodResult(result);

    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexPropertyFunction"));
    item.insert("from", 10);
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexPropertyFunction"));
    item.insert("to", 42);
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexPropertyFunction"));
    item.insert("from", 0);
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexPropertyFunction"));
    item.insert("to", -64);
    result = create(mOwner, item);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult = find(mOwner,
                                         QString("[?_type=\"IndexPropertyFunction\"][?propertyFunctionIndex > 10][/propertyFunctionIndex]"));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("to").toDouble(), (double)42);

    queryResult = find(mOwner,
                       QLatin1String("[?_type=\"IndexPropertyFunction\"][?propertyFunctionIndex >= 10][/propertyFunctionIndex]"));
    QCOMPARE(queryResult.data.size(), 2);
    QCOMPARE(queryResult.data.at(0).value("from").toDouble(), (double)10);
    QCOMPARE(queryResult.data.at(1).value("to").toDouble(), (double)42);

    queryResult = find(mOwner, QLatin1String("[?_type=\"IndexPropertyFunction\"][?propertyFunctionIndex < 0]"));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("to").toDouble(), (double)-64);

    // verify we can fetch the index values
    queryResult = find(mOwner, QLatin1String("[?_type=\"IndexPropertyFunction\"][/propertyFunctionIndex]"));
    QCOMPARE(queryResult.data.size(), 4);
    JsonDbObjectList data = queryResult.data;
    for (int i = 0; i < data.size(); i++) {
        JsonDbObject o = queryResult.data.at(i);
        QVERIFY(o.contains("_indexValue"));
        QCOMPARE(o.value("_indexValue").toDouble(), (o.contains("to") ? o.value("to").toDouble() : o.value("from").toDouble()));
    }

    // verify we can fetch the index values
    queryResult = find(mOwner, QLatin1String("[?_type=\"IndexPropertyFunction\"][?propertyFunctionIndex > 10][/propertyFunctionIndex][= { _uuid: _uuid, _indexValue: _indexValue, to: to }]"));
    QCOMPARE(queryResult.data.size(), 1);
    QCOMPARE(queryResult.data.at(0).value("to").toDouble(), (double)42);
    QVERIFY(queryResult.data.at(0).contains("_indexValue"));
    QCOMPARE(queryResult.data.at(0).value("_indexValue").toDouble(), (double)42);

    // now cleanup QJSEngine and verify that propertyFunction is still invoked
    mJsonDbPartition->flushCaches();

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexPropertyFunction"));
    item.insert("from", 128);
    result = create(mOwner, item);
    verifyGoodResult(result);

    queryResult = find(mOwner, QLatin1String("[?_type=\"IndexPropertyFunction\"][/propertyFunctionIndex]"));
    QCOMPARE(queryResult.data.size(), 5);
    QCOMPARE((int)queryResult.data.at(4).value(QLatin1String("from")).toDouble(), 128);
}

void TestPartition::indexCollation()
{
#ifndef NO_COLLATION_SUPPORT
    JsonDbObject item;
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u4e00"));
    item.insert("lastName", QLatin1String("1-Yi"));
    JsonDbWriteResult result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u4e8c"));
    item.insert("lastName", QLatin1String("2-Er"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u4e09"));
    item.insert("lastName", QLatin1String("3-San"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u82b1"));
    item.insert("lastName", QLatin1String("4-Hua"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u9489"));
    item.insert("lastName", QLatin1String("5-Ding"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u516d"));
    item.insert("lastName", QLatin1String("6-Liu"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    item = JsonDbObject();
    item.insert(JsonDbString::kTypeStr, QLatin1String("IndexCollation"));
    item.insert("firstName", QString::fromUtf8("\u5b54"));
    item.insert("lastName", QLatin1String("7-Kong"));
    result = create(mOwner, item);
    verifyGoodResult(result);

    JsonDbObject pinyinIndex;
    pinyinIndex.insert(JsonDbString::kTypeStr, QLatin1String("Index"));
    pinyinIndex.insert(QLatin1String("name"), QLatin1String("pinyinIndex"));
    pinyinIndex.insert(QLatin1String("propertyName"), QLatin1String("firstName"));
    pinyinIndex.insert(QLatin1String("propertyType"), QLatin1String("string"));
    pinyinIndex.insert(QLatin1String("locale"), QLatin1String("zh_CN"));
    pinyinIndex.insert(QLatin1String("collation"), QLatin1String("pinyin"));
    result = create(mOwner, pinyinIndex);
    verifyGoodResult(result);

    JsonDbObject strokeIndex;
    strokeIndex.insert(JsonDbString::kTypeStr, QLatin1String("Index"));
    strokeIndex.insert(QLatin1String("name"), QLatin1String("strokeIndex"));
    strokeIndex.insert(QLatin1String("propertyName"), QLatin1String("firstName"));
    strokeIndex.insert(QLatin1String("propertyType"), QLatin1String("string"));
    strokeIndex.insert(QLatin1String("locale"), QLatin1String("zh_CN"));
    strokeIndex.insert(QLatin1String("collation"), QLatin1String("stroke"));
    result = create(mOwner, strokeIndex);
    verifyGoodResult(result);

    JsonDbQueryResult queryResult1 = find(mOwner, QLatin1String("[?_type=\"IndexCollation\"][/pinyinIndex]"));
    QCOMPARE(queryResult1.data.size(), 7);
    QCOMPARE(queryResult1.data.at(0).value("lastName").toString(), QLatin1String("5-Ding"));
    QCOMPARE(queryResult1.data.at(1).value("lastName").toString(), QLatin1String("2-Er"));
    QCOMPARE(queryResult1.data.at(2).value("lastName").toString(), QLatin1String("4-Hua"));
    QCOMPARE(queryResult1.data.at(3).value("lastName").toString(), QLatin1String("7-Kong"));
    QCOMPARE(queryResult1.data.at(4).value("lastName").toString(), QLatin1String("6-Liu"));
    QCOMPARE(queryResult1.data.at(5).value("lastName").toString(), QLatin1String("3-San"));
    QCOMPARE(queryResult1.data.at(6).value("lastName").toString(), QLatin1String("1-Yi"));

    JsonDbQueryResult queryResult2 = find(mOwner, QLatin1String("[?_type=\"IndexCollation\"][/strokeIndex]"));
    QCOMPARE(queryResult2.data.size(), 7);
    QCOMPARE(queryResult2.data.at(0).value("lastName").toString(), QLatin1String("1-Yi"));
    QCOMPARE(queryResult2.data.at(1).value("lastName").toString(), QLatin1String("2-Er"));
    QCOMPARE(queryResult2.data.at(2).value("lastName").toString(), QLatin1String("3-San"));
    QCOMPARE(queryResult2.data.at(3).value("lastName").toString(), QLatin1String("6-Liu"));
    QCOMPARE(queryResult2.data.at(4).value("lastName").toString(), QLatin1String("7-Kong"));
    // TODO: a bug in CLDR collation data in ICU 4.8.1
    //QCOMPARE(queryResult2.data.at(5).value("lastName").toString(), QLatin1String("4-Hua"));
    //QCOMPARE(queryResult2.data.at(6).value("lastName").toString(), QLatin1String("5-Ding"));
#else
    QSKIP("This test requires NO_COLLATION_SUPPORT is not defined!");
#endif
}

void TestPartition::indexCaseSensitive()
{
    QJsonArray objects(readJsonFile(":/partition/json/index-casesensitive.json").toArray());
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
    }

    JsonDbQueryResult queryResult1 = find(mOwner, QLatin1String("[?_type=\"IndexCaseSensitive\"][/caseSensitiveIndex]"));
    QCOMPARE(queryResult1.data.size(), 7);
    QCOMPARE(queryResult1.data.at(0).value("field").toString(), QLatin1String("AAA"));
    QCOMPARE(queryResult1.data.at(1).value("field").toString(), QLatin1String("AAa"));
    QCOMPARE(queryResult1.data.at(2).value("field").toString(), QLatin1String("Aaa"));
    QCOMPARE(queryResult1.data.at(3).value("field").toString(), QLatin1String("Aab"));
    QCOMPARE(queryResult1.data.at(4).value("field").toString(), QLatin1String("Aba"));
    QCOMPARE(queryResult1.data.at(5).value("field").toString(), QLatin1String("aBB"));
    QCOMPARE(queryResult1.data.at(6).value("field").toString(), QLatin1String("aaa"));

    JsonDbQueryResult queryResult2 = find(mOwner, QLatin1String("[?_type=\"IndexCaseSensitive\"][/caseInSensitiveIndex]"));
    QCOMPARE(queryResult2.data.size(), 7);
    QCOMPARE(queryResult2.data.at(4).value("field").toString(), QLatin1String("Aab"));
    QCOMPARE(queryResult2.data.at(5).value("field").toString(), QLatin1String("Aba"));
    QCOMPARE(queryResult2.data.at(6).value("field").toString(), QLatin1String("aBB"));
}

void TestPartition::indexCasePreference()
{
#ifndef NO_COLLATION_SUPPORT
    QJsonArray objects(readJsonFile(":/partition/json/index-casepreference.json").toArray());
    for (int ii = 0; ii < objects.size(); ii++) {
        JsonDbObject object(objects.at(ii).toObject());
        JsonDbWriteResult result = create(mOwner, object);
        verifyGoodResult(result);
    }

    JsonDbQueryResult queryResult1 = find(mOwner, QLatin1String("[?_type=\"IndexCasePreference\"][/casePreferenceIndex1]"));
    QCOMPARE(queryResult1.data.size(), 7);
    QCOMPARE(queryResult1.data.at(0).value("field").toString(), QLatin1String("aaa"));
    QCOMPARE(queryResult1.data.at(1).value("field").toString(), QLatin1String("Aaa"));
    QCOMPARE(queryResult1.data.at(2).value("field").toString(), QLatin1String("AAa"));
    QCOMPARE(queryResult1.data.at(3).value("field").toString(), QLatin1String("AAA"));
    QCOMPARE(queryResult1.data.at(4).value("field").toString(), QLatin1String("Aab"));
    QCOMPARE(queryResult1.data.at(5).value("field").toString(), QLatin1String("Aba"));
    QCOMPARE(queryResult1.data.at(6).value("field").toString(), QLatin1String("aBB"));

    JsonDbQueryResult queryResult2 = find(mOwner, QLatin1String("[?_type=\"IndexCasePreference\"][/casePreferenceIndex2]"));    QCOMPARE(queryResult2.data.size(), 7);
    QCOMPARE(queryResult2.data.at(0).value("field").toString(), QLatin1String("AAA"));
    QCOMPARE(queryResult2.data.at(1).value("field").toString(), QLatin1String("AAa"));
    QCOMPARE(queryResult2.data.at(2).value("field").toString(), QLatin1String("Aaa"));
    QCOMPARE(queryResult2.data.at(3).value("field").toString(), QLatin1String("aaa"));
    QCOMPARE(queryResult2.data.at(4).value("field").toString(), QLatin1String("Aab"));
    QCOMPARE(queryResult2.data.at(5).value("field").toString(), QLatin1String("Aba"));
    QCOMPARE(queryResult2.data.at(6).value("field").toString(), QLatin1String("aBB"));

    JsonDbQueryResult queryResult3 = find(mOwner, QLatin1String("[?_type=\"IndexCasePreference\"][/casePreferenceIndex3]"));    QCOMPARE(queryResult3.data.size(), 7);
    QCOMPARE(queryResult3.data.at(0).value("field").toString(), QLatin1String("aaa"));
    QCOMPARE(queryResult3.data.at(1).value("field").toString(), QLatin1String("Aaa"));
    QCOMPARE(queryResult3.data.at(2).value("field").toString(), QLatin1String("AAa"));
    QCOMPARE(queryResult3.data.at(3).value("field").toString(), QLatin1String("AAA"));
    QCOMPARE(queryResult3.data.at(4).value("field").toString(), QLatin1String("Aab"));
    QCOMPARE(queryResult3.data.at(5).value("field").toString(), QLatin1String("Aba"));
    QCOMPARE(queryResult3.data.at(6).value("field").toString(), QLatin1String("aBB"));
#else
    QSKIP("This test requires NO_COLLATION_SUPPORT is not defined!");
#endif
}

void TestPartition::settings()
{
    JsonDbSettings currentSettings;
    const QMetaObject *metaObject = jsondbSettings->metaObject();
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        QByteArray property = metaObject->property(i).name();
        currentSettings.setProperty(property, jsondbSettings->property(property));
    }

    // first explicitly set the values
    jsondbSettings->setRejectStaleUpdates(true);
    jsondbSettings->setDebug(false);
    jsondbSettings->setVerbose(false);
    jsondbSettings->setPerformanceLog(true);
    jsondbSettings->setCacheSize(64);
    jsondbSettings->setCompactRate(2000);
    jsondbSettings->setEnforceAccessControl(true);
    jsondbSettings->setTransactionSize(50);
    jsondbSettings->setValidateSchemas(true);
    jsondbSettings->setSyncInterval(10000);
    jsondbSettings->setIndexSyncInterval(20000);
    jsondbSettings->setDebugQuery(true);

    QVERIFY(jsondbSettings->rejectStaleUpdates());
    QVERIFY(!jsondbSettings->debug());
    QVERIFY(!jsondbSettings->verbose());
    QVERIFY(jsondbSettings->performanceLog());
    QCOMPARE(jsondbSettings->cacheSize(), 64);
    QCOMPARE(jsondbSettings->compactRate(), 2000);
    QVERIFY(jsondbSettings->enforceAccessControl());
    QCOMPARE(jsondbSettings->transactionSize(), 50);
    QVERIFY(jsondbSettings->validateSchemas());
    QCOMPARE(jsondbSettings->syncInterval(), 10000);
    QCOMPARE(jsondbSettings->indexSyncInterval(), 20000);
    QVERIFY(jsondbSettings->debugQuery());

    jsondbSettings->setRejectStaleUpdates(false);
    jsondbSettings->setDebug(true);
    jsondbSettings->setVerbose(true);
    jsondbSettings->setPerformanceLog(false);
    jsondbSettings->setEnforceAccessControl(false);
    jsondbSettings->setValidateSchemas(false);
    jsondbSettings->setDebugQuery(false);

    // then with environment variables
    qputenv("JSONDB_REJECT_STALE_UPDATES", "true");
    qputenv("JSONDB_DEBUG", "false");
    qputenv("JSONDB_VERBOSE", "false");
    qputenv("JSONDB_PERFORMANCE_LOG", "true");
    qputenv("JSONDB_CACHE_SIZE", "256");
    qputenv("JSONDB_COMPACT_RATE", "1500");
    qputenv("JSONDB_ENFORCE_ACCESS_CONTROL", "true");
    qputenv("JSONDB_TRANSACTION_SIZE", "75");
    qputenv("JSONDB_VALIDATE_SCHEMAS", "true");
    qputenv("JSONDB_SYNC_INTERVAL", "6000");
    qputenv("JSONDB_INDEX_SYNC_INTERVAL", "17000");
    qputenv("JSONDB_DEBUG_QUERY", "true");
    jsondbSettings->reload();

    QVERIFY(jsondbSettings->rejectStaleUpdates());
    QVERIFY(!jsondbSettings->debug());
    QVERIFY(!jsondbSettings->verbose());
    QVERIFY(jsondbSettings->performanceLog());
    QCOMPARE(jsondbSettings->cacheSize(), 256);
    QCOMPARE(jsondbSettings->compactRate(), 1500);
    QVERIFY(jsondbSettings->enforceAccessControl());
    QCOMPARE(jsondbSettings->transactionSize(), 75);
    QVERIFY(jsondbSettings->validateSchemas());
    QCOMPARE(jsondbSettings->syncInterval(), 6000);
    QCOMPARE(jsondbSettings->indexSyncInterval(), 17000);
    QVERIFY(jsondbSettings->debugQuery());

    // restore settings
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        QByteArray property = metaObject->property(i).name();
        jsondbSettings->setProperty(property, currentSettings.property(property));
    }
}

void TestPartition::typeChangeIndex()
{
    JsonDbObject test1;
    test1.insert(JsonDbString::kTypeStr, QLatin1String("TestContactTCI"));
    test1.insert(QLatin1String("firstName"), QLatin1String("adam"));
    verifyGoodResult(create(mOwner, test1));

    JsonDbObject test2;
    test2.insert(JsonDbString::kTypeStr, QLatin1String("TestContactTCI"));
    test2.insert(QLatin1String("firstName"), QLatin1String("Betty"));
    verifyGoodResult(create(mOwner, test2));

    // create an object that's not an index
    QString uuid = QUuid::createUuid().toString();
    JsonDbObject changing;
    changing.insert(JsonDbString::kUuidStr, uuid);
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.an.index"));
    verifyGoodResult(create(mOwner, changing));

    // FIXME: I think this only works currently because we auto-create the index
    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"TestContactTCI\"][/firstName]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.count(), 2);
    QCOMPARE(queryResult.data.at(0).value(JsonDbString::kUuidStr).toString(),
             test2.value(JsonDbString::kUuidStr).toString());
    QCOMPARE(queryResult.data.at(1).value(JsonDbString::kUuidStr).toString(),
             test1.value(JsonDbString::kUuidStr).toString());

    // change the object into an Index
    changing.insert(JsonDbString::kTypeStr, JsonDbString::kIndexTypeStr);
    changing.insert(JsonDbString::kNameStr, QLatin1String("firstName"));
    changing.insert(JsonDbString::kPropertyNameStr, QLatin1String("firstName"));
    changing.insert(JsonDbString::kCaseSensitiveStr, false);
    verifyGoodResult(update(mOwner, changing));

    queryResult = find(mOwner, QLatin1String("[?_type=\"TestContactTCI\"][/firstName]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.count(), 2);
    QCOMPARE(queryResult.data.at(0).value(JsonDbString::kUuidStr).toString(),
             test1.value(JsonDbString::kUuidStr).toString());
    QCOMPARE(queryResult.data.at(1).value(JsonDbString::kUuidStr).toString(),
             test2.value(JsonDbString::kUuidStr).toString());

    // change it back into a non-Index
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.an.index"));
    verifyGoodResult(update(mOwner, changing));

    // make sure the old ordering is back
    queryResult = find(mOwner, QLatin1String("[?_type=\"TestContactTCI\"][/firstName]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.count(), 2);
    QCOMPARE(queryResult.data.at(0).value(JsonDbString::kUuidStr).toString(),
             test2.value(JsonDbString::kUuidStr).toString());
    QCOMPARE(queryResult.data.at(1).value(JsonDbString::kUuidStr).toString(),
             test1.value(JsonDbString::kUuidStr).toString());

    verifyGoodResult(remove(mOwner, changing));
    verifyGoodResult(remove(mOwner, test1));
    verifyGoodResult(remove(mOwner, test2));
}

void TestPartition::typeChangeMap()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-reduce.json").toArray());

    JsonDbObjectList schemas;
    JsonDbObject map;

    QMap<QString, JsonDbObject> toDelete;

    for (int i = 0; i < objects.size(); ++i) {
        QJsonObject object(objects.at(i).toObject());
        JsonDbObject doc(object);

        if (doc.type() == JsonDbString::kMapTypeStr)
            doc.insert(JsonDbString::kTypeStr, QLatin1String("not.a.Map"));

        if (doc.type() != JsonDbString::kReduceTypeStr) {
            JsonDbWriteResult result = create(mOwner, doc);
            verifyGoodResult(result);

            if (doc.value(JsonDbString::kTypeStr).toString() == QLatin1String("not.a.Map"))
                map = doc;
            else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kSchemaTypeStr)
                schemas.append(doc);
            else
                toDelete.insert(doc.value("_uuid").toString(), doc);
        }
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    // change the object into a Map and test the result
    map.insert(JsonDbString::kTypeStr, JsonDbString::kMapTypeStr);
    verifyGoodResult(update(mOwner, map));

    queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 5);

    // make it not a Map again and test the result
    map.insert(JsonDbString::kTypeStr, QLatin1String("not.a.Map"));
    verifyGoodResult(update(mOwner, map));

    queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    verifyGoodResult(remove(mOwner, map));
    foreach (JsonDbObject schema, schemas)
        verifyGoodResult(remove(mOwner, schema));

    foreach (JsonDbObject del, toDelete.values())
        verifyGoodResult(remove(mOwner, del));
}

void TestPartition::typeChangeReduce()
{
    JsonDbObject test1;
    test1.insert(JsonDbString::kTypeStr, QLatin1String("MyContact"));
    test1.insert(QLatin1String("firstName"), QLatin1String("Bill"));
    verifyGoodResult(create(mOwner, test1));

    JsonDbObject test2;
    test2.insert(JsonDbString::kTypeStr, QLatin1String("MyContact"));
    test2.insert(QLatin1String("firstName"), QLatin1String("Alice"));
    verifyGoodResult(create(mOwner, test2));

    QJsonArray objects(readJsonFile(":/partition/json/reduce.json").toArray());

    JsonDbObject reduce;
    JsonDbObject schema;

    for (int i = 0; i < objects.size(); ++i) {
        QJsonObject object(objects.at(i).toObject());
        JsonDbObject doc(object);

        if (doc.type() == JsonDbString::kReduceTypeStr)
            doc.insert(JsonDbString::kTypeStr, QLatin1String("not.a.Reduce"));

        JsonDbWriteResult result = create(mOwner, doc);
        verifyGoodResult(result);

        if (doc.value(JsonDbString::kTypeStr).toString() == QLatin1String("not.a.Reduce"))
            reduce = doc;
        else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kSchemaTypeStr)
            schema = doc;
    }

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    // make it into a Reduce and check the result
    reduce.insert(JsonDbString::kTypeStr, JsonDbString::kReduceTypeStr);
    verifyGoodResult(update(mOwner, reduce));

    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    // make it not a Reduce again and check the result
    reduce.insert(JsonDbString::kTypeStr, QLatin1String("not.a.Reduce"));
    verifyGoodResult(update(mOwner, reduce));

    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 0);

    verifyGoodResult(remove(mOwner, reduce));
    verifyGoodResult(remove(mOwner, schema));
    verifyGoodResult(remove(mOwner, test1));
    verifyGoodResult(remove(mOwner, test2));
}

void TestPartition::typeChangeMapSource()
{
    QJsonArray objects(readJsonFile(":/partition/json/map-reduce.json").toArray());

    JsonDbObject map;
    JsonDbObject schema;
    QMap<QString, JsonDbObject> toDelete;

    for (int i = 0; i < objects.size(); ++i) {
        QJsonObject object(objects.at(i).toObject());
        JsonDbObject doc(object);

        if (doc.type() != JsonDbString::kReduceTypeStr) {
            JsonDbWriteResult result = create(mOwner, doc);
            verifyGoodResult(result);

            if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kMapTypeStr)
                map = doc;
            else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kSchemaTypeStr)
                schema = doc;
            else
                toDelete.insert(doc.value("_uuid").toString(), doc);
        }
    }

    JsonDbObject changing;
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.a.Contact"));
    QJsonObject phoneNumber1;
    phoneNumber1.insert(QLatin1String("type"), QLatin1String("home"));
    phoneNumber1.insert(QLatin1String("number"), QLatin1String("+4700112233"));
    QJsonObject phoneNumber2;
    phoneNumber2.insert(QLatin1String("type"), QLatin1String("work"));
    phoneNumber2.insert(QLatin1String("number"), QLatin1String("+4711223344"));
    QJsonArray phoneNumbers;
    phoneNumbers.append(phoneNumber1);
    phoneNumbers.append(phoneNumber2);
    changing.insert(QLatin1String("phoneNumbers"), phoneNumbers);

    verifyGoodResult(create(mOwner, changing));

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 5);

    // change the _type to a source type of the Map
    changing.insert(JsonDbString::kTypeStr, QLatin1String("Contact"));
    verifyGoodResult(update(mOwner, changing));

    queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 7);

    // change it back to a non-source type
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.a.Contact"));
    verifyGoodResult(update(mOwner, changing));

    queryResult = find(mOwner, QLatin1String("[?_type=\"Phone\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 5);

    verifyGoodResult(remove(mOwner, map));
    verifyGoodResult(remove(mOwner, schema));

    foreach (JsonDbObject del, toDelete.values())
        verifyGoodResult(remove(mOwner, del));
}

void TestPartition::typeChangeReduceSource()
{
    JsonDbObject test1;
    test1.insert(JsonDbString::kTypeStr, QLatin1String("MyContact"));
    test1.insert(QLatin1String("firstName"), QLatin1String("Bill"));
    verifyGoodResult(create(mOwner, test1));

    JsonDbObject test2;
    test2.insert(JsonDbString::kTypeStr, QLatin1String("MyContact"));
    test2.insert(QLatin1String("firstName"), QLatin1String("Alice"));
    verifyGoodResult(create(mOwner, test2));

    QJsonArray objects(readJsonFile(":/partition/json/reduce.json").toArray());

    JsonDbObject reduce;
    JsonDbObject schema;

    for (int i = 0; i < objects.size(); ++i) {
        QJsonObject object(objects.at(i).toObject());
        JsonDbObject doc(object);

        JsonDbWriteResult result = create(mOwner, doc);
        verifyGoodResult(result);

        if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kReduceTypeStr)
            reduce = doc;
        else if (object.value(JsonDbString::kTypeStr).toString() == JsonDbString::kSchemaTypeStr)
            schema = doc;
    }

    JsonDbObject changing;
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.a.MyContact"));
    changing.insert(QLatin1String("firstName"), QLatin1String("Bob"));
    verifyGoodResult(create(mOwner, changing));

    JsonDbQueryResult queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    // change the object so it's the source type of the Reduce
    changing.insert(JsonDbString::kTypeStr, QLatin1String("MyContact"));
    verifyGoodResult(update(mOwner, changing));

    // re-query to see if the Reduce picked it up
    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 3);

    // change the object back to not being a source type of the Reduce
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.a.MyContact"));
    verifyGoodResult(update(mOwner, changing));

    // re-query to see if the Reduce picked it up
    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    // one more time, but this time adding to an existing Reduce target object
    changing.insert(JsonDbString::kTypeStr, QLatin1String("MyContact"));
    changing.insert(QLatin1String("firstName"), QLatin1String("Alice"));
    verifyGoodResult(update(mOwner, changing));

    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    foreach (const JsonDbObject &object, queryResult.data) {
        if (object.value(QLatin1String("firstName")).toString() == QLatin1String("Alice")) {
            QCOMPARE(static_cast<int>(object.value(QLatin1String("count")).toDouble()), 2);
        } else {
            QVERIFY(object.value(QLatin1String("firstName")).toString() == QLatin1String("Bill"));
            QCOMPARE(static_cast<int>(object.value(QLatin1String("count")).toDouble()), 1);
        }
    }

    // change the type again and make sure the count goes back down
    changing.insert(JsonDbString::kTypeStr, QLatin1String("not.a.MyContact"));
    verifyGoodResult(update(mOwner, changing));

    queryResult = find(mOwner, QLatin1String("[?_type=\"MyContactCount\"]"));
    verifyGoodQueryResult(queryResult);
    QCOMPARE(queryResult.data.size(), 2);

    foreach (const JsonDbObject &object, queryResult.data)
        QCOMPARE(static_cast<int>(object.value(QLatin1String("count")).toDouble()), 1);

    verifyGoodResult(remove(mOwner, reduce));
    verifyGoodResult(remove(mOwner, schema));
    verifyGoodResult(remove(mOwner, test1));
    verifyGoodResult(remove(mOwner, test2));
    verifyGoodResult(remove(mOwner, changing));
}

void TestPartition::typeChangeSchema()
{
    bool currentValidateSchemas = jsondbSettings->validateSchemas();
    jsondbSettings->setValidateSchemas(true);
    QJsonObject schemaDef(readJsonFile(":/json-validation/required-schema.json").toObject());
    JsonDbObject schema;
    schema.insert(JsonDbString::kTypeStr, QLatin1String("not.a._schemaType"));
    schema.insert(JsonDbString::kSchemaStr, schemaDef);
    schema.insert(JsonDbString::kNameStr, QLatin1String("TestObject"));
    verifyGoodResult(create(mOwner, schema));

    // test object which is missing the required "important" field
    JsonDbObject test;
    test.insert(JsonDbString::kTypeStr, QLatin1String("TestObject"));
    verifyGoodResult(create(mOwner, test));

    // change the object into a schema and try the invalid update
    schema.insert(JsonDbString::kTypeStr, JsonDbString::kSchemaTypeStr);
    verifyGoodResult(update(mOwner, schema));

    test.insert(QLatin1String("notimportant"), QLatin1String("foo"));
    JsonDbWriteResult result = update(mOwner, test);
    verifyErrorResult(result);
    QCOMPARE(result.code, JsonDbError::FailedSchemaValidation);
    verifyGoodResult(remove(mOwner, test));

    // change it back into a non-schema and make sure the update goes through
    schema.insert(JsonDbString::kTypeStr, QLatin1String("not.a._schemaType"));
    verifyGoodResult(update(mOwner, schema));

    test.remove(JsonDbString::kDeletedStr);
    verifyGoodResult(update(mOwner, test));

    verifyGoodResult(remove(mOwner, test));
    verifyGoodResult(remove(mOwner, schema));
    jsondbSettings->setValidateSchemas(currentValidateSchemas);
}

void TestPartition::updateListWithIndex()
{
    JsonDbObject zed;
    {
        QList<JsonDbObject> list;
        JsonDbObject index;
        index.insert(JsonDbString::kUuidStr, QLatin1String("{a2f52c25-810f-4162-ad1f-6e3409f9eb6a}"));
        index.insert(JsonDbString::kTypeStr, JsonDbString::kIndexTypeStr);
        index.insert(JsonDbString::kNameStr, QLatin1String("updateListWithIndex"));
        index.insert(JsonDbString::kPropertyNameStr, QLatin1String("foobar"));
        index.insert(JsonDbString::kPropertyTypeStr, QLatin1String("string"));
        list.append(index);
        JsonDbObject item;
        item.insert(JsonDbString::kUuidStr, QLatin1String("{9f0f3b84-d101-45b5-9291-a0d0498add72}"));
        item.insert(JsonDbString::kTypeStr, QLatin1String("updateListWithIndex"));
        item.insert(QLatin1String("foobar"), QLatin1String("Malin"));
        list.append(item);
        zed.insert(JsonDbString::kTypeStr, QLatin1String("updateListWithIndex"));
        zed.insert(JsonDbString::kUuidStr, QLatin1String("{c3c416f9-ab0b-4813-ab4f-8833025b320b}"));
        zed.insert(QLatin1String("foobar"), QLatin1String("Zed"));
        list.append(zed);

        JsonDbWriteResult result = mJsonDbPartition->updateObjects(mOwner, list);
        verifyGoodResult(result);
        QCOMPARE(result.objectsWritten.count(), 3);

        JsonDbQueryResult findResult = find(mOwner, QLatin1String("[?_type=\"updateListWithIndex\"][/updateListWithIndex]"));
        QCOMPARE(findResult.data.size(), 2);
        QCOMPARE(findResult.data.at(0).value(QLatin1String("foobar")).toString(), QLatin1String("Malin"));
        QCOMPARE(findResult.data.at(1).value(QLatin1String("foobar")).toString(), QLatin1String("Zed"));
        zed = findResult.data.at(1);
    }

    mJsonDbPartition->closeIndexes();

    // create one more object and update an existing one
    {
        QList<JsonDbObject> list;
        zed.insert(QLatin1String("foobar"), QLatin1String("Bob"));
        list.append(zed);
        JsonDbObject item;
        item.insert(JsonDbString::kUuidStr, QLatin1String("{a7056c76-5921-4eb3-8487-ef697ea75899}"));
        item.insert(JsonDbString::kTypeStr, QLatin1String("updateListWithIndex"));
        item.insert(QLatin1String("foobar"), QLatin1String("Darla"));
        list.append(item);

        JsonDbWriteResult result = mJsonDbPartition->updateObjects(mOwner, list);
        verifyGoodResult(result);
        QCOMPARE(result.objectsWritten.count(), 2);

        JsonDbQueryResult findResult = find(mOwner, QLatin1String("[?_type=\"updateListWithIndex\"][/updateListWithIndex]"));
        QCOMPARE(findResult.data.size(), 3);
        QCOMPARE(findResult.data.at(0).value(QLatin1String("foobar")).toString(), QLatin1String("Bob"));
        QCOMPARE(findResult.data.at(1).value(QLatin1String("foobar")).toString(), QLatin1String("Darla"));
        QCOMPARE(findResult.data.at(2).value(QLatin1String("foobar")).toString(), QLatin1String("Malin"));
    }
}

void TestPartition::addBigIndex()
{
    addIndex(QLatin1String("subject"));

    JsonDbObject indexObject;
    indexObject.insert(JsonDbString::kTypeStr, QStringLiteral("Index"));
    indexObject.insert("objectType", QStringLiteral("foobar"));
    indexObject.insert("propertyName", QStringLiteral("foo"));
    indexObject.insert("propertyType", QStringLiteral("string"));

    JsonDbWriteResult result = create(mOwner, indexObject);
    verifyGoodResult(result);
    QVERIFY(mJsonDbPartition->findObjectTable(JsonDbString::kSchemaTypeStr)->index("foo") != 0);

    JsonDbObject obj;
    obj.insert(JsonDbString::kTypeStr, QStringLiteral("foobar"));
    obj.insert(QStringLiteral("foo"), QString(QByteArray(2000, 'a')));
    obj.insert(QStringLiteral("bar"), QString(QByteArray(2000, 'b')));

    result = create(mOwner, obj);
    verifyGoodResult(result);

    QString query = QString("[?foo=\"%1\"][/foo]").arg(QString(QByteArray(2000, 'a')));
    JsonDbQueryResult queryResult = find(mOwner, query);
    verifyGoodQueryResult(queryResult);

    QCOMPARE(queryResult.data.size(), 1);

    remove(mOwner, indexObject);
}

void TestPartition::ensureBadPartitionFunctionCalls_data()
{
    QTest::addColumn<bool>("callOpen");
    QTest::newRow("Failed open call") << true;
    QTest::newRow("No open call") << false;
}

void TestPartition::ensureBadPartitionFunctionCalls()
{
    QFETCH(bool, callOpen);
    JsonDbPartition *partition = new JsonDbPartition();
    partition->setDefaultOwner(mOwner);

    if (callOpen) {
        JsonDbPartitionSpec spec;
        spec.name = QString::fromAscii(QByteArray(10000, 'a')); // Make the open fail
        partition->setPartitionSpec(spec);
        QVERIFY(!partition->open());
    }

    QVERIFY(!partition->isOpen());
    QCOMPARE(partition->changesSince(0, QSet<QString>()).code, JsonDbError::PartitionUnavailable);
    if (!callOpen)
        QTest::ignoreMessage(QtWarningMsg, "QFileInfo::absolutePath: Constructed with empty filename");
    QVERIFY(partition->clear());
    QVERIFY(!partition->compact());
    QCOMPARE(partition->queryObjects(mOwner, JsonDbQuery()).code, JsonDbError::PartitionUnavailable);
    QCOMPARE(partition->updateObjects(mOwner, JsonDbObjectList()).code, JsonDbError::PartitionUnavailable);
    QCOMPARE(partition->updateObject(mOwner, JsonDbObject()).code, JsonDbError::PartitionUnavailable);
    QCOMPARE(partition->changesSince(0).code, JsonDbError::PartitionUnavailable);
    bool ok = false;
    QCOMPARE(partition->flush(&ok), -1);
    QCOMPARE(ok, false);
    QCOMPARE(partition->mainObjectTable() != NULL, callOpen);
    QVERIFY(partition->fileSizes().isEmpty());

    delete partition;
}

QTEST_MAIN(TestPartition)
#include "testpartition.moc"
