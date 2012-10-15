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

#ifndef JSONDB_STRINGS_H
#define JSONDB_STRINGS_H

#include <QString>
#include "jsondbpartitionglobal.h"

QT_BEGIN_NAMESPACE_JSONDB_PARTITION

class Q_JSONDB_PARTITION_EXPORT JsonDbString {
public:
    static const QString kActionStr;
    static const QString kActionsStr;
    static const QString kActiveStr;
    static const QString kAddIndexStr;
    static const QString kCodeStr;
    static const QString kConflictsStr;
    static const QString kConnectStr;
    static const QString kCountStr;
    static const QString kCreateStr;
    static const QString kDropStr;
    static const QString kCurrentStr;
    static const QString kDataStr;
    static const QString kDeletedStr;
    static const QString kDisconnectStr;
    static const QString kDomainStr;
    static const QString kErrorStr;
    static const QString kExplanationStr;
    static const QString kFieldNameStr;
    static const QString kFindStr;
    static const QString kNameStr;
    static const QString kIdStr;
    static const QString kIndexValueStr;
    static const QString kLengthStr;
    static const QString kLimitStr;
    static const QString kMapTypeStr;
    static const QString kMessageStr;
    static const QString kMetaStr;
    static const QString kNotifyStr;
    static const QString kNotificationTypeStr;
    static const QString kObjectStr;
    static const QString kParentStr;
    static const QString kOffsetStr;
    static const QString kOwnerStr;
    static const QString kQueryStr;
    static const QString kReduceTypeStr;
    static const QString kRemoveStr;
    static const QString kResultStr;
    static const QString kSchemaStr;
    static const QString kSchemaTypeStr;
    static const QString kTypeStr;
    static const QString kTypesStr;
    static const QString kUpdateStr;
    static const QString kUuidStr;
    static const QString kVersionStr;
    static const QString kViewTypeStr;
    static const QString kTokenStr;
    static const QString kFlushStr;
    static const QString kReloadStr;
    static const QString kSettingsStr;
    static const QString kChangesSinceStr;
    static const QString kStateNumberStr;
    static const QString kCollapsedStr;
    static const QString kCurrentStateNumberStr;
    static const QString kStartingStateNumberStr;
    static const QString kTombstoneStr;
    static const QString kPartitionTypeStr;
    static const QString kPartitionStr;
    static const QString kLogStr;
    static const QString kPropertyNameStr;
    static const QString kPropertyTypeStr;
    static const QString kPropertyFunctionStr;
    static const QString kObjectTypeStr;
    static const QString kDbidTypeStr;
    static const QString kIndexTypeStr;
    static const QString kLocaleStr;
    static const QString kLocalStr;
    static const QString kCollationStr;
    static const QString kCaseSensitiveStr;
    static const QString kCasePreferenceStr;
    static const QString kDatabaseSchemaVersionStr;
    static const QString kPathStr;
    static const QString kDefaultStr;
    static const QString kConflictResolutionModeStr;
    static const QString kJoinStr;
    static const QString kSourceUuidsDotStarStr;
    static const QString kCapabilityTypeStr;
    static const QString kRemovableStr;
    static const QString kAvailableStr;
};

QT_END_NAMESPACE_JSONDB_PARTITION

#endif /* JSONDB-STRINGS_H */
