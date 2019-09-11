// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/dbus_objects/wifi_access_point.h"

#include <glibmm.h>

#include <string>
#include <type_traits>

#include "common/dbus.h"
#include "common/string_to_uint64.h"

namespace ConnectivityManager::Daemon
{
    namespace
    {
        Glib::ustring wifi_security_to_str(Backend::WiFiSecurity security)
        {
            switch (security) {
            case Backend::WiFiSecurity::NONE:
                return "";
            case Backend::WiFiSecurity::WEP:
                return "wep";
            case Backend::WiFiSecurity::WPA_PSK:
                return "wpa-psk";
            case Backend::WiFiSecurity::WPA_EAP:
                return "wpa-eap";
            }
            return "";
        }
    }

    WiFiAccessPoint::WiFiAccessPoint(const Backend::WiFiAccessPoint &backend_ap) :
        id_(backend_ap.id),
        ssid_(backend_ap.ssid),
        strength_(backend_ap.strength),
        connected_(backend_ap.connected),
        security_(wifi_security_to_str(backend_ap.security))
    {
    }

    Glib::ustring WiFiAccessPoint::object_path() const
    {
        return object_path_prefix() + std::to_string(id_);
    }

    std::optional<WiFiAccessPoint::Id> WiFiAccessPoint::object_path_to_id(
        const Glib::DBusObjectPathString &path)
    {
        Glib::ustring prefix = object_path_prefix();
        if (path.compare(0, prefix.size(), prefix) != 0)
            return {};

        Glib::ustring id_str = path.substr(prefix.size());

        static_assert(std::is_same_v<Id, std::uint64_t>);
        return Common::string_to_uint64(id_str.raw());
    }

    Glib::ustring WiFiAccessPoint::object_path_prefix()
    {
        return Glib::ustring(Common::DBus::MANAGER_OBJECT_PATH) + "/WiFiAccessPoints/";
    }

    bool WiFiAccessPoint::SSID_setHandler(const std::string &value)
    {
        bool changed = ssid_ != value;
        ssid_ = value;

        return changed;
    }

    std::string WiFiAccessPoint::SSID_get()
    {
        return ssid_;
    }

    bool WiFiAccessPoint::Strength_setHandler(guchar value)
    {
        bool changed = strength_ != value;
        strength_ = value;

        return changed;
    }

    guchar WiFiAccessPoint::Strength_get()
    {
        return strength_;
    }

    bool WiFiAccessPoint::Connected_setHandler(bool value)
    {
        bool changed = connected_ != value;
        connected_ = value;

        return changed;
    }

    bool WiFiAccessPoint::Connected_get()
    {
        return connected_;
    }

    void WiFiAccessPoint::security_set(Backend::WiFiSecurity security)
    {
        Security_set(wifi_security_to_str(security));
    }

    bool WiFiAccessPoint::Security_setHandler(const Glib::ustring &value)
    {
        bool changed = security_ != value;
        security_ = value;

        return changed;
    }

    Glib::ustring WiFiAccessPoint::Security_get()
    {
        return security_;
    }
}
