/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2010-2011 Collabora Ltd. <http://www.collabora.co.uk/>
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

#include <TelepathyQt4/StreamTubeChannel>

#include "TelepathyQt4/_gen/stream-tube-channel.moc.hpp"

#include "TelepathyQt4/debug-internal.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingVariantMap>

#include <QHostAddress>

namespace Tp
{

struct TELEPATHY_QT4_NO_EXPORT StreamTubeChannel::Private
{
    Private(StreamTubeChannel *parent);

    static void introspectStreamTube(Private *self);
    static void introspectConnectionMonitoring(Private *self);

    void extractStreamTubeProperties(const QVariantMap &props);

    // Public object
    StreamTubeChannel *parent;

    ReadinessHelper *readinessHelper;

    // Introspection
    SupportedSocketMap socketTypes;
    QString serviceName;

    UIntList connections;
    QPair<QHostAddress, quint16> ipAddress;
    QString unixAddress;
    SocketAddressType addressType;
    SocketAccessControl accessControl;
};

StreamTubeChannel::Private::Private(StreamTubeChannel *parent)
    : parent(parent),
      readinessHelper(parent->readinessHelper()),
      addressType(SocketAddressTypeUnix),
      accessControl(SocketAccessControlLocalhost)
{
    ReadinessHelper::Introspectables introspectables;

    ReadinessHelper::Introspectable introspectableStreamTube(
            QSet<uint>() << 0,                                                      // makesSenseForStatuses
            Features() << TubeChannel::FeatureTube,                                 // dependsOnFeatures (core)
            QStringList(),                                                          // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc) &StreamTubeChannel::Private::introspectStreamTube,
            this);
    introspectables[StreamTubeChannel::FeatureStreamTube] = introspectableStreamTube;

    ReadinessHelper::Introspectable introspectableConnectionMonitoring(
            QSet<uint>() << 0,                                                            // makesSenseForStatuses
            Features() << StreamTubeChannel::FeatureStreamTube,                           // dependsOnFeatures (core)
            QStringList(),                                                                // dependsOnInterfaces
            (ReadinessHelper::IntrospectFunc)
                    &StreamTubeChannel::Private::introspectConnectionMonitoring,
            this);
    introspectables[StreamTubeChannel::FeatureConnectionMonitoring] =
            introspectableConnectionMonitoring;

    readinessHelper->addIntrospectables(introspectables);
}

void StreamTubeChannel::Private::introspectStreamTube(
        StreamTubeChannel::Private *self)
{
    StreamTubeChannel *parent = self->parent;

    debug() << "Introspecting stream tube properties";
    Client::ChannelTypeStreamTubeInterface *streamTubeInterface =
            parent->interface<Client::ChannelTypeStreamTubeInterface>();

    PendingVariantMap *pvm = streamTubeInterface->requestAllProperties();
    parent->connect(pvm,
            SIGNAL(finished(Tp::PendingOperation *)),
            SLOT(gotStreamTubeProperties(Tp::PendingOperation *)));
}

void StreamTubeChannel::Private::introspectConnectionMonitoring(
        StreamTubeChannel::Private *self)
{
    StreamTubeChannel *parent = self->parent;

    Client::ChannelTypeStreamTubeInterface *streamTubeInterface =
            parent->interface<Client::ChannelTypeStreamTubeInterface>();

    parent->connect(streamTubeInterface,
            SIGNAL(ConnectionClosed(uint,QString,QString)),
            SLOT(onConnectionClosed(uint,QString,QString)));

    if (parent->isRequested()) {
        parent->connect(streamTubeInterface,
                SIGNAL(NewRemoteConnection(uint,QDBusVariant,uint)),
                SLOT(onNewRemoteConnection(uint,QDBusVariant,uint)));
    } else {
        parent->connect(streamTubeInterface,
                SIGNAL(NewLocalConnection(uint)),
                SLOT(onNewLocalConnection(uint)));
    }

    self->readinessHelper->setIntrospectCompleted(
            StreamTubeChannel::FeatureConnectionMonitoring, true);
}

void StreamTubeChannel::Private::extractStreamTubeProperties(const QVariantMap &props)
{
    serviceName = qdbus_cast<QString>(props[QLatin1String("Service")]);
    socketTypes = qdbus_cast<SupportedSocketMap>(props[QLatin1String("SupportedSocketTypes")]);
}

/**
 * \class StreamTubeChannel
 * \ingroup clientchannel
 * \headerfile TelepathyQt4/stream-tube-channel.h <TelepathyQt4/StreamTubeChannel>
 *
 * \brief The StreamTubeChannel class represents a Telepathy channel of type StreamTube.
 *
 * It provides a transport for reliable and ordered data transfer, similar to SOCK_STREAM sockets.
 *
 * This class provides high-level methods for managing both incoming and outgoing stream tubes -
 * however, you probably want to use one of its subclasses, OutgoingStreamTubeChannel or
 * IncomingStreamTubeChannel, which both provide higher level methods for accepting
 * or offering tubes.
 *
 * For more details, please refer to \telepathy_spec.
 *
 * See \ref async_model, \ref shared_ptr
 */

/**
 * Feature representing the core that needs to become ready to make the
 * StreamTubeChannel object usable.
 *
 * Note that this feature must be enabled in order to use most
 * StreamTubeChannel methods.
 * See specific methods documentation for more details.
 */
const Feature StreamTubeChannel::FeatureStreamTube =
        Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 0);

/**
 * Feature used in order to monitor connections to this stream tube.
 *
 * See connection monitoring specific methods' documentation for more details.
 *
 * \sa newConnection(), connectionClosed()
 */
const Feature StreamTubeChannel::FeatureConnectionMonitoring =
        Feature(QLatin1String(StreamTubeChannel::staticMetaObject.className()), 1);

/**
 * Create a new StreamTubeChannel channel.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \return A StreamTubeChannelPtr object pointing to the newly created
 *         StreamTubeChannel object.
 */
StreamTubeChannelPtr StreamTubeChannel::create(const ConnectionPtr &connection,
        const QString &objectPath, const QVariantMap &immutableProperties)
{
    return StreamTubeChannelPtr(new StreamTubeChannel(connection, objectPath,
            immutableProperties, StreamTubeChannel::FeatureStreamTube));
}

/**
 * Construct a new StreamTubeChannel object.
 *
 * \param connection Connection owning this channel, and specifying the
 *                   service.
 * \param objectPath The channel object path.
 * \param immutableProperties The channel immutable properties.
 * \param coreFeature The core feature of the channel type. The corresponding introspectable should
 *                    depend on Channel::FeatureCore.
 */
StreamTubeChannel::StreamTubeChannel(const ConnectionPtr &connection,
        const QString &objectPath,
        const QVariantMap &immutableProperties,
        const Feature &coreFeature)
    : TubeChannel(connection, objectPath, immutableProperties, coreFeature),
      mPriv(new Private(this))
{
}

/**
 * Class destructor.
 */
StreamTubeChannel::~StreamTubeChannel()
{
    delete mPriv;
}

/**
 * Return the service name which will be used over this stream tube. This should be a
 * well-known TCP service name, for instance "rsync" or "daap".
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return The service name that will be used over this stream tube.
 */
QString StreamTubeChannel::service() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::service() used with "
                "FeatureStreamTube not ready";
        return QString();
    }

    return mPriv->serviceName;
}

/**
 * Return whether this stream tube is capable to accept or offer an IPv4 socket accepting all
 * incoming connections coming from localhost.
 *
 * When this capability is available, the stream tube can be accepted or offered without any
 * restriction on the access control on the other end.
 *
 * Note that the \telepathy_spec implies that any connection manager, if capable of providing
 * stream tubes, must at least support IPv4 sockets with localhost access control.
 * For this reason, this method should always return \c true.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept or offer an IPv4 socket
 *         accepting all incoming connections coming from localhost.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     supportsIPv4SocketsWithSpecifiedAddress()
 */
bool StreamTubeChannel::supportsIPv4SocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv4SocketsOnLocalhost() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept an IPv4 socket accepting all
 * incoming connections coming from a specific address for incoming tubes or whether
 * this stream tube is capable of mapping connections to the socket's source address for outgoing
 * tubes.
 *
 * For incoming tubes, when this capability is available, the stream tube can be accepted specifying
 * an IPv4 address. Every connection coming from any other address than the specified one will be
 * rejected.
 *
 * For outgoing tubes, when this capability is available, one can keep track of incoming connections
 * by enabling StreamTubeChannel::FeatureConnectionMonitoring (possibly before
 * opening the stream tube itself), and checking OutgoingStreamTubeChannel::contactsForConnections()
 * or OutgoingStreamTubeChannel::connectionsForSourceAddresses().
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsTcpSocket() or
 * OutgoingStreamTubeChannel::offerTcpSocket() with a specified address to prevent failures,
 * as the spec implies this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept an IPv4 socket accepting all
 *         incoming connections coming from a specific address for incoming tubes or whether
 *         this stream tube is capable of mapping connections to the socket's source address for
 *         outgoing tubes.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     OutgoingStreamTubeChannel::connectionsForSourceAddresses(),
 *     OutgoingStreamTubeChannel::contactsForConnections(),
 *     supportsIPv4SocketsOnLocalhost()
 */
bool StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv4SocketsWithSpecifiedAddress() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv4).contains(SocketAccessControlPort);
}

/**
 * Return whether this stream tube is capable to accept or offer an IPv6 socket accepting all
 * incoming connections coming from localhost.
 *
 * When this capability is available, the stream tube can be accepted or offered without any
 * restriction on the access control on the other end.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsTcpSocket() or
 * OutgoingStreamTubeChannel::offerTcpSocket() with a specified address to prevent failures,
 * as the spec implies this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept or offer an IPv6 socket
 *         accepting all incoming connections coming from localhost.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     supportsIPv6SocketsWithSpecifiedAddress()
 */
bool StreamTubeChannel::supportsIPv6SocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv6SocketsOnLocalhost() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept an IPv6 socket accepting all
 * incoming connections coming from a specific address for incoming tubes or whether
 * this stream tube is capable of mapping connections to the socket's source address for outgoing
 * tubes.
 *
 * For incoming tubes, when this capability is available, the stream tube can be accepted specifying
 * an IPv6 address. Every connection coming from any other address than the specified one will be
 * rejected.
 *
 * For outgoing tubes, when this capability is available, one can keep track of incoming connections
 * by enabling StreamTubeChannel::FeatureConnectionMonitoring (possibly before
 * opening the stream tube itself), and checking OutgoingStreamTubeChannel::contactsForConnections()
 * or OutgoingStreamTubeChannel::connectionsForSourceAddresses().
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsTcpSocket() or
 * OutgoingStreamTubeChannel::offerTcpSocket() with a specified address to prevent failures,
 * as the spec implies this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept an IPv6 socket accepting all
 *         incoming connections coming from a specific address for incoming tubes or whether
 *         this stream tube is capable of mapping connections to the socket's source address for
 *         outgoing tubes.
 * \sa IncomingStreamTubeChannel::acceptTubeAsTcpSocket(),
 *     OutgoingStreamTubeChannel::offerTcpSocket(),
 *     OutgoingStreamTubeChannel::connectionsForSourceAddresses(),
 *     OutgoingStreamTubeChannel::contactsForConnections(),
 *     supportsIPv6SocketsOnLocalhost()
 */
bool StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsIPv6SocketsWithSpecifiedAddress() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeIPv6).contains(SocketAccessControlPort);
}

/**
 * Return whether this stream tube is capable to accept or offer an Unix socket accepting all
 * incoming connections coming from localhost.
 *
 * When this capability is available, the stream tube can be accepted or offered without any
 * restriction on the access control on the other end.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() without credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept or offer an Unix socket
 *         accepting all incoming connections coming from localhost.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsWithCredentials()
 *     supportsAbstractUnixSocketsOnLocalhost(),
 *     supportsAbstractUnixSocketsWithCredentials(),
 */
bool StreamTubeChannel::supportsUnixSocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsOnLocalhost() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes.value(SocketAddressTypeUnix).contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept or offer an Unix socket which will require
 * credentials upon connection.
 *
 * When this capability is available and enabled, the connecting process must send a byte when
 * it first connects, which is not considered to be part of the data stream.
 * If the operating system uses sendmsg() with SCM_CREDS or SCM_CREDENTIALS to pass
 * credentials over sockets, the connecting process must do so if possible;
 * if not, it must still send the byte.
 *
 * The listening process will disconnect the connection unless it can determine
 * by OS-specific means that the connecting process has the same user ID as the listening process.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() with credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept or offer an Unix socket
 *         which will require credentials upon connection.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsOnLocalhost(),
 *     supportsAbstractUnixSocketsOnLocalhost(),
 *     supportsAbstractUnixSocketsWithCredentials(),
 */
bool StreamTubeChannel::supportsUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsUnixSocketsWithCredentials() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes[SocketAddressTypeUnix].contains(SocketAccessControlCredentials);
}

/**
 * Return whether this stream tube is capable to accept or offer an abstract Unix socket accepting
 * all incoming connections coming from localhost.
 *
 * When this capability is available, the stream tube can be accepted or offered without any
 * restriction on the access control on the other end.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() without credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept or offer an abstract Unix socket
 *         accepting all incoming connections coming from localhost.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsOnLocalhost(),
 *     supportsUnixSocketsWithCredentials(),
 *     supportsAbstractUnixSocketsWithCredentials()
 */
bool StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsAbstractUnixSocketsOnLocalhost() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes[SocketAddressTypeAbstractUnix].contains(SocketAccessControlLocalhost);
}

/**
 * Return whether this stream tube is capable to accept or offer an abstract Unix socket which will
 * require credentials upon connection.
 *
 * When this capability is available and enabled, the connecting process must send a byte when
 * it first connects, which is not considered to be part of the data stream.
 * If the operating system uses sendmsg() with SCM_CREDS or SCM_CREDENTIALS to pass
 * credentials over sockets, the connecting process must do so if possible;
 * if not, it must still send the byte.
 *
 * The listening process will disconnect the connection unless it can determine
 * by OS-specific means that the connecting process has the same user ID as the listening process.
 *
 * Note that it is strongly advised to call this method before attempting to call
 * IncomingStreamTubeChannel::acceptTubeAsUnixSocket() or
 * OutgoingStreamTubeChannel::offerUnixSocket() with credentials enabled, as the spec implies
 * this feature is not compulsory for connection managers.
 *
 * This method requires StreamTubeChannel::FeatureStreamTube to be enabled.
 *
 * \return Whether this stream tube is capable to accept or offer an abstract Unix socket
 *         which will require credentials upon connection.
 * \sa IncomingStreamTubeChannel::acceptTubeAsUnixSocket(),
 *     OutgoingStreamTubeChannel::offerUnixSocket(),
 *     supportsUnixSocketsOnLocalhost(),
 *     supportsUnixSocketsWithCredentials(),
 *     supportsAbstractUnixSocketsOnLocalhost()
 */
bool StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials() const
{
    if (!isReady(FeatureStreamTube)) {
        warning() << "StreamTubeChannel::supportsAbstractUnixSocketsWithCredentials() used with "
                "FeatureStreamTube not ready";
        return false;
    }

    return mPriv->socketTypes[SocketAddressTypeAbstractUnix].contains(SocketAccessControlCredentials);
}

/**
 * Return all the known active connections since StreamTubeChannel::FeatureConnectionMonitoring has
 * been enabled.
 *
 * For this method to return all known connections, you need to make
 * StreamTubeChannel::FeatureConnectionMonitoring ready before accepting or offering the stream
 * tube.
 *
 * This method requires StreamTubeChannel::FeatureConnectionMonitoring to be enabled.
 *
 * \return A list of active connection ids known to this stream tube.
 */
UIntList StreamTubeChannel::connections() const
{
    if (!isReady(FeatureConnectionMonitoring)) {
        warning() << "StreamTubeChannel::connections() used with "
                "FeatureConnectionMonitoring not ready";
        return UIntList();
    }

    return mPriv->connections;
}

/**
 * Return the type of socket this StreamTube is using.
 *
 * \return The type of socket this StreamTube is using
 *
 * \note This function will return a valid value only after the stream tube has been opened
 *
 * \sa localAddress
 * \sa tcpAddress
 */
SocketAddressType StreamTubeChannel::addressType() const
{
    return mPriv->addressType;
}

/**
 * Return the access control used by this stream tube.
 *
 * Note that this function will only return a valid value after the stream tube has been opened.
 *
 * \return The access control used by this stream tube.
 * \sa addressType(), TubeChannel::tubeState()
 */
SocketAccessControl StreamTubeChannel::accessControl() const
{
    return mPriv->accessControl;
}

/**
 * Return the IP address/port combination used by this stream tube.
 *
 * This method will return a meaningful value only if this stream tube socket is a TCP socket.
 *
 * Note that this function will return a valid value only after the stream tube has been opened.
 *
 * \return The IP address and port used by this stream tube.
 * \sa addressType()
 */
QPair<QHostAddress, quint16> StreamTubeChannel::ipAddress() const
{
    if (tubeState() != TubeChannelStateOpen) {
        warning() << "Tube not open, returning invalid IP address";
        return qMakePair<QHostAddress, quint16>(QHostAddress::Null, 0);
    }

    return mPriv->ipAddress;
}

/**
 * Return the local address used by this stream tube.
 *
 * This method will return a meaningful value only if this stream tube socket is a Unix socket.
 *
 * Note that this function will return a valid value only after the stream tube has been opened.
 *
 * \return The local address used by this stream tube.
 * \sa addressType()
 */
QString StreamTubeChannel::localAddress() const
{
    if (tubeState() != TubeChannelStateOpen) {
        warning() << "Tube not open, returning invalid local socket address";
        return QString();
    }

    return mPriv->unixAddress;
}

void StreamTubeChannel::setConnections(UIntList connections)
{
    mPriv->connections = connections;
}

void StreamTubeChannel::setAddressType(SocketAddressType type)
{
    mPriv->addressType = type;
}

void StreamTubeChannel::setAccessControl(SocketAccessControl accessControl)
{
    mPriv->accessControl = accessControl;
}

void StreamTubeChannel::setIpAddress(const QPair<QHostAddress, quint16> &address)
{
    mPriv->ipAddress = address;
}

void StreamTubeChannel::setLocalAddress(const QString &address)
{
    mPriv->unixAddress = address;
}

void StreamTubeChannel::gotStreamTubeProperties(PendingOperation *op)
{
    if (!op->isError()) {
        PendingVariantMap *pvm = qobject_cast<PendingVariantMap *>(op);

        mPriv->extractStreamTubeProperties(pvm->result());

        debug() << "Got reply to Properties::GetAll(StreamTubeChannel)";
        mPriv->readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureStreamTube, true);
    }
    else {
        warning().nospace() << "Properties::GetAll(StreamTubeChannel) failed "
                "with " << op->errorName() << ": " << op->errorMessage();
        mPriv->readinessHelper->setIntrospectCompleted(StreamTubeChannel::FeatureStreamTube, false,
                op->errorName(), op->errorMessage());
    }
}

void StreamTubeChannel::onConnectionClosed(uint connectionId,
        const QString &errorName, const QString &errorMessage)
{
    emit connectionClosed(connectionId, errorName, errorMessage);

    processConnectionClosed(connectionId, errorName, errorMessage);
}

void StreamTubeChannel::processConnectionClosed(uint connectionId, const QString &errorName,
            const QString &errorMessage)
{
    /* does nothing */
}

/**
 * \fn void StreamTubeChannel::connectionClosed(uint connectionId,
 *             const QString &errorName, const QString &errorMessage)
 *
 * This signal is emitted when a connection on this stream tube has been closed.
 *
 * \param connectionId The unique ID associated with the connection that was closed.
 * \param errorName The name of a D-Bus error describing the error that occurred.
 * \param errorMessage A debugging message associated with the error.
 */

} // Tp
