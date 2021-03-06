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

#include "jsondbproxy.h"
#include "qjsondbobject.h"
#include "qjsondbreadrequest.h"
#include "qjsondbwriterequest.h"

#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

QT_USE_NAMESPACE

QVariantMap _waitForResponse(QtJsonDb::QJsonDbRequest *request) {
    QEventLoop e;
    while (!(request->status() == QtJsonDb::QJsonDbRequest::Error || request->status() == QtJsonDb::QJsonDbRequest::Finished))
        e.processEvents();

    QVariantMap res;

    if (request->status() != QtJsonDb::QJsonDbRequest::Error) {
        QList<QJsonObject> results = request->takeResults();
        QVariantList data;
        foreach (const QJsonObject &obj, results)
            data.append(obj.toVariantMap());
        res.insert(QLatin1Literal("result"), data);
    } else {
        QVariantMap error;
        error.insert(QLatin1String("code"), 1);
        error.insert(QLatin1String("message"), "Error processing request");
        res.insert(QLatin1String("error"), error);
    }
    return res;
}


JsonDbProxy::JsonDbProxy(QtJsonDb::QJsonDbConnection *conn, const QString &partition, QObject *parent) :
    QObject(parent)
  , mConnection(conn)
  , mPartition(partition)
{
}

QVariantMap JsonDbProxy::find(QVariantMap object)
{
    QtJsonDb::QJsonDbReadRequest *request = new QtJsonDb::QJsonDbReadRequest(this);
    request->setPartition(mPartition);
    request->setQuery(object.value(QLatin1String("query")).toString());
    if (object.contains(QLatin1String("limit")))
        request->setQueryLimit(object.value(QLatin1String("limit")).toInt());
    mConnection->send(request);
    return _waitForResponse(request);
}

QVariantMap JsonDbProxy::create(QVariantMap object)
{
    // handle the to-be-deprecated _id property
    QtJsonDb::QJsonDbObject obj = QJsonObject::fromVariantMap(object);
    if (obj.uuid().isNull() && obj.contains(QLatin1String("_id"))) {
        obj.setUuid(QtJsonDb::QJsonDbObject::createUuidFromString(obj.value(QLatin1String("_id")).toString()));
        obj.remove(QLatin1String("_id"));
    }
    QtJsonDb::QJsonDbCreateRequest *request = new QtJsonDb::QJsonDbCreateRequest(QList<QJsonObject>() << obj,
                                                                                 this);
    request->setPartition(mPartition);

    mConnection->send(request);
    return _waitForResponse(request);
}

QVariantMap JsonDbProxy::update(QVariantMap object)
{
    QtJsonDb::QJsonDbUpdateRequest *request = new QtJsonDb::QJsonDbUpdateRequest(QList<QJsonObject>() << QJsonObject::fromVariantMap(object),
                                                                                 this);
    request->setPartition(mPartition);

    mConnection->send(request);
    return _waitForResponse(request);
}

QVariantMap JsonDbProxy::remove(QVariantMap object )
{
    QtJsonDb::QJsonDbRemoveRequest *request = new QtJsonDb::QJsonDbRemoveRequest(QList<QJsonObject>() << QJsonObject::fromVariantMap(object),
                                                                                this);
    request->setPartition(mPartition);

    mConnection->send(request);
    return _waitForResponse(request);
}

QVariantMap JsonDbProxy::createList(QVariantList list)
{
    QList<QJsonObject> objects;
    foreach (const QVariant &object, list) {
        QtJsonDb::QJsonDbObject obj = QJsonObject::fromVariantMap(object.toMap());
        if (!obj.uuid().isNull() && obj.contains(QLatin1String("_id"))) {
            obj.setUuid(QtJsonDb::QJsonDbObject::createUuidFromString(obj.value(QLatin1String("_id")).toString()));
            obj.remove(QLatin1String("_id"));
        }
        objects << obj;
    }
    QtJsonDb::QJsonDbCreateRequest *request = new QtJsonDb::QJsonDbCreateRequest(objects, this);
    request->setPartition(mPartition);

    mConnection->send(request);
    return _waitForResponse(request);
}

QVariantMap JsonDbProxy::updateList(QVariantList list)
{
    QList<QJsonObject> objects;
    foreach (const QVariant &obj, list)
        objects << QJsonObject::fromVariantMap(obj.toMap());
    QtJsonDb::QJsonDbUpdateRequest *request = new QtJsonDb::QJsonDbUpdateRequest(objects, this);
    request->setPartition(mPartition);

    mConnection->send(request);
    return _waitForResponse(request);
}

QVariantMap JsonDbProxy::removeList(QVariantList list)
{
    QList<QJsonObject> objects;
    foreach (const QVariant &obj, list)
        objects << QJsonObject::fromVariantMap(obj.toMap());
    QtJsonDb::QJsonDbRemoveRequest *request = new QtJsonDb::QJsonDbRemoveRequest(objects, this);
    request->setPartition(mPartition);

    mConnection->send(request);
    return _waitForResponse(request);
}

void JsonDbProxy::log(const QString &msg)
{
    qDebug() << msg;
}

void JsonDbProxy::debug(const QString &msg)
{
    qDebug() << msg;
}

