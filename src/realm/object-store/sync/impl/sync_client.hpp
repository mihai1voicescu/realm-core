////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#ifndef REALM_OS_SYNC_CLIENT_HPP
#define REALM_OS_SYNC_CLIENT_HPP

#include <realm/sync/client.hpp>
#include <realm/sync/client_base.hpp>
#include <realm/sync/network/default_socket.hpp>
#include <realm/util/platform_info.hpp>
#include <realm/util/random.hpp>
#include <realm/util/scope_exit.hpp>

#include <thread>

#include <realm/object-store/sync/sync_manager.hpp>
#include <realm/object-store/sync/impl/network_reachability.hpp>

#if NETWORK_REACHABILITY_AVAILABLE
#include <realm/object-store/sync/impl/apple/network_reachability_observer.hpp>
#endif

#ifdef __EMSCRIPTEN__
#include <realm/object-store/sync/impl/emscripten/socket_provider.hpp>
#endif

namespace realm {
namespace _impl {

struct SyncClient {
    SyncClient(const std::shared_ptr<util::Logger>& logger, SyncClientConfig const& config,
               std::weak_ptr<const SyncManager> weak_sync_manager)
        : m_socket_provider([&]() -> std::shared_ptr<sync::SyncSocketProvider> {
            if (config.socket_provider) {
                return config.socket_provider;
            }
#ifdef __EMSCRIPTEN__
            return std::make_shared<EmscriptenSocketProvider>();
#else
            auto user_agent = util::format("RealmSync/%1 (%2) %3 %4", REALM_VERSION_STRING, util::get_platform_info(),
                                           config.user_agent_binding_info, config.user_agent_application_info);
            return std::make_shared<sync::websocket::DefaultSocketProvider>(
                logger, std::move(user_agent), config.default_socket_provider_thread_observer);
#endif
        }())
        , m_random{}
        , m_client(
              [&] {
                  sync::Client::Config c;
                  c.logger = logger;
                  c.socket_provider = m_socket_provider;
                  c.reconnect_mode = config.reconnect_mode;
                  c.one_connection_per_session = !config.multiplex_sessions;

                  // Only set the timeouts if they have sensible values
                  if (config.timeouts.connect_timeout >= 1000)
                      c.connect_timeout = config.timeouts.connect_timeout;
                  if (config.timeouts.connection_linger_time > 0)
                      c.connection_linger_time = config.timeouts.connection_linger_time;
                  if (config.timeouts.ping_keepalive_period > 5000)
                      c.ping_keepalive_period = config.timeouts.ping_keepalive_period;
                  if (config.timeouts.pong_keepalive_timeout > 5000)
                      c.pong_keepalive_timeout = config.timeouts.pong_keepalive_timeout;
                  if (config.timeouts.fast_reconnect_limit > 1000)
                      c.fast_reconnect_limit = config.timeouts.fast_reconnect_limit;
                  if (config.timeouts.resumption_delay_interval > 1000)
                      c.reconnect_backoff_info.resumption_delay_interval =
                          std::chrono::milliseconds(config.timeouts.resumption_delay_interval);
                  if (config.timeouts.max_resumption_delay_interval > 30000)
                      c.reconnect_backoff_info.max_resumption_delay_interval =
                          std::chrono::milliseconds(config.timeouts.max_resumption_delay_interval);
                  if (config.timeouts.resumption_delay_backoff_multiplier > 0)
                      c.reconnect_backoff_info.resumption_delay_backoff_multiplier =
                          config.timeouts.resumption_delay_backoff_multiplier;
                  if (config.timeouts.resumption_delay_jitter_divisor >= 0)
                      c.reconnect_backoff_info.delay_jitter_divisor = config.timeouts.resumption_delay_jitter_divisor;

                  return c;
              }(),
              m_random)
        , m_logger_ptr(logger)
        , m_logger(*m_logger_ptr)
#if NETWORK_REACHABILITY_AVAILABLE
        , m_reachability_observer(none, [weak_sync_manager](const NetworkReachabilityStatus status) {
            if (status != NotReachable) {
                if (auto sync_manager = weak_sync_manager.lock()) {
                    sync_manager->reconnect();
                }
            }
        })
    {
        // FIXME: Would be better if seeding was up to the application.
        util::seed_prng_nondeterministically(m_random); // Throws

        if (!m_reachability_observer.start_observing())
            m_logger.error("Failed to set up network reachability observer");
    }
#else
    {
        static_cast<void>(weak_sync_manager);
    }
#endif

    void cancel_reconnect_delay()
    {
        m_client.cancel_reconnect_delay();
    }

    void stop()
    {
        m_client.shutdown();
    }

    void voluntary_disconnect_all_connections()
    {
        m_client.voluntary_disconnect_all_connections();
    }

    std::unique_ptr<sync::Session> make_session(std::shared_ptr<DB> db,
                                                std::shared_ptr<sync::SubscriptionStore> flx_sub_store,
                                                std::shared_ptr<sync::MigrationStore> migration_store,
                                                sync::Session::Config config)
    {
        return std::make_unique<sync::Session>(m_client, std::move(db), std::move(flx_sub_store),
                                               std::move(migration_store), std::move(config));
    }

    bool decompose_server_url(const std::string& url, sync::ProtocolEnvelope& protocol, std::string& address,
                              sync::Client::port_type& port, std::string& path) const
    {
        return m_client.decompose_server_url(url, protocol, address, port, path);
    }

    void wait_for_session_terminations()
    {
        m_client.wait_for_session_terminations_or_client_stopped();
    }

    // Async version of wait_for_session_terminations().
    util::Future<void> notify_session_terminated()
    {
        return m_client.notify_session_terminated();
    }

    sync::SyncSocketProvider::SyncTimer create_timer(std::chrono::milliseconds delay,
                                                     sync::SyncSocketProvider::FunctionHandler&& handler)
    {
        REALM_ASSERT(m_socket_provider);
        return m_socket_provider->create_timer(delay, std::move(handler));
    }

    void post(sync::SyncSocketProvider::FunctionHandler&& handler)
    {
        REALM_ASSERT(m_socket_provider);
        m_socket_provider->post(std::move(handler));
    }

    sync::RandomEngine& get_random()
    {
        return m_random;
    }

    ~SyncClient() {}

private:
    std::shared_ptr<sync::SyncSocketProvider> m_socket_provider;
    sync::RandomEngine m_random;
    sync::Client m_client;
    std::shared_ptr<util::Logger> m_logger_ptr;
    util::Logger& m_logger;
#if NETWORK_REACHABILITY_AVAILABLE
    NetworkReachabilityObserver m_reachability_observer;
#endif
};

} // namespace _impl
} // namespace realm

#endif // REALM_OS_SYNC_CLIENT_HPP
