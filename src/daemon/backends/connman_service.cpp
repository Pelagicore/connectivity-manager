// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backends/connman_service.h"

#include <giomm.h>
#include <glibmm.h>

#include <cstdint>
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
        constexpr char PROPERTY_NAME_SECURITY[] = "Security";
        constexpr char PROPERTY_NAME_STATE[] = "State";
        constexpr char PROPERTY_NAME_STRENGTH[] = "Strength";

        // Incomplete. See src/service.c:__connman_service_type2string() in the ConnMan repo.
        // ConnMan reuses the "connman_service_type" enum for technology type as well. Need to
        // verify what types are valid/used for services.
        constexpr char TYPE_STR_BLUETOOTH[] = "bluetooth";
        constexpr char TYPE_STR_ETHERNET[] = "ethernet";
        constexpr char TYPE_STR_WIFI[] = "wifi";

        constexpr char SECURITY_STR_NONE[] = "none";
        constexpr char SECURITY_STR_WEP[] = "wep";
        constexpr char SECURITY_STR_WPA_PSK[] = "psk";
        constexpr char SECURITY_STR_WPA_EAP[] = "ieee8021x";
        // constexpr char SECURITY_STR_WPS[] = "wps";
        // constexpr char SECURITY_STR_WPS_ADVERTISING[] = "wps_advertising";

        constexpr char STATE_STR_IDLE[] = "idle";
        constexpr char STATE_STR_FAILURE[] = "failure";
        constexpr char STATE_STR_ASSOCIATION[] = "association";
        constexpr char STATE_STR_CONFIGURATION[] = "configuration";
        constexpr char STATE_STR_READY[] = "ready";
        constexpr char STATE_STR_DISCONNECT[] = "disconnect";
        constexpr char STATE_STR_ONLINE[] = "online";

        ConnManService::Type type_from_string(const Glib::ustring &str)
        {
            if (str == TYPE_STR_BLUETOOTH)
                return ConnManService::Type::BLUETOOTH;

            if (str == TYPE_STR_ETHERNET)
                return ConnManService::Type::ETHERNET;

            if (str == TYPE_STR_WIFI)
                return ConnManService::Type::WIFI;

            return ConnManService::Type::UNKNOWN;
        }

        Glib::ustring type_to_string(ConnManService::Type type)
        {
            switch (type) {
            case ConnManService::Type::UNKNOWN:
                break;
            case ConnManService::Type::BLUETOOTH:
                return TYPE_STR_BLUETOOTH;
            case ConnManService::Type::ETHERNET:
                return TYPE_STR_ETHERNET;
            case ConnManService::Type::WIFI:
                return TYPE_STR_WIFI;
            }
            return "unknown";
        }

        std::optional<Backend::WiFiSecurity> wifi_security_from_security_string(
            const Glib::ustring &str)
        {
            if (str == SECURITY_STR_NONE)
                return Backend::WiFiSecurity::NONE;

            if (str == SECURITY_STR_WEP)
                return Backend::WiFiSecurity::WEP;

            if (str == SECURITY_STR_WPA_PSK)
                return Backend::WiFiSecurity::WPA_PSK;

            if (str == SECURITY_STR_WPA_EAP)
                return Backend::WiFiSecurity::WPA_EAP;

            return {};
        }

        Glib::ustring security_to_string(const ConnManService::Security &security)
        {
            Glib::ustring ret;

            for (const Glib::ustring &str : security) {
                if (!ret.empty()) {
                    ret += ", ";
                }
                ret += str;
            }

            return ret;
        }

        ConnManService::State state_from_string(const Glib::ustring &str)
        {
            if (str == STATE_STR_IDLE)
                return ConnManService::State::IDLE;

            if (str == STATE_STR_FAILURE)
                return ConnManService::State::FAILURE;

            if (str == STATE_STR_ASSOCIATION)
                return ConnManService::State::ASSOCIATION;

            if (str == STATE_STR_CONFIGURATION)
                return ConnManService::State::CONFIGURATION;

            if (str == STATE_STR_READY)
                return ConnManService::State::READY;

            if (str == STATE_STR_DISCONNECT)
                return ConnManService::State::DISCONNECT;

            if (str == STATE_STR_ONLINE)
                return ConnManService::State::ONLINE;

            g_warning(R"(Received unknown ConnMan service state "%s", defaulting to "idle")",
                      str.c_str());

            return ConnManService::State::IDLE;
        }

        std::optional<ConnManService::State> state_from_string(
            const std::optional<Glib::ustring> &str)
        {
            if (!str)
                return {};
            return state_from_string(*str);
        }

        ConnManService::Strength strength_from_uint8(std::uint8_t i)
        {
            constexpr std::uint8_t MAX = 100;
            return i <= MAX ? i : MAX;
        }

        std::optional<ConnManService::Strength> strength_from_uint8(std::optional<std::uint8_t> i)
        {
            if (!i)
                return {};
            return strength_from_uint8(*i);
        }

        template <typename T>
        std::optional<T> value_from_variant(const Glib::VariantBase &variant,
                                            const Glib::ustring &name)
        {
            try {
                return Glib::VariantBase::cast_dynamic<Glib::Variant<T>>(variant).get();
            } catch (const std::bad_cast &) {
                g_warning("Invalid type %s for ConnMan service property \"%s\"",
                          variant.get_type_string().c_str(),
                          name.c_str());
            }
            return {};
        }

        template <typename T>
        T value_from_property_map(const ConnManService::PropertyMap &properties,
                                  const Glib::ustring &name,
                                  const T &default_value)
        {
            auto i = properties.find(name);
            if (i == properties.cend())
                return default_value;

            return value_from_variant<T>(i->second, name).value_or(default_value);
        }
    }

    ConnManService::ConnManService(Listener &listener,
                                   const Glib::DBusObjectPathString &path,
                                   const PropertyMap &properties) :
        listener_(listener),
        type_(type_from_string(
            value_from_property_map<Glib::ustring>(properties, PROPERTY_NAME_TYPE, ""))),
        name_(value_from_property_map<Glib::ustring>(properties, PROPERTY_NAME_NAME, "")),
        security_(value_from_property_map<Security>(properties, PROPERTY_NAME_SECURITY, {})),
        state_(state_from_string(value_from_property_map<Glib::ustring>(properties,
                                                                        PROPERTY_NAME_STATE,
                                                                        STATE_STR_IDLE))),
        strength_(strength_from_uint8(
            value_from_property_map<std::uint8_t>(properties, PROPERTY_NAME_STRENGTH, 0)))
    {
        Proxy::createForBus(Gio::DBus::BUS_TYPE_SYSTEM,
                            Gio::DBus::PROXY_FLAGS_NONE,
                            ConnManDBus::SERVICE_NAME,
                            path,
                            sigc::mem_fun(*this, &ConnManService::proxy_create_finish));
    }

    ConnManService::~ConnManService() = default;

    Glib::ustring ConnManService::log_id_str() const
    {
        return Glib::ustring("ConnMan service \"") + name_ + "\" (" + type_to_string(type_) + ")";
    }

    void ConnManService::proxy_create_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        try {
            proxy_ = Proxy::createForBusFinish(result);
        } catch (const Glib::Error &e) {
            g_warning(
                "Failed to create D-Bus proxy for %s: %s", log_id_str().c_str(), e.what().c_str());
            return;
        }

        proxy_->PropertyChanged_signal.connect(
            sigc::mem_fun(*this, &ConnManService::property_changed));

        listener_.service_proxy_created(*this);
    }

    void ConnManService::properties_changed(const PropertyMap &properties)
    {
        for (const auto &[name, value] : properties)
            property_changed(name, value);
    }

    void ConnManService::property_changed(const Glib::ustring &property_name,
                                          const Glib::VariantBase &value)
    {
        auto changed = [this](auto &property, PropertyId id, auto received) {
            if (!received || property == *received)
                return;

            property = std::move(*received);

            if (proxy_)
                listener_.service_property_changed(*this, id);
        };

        if (property_name == PROPERTY_NAME_NAME) {
            changed(
                name_, PropertyId::NAME, value_from_variant<Glib::ustring>(value, property_name));

        } else if (property_name == PROPERTY_NAME_SECURITY) {
            changed(security_,
                    PropertyId::SECURITY,
                    value_from_variant<Security>(value, property_name));

        } else if (property_name == PROPERTY_NAME_STATE) {
            changed(state_,
                    PropertyId::STATE,
                    state_from_string(value_from_variant<Glib::ustring>(value, property_name)));

        } else if (property_name == PROPERTY_NAME_STRENGTH) {
            changed(strength_,
                    PropertyId::STRENGTH,
                    strength_from_uint8(value_from_variant<std::uint8_t>(value, property_name)));

        } else if (property_name == PROPERTY_NAME_TYPE) {
            g_warning("Assumed to be constant property \"%s\" changed for %s",
                      property_name.c_str(),
                      log_id_str().c_str());

        } else {
            // Many properties are left out, does not make sense to log "unknown property".
        }
    }

    Backend::WiFiSecurity ConnManService::security_to_wifi_security() const
    {
        for (const Glib::ustring &str : security_) {
            auto wifi_security = wifi_security_from_security_string(str);

            if (wifi_security) {
                return *wifi_security;
            }
        }

        g_warning("Failed to convert security (%s) for %s to Wi-Fi security",
                  security_to_string(security_).c_str(),
                  log_id_str().c_str());

        return Backend::WiFiSecurity::NONE;
    }

    void ConnManService::connect()
    {
        constexpr int TIMEOUT_MS = 5 * 60 * 1000;

        proxy_->Connect(sigc::mem_fun(*this, &ConnManService::connect_finish), {}, TIMEOUT_MS);
    }

    void ConnManService::connect_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        bool success = false;

        try {
            proxy_->Connect_finish(result);
            success = true;
        } catch (const Glib::Error &e) {
            // TODO: Need to do some extra checking here.
            //
            // - net.connman.Error.AlreadyConnected is returned if already connected. Should
            //   probably not see that as an error...? Set success to true if this error occurs...?
            //
            // - net.connman.Error.InProgress is returned connecting is in progress. See as error?
            //   Unreasonable to wait for state to change, I think. Perhaps "connecting" needs to be
            //   propagated upwards so e.g. UI can choose to not connect to a "service" that is
            //   already "connecting". Still potential for race condition though... should only
            //   occur if source other than UI connects service and UI tries to connect at the same
            //   time... so perhaps this particular ConnMan error should just be reported as a
            //   "Connect() failed" (KISS).
            g_warning("Failed to connect %s: %s", log_id_str().c_str(), e.what().c_str());
        }

        listener_.service_connect_finished(*this, success);
    }

    void ConnManService::disconnect()
    {
        proxy_->Disconnect(sigc::mem_fun(*this, &ConnManService::disconnect_finish));
    }

    void ConnManService::disconnect_finish(const Glib::RefPtr<Gio::AsyncResult> &result)
    {
        try {
            proxy_->Disconnect_finish(result);
        } catch (const Glib::Error &e) {
            g_warning("Failed to disconnect %s: %s", log_id_str().c_str(), e.what().c_str());
        }
    }
}
