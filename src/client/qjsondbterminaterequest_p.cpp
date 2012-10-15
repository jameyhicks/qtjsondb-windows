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

#include "qjsondbterminaterequest_p_p.h"
#include "qjsondbstrings_p.h"
#include "qjsondbobject.h"

#include <QJsonArray>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONDB

/*!
    \class QJsonDbTerminateRequest
    \inmodule QtJsonDb
    \internal

    \brief The QJsonDbTerminateRequest class allows to ensure the data in the partition is terminateed to disk.
*/
/*!
    \enum QJsonDbTerminateRequest::ErrorCode

    This enum describes database connection errors for write requests that can
    be emitted by the error() signal.

    \value NoError

    \sa error(), QJsonDbRequest::ErrorCode
*/

QJsonDbTerminateRequestPrivate::QJsonDbTerminateRequestPrivate(QJsonDbTerminateRequest *q)
    : QJsonDbRequestPrivate(q), stateNumber(0)
{
}

/*!
    Constructs a new request object with the given \a parent.
*/
QJsonDbTerminateRequest::QJsonDbTerminateRequest(QObject *parent)
    : QJsonDbRequest(new QJsonDbTerminateRequestPrivate(this), parent)
{
}

/*!
    Destroys the request object.
*/
QJsonDbTerminateRequest::~QJsonDbTerminateRequest()
{
}

/*!
    \property QJsonDbTerminateRequest::stateNumber

    Returns a database state number that the request was executed on.

    The property is populated after started() signal was emitted.

    \sa started()
*/
quint32 QJsonDbTerminateRequest::stateNumber() const
{
    Q_D(const QJsonDbTerminateRequest);
    return d->stateNumber;
}

QJsonObject QJsonDbTerminateRequestPrivate::getRequest() const
{
    QJsonObject request;
    request.insert(JsonDbStrings::Protocol::action(), JsonDbStrings::Protocol::terminate());
    request.insert(JsonDbStrings::Protocol::partition(), partition);
    request.insert(JsonDbStrings::Protocol::requestId(), requestId);
    return request;
}

void QJsonDbTerminateRequestPrivate::handleResponse(const QJsonObject &response)
{
    Q_Q(QJsonDbTerminateRequest);
    stateNumber = static_cast<quint32>(response.value(JsonDbStrings::Protocol::stateNumber()).toDouble());
    qDebug() << __FILE__ << __FUNCTION__ << stateNumber;

    setStatus(QJsonDbRequest::Receiving);
    emit q->started();
    setStatus(QJsonDbRequest::Finished);
    emit q->finished();
}

void QJsonDbTerminateRequestPrivate::handleError(int code, const QString &message)
{
    Q_Q(QJsonDbTerminateRequest);
    qDebug() << __FILE__ << __FUNCTION__ << code << message;
    setStatus(QJsonDbRequest::Error);
    emit q->error(QJsonDbRequest::ErrorCode(code), message);
}

#include "moc_qjsondbterminaterequest_p.cpp"

QT_END_NAMESPACE_JSONDB
