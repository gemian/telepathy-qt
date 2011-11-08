/**
 * This file is part of TelepathyQt4
 *
 * @copyright Copyright (C) 2009 Collabora Ltd. <http://www.collabora.co.uk/>
 * @copyright Copyright (C) 2009 Nokia Corporation
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

#ifndef _TelepathyQt4_pending_send_message_h_HEADER_GUARD_
#define _TelepathyQt4_pending_send_message_h_HEADER_GUARD_

#ifndef IN_TP_QT_HEADER
#error IN_TP_QT_HEADER
#endif

#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Types>

class QDBusPendingCallWatcher;
class QString;

namespace Tp
{

class Message;

class TP_QT_EXPORT PendingSendMessage : public PendingOperation
{
    Q_OBJECT
    Q_DISABLE_COPY(PendingSendMessage)

public:
    ~PendingSendMessage();

    TextChannelPtr channel() const;

    ContactMessengerPtr messenger() const;

    QString sentMessageToken() const;
    Message message() const;

private Q_SLOTS:
    TP_QT_NO_EXPORT void onTextSent(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onMessageSent(QDBusPendingCallWatcher *watcher);
    TP_QT_NO_EXPORT void onCDMessageSent(QDBusPendingCallWatcher *watcher);

private:
    friend class TextChannel;
    friend class ContactMessenger;

    TP_QT_NO_EXPORT PendingSendMessage(const TextChannelPtr &channel,
            const Message &message);
    TP_QT_NO_EXPORT PendingSendMessage(const ContactMessengerPtr &messenger,
            const Message &message);

    struct Private;
    friend struct Private;
    Private *mPriv;
};

} // Tp

#endif
