/**
 * This file is part of TelepathyQt
 *
 * @copyright Copyright (C) 2017 Alexandr Akulich <akulichalexander@gmail.com>
 * @license LGPL 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt/PendingRooms>
//#include "TelepathyQt/pending-rooms-internal.h"

#include "TelepathyQt/_gen/pending-rooms.moc.hpp"
//#include "TelepathyQt/_gen/pending-rooms-internal.moc.hpp"

#include "TelepathyQt/debug-internal.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/RoomManager>
#include <TelepathyQt/ContactFactory>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/ReferencedHandles>

// FIXME: Refactor PendingContacts code to make it more readable/maintainable and reuse common code
//        when appropriate.

namespace Tp
{

PendingPeers::PendingPeers(const SharedPtr<RefCounted> &object) :
    PendingOperation(object)
{
}

struct TP_QT_NO_EXPORT PendingRooms::Private
{
    Private(PendingRooms *parent, const RoomManagerPtr &manager)
        : parent(parent),
          manager(manager)
    {
    }

    void setFinished();

    bool checkRequestTypeAndState(const char *methodName, const char *debug, RequestType type);

    // Public object
    PendingRooms *parent;

    // Generic parameters
    RoomManagerPtr manager;
    Features features;
    Features missingFeatures;

    // Results
    QList<RoomPtr> rooms;
};

PendingRooms::~PendingRooms()
{
    delete mPriv;
}

RoomManagerPtr PendingRooms::manager() const
{
    return mPriv->manager;
}

Features PendingRooms::features() const
{
    return mPriv->features;
}

QList<RoomPtr> PendingRooms::rooms() const
{
    if (!isFinished()) {
        warning() << "PendingRooms::rooms() called before finished";
    } else if (isError()) {
        warning() << "PendingRooms::rooms() called when errored";
    }

    return mPriv->rooms;
}

PendingRooms::PendingRooms(const RoomManagerPtr &manager, const QString &errorName, const QString &errorMessage)
    : PendingPeers(manager->connection()),
      mPriv(new Private(this, manager))
{
    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }
}

PendingRooms::PendingRooms(const RoomManagerPtr &manager, const UIntList &handles,
                           const Features &features, const Features &missingFeatures,
                           const QStringList &interfaces, const QMap<uint, RoomPtr> &satisfyingRooms,
                           const QSet<uint> &otherRooms,
                           const QString &errorName, const QString &errorMessage)
    : PendingPeers(manager->connection()),
      mPriv(new Private(this, manager))
{
    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }
}

PendingRooms::PendingRooms(const RoomManagerPtr &manager, const QStringList &identifiers,
                           PendingRooms::RequestType requestType, const Features &features,
                           const QStringList &interfaces,
                           const QString &errorName, const QString &errorMessage)
    : PendingPeers(manager->connection()),
      mPriv(new Private(this, manager))
{
    Q_UNUSED(identifiers)
    Q_UNUSED(requestType)
    Q_UNUSED(interfaces)

    if (!errorName.isEmpty()) {
        setFinishedWithError(errorName, errorMessage);
        return;
    }
}

} // Tp
