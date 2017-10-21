
#include <TelepathyQt/RoomManager>
//#include "TelepathyQt/room-manager-internal.h"

#include "TelepathyQt/_gen/room-manager.moc.hpp"

#include "TelepathyQt/debug-internal.h"
#include "TelepathyQt/future-internal.h"

#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ConnectionLowlevel>
#include <TelepathyQt/Channel>
//#include <TelepathyQt/RoomFactory>
#include <TelepathyQt/PendingChannel>
//#include <TelepathyQt/PendingContactAttributes>
#include <TelepathyQt/PendingRooms>
#include <TelepathyQt/PendingFailure>
#include <TelepathyQt/PendingHandles>
#include <TelepathyQt/PendingVariantMap>
#include <TelepathyQt/ReferencedHandles>
#include <TelepathyQt/Utils>

#include <QMap>

namespace Tp
{

struct TP_QT_NO_EXPORT RoomManager::Private
{
    Private(RoomManager *parent, Connection *connection);
    ~Private();

    // avatar specific methods
//    bool buildAvatarFileName(QString token, bool createDir,
//        QString &avatarFileName, QString &mimeTypeFileName);
//    Features realFeatures(const Features &features);
    QSet<QString> interfacesForFeatures(const Features &features);

    static QString featureToInterface(const Feature &feature);
    void ensureTracking(const Feature &feature);

    void introspectRoomChannel(const ChannelPtr &channel, const RoomPtr &room);
    void introspectRoomConfig();

    RoomManager *parent;
    WeakPtr<Connection> connection;
    WeakPtr<Channel> existsRoomChannel;

    QHash<uint, WeakPtr<Room> > rooms;

    QHash<Feature, bool> tracking;
    Features supportedFeatures;

    // Introspection
    QQueue<void (Private::*)()> introspectQueue;

    // avatar
    QSet<RoomPtr> requestAvatarsQueue;
    bool requestAvatarsIdle;
};

RoomManager::Private::Private(RoomManager *parent, Connection *connection)
    : parent(parent),
      connection(connection),
      requestAvatarsIdle(false)
{
}

RoomManager::Private::~Private()
{
}

QSet<QString> RoomManager::Private::interfacesForFeatures(const Features &features)
{
    Features supported = parent->supportedFeatures();
    QSet<QString> ret;
    foreach (const Feature &feature, features) {
        ensureTracking(feature);

        if (supported.contains(feature)) {
            // Only query interfaces which are reported as supported to not get an error
            ret.insert(featureToInterface(feature));
        }
    }
    return ret;
}

QString RoomManager::Private::featureToInterface(const Feature &feature)
{
    if (feature == Room::FeatureRoomConfig) {
        return TP_QT_IFACE_CHANNEL_INTERFACE_ROOM_CONFIG;
    } else {
        warning() << "RoomManager doesn't know which interface corresponds to feature"
            << feature;
        return QString();
    }
}

void RoomManager::Private::ensureTracking(const Feature &feature)
{
    if (tracking[feature]) {
        return;
    }

    ConnectionPtr conn(parent->connection());

//    if (feature == Contact::FeatureAlias) {
//        Client::ConnectionInterfaceAliasingInterface *aliasingInterface =
//            conn->interface<Client::ConnectionInterfaceAliasingInterface>();

//        connect(aliasingInterface,
//                SIGNAL(AliasesChanged(Tp::AliasPairList)),
//                SLOT(onAliasesChanged(Tp::AliasPairList)));
//    } else if (feature == Contact::FeatureAvatarData) {
//        Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
//            conn->interface<Client::ConnectionInterfaceAvatarsInterface>();

//        connect(avatarsInterface,
//                SIGNAL(AvatarRetrieved(uint,QString,QByteArray,QString)),
//                SLOT(onAvatarRetrieved(uint,QString,QByteArray,QString)));
//    } else if (feature == Contact::FeatureAvatarToken) {
//        Client::ConnectionInterfaceAvatarsInterface *avatarsInterface =
//            conn->interface<Client::ConnectionInterfaceAvatarsInterface>();

//        connect(avatarsInterface,
//                SIGNAL(AvatarUpdated(uint,QString)),
//                SLOT(onAvatarUpdated(uint,QString)));
//    } else if (feature == Contact::FeatureCapabilities) {
//        Client::ConnectionInterfaceContactCapabilitiesInterface *contactCapabilitiesInterface =
//            conn->interface<Client::ConnectionInterfaceContactCapabilitiesInterface>();

//        connect(contactCapabilitiesInterface,
//                SIGNAL(ContactCapabilitiesChanged(Tp::ContactCapabilitiesMap)),
//                SLOT(onCapabilitiesChanged(Tp::ContactCapabilitiesMap)));
//    } else if (feature == Contact::FeatureInfo) {
//        Client::ConnectionInterfaceContactInfoInterface *contactInfoInterface =
//            conn->interface<Client::ConnectionInterfaceContactInfoInterface>();

//        connect(contactInfoInterface,
//                SIGNAL(ContactInfoChanged(uint,Tp::ContactInfoFieldList)),
//                SLOT(onContactInfoChanged(uint,Tp::ContactInfoFieldList)));
//    } else if (feature == Contact::FeatureLocation) {
//        Client::ConnectionInterfaceLocationInterface *locationInterface =
//            conn->interface<Client::ConnectionInterfaceLocationInterface>();

//        connect(locationInterface,
//                SIGNAL(LocationUpdated(uint,QVariantMap)),
//                SLOT(onLocationUpdated(uint,QVariantMap)));
//    } else if (feature == Contact::FeatureSimplePresence) {
//        Client::ConnectionInterfaceSimplePresenceInterface *simplePresenceInterface =
//            conn->interface<Client::ConnectionInterfaceSimplePresenceInterface>();

//        connect(simplePresenceInterface,
//                SIGNAL(PresencesChanged(Tp::SimpleContactPresences)),
//                SLOT(onPresencesChanged(Tp::SimpleContactPresences)));
//    } else if (feature == Contact::FeatureClientTypes) {
//        Client::ConnectionInterfaceClientTypesInterface *clientTypesInterface =
//            conn->interface<Client::ConnectionInterfaceClientTypesInterface>();

//        connect(clientTypesInterface,
//                SIGNAL(ClientTypesUpdated(uint,QStringList)),
//                SLOT(onClientTypesUpdated(uint,QStringList)));
//    } else if (feature == Contact::FeatureRosterGroups || feature == Contact::FeatureAddresses) {
//        // nothing to do here, but we don't want to warn
//        ;
//    } else {
//        warning() << " Unknown feature" << feature
//            << "when trying to figure out how to connect change notification!";
//    }

    tracking[feature] = true;
}

void RoomManager::Private::introspectRoomChannel(const ChannelPtr &channel, const RoomPtr &room)
{
    existsRoomChannel = channel;
    const QStringList interfaces = channel->interfaces();

    if (interfaces.contains(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM_CONFIG)) {
        introspectQueue.enqueue(&Private::introspectRoomConfig);
    }

//    if (interfaces.contains(TP_QT_IFACE_CHANNEL_INTERFACE_GROUP)) {
//        introspectQueue.enqueue(&Private::introspectGroup);
//    }

//    if (interfaces.contains(TP_QT_IFACE_CHANNEL_INTERFACE_CONFERENCE)) {
//        introspectQueue.enqueue(&Private::introspectConference);
//    }
}

void RoomManager::Private::introspectRoomConfig()
{
//    Q_ASSERT(properties != 0);
//    Q_ASSERT(roomConfig == 0);

    debug() << "Introspecting RoomConfig interface";
//    roomConfig = existsRoomChannel->interface<Client::ChannelInterfaceRoomConfigInterface>();
//    Q_ASSERT(roomConfig != 0);

//    introspectingRoomConfig = true;

    debug() << "Calling Properties::GetAll(Channel.Interface.RoomConfig)";
//    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
//            properties->GetAll(TP_QT_IFACE_CHANNEL_INTERFACE_ROOM_CONFIG),
//            parent);
//    parent->connect(watcher,
//            SIGNAL(finished(QDBusPendingCallWatcher*)),
//                    SLOT(gotRoomConfigProperties(QDBusPendingCallWatcher*)));
}

//Features RoomManager::Private::realFeatures(const Features &features)
//{
//    return features;
//}

/**
 * \class RoomManager
 * \ingroup clientconn
 * \headerfile TelepathyQt/room-manager.h <TelepathyQt/RoomManager>
 *
 * \brief The RoomManager class is responsible for managing rooms.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Construct a new RoomManager object.
 *
 * \param connection The connection owning this RoomManager.
 */
RoomManager::RoomManager(Connection *connection)
    : Object(),
      mPriv(new Private(this, connection))
{
}

/**
 * Class destructor.
 */
RoomManager::~RoomManager()
{
}

/**
 * Return the connection owning this RoomManager.
 *
 * \return A pointer to the Connection object.
 */
ConnectionPtr RoomManager::connection() const
{
    return ConnectionPtr(mPriv->connection);
}

/**
 * Return the features that are expected to work on contacts on this RoomManager connection.
 *
 * This method requires Connection::FeatureCore to be ready.
 *
 * \return The supported features as a set of Feature objects.
 */
Features RoomManager::supportedFeatures() const
{
//    if (mPriv->supportedFeatures.isEmpty() &&
//        connection()->interfaces().contains(TP_QT_IFACE_CONNECTION_INTERFACE_ROOMS)) {
//        Features allFeatures = Features()
//            << Contact::FeatureAlias
//            << Contact::FeatureAvatarToken
//            << Contact::FeatureAvatarData
//            << Contact::FeatureSimplePresence
//            << Contact::FeatureCapabilities
//            << Contact::FeatureLocation
//            << Contact::FeatureInfo
//            << Contact::FeatureRosterGroups
//            << Contact::FeatureAddresses
//            << Contact::FeatureClientTypes;
//        QStringList interfaces = connection()->lowlevel()->contactAttributeInterfaces();
//        foreach (const Feature &feature, allFeatures) {
//            if (interfaces.contains(featureToInterface(feature))) {
//                mPriv->supportedFeatures.insert(feature);
//            }
//        }

//        debug() << mPriv->supportedFeatures.size() << "contact features supported using" << this;
//    }

    return mPriv->supportedFeatures;
}

PendingRooms *RoomManager::roomsForHandles(const UIntList &handles, const Features &features)
{
    QMap<uint, RoomPtr> satisfyingRooms;
    QSet<uint> otherRooms;
//    Features features = Features() << Room::FeatureRoomConfig;

    if (!connection()->isValid()) {
        return new PendingRooms(RoomManagerPtr(this), handles, features, Features(),
                QStringList(), satisfyingRooms, otherRooms,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection is invalid"));
    } else if (!connection()->isReady(Connection::FeatureCore)) {
        return new PendingRooms(RoomManagerPtr(this), handles, features, Features(),
                QStringList(), satisfyingRooms, otherRooms,
                TP_QT_ERROR_NOT_AVAILABLE,
                QLatin1String("Connection::FeatureCore is not ready"));
    }

    Features missingFeatures;

//    foreach (uint handle, handles) {
//        RoomPtr room = lookupRoomByHandle(handle);
//        if (room) {
//            if ((realFeatures - room->requestedFeatures()).isEmpty()) {
//                // Contact exists and has all the requested features
//                satisfyingRooms.insert(handle, room);
//            } else {
//                // Contact exists but is missing features
//                otherRooms.insert(handle);
//                missingFeatures.unite(realFeatures - room->requestedFeatures());
//            }
//        } else {
//            // Contact doesn't exist - we need to get all of the features (same as unite(features))
//            missingFeatures = realFeatures;
//            otherRooms.insert(handle);
//        }
//    }

    QSet<QString> interfaces = mPriv->interfacesForFeatures(missingFeatures);

    PendingRooms *rooms =
            new PendingRooms(RoomManagerPtr(this), handles, features, missingFeatures,
                             interfaces.toList(), satisfyingRooms, otherRooms);
    return rooms;
}

PendingRooms *RoomManager::roomsForIdentifiers(const QStringList &identifiers, const Features &features)
{
    Q_UNUSED(features)
    qWarning() << Q_FUNC_INFO << "Not implemented, request for" << identifiers;

    return new PendingRooms(RoomManagerPtr(this),
                            TP_QT_ERROR_NOT_IMPLEMENTED, QLatin1String("Not implemented"));
}

RoomPtr RoomManager::lookupRoomByHandle(uint handle)
{
    RoomPtr room;

    if (mPriv->rooms.contains(handle)) {
        room = RoomPtr(mPriv->rooms.value(handle));
        if (!room) {
            // Dangling weak pointer, remove it
            mPriv->rooms.remove(handle);
        }
    }

    return room;
}

} // Tp
