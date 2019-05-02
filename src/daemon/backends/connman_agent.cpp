// Copyright (C) 2019 Luxoft Sweden AB
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// SPDX-License-Identifier: MPL-2.0

#include "daemon/backends/connman_agent.h"

#include <giomm.h>
#include <glibmm.h>

#include <cstdint>
#include <optional>
#include <typeinfo>
#include <utility>
#include <vector>

#include "common/credentials.h"
#include "common/dbus.h"
#include "common/string_to_valid_utf8.h"

namespace ConnectivityManager::Daemon
{
    namespace
    {
        constexpr char FIELD_NAME_HIDDEN_SSID_UTF8[] = "Name";
        constexpr char FIELD_NAME_HIDDEN_SSID[] = "SSID";
        constexpr char FIELD_NAME_EAP_USERNAME[] = "Identity";
        constexpr char FIELD_NAME_PASSPHRASE[] = "Passphrase";
        constexpr char FIELD_NAME_PREVIOUS_PASSPHRASE[] = "PreviousPassphrase";
        constexpr char FIELD_NAME_WPS[] = "WPS";
        constexpr char FIELD_NAME_WISPR_USERNAME[] = "Username";
        constexpr char FIELD_NAME_WISPR_PASSWORD[] = "Password";

        constexpr char FIELD_ARGUMENT_TYPE[] = "Type";
        constexpr char FIELD_ARGUMENT_VALUE[] = "Value";
        // constexpr char FIELD_ARGUMENT_REQUIREMENT[] = "Requirement"; Not checked at the moment.
        // constexpr char FIELD_ARGUMENT_ALTERNATES[] = "Alternates"; Not checked at the moment.

        constexpr char FIELD_ARGUMENT_TYPE_PSK[] = "psk";
        constexpr char FIELD_ARGUMENT_TYPE_WEP[] = "wep";
        constexpr char FIELD_ARGUMENT_TYPE_PASSPHRASE[] = "passphrase";
        constexpr char FIELD_ARGUMENT_TYPE_RESPONSE[] = "response";
        constexpr char FIELD_ARGUMENT_TYPE_WPS_PIN[] = "wpspin";
        constexpr char FIELD_ARGUMENT_TYPE_STRING[] = "string";
        // constexpr char FIELD_ARGUMENT_TYPE_SSID[] = "ssid"; Not checked at the moment.

        using Fields = std::map<Glib::ustring, Glib::VariantBase>;
        using Arguments = std::map<Glib::ustring, Glib::VariantBase>;
        using Credentials = Common::Credentials;
        using Password = Common::Credentials::Password;

        std::optional<Arguments> arguments_from_variant(const Glib::ustring &field_name,
                                                        const Glib::VariantBase &variant)
        {
            try {
                return Glib::VariantBase::cast_dynamic<Glib::Variant<Arguments>>(variant).get();
            } catch (const std::bad_cast &) {
                g_warning("Received ConnMan agent field %s with arguments of wrong type %s",
                          field_name.c_str(),
                          variant.get_type_string().c_str());
            }
            return {};
        }

        Glib::ustring argument_lookup(const Arguments &arguments,
                                      const Glib::ustring &name,
                                      const Glib::ustring &default_value)
        {
            auto i = arguments.find(name);
            if (i == arguments.cend())
                return default_value;

            const Glib::VariantBase &variant = i->second;

            try {
                return Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(variant).get();
            } catch (const std::bad_cast &) {
                g_warning("Received ConnMan agent field argument %s with wrong type %s",
                          name.c_str(),
                          variant.get_type_string().c_str());
            }

            return default_value;
        }

        std::optional<Password> arguments_to_password(const Glib::ustring &name,
                                                      const Arguments &arguments)
        {
            Password password;

            Glib::ustring type = argument_lookup(arguments, FIELD_ARGUMENT_TYPE, "");
            if (type.empty()) {
                g_warning("Received ConnMan agent password field %s without type", name.c_str());
                return {};
            }

            if (type == FIELD_ARGUMENT_TYPE_PASSPHRASE || type == FIELD_ARGUMENT_TYPE_RESPONSE ||
                type == FIELD_ARGUMENT_TYPE_STRING) {
                password.type = Password::Type::PASSPHRASE;
            } else if (type == FIELD_ARGUMENT_TYPE_PSK) {
                password.type = Password::Type::WPA_PSK;
            } else if (type == FIELD_ARGUMENT_TYPE_WEP) {
                password.type = Password::Type::WEP_KEY;
            } else if (type == FIELD_ARGUMENT_TYPE_WPS_PIN) {
                password.type = Password::Type::WPS_PIN;
            } else {
                g_warning("Received ConnMan agent password field %s with unknown type %s",
                          name.c_str(),
                          type.c_str());
                return {};
            }

            password.value = argument_lookup(arguments, FIELD_ARGUMENT_VALUE, "");

            return password;
        }

        // Map fields received in RequestInput() to a Common::Credentials struct.
        //
        // See doc/agent-api.txt for some examples of contents of fields. "Value" argument of
        // "Passphrase", "Password" and "WPS" is preferred over "PreviousPassphrase".
        // "PreviousPassphrase" is used if "Value" is not set and the type matches.
        //
        // TODO: A lot of the cases are not tested. Should try to trigger all possible inputs from
        //       ConnMan that we want to handle. Maybe also be a bit more strict and check
        //       "Requirement" and "Alternates" etc. and fail if values are not correct. Or? *Sigh*
        //       The ConnMan D-Bus API for this is under specified in doc/agent-api.txt (have to
        //       check code) and so... needlessly complicated! *Sigh*
        std::optional<Credentials> received_fields_to_credentials(const Fields &received_fields)
        {
            Credentials credentials;
            std::optional<Password> previous_password;

            for (const auto &[name, arguments_variant] : received_fields) {
                std::optional<Arguments> arguments =
                    arguments_from_variant(name, arguments_variant);
                if (!arguments)
                    return {};

                if (name == FIELD_NAME_HIDDEN_SSID_UTF8 || name == FIELD_NAME_HIDDEN_SSID) {
                    credentials.ssid = argument_lookup(*arguments, FIELD_ARGUMENT_VALUE, "");

                } else if (name == FIELD_NAME_EAP_USERNAME || name == FIELD_NAME_WISPR_USERNAME) {
                    if (credentials.username) {
                        g_warning("Received ConnMan agent fields with both %s and %s",
                                  FIELD_NAME_EAP_USERNAME,
                                  FIELD_NAME_WISPR_USERNAME);
                        return {};
                    }
                    credentials.username = argument_lookup(*arguments, FIELD_ARGUMENT_VALUE, "");

                } else if (name == FIELD_NAME_PASSPHRASE || name == FIELD_NAME_WISPR_PASSWORD) {
                    if (credentials.password) {
                        g_warning("Received ConnMan agent fields with both %s and %s",
                                  FIELD_NAME_PASSPHRASE,
                                  FIELD_NAME_WISPR_PASSWORD);
                        return {};
                    }
                    credentials.password = arguments_to_password(name, *arguments);
                    if (!credentials.password)
                        return {};

                } else if (name == FIELD_NAME_PREVIOUS_PASSPHRASE) {
                    previous_password = arguments_to_password(name, *arguments);
                    if (!previous_password)
                        return {};

                } else if (name == FIELD_NAME_WPS) {
                    credentials.password_alternative = arguments_to_password(name, *arguments);

                    if (!credentials.password_alternative ||
                        credentials.password_alternative->type != Password::Type::WPS_PIN) {
                        g_warning("Received ConnMan agent WPS field with wrong type");
                        return {};
                    }

                } else {
                    g_warning("Received unknown ConnMan agent field \"%s\"", name.c_str());
                    return {};
                }
            }

            if (credentials.password_alternative) {
                if (!credentials.password) {
                    g_warning("Received ConnMan agent fields with password alternative field and "
                              "no password field");
                    return {};
                }

                if (credentials.password->type == credentials.password_alternative->type) {
                    g_warning("Received ConnMan agent fields with password and password "
                              "alternative of same type");
                    return {};
                }
            }

            if (previous_password) {
                if (!credentials.password) {
                    g_warning("Received ConnMan agent fields with previous password field and no "
                              "password field");
                    return {};
                }

                if (credentials.password->type == previous_password->type) {
                    if (credentials.password->value.empty())
                        credentials.password->value = previous_password->value;

                } else if (credentials.password_alternative &&
                           credentials.password_alternative->type == previous_password->type) {
                    if (credentials.password_alternative->value.empty())
                        credentials.password->value = previous_password->value;
                }
            }

            return credentials;
        }

        // TODO: Just like for received_fields_to_credentials(), a lot of the cases are not tested.
        //
        // Perhaps SSID handling should be simplified as well to always send SSID as byte array. Do
        // not understand why ConnMan does not always take it as byte array. Do not think D-Bus
        // allows non valid UTF-8 strings to be sent over the bus (understandable), but that coupled
        // with ConnMan being able to request both UTF-8 and non UTF-8 makes it... more complicated
        // than it needs to be. *sigh*
        Fields credentials_to_reply_fields(const Credentials &credentials,
                                           const Fields &received_fields)
        {
            Fields fields;

            auto add_string_reply = [&fields](const Glib::ustring &name,
                                              const Glib::ustring &value) {
                fields.emplace(name, Glib::Variant<Glib::ustring>::create(value));
            };

            auto add_byte_array_reply = [&fields](const Glib::ustring &name,
                                                  const std::vector<std::uint8_t> &value) {
                fields.emplace(name, Glib::Variant<std::vector<std::uint8_t>>::create(value));
            };

            auto was_requested = [&received_fields](const Glib::ustring &name) {
                return received_fields.find(name) != received_fields.cend();
            };

            if (credentials.ssid) {
                Glib::ustring ssid_utf8 = *credentials.ssid;
                bool utf8_was_requested = was_requested(FIELD_NAME_HIDDEN_SSID_UTF8);
                bool non_utf8_was_requested = was_requested(FIELD_NAME_HIDDEN_SSID);

                if (ssid_utf8.validate() && utf8_was_requested) {
                    add_string_reply(FIELD_NAME_HIDDEN_SSID_UTF8, ssid_utf8);
                } else if (non_utf8_was_requested) {
                    add_byte_array_reply(FIELD_NAME_HIDDEN_SSID,
                                         std::vector<std::uint8_t>(credentials.ssid->begin(),
                                                                   credentials.ssid->end()));
                } else if (utf8_was_requested) {
                    add_string_reply(FIELD_NAME_HIDDEN_SSID_UTF8,
                                     Common::string_to_valid_utf8(*credentials.ssid));
                }
            }

            if (credentials.username) {
                if (was_requested(FIELD_NAME_EAP_USERNAME))
                    add_string_reply(FIELD_NAME_EAP_USERNAME, *credentials.username);
                else if (was_requested(FIELD_NAME_WISPR_USERNAME))
                    add_string_reply(FIELD_NAME_WISPR_USERNAME, *credentials.username);
            }

            if (credentials.password) {
                bool wps_reply = credentials.password->type == Password::Type::WPS_PIN &&
                                 was_requested(FIELD_NAME_WPS);
                if (wps_reply)
                    add_string_reply(FIELD_NAME_WPS, credentials.password->value);
                else if (was_requested(FIELD_NAME_PASSPHRASE))
                    add_string_reply(FIELD_NAME_PASSPHRASE, credentials.password->value);
                else if (was_requested(FIELD_NAME_WISPR_PASSWORD))
                    add_string_reply(FIELD_NAME_WISPR_PASSWORD, credentials.password->value);
            }

            return fields;
        }
    }

    ConnManAgent::ConnManAgent(Listener &listener) : listener_(listener)
    {
    }

    Glib::ustring ConnManAgent::object_path() const
    {
        return Glib::ustring(Common::DBus::MANAGER_OBJECT_PATH) + "/ConnManAgent";
    }

    bool ConnManAgent::register_object(const Glib::RefPtr<Gio::DBus::Connection> &connection)
    {
        return AgentStub::register_object(connection, object_path()) != 0;
    }

    void ConnManAgent::Release(MethodInvocation &invocation)
    {
        state_ = State::NOT_REGISTERED_WITH_MANAGER;
        invocation.ret();

        listener_.agent_released();
    }

    void ConnManAgent::ReportError(const Glib::DBusObjectPathString & /*service*/,
                                   const Glib::ustring & /*error*/,
                                   MethodInvocation &invocation)
    {
        // TODO: Documentation says "This method gets called when an error has to be reported to the
        // user." but think it is OK to not do anything here. Connect() method on service will
        // return error if connecting fails. Test by logging here and see if expected behavior is
        // observed when connecting a service fails.
        invocation.ret();
    }

    void ConnManAgent::RequestBrowser(const Glib::DBusObjectPathString & /*service*/,
                                      const Glib::ustring & /*url*/,
                                      MethodInvocation &invocation)
    {
        invocation.ret(Gio::DBus::Error(Gio::DBus::Error::NOT_SUPPORTED,
                                        "RequestBrowser not implemented yet"));
    }

    void ConnManAgent::RequestInput(const Glib::DBusObjectPathString &service,
                                    const Fields &fields,
                                    MethodInvocation &invocation)
    {
        auto credentials = received_fields_to_credentials(fields);
        if (!credentials) {
            invocation.ret(Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS,
                                            "Could not parse fields argument"));
            return;
        }

        Backend::RequestCredentialsFromUserReply reply_callback =
            [fields, invocation](const std::optional<Common::Credentials> &result) mutable {
                if (result)
                    invocation.ret(credentials_to_reply_fields(*result, fields));
                else
                    invocation.ret(Gio::DBus::Error(Gio::DBus::Error::FAILED,
                                                    "Failed to request credentials"));
            };

        listener_.agent_request_input(service, std::move(*credentials), std::move(reply_callback));
    }

    void ConnManAgent::Cancel(MethodInvocation &invocation)
    {
        // TODO: Verify that it is OK to do nothing here. ConnMan canceling an agent request should
        // lead to service Connect() call failing which should lead to pending RequestInput method
        // invocation being returned. Test by logging here and see if expected behavior is observed
        // (should be called when ConnMan's D-Bus call to agent times out, for instance).
        //
        // Doubt about behavior here is what has lead to convoluted ConnManConnectQueue
        // implementation (only a single request at a time, see TODO about lifting this restriction
        // in ConnMannConnectQueue if the assumption that nothing needs to be done here is correct).
        invocation.ret();
    }
}
