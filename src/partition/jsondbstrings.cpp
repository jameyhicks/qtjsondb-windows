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

#include "jsondbstrings.h"

QT_BEGIN_NAMESPACE_JSONDB_PARTITION

const QString JsonDbString::kUuidStr    = QString::fromLatin1("_uuid");
const QString JsonDbString::kVersionStr = QString::fromLatin1("_version");
const QString JsonDbString::kIdStr      = QString::fromLatin1("id");
const QString JsonDbString::kResultStr  = QString::fromLatin1("result");
const QString JsonDbString::kErrorStr   = QString::fromLatin1("error");
const QString JsonDbString::kFieldNameStr   = QString::fromLatin1("fieldName");
const QString JsonDbString::kCodeStr    = QString::fromLatin1("code");
const QString JsonDbString::kMessageStr = QString::fromLatin1("message");
const QString JsonDbString::kNameStr    = QString::fromLatin1("name");
const QString JsonDbString::kCountStr   = QString::fromLatin1("count");
const QString JsonDbString::kCurrentStr = QString::fromLatin1("_current");
const QString JsonDbString::kDomainStr    = QString::fromLatin1("domain");
const QString JsonDbString::kIndexValueStr = QString::fromLatin1("_indexValue");
const QString JsonDbString::kOwnerStr   = QString::fromLatin1("_owner");
const QString JsonDbString::kTypeStr    = QString::fromLatin1("_type");
const QString JsonDbString::kTypesStr   = QString::fromLatin1("types");
const QString JsonDbString::kParentStr  = QString::fromLatin1("_parent");
const QString JsonDbString::kSchemaTypeStr = QString::fromLatin1("_schemaType");

const QString JsonDbString::kActionStr   = QString::fromLatin1("action");
const QString JsonDbString::kActionsStr  = QString::fromLatin1("actions");
const QString JsonDbString::kActiveStr   = QString::fromLatin1("active");
const QString JsonDbString::kAddIndexStr = QString::fromLatin1("addIndex");
const QString JsonDbString::kCreateStr   = QString::fromLatin1("create");
const QString JsonDbString::kDropStr   = QString::fromLatin1("drop");
const QString JsonDbString::kConflictsStr = QString::fromLatin1("conflicts");
const QString JsonDbString::kConnectStr  = QString::fromLatin1("connect");
const QString JsonDbString::kDataStr     = QString::fromLatin1("data");
const QString JsonDbString::kDeletedStr   = QString::fromLatin1("_deleted");
const QString JsonDbString::kDisconnectStr  = QString::fromLatin1("disconnect");
const QString JsonDbString::kExplanationStr = QString::fromLatin1("explanation");
const QString JsonDbString::kFindStr     = QString::fromLatin1("find");
const QString JsonDbString::kLengthStr   = QString::fromLatin1("length");
const QString JsonDbString::kLimitStr    = QString::fromLatin1("limit");
const QString JsonDbString::kMapTypeStr  = QString::fromLatin1("Map");
const QString JsonDbString::kMetaStr     = QString::fromLatin1("_meta");
const QString JsonDbString::kNotifyStr   = QString::fromLatin1("notify");
const QString JsonDbString::kNotificationTypeStr = QString::fromLatin1("notification");
const QString JsonDbString::kObjectStr   = QString::fromLatin1("object");
const QString JsonDbString::kOffsetStr   = QString::fromLatin1("offset");
const QString JsonDbString::kQueryStr    = QString::fromLatin1("query");
const QString JsonDbString::kReduceTypeStr   = QString::fromLatin1("Reduce");
const QString JsonDbString::kRemoveStr   = QString::fromLatin1("remove");
const QString JsonDbString::kSchemaStr   = QString::fromLatin1("schema");
const QString JsonDbString::kUpdateStr   = QString::fromLatin1("update");
const QString JsonDbString::kTokenStr    = QString::fromLatin1("token");
const QString JsonDbString::kFlushStr    = QString::fromLatin1("flush");
const QString JsonDbString::kReloadStr   = QString::fromLatin1("reload");
const QString JsonDbString::kSettingsStr    = QString::fromLatin1("settings");
const QString JsonDbString::kViewTypeStr = QString::fromLatin1("View");
const QString JsonDbString::kChangesSinceStr = QString::fromLatin1("changesSince");
const QString JsonDbString::kStateNumberStr = QString::fromLatin1("stateNumber");
const QString JsonDbString::kCollapsedStr = QString::fromLatin1("collapsed");
const QString JsonDbString::kCurrentStateNumberStr = QString::fromLatin1("currentStateNumber");
const QString JsonDbString::kStartingStateNumberStr = QString::fromLatin1("startingStateNumber");
const QString JsonDbString::kTombstoneStr = QString::fromLatin1("Tombstone");
const QString JsonDbString::kPartitionTypeStr = QString::fromLatin1("Partition");
const QString JsonDbString::kPartitionStr = QString::fromLatin1("partition");
const QString JsonDbString::kLogStr = QString::fromLatin1("log");
const QString JsonDbString::kPropertyNameStr = QString::fromLatin1("propertyName");
const QString JsonDbString::kPropertyTypeStr = QString::fromLatin1("propertyType");
const QString JsonDbString::kPropertyFunctionStr = QString::fromLatin1("propertyFunction");
const QString JsonDbString::kObjectTypeStr = QString::fromLatin1("objectType");
const QString JsonDbString::kDbidTypeStr = QString::fromLatin1("DatabaseId");
const QString JsonDbString::kIndexTypeStr = QString::fromLatin1("Index");
const QString JsonDbString::kLocalStr = QString::fromLatin1("_local");
const QString JsonDbString::kLocaleStr = QString::fromLatin1("locale");
const QString JsonDbString::kCollationStr = QString::fromLatin1("collation");
const QString JsonDbString::kCaseSensitiveStr = QString::fromLatin1("caseSensitive");
const QString JsonDbString::kCasePreferenceStr = QString::fromLatin1("casePreference");
const QString JsonDbString::kDatabaseSchemaVersionStr = QString::fromLatin1("databaseSchemaVersion");
const QString JsonDbString::kPathStr = QString::fromLatin1("path");
const QString JsonDbString::kDefaultStr = QString::fromLatin1("default");
const QString JsonDbString::kConflictResolutionModeStr = QString::fromLatin1("conflictResolutionMode");
const QString JsonDbString::kJoinStr = QString::fromLatin1("join");
const QString JsonDbString::kSourceUuidsDotStarStr = QString::fromLatin1("_sourceUuids.*");
const QString JsonDbString::kCapabilityTypeStr = QString::fromLatin1("Capability");
const QString JsonDbString::kRemovableStr = QString::fromLatin1("removable");
const QString JsonDbString::kAvailableStr = QString::fromLatin1("available");

QT_END_NAMESPACE_JSONDB_PARTITION
