// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backends/connman_technology.h"

#include <glibmm.h>

#include <optional>
#include <typeinfo>
#include <utility>

#include "daemon/backends/connman_dbus.h"

namespace ConnectivityManager::Daemon
{
    namespace
    {
        constexpr char PROPERTY_NAME_TYPE[] = "Type";
        constexpr char PROPERTY_NAME_NAME[] = "Name";
        constexpr char PROPERTY_NAME_CONNECTED[] = "Connected";
        constexpr char PROPERTY_NAME_POWERED[] = "Powered";
        constexpr char PROPERTY_NAME_TETHERING[] = "Tethering";
        constexpr char PROPERTY_NAME_TETHERING_IDENTIFIER[] = "TetheringIdentifier";
        constexpr char PROPERTY_NAME_TETHERING_PASSPHRASE[] = "TetheringPassphrase";

        // Incomplete. See src/service.c:__connman_service_type2string() in the ConnMan repo.
        // "service" here is not a mistake. ConnMan reuses the "connman_service_type" enum for
        // technology type as well. Need to verify what types are valid/used for technologies.
        constexpr char TYPE_STR_BLUETOOTH[] = "bluetooth";
        constexpr char TYPE_STR_ETHERNET[] = "ethernet";
        constexpr char TYPE_STR_WIFI[] = "wifi";

        ConnManTechnology::Type type_from_string(const Glib::ustring &str)
        {
            if (str == TYPE_STR_BLUETOOTH)
                return ConnManTechnology::Type::BLUETOOTH;

            if (str == TYPE_STR_ETHERNET)
                return ConnManTechnology::Type::ETHERNET;

            if (str == TYPE_STR_WIFI)
                return ConnManTechnology::Type::WIFI;

            return ConnManTechnology::Type::UNKNOWN;
        }

        Glib::ustring type_to_string(ConnManTechnology::Type type)
        {
            switch (type) {
            case ConnManTechnology::Type::UNKNOWN:
                break;
            case ConnManTechnology::Type::BLUETOOTH:
                return TYPE_STR_BLUETOOTH;
            case ConnManTechnology::Type::ETHERNET:
                return TYPE_STR_ETHERNET;
            case ConnManTechnology::Type::WIFI:
                return TYPE_STR_WIFI;
            }
            return "unknown";
        }

        template <typename T>
        std::optional<T> value_from_variant(const Glib::VariantBase &variant,
                                            const Glib::ustring &name)
        {
            try {
                return Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(variant).get();
            } catch (const std::bad_cast &) {
                g_warning("Invalid type %s for ConnMan technology property \"%s\"",
                          variant.get_type_string().c_str(),
                          name.c_str());
            }
            return {};
        }

        template <typename T>
        T value_from_property_map(const ConnManTechnology::PropertyMap &properties,
                                  const Glib::ustring &name,
                                  const T &default_value)
        {
            auto i = properties.find(name);
            if (i == properties.cend())
                return default_value;

            return value_from_variant<T>(i->second, name).value_or(default_value);
        }
    }

    ConnManTechnology::ConnManTechnology(Listener &listener,
                                         const Glib::DBusObjectPathString &path,
                                         const PropertyMap &properties) :
        listener_(listener),
        type_(type_from_string(
            value_from_property_map<Glib::ustring>(properties, PROPERTY_NAME_TYPE, ""))),
        name_(value_from_property_map<Glib::ustring>(properties, PROPERTY_NAME_NAME, "")),
        connected_(value_from_property_map<bool>(properties, PROPERTY_NAME_CONNECTED, false)),
        powered_(*this, PropertyId::POWERED, PROPERTY_NAME_POWERED, properties, false),
        tethering_(*this, PropertyId::TETHERING, PROPERTY_NAME_TETHERING, properties, false),
        tethering_identifier_(*this,
                              PropertyId::TETHERING_IDENTIFIER,
                              PROPERTY_NAME_TETHERING_IDENTIFIER,
                              properties,
                              ""),
        tethering_passphrase_(*this,
                              PropertyId::TETHERING_PASSPHRASE,
                              PROPERTY_NAME_TETHERING_PASSPHRASE,
                              properties,
                              "")
    {
        Proxy::createForBus(Gio::DBus::BUS_TYPE_SYSTEM,
                            Gio::DBus::PROXY_FLAGS_NONE,
                            ConnManDBus::SERVICE_NAME,
                            path,
                            sigc::mem_fun(*this, &ConnManTechnology::proxy_create_finish));
    }

    ConnManTechnology::~ConnManTechnology() = default;

    Glib::ustring ConnManTechnology::log_id_str() const
    {
        Glib::ustring type_str = type_to_string(type_);
        return Glib::ustring("ConnMan technology \"") + name_ + "\" (" + type_str + ")";
    }

    void ConnManTechnology::proxy_create_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        try {
            proxy_ = Proxy::createForBusFinish(result);
        } catch (const Glib::Error &e) {
            g_warning(
                "Failed to create D-Bus proxy for %s: %s", log_id_str().c_str(), e.what().c_str());
            return;
        }

        proxy_->PropertyChanged_signal.connect(
            sigc::mem_fun(*this, &ConnManTechnology::property_changed));

        listener_.technology_proxy_created(*this);
    }

    void ConnManTechnology::property_changed(const Glib::ustring &property_name,
                                             const Glib::VariantBase &value)
    {
        auto changed = [this](auto &property, PropertyId id, auto received) {
            if (!received || property == *received)
                return;

            property = std::move(*received);
            listener_.technology_property_changed(*this, id);
        };

        if (property_name == PROPERTY_NAME_CONNECTED) {
            changed(
                connected_, PropertyId::CONNECTED, value_from_variant<bool>(value, property_name));

        } else if (property_name == PROPERTY_NAME_POWERED) {
            powered_.changed(value);

        } else if (property_name == PROPERTY_NAME_TETHERING) {
            tethering_.changed(value);

        } else if (property_name == PROPERTY_NAME_TETHERING_IDENTIFIER) {
            tethering_identifier_.changed(value);

        } else if (property_name == PROPERTY_NAME_TETHERING_PASSPHRASE) {
            tethering_passphrase_.changed(value);

        } else if (property_name == PROPERTY_NAME_TYPE || property_name == PROPERTY_NAME_NAME) {
            g_warning("Assumed to be constant property \"%s\" changed for %s",
                      property_name.c_str(),
                      log_id_str().c_str());

        } else {
            g_warning("Received unknown property \"%s\" for %s",
                      property_name.c_str(),
                      log_id_str().c_str());
        }
    }

    ConnManTechnology::Type ConnManTechnology::type() const
    {
        return type_;
    }

    const Glib::ustring &ConnManTechnology::name() const
    {
        return name_;
    }

    bool ConnManTechnology::connected() const
    {
        return connected_;
    }

    bool ConnManTechnology::powered() const
    {
        return powered_.value();
    }

    void ConnManTechnology::set_powered(bool powered)
    {
        powered_.set(powered);
    }

    bool ConnManTechnology::tethering() const
    {
        return tethering_.value();
    }

    void ConnManTechnology::set_tethering(bool tethering)
    {
        tethering_.set(tethering);
    }

    const Glib::ustring &ConnManTechnology::tethering_identifier() const
    {
        return tethering_identifier_.value();
    }

    void ConnManTechnology::set_tethering_identifier(const Glib::ustring &identifier)
    {
        tethering_identifier_.set(identifier);
    }

    const Glib::ustring &ConnManTechnology::tethering_passphrase() const
    {
        return tethering_passphrase_.value();
    }

    void ConnManTechnology::set_tethering_passphrase(const Glib::ustring &passphrase)
    {
        tethering_passphrase_.set(passphrase);
    }

    void ConnManTechnology::scan()
    {
        proxy_->Scan(sigc::mem_fun(*this, &ConnManTechnology::scan_finish));
    }

    void ConnManTechnology::scan_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        try {
            proxy_->Scan_finish(result);
        } catch (const Glib::Error &e) {
            g_warning("Failed to scan %s: %s", log_id_str().c_str(), e.what().c_str());
        }
    }

    template <typename V>
    ConnManTechnology::SettableProperty<V>::SettableProperty(ConnManTechnology &technology,
                                                             PropertyId id,
                                                             const Glib::ustring &name,
                                                             const PropertyMap &property_map,
                                                             const V &default_value) :
        technology_(technology),
        id_(id),
        name_(name),
        value_(value_from_property_map<V>(property_map, name, default_value))
    {
    }

    template <typename V>
    const V &ConnManTechnology::SettableProperty<V>::value() const
    {
        if (queued_)
            return *queued_;

        if (pending_)
            return *pending_;

        return value_;
    }

    template <typename V>
    void ConnManTechnology::SettableProperty<V>::set(const V &new_value)
    {
        if (value() == new_value)
            return;

        if (!pending_) {
            pending_ = new_value;
            set_property(*pending_);
        } else {
            queued_ = new_value;
        }

        technology_.listener_.technology_property_changed(technology_, id_);
    }

    template <typename V>
    void ConnManTechnology::SettableProperty<V>::changed(const Glib::VariantBase &received_variant)
    {
        auto received = value_from_variant<V>(received_variant, name_);
        if (!received)
            return;

        if (pending_) {
            received_ = std::move(*received);
        } else if (value_ != *received) {
            value_ = std::move(*received);
            technology_.listener_.technology_property_changed(technology_, id_);
        }
    }

    template <typename V>
    void ConnManTechnology::SettableProperty<V>::set_property(const V &value)
    {
        technology_.proxy_->SetProperty(
            name_,
            Glib::Variant<V>::create(value),
            sigc::mem_fun(*this, &ConnManTechnology::SettableProperty<V>::set_property_finish));
    }

    template <typename V>
    void ConnManTechnology::SettableProperty<V>::set_property_finish(
        const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        bool success = false;

        try {
            technology_.proxy_->SetProperty_finish(result);
            success = true;
        } catch (const Glib::Error &e) {
            g_warning("Failed to set property \"%s\" for %s: %s)",
                      name_.c_str(),
                      technology_.log_id_str().c_str(),
                      e.what().c_str());
        }

        if (success)
            value_ = std::move(*pending_);
        pending_.reset();

        if (queued_) {
            pending_ = std::move(*queued_);
            queued_.reset();
            set_property(*pending_);
        }

        if (!pending_) {
            bool signal_reverted_due_to_failure = !success;
            bool changed_while_pending = false;

            if (received_) {
                if (value_ != *received_) {
                    value_ = std::move(*received_);
                    changed_while_pending = true;
                }
                received_.reset();
            }

            if (signal_reverted_due_to_failure || changed_while_pending)
                technology_.listener_.technology_property_changed(technology_, id_);
        }
    }
}
