// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/dbus_service.h"

#include <giomm.h>
#include <glibmm.h>

#include "common/dbus.h"

namespace TemplateDBusService::Daemon
{
    DBusService::DBusService(const Glib::RefPtr<Glib::MainLoop> &main_loop) : main_loop_(main_loop)
    {
    }

    DBusService::~DBusService()
    {
        unown_name();
    }

    void DBusService::own_name()
    {
        if (connection_id_ != 0)
            return;

        connection_id_ = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SYSTEM,
                                             Common::DBus::TEMPLATE_SERVICE_NAME,
                                             sigc::mem_fun(*this, &DBusService::bus_acquired),
                                             sigc::mem_fun(*this, &DBusService::name_acquired),
                                             sigc::mem_fun(*this, &DBusService::name_lost));
    }

    void DBusService::unown_name()
    {
        if (connection_id_ == 0)
            return;

        Gio::DBus::unown_name(connection_id_);
        connection_id_ = 0;
    }

    void DBusService::bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connection,
                                   const Glib::ustring & /*name*/)
    {
        if (service_.register_object(connection, Common::DBus::TEMPLATE_OBJECT_PATH) == 0)
            main_loop_->quit();
    }

    void DBusService::name_acquired(const Glib::RefPtr<Gio::DBus::Connection> & /*connection*/,
                                    const Glib::ustring &name)
    {
        g_info("Acquired D-Bus name %s", name.c_str());
    }

    void DBusService::name_lost(const Glib::RefPtr<Gio::DBus::Connection> & /*connection*/,
                                const Glib::ustring &name)
    {
        g_warning("Lost D-Bus name %s, quitting", name.c_str());
        main_loop_->quit();
    }

    void DBusService::Service::RemoveMeFoo(guint32 number, MethodInvocation &invocation)
    {
        bool is_odd = (number % 2) != 0;
        invocation.ret(is_odd);
    }

    bool DBusService::Service::RemoveMeBaz_setHandler(bool value)
    {
        remove_me_baz_ = value;
        return true;
    }

    bool DBusService::Service::RemoveMeBaz_get()
    {
        return remove_me_baz_;
    }
}
