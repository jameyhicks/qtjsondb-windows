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
#include "private/qjsondbterminaterequest_p.h"

#include "testhelper.h"

#include <QDebug>
#include <QTest>

QT_USE_NAMESPACE_JSONDB

class TestQJsonDbTerminateRequest: public TestHelper
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testTerminate();
};

void TestQJsonDbTerminateRequest::initTestCase()
{
    removeDbFiles();

    QStringList arg_list = QStringList() << "-validate-schemas";
    launchJsonDbDaemon(arg_list, __FILE__);
}

void TestQJsonDbTerminateRequest::cleanupTestCase()
{
    removeDbFiles();
    stopDaemon();
}

void TestQJsonDbTerminateRequest::init()
{
    clearHelperData();
    connectToServer();
}

void TestQJsonDbTerminateRequest::cleanup()
{
    disconnectFromServer();
}

void TestQJsonDbTerminateRequest::testTerminate()
{
    QJsonDbReadRequest read;
    read.setQuery(QStringLiteral("[?_type=%type]"));
    read.bindValue(QStringLiteral("type"), QStringLiteral("foo"));
    mConnection->send(&read);
    QVERIFY(waitForResponse(&read));

    uint state = read.stateNumber();

    QJsonDbObject obj;
    obj.setUuid(QJsonDbObject::createUuid());
    obj.insert(QStringLiteral("_type"), QStringLiteral("foo"));
    QJsonDbWriteRequest write;
    write.setObjects(QList<QJsonObject>() << obj);
    mConnection->send(&write);
    QVERIFY(waitForResponse(&write));

    QJsonDbTerminateRequest terminate;
    mConnection->send(&terminate);
    QVERIFY(waitForResponse(&terminate));
}

QTEST_MAIN(TestQJsonDbTerminateRequest)

#include "testqjsondbterminaterequest.moc"
