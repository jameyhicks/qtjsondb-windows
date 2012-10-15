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

#include "qjsondbreloadrequest_p_p.h"
#include "qjsondbstrings_p.h"
#include "qjsondbobject.h"

#include <QJsonArray>
#include <QDebug>

QT_BEGIN_NAMESPACE_JSONDB

/*!
    \class QJsonDbReloadRequest
    \inmodule QtJsonDb
    \internal

    \brief The QJsonDbReloadRequest requests the daemon to reload the configuration file
*/
/*!
    \enum QJsonDbReloadRequest::ErrorCode

    This enum describes database connection errors for write requests that can
    be emitted by the error() signal.

    \value NoError

    \sa error(), QJsonDbRequest::ErrorCode
*/

QJsonDbReloadRequestPrivate::QJsonDbReloadRequestPrivate(QJsonDbReloadRequest *q)
    : QJsonDbRequestPrivate(q)
{
}

/*!
    Constructs a new request object with the given \a parent.
*/
QJsonDbReloadRequest::QJsonDbReloadRequest(QObject *parent)
    : QJsonDbRequest(new QJsonDbReloadRequestPrivate(this), parent)
{
}

/*!
    Destroys the request object.
*/
QJsonDbReloadRequest::~QJsonDbReloadRequest()
{
}

QJsonObject QJsonDbReloadRequestPrivate::getRequest() const
{
    QJsonObject request;
    request.insert(JsonDbStrings::Protocol::action(), JsonDbStrings::Protocol::reload());
    request.insert(JsonDbStrings::Protocol::partition(), partition);
    request.insert(JsonDbStrings::Protocol::requestId(), requestId);
    return request;
}

void QJsonDbReloadRequestPrivate::handleResponse(const QJsonObject &response)
{
    Q_Q(QJsonDbReloadRequest);

    setStatus(QJsonDbRequest::Receiving);
    emit q->started();
    setStatus(QJsonDbRequest::Finished);
    emit q->finished();
}

void QJsonDbReloadRequestPrivate::handleError(int code, const QString &message)
{
    Q_Q(QJsonDbReloadRequest);
    setStatus(QJsonDbRequest::Error);
    emit q->error(QJsonDbRequest::ErrorCode(code), message);
}

#include "moc_qjsondbreloadrequest_p.cpp"

QT_END_NAMESPACE_JSONDB
