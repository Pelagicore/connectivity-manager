// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backends/connman_connect_queue.h"

#include <utility>
#include <vector>

#include "daemon/backend.h"
#include "daemon/backends/connman_service.h"

namespace ConnectivityManager::Daemon
{
    void ConnManConnectQueue::enqueue(ConnManService &service,
                                      Backend::ConnectFinished &&finished,
                                      Backend::RequestCredentialsFromUser &&request_credentials,
                                      bool connect_if_queue_empty)
    {
        bool connect = connect_if_queue_empty && entries_.empty();

        entries_.emplace_back(
            Entry{&service, false, std::move(finished), std::move(request_credentials)});

        if (connect) {
            Entry &entry = entries_.front();
            entry.connecting = true;
            entry.service->connect();
        }
    }

    void ConnManConnectQueue::remove_service(const ConnManService &service)
    {
        std::vector<Backend::ConnectFinished> finished_callbacks; // Callbacks can modify entries_.

        for (auto i = entries_.begin(); i != entries_.end();) {
            if (i->service == &service) {
                finished_callbacks.emplace_back(std::move(i->finished));
                i = entries_.erase(i);
            } else {
                i++;
            }
        }

        for (auto &finished_callback : finished_callbacks) {
            finished_callback(Backend::ConnectResult::FAILED);
        }
    }

    void ConnManConnectQueue::fail_all_and_clear()
    {
        if (entries_.empty()) {
            return;
        }

        std::deque<Entry> entries_to_fail = std::move(entries_); // Callbacks can modify entries_.

        for (const auto &entry : entries_to_fail) {
            entry.finished(Backend::ConnectResult::FAILED);
        }

        entries_ = std::deque<Entry>();
    }

    void ConnManConnectQueue::connect_if_not_empty()
    {
        if (entries_.empty()) {
            return;
        }

        Entry &entry = entries_.front();

        if (!entry.connecting) {
            entry.connecting = true;
            entry.service->connect();
        }
    }

    void ConnManConnectQueue::connect_finished(const ConnManService &service, bool success)
    {
        if (entries_.empty()) {
            g_warning("Service finished connecting but connect queue is empty");
            return;
        }

        if (entries_.front().service != &service) {
            // TODO: So... weird state now? Bring forth the sledge hammer and just
            //       fail_all_and_clear()? Better than continuing in a world of lies... gets us back
            //       to sane state faster...? If "single connect at a time" limitation is removed
            //       (see TODO in connman_connect_queue.h), this will not be a problem since
            //       "connect finished" will be handled locally in ConnManService and this
            //       method will not exist.
            g_warning("Service finished connecting but not first in queue");
            return;
        }

        Entry entry = std::move(entries_.front());
        entries_.pop_front();

        entry.finished(success ? Backend::ConnectResult::SUCCESS : Backend::ConnectResult::FAILED);

        connect_if_not_empty();
    }

    void ConnManConnectQueue::request_credentials(
        const ConnManService &service,
        const Common::Credentials::Requested &requested,
        Backend::RequestCredentialsFromUserReply &&reply) const
    {
        if (entries_.empty()) {
            g_warning("Received unexpected credentials request, queue empty");
            reply(Common::Credentials::NONE);
            return;
        }

        const Entry &entry = entries_.front();

        if (entry.service != &service) {
            g_warning("Received unexpected credentials request for service not first in queue");
            reply(Common::Credentials::NONE);
            return;
        }

        entry.request_credentials(requested, std::move(reply));
    }
}
