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

#include <QDebug>
#include <QFile>
#include <errno.h>

#include "jsondbbtree.h"
#include "jsondbsettings.h"

QT_BEGIN_NAMESPACE_JSONDB_PARTITION

JsonDbBtree::JsonDbBtree()
    : mBtree(new Btree())
{
}

JsonDbBtree::~JsonDbBtree()
{
    close();
    delete mBtree;
}

void JsonDbBtree::setFileName(const QString &filename)
{
    mBtree->setFileName(filename);
}

QString JsonDbBtree::fileName() const
{
    return mBtree->fileName();
}

bool JsonDbBtree::open(OpenFlags flags)
{
    HBtree::OpenMode mode = HBtree::ReadWrite;
    if (flags & ReadOnly)
        mode = HBtree::ReadOnly;
    mBtree->setOpenMode(mode);
    return mBtree->open();
}

void JsonDbBtree::close()
{
    Q_ASSERT(mBtree);
    if (mBtree->isOpen())
        mBtree->close();
}

bool JsonDbBtree::isOpen() const
{
    return mBtree->isOpen();
}

bool JsonDbBtree::putOne(const QByteArray &key, const QByteArray &value)
{
    bool inTransaction = mBtree->isWriting();
    Transaction *txn = inTransaction ? mBtree->writeTransaction() : mBtree->beginWrite();
    bool ok = txn->put(key, value);
    if (!inTransaction) {
        qWarning() << "JsonDbBtree::putOne" << "auto commiting tag 0";
        ok &= txn->commit(0);
    }
    return ok;
}

bool JsonDbBtree::getOne(const QByteArray &key, QByteArray *value)
{
    bool inTransaction = mBtree->isWriting();
    Transaction *txn = inTransaction ? mBtree->writeTransaction() : mBtree->beginWrite();
    bool ok = txn->get(key, value);
    if (!inTransaction)
        txn->abort();
    return ok;
}

bool JsonDbBtree::removeOne(const QByteArray &key)
{
    bool inTransaction = mBtree->isWriting();
    Transaction *txn = inTransaction ? mBtree->writeTransaction() : mBtree->beginWrite();
    bool ok = txn->remove(key);
    if (!inTransaction){
        qWarning() << "JsonDbBtree::removeOne" << "auto commiting tag 0";
        ok &= txn->commit(0);
    }
    return ok;
}

bool JsonDbBtree::clearData()
{
    Q_ASSERT(isWriting() == false);
    return mBtree->clearData();
}

bool JsonDbBtree::compact()
{
    Q_ASSERT(mBtree);
    return true;
}

bool JsonDbBtree::rollback()
{
    Q_ASSERT(mBtree && !isWriting());
    return mBtree->rollback();
}

void JsonDbBtree::setAutoCompactRate(int rate) const
{
    Q_ASSERT(mBtree);
    Q_UNUSED(rate);
}

JsonDbBtree::Stat JsonDbBtree::stats() const
{
    if (mBtree)
        return mBtree->stats();
    else
      return Stat();
}

QT_END_NAMESPACE_JSONDB_PARTITION
