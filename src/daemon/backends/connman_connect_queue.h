// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_CONNECT_QUEUE_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_CONNECT_QUEUE_H

#include <deque>

#include "common/credentials.h"
#include "daemon/backend.h"
#include "daemon/backends/connman_service.h"

namespace ConnectivityManager::Daemon
{
    // Queue for service connect requests.
    //
    // Needed because agent (ConnManAgent) may not have been registered with ConnMan when a connect
    // request is received. Requests should be processed in order when agent has been registered.
    //
    // TODO: Only a single connect request is processed at a time now. The thinking was that due to
    // ConnMan's agent D-Bus API has a Cancel() method that does not mention what service the
    // Cancel() call is for, only a single connect could be handled at a time. Turns out that maybe
    // nothing needs to be done in Cancel() in the agent though. See TODO on ConnManAgent::Cancel()
    // for verifying this. If that is the case the "one pending connect" at a time limitation can be
    // removed. It probably makes sense to modify it so that:
    // - Connect requests are only queued up in this class if the agent is not registered with
    //   ConnMan. If the agent is ready, ConnManService::connect() should be called immediately.
    //   - Modify ConnManService::Connect() to take Backend::ConnectFinished &&finished and
    //     Backend::RequestCredentialsFromUser &&request_credentials and store these in
    //     ConnManService. And invoke when appropriate.
    //   - ConnManService::Listener::service_connect_finished() can be removed since ConnectFinished
    //     callbacks are invoked in ConnManService::connect_finish().
    //   - Probably also makes sense to only store ConnectFinished callback and not invoke the
    //     service's Connect() method if a Connect() is already pending. (Callbacks should be
    //     invokes in FIFO order.)
    //   - Only makes sense to store a single Backend::RequestCredentialsFromUser callback? Perhaps
    //     make it so that it is optional to pass it as well?
    // - When agent registration reply is received:
    //   - if it failed, keep current behavior and call fail_all_and_clear().
    //   - if success, call ConnManService::connect() for all entries and move out of this queue.
    // - connect_if_not_empty() should be renamed to connect_all_queued_up() or something...
    // - connect_finished() and request_credentials() should be removed.
    class ConnManConnectQueue
    {
    public:
        void enqueue(ConnManService &service,
                     Backend::ConnectFinished &&finished,
                     Backend::RequestCredentialsFromUser &&request_credentials,
                     bool connect_if_queue_empty);

        void remove_service(const ConnManService &service);

        void fail_all_and_clear();

        void connect_if_not_empty();

        void connect_finished(const ConnManService &service, bool success);

        void request_credentials(const ConnManService &service,
                                 const Common::Credentials::Requested &requested,
                                 Backend::RequestCredentialsFromUserReply &&reply) const;

    private:
        struct Entry
        {
            ConnManService *service = nullptr;
            bool connecting = false;
            Backend::ConnectFinished finished;
            Backend::RequestCredentialsFromUser request_credentials;
        };

        std::deque<Entry> entries_;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_CONNECT_QUEUE_H
