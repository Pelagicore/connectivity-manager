// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#ifndef CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_TECHNOLOGY_H
#define CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_TECHNOLOGY_H

#include <giomm.h>
#include <glibmm.h>
#include <sigc++/sigc++.h>

#include <map>
#include <optional>

#include "generated/dbus/connman_proxy.h"

namespace ConnectivityManager::Daemon
{
    // Helper class for ConnMan technologies. See doc/technology-api.txt in the ConnMan repo.
    //
    // Encapsulates asynchronous creation of D-Bus proxy and handling of properties.
    //
    // ConnMan does not use the standard org.freedesktop.DBus.Properties interface. The D-Bus
    // generator can not generate setters, getters and signals for ConnMan's custom properties
    // interface so it must be handled manually. Trivial for read-only properties but more
    // complicated for read/write properties. See SettableProperty.
    class ConnManTechnology : public sigc::trackable
    {
    public:
        using PropertyMap = std::map<Glib::ustring, Glib::VariantBase>;

        enum class Type
        {
            UNKNOWN,

            BLUETOOTH,
            ETHERNET,
            WIFI
        };

        enum class PropertyId
        {
            CONNECTED,
            POWERED,
            TETHERING,
            TETHERING_IDENTIFIER,
            TETHERING_PASSPHRASE
        };

        class Listener;

        ConnManTechnology(Listener &listener,
                          const Glib::DBusObjectPathString &path,
                          const PropertyMap &properties);
        ~ConnManTechnology();

        Type type() const;

        const Glib::ustring &name() const;

        bool connected() const;

        bool powered() const;
        void set_powered(bool powered);

        bool tethering() const;
        void set_tethering(bool tethering);

        const Glib::ustring &tethering_identifier() const;
        void set_tethering_identifier(const Glib::ustring &identifier);

        const Glib::ustring &tethering_passphrase() const;
        void set_tethering_passphrase(const Glib::ustring &passphrase);

        void scan();

    private:
        using Proxy = net::connman::TechnologyProxy;

        // Helper for properties that are settable.
        //
        // Needed since ConnMan does not use the org.freedesktop.DBus.Properties interface.
        //
        // Local value is changed immediately when set (Listener::technology_property_changed() is
        // called). If ConnMan reports that setting the value failed, local value is reverted to the
        // last known value received from ConnMan (Listener::technology_property_changed() is called
        // again).
        //
        // If setting a value in ConnMan is already pending, the new value is queued to be set when
        // result of pending set is received. If a value has been queued and a set is performed
        // again, the old queued value will be discarded and never sent to ConnMan. If this turns
        // out to be a problem, a real queue has to be added.
        //
        // If a value is received from ConnMan, it is updated directly "property changed" is
        // signalled if there is no pending set. If a set is pending, the value is updated when the
        // set is finished if there is no other value queued. The received value may be due to our
        // own set call so "property changed" is only signalled internally if the value is different
        // since otherwise it has already been done for that particular value.
        //
        // TODO: Can the logic be simplified? Should be possible to implement current behavior
        //       without three std::optional:s. Try and see if it simplifies code.
        // TODO: Add temporary extensive logging and make sure behavior is correct.
        template <typename V>
        struct SettableProperty : public sigc::trackable
        {
        public:
            SettableProperty(ConnManTechnology &technology,
                             PropertyId id,
                             const Glib::ustring &name,
                             const PropertyMap &property_map,
                             const V &default_value);

            const V &value() const;
            void set(const V &new_value);
            void changed(const Glib::VariantBase &received_variant);

        private:
            void set_property(const V &value);
            void set_property_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

            ConnManTechnology &technology_;
            const PropertyId id_;
            const Glib::ustring name_;

            V value_;
            std::optional<V> pending_;
            std::optional<V> queued_;
            std::optional<V> received_;
        };

        Glib::ustring log_id_str() const;

        void proxy_create_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

        void property_changed(const Glib::ustring &property_name, const Glib::VariantBase &value);

        void scan_finish(const Glib::RefPtr<Gio::AsyncResult> &result);

        Listener &listener_;
        Glib::RefPtr<Proxy> proxy_;

        const Type type_ = Type::UNKNOWN;
        const Glib::ustring name_;
        bool connected_ = false;

        SettableProperty<bool> powered_;

        SettableProperty<bool> tethering_;
        SettableProperty<Glib::ustring> tethering_identifier_;
        SettableProperty<Glib::ustring> tethering_passphrase_;
    };

    // Listener for technology events.
    //
    // technology_proxy_created() is guaranteed to be called before any other method in Listener.
    // ConnManBackend will not consider the technology available until this has been done. This is
    // to make sure that it is not used by the rest of the program before the proxy has been
    // created.
    //
    // technology_property_changed() is only called for properties that can change (not called for
    // constant properties at creation). PropertyId only has entries for these properties.
    class ConnManTechnology::Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void technology_proxy_created(ConnManTechnology &technology) = 0;
        virtual void technology_property_changed(ConnManTechnology &technology, PropertyId id) = 0;
    };
}

#endif // CONNECTIVITY_MANAGER_DAEMON_BACKENDS_CONNMAN_TECHNOLOGY_H
