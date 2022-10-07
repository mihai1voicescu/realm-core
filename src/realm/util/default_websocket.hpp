#ifndef REALM_UTIL_DEFAULT_WEBSOCKET_HPP
#define REALM_UTIL_DEFAULT_WEBSOCKET_HPP

#include <chrono>
#include <map>
#include <memory>
#include <random>
#include <system_error>
#include <thread>
#include <utility>

#include <realm/object-store/binding_callback_thread_observer.hpp>

#include <realm/sync/config.hpp>
#include <realm/util/checked_mutex.hpp>
#include <realm/util/http.hpp>
#include <realm/util/network.hpp>
#include <realm/util/client_websocket.hpp>

namespace realm::util::network {
class Service;
}

namespace realm::util::websocket {

class DefaultEventLoopClient : public EventLoopClient {
public:
    enum State { NotStarted = 0, Running = 1, Stopped = 2 };

    class Timer : public EventLoopClient::Timer {
    public:
        Timer() = default;

        Timer(util::network::Service& service, std::chrono::milliseconds delay,
              util::UniqueFunction<void(std::error_code)>&& handler)
            : m_timer(std::make_unique<util::network::DeadlineTimer>(service))
        {
            m_timer->async_wait(delay, std::move(handler)); // Throws
        }

        ~Timer() noexcept
        {
            cancel();
        }

        static std::unique_ptr<EventLoopClient::Timer>
        async_wait(util::network::Service& service, std::chrono::milliseconds delay,
                   util::UniqueFunction<void(std::error_code)>&& handler) // Throws
        {
            return std::make_unique<DefaultEventLoopClient::Timer>(service, delay, std::move(handler));
        }


        void cancel() noexcept override
        {
            if (auto timer_ptr = std::move(m_timer); timer_ptr != nullptr) {
                timer_ptr->cancel();
            }
        }

    private:
        std::unique_ptr<util::network::DeadlineTimer> m_timer;
    };

    /// \brief The trigger object used to activate the trigger callback handler.
    ///
    /// This object provides a trigger mechanism to run a callback handler one or more times. The trigger
    /// will remain registered and can be called multiple times until the Trigger object is destroyed.
    class Trigger : public EventLoopClient::Trigger {
    public:
        Trigger() = default;

        Trigger(util::network::Service& service, util::UniqueFunction<void()>&& handler)
            : m_trigger(service, std::move(handler))
        {
        }

        virtual ~Trigger() noexcept = default;

        void trigger() noexcept override
        {
            m_trigger.trigger();
        }

    private:
        util::network::Trigger m_trigger;
    };

    DefaultEventLoopClient(const std::shared_ptr<util::Logger>& logger)
        : m_logger_ptr{logger}
        , m_logger{*m_logger_ptr}
        , m_service{}
        , m_state(State::NotStarted)
    {
        // Lazy start the service until start() is called or the first operation is performed
    }

    virtual ~DefaultEventLoopClient() REQUIRES(!m_mutex)
    {
        stop();
    }

    void post(util::UniqueFunction<void()>&& handler) REQUIRES(!m_mutex) override
    {
        REALM_ASSERT(ensure_service_is_running());
        m_service.post(std::move(handler));
    }

    std::unique_ptr<EventLoopClient::Trigger> create_trigger(util::UniqueFunction<void()>&& handler)
        REQUIRES(!m_mutex) override
    {
        REALM_ASSERT(ensure_service_is_running());
        return std::make_unique<DefaultEventLoopClient::Trigger>(m_service, std::move(handler));
    }

    bool is_running() REQUIRES(!m_mutex) override
    {
        util::CheckedLockGuard lock(m_mutex);
        return m_state;
    }

    void start() REQUIRES(!m_mutex) override
    {
        ensure_service_is_running();
    }

    void stop() REQUIRES(!m_mutex) override;

    util::network::Service& get_service()
    {
        return m_service;
    }

private:
    std::unique_ptr<EventLoopClient::Timer> do_create_timer(std::chrono::milliseconds delay,
                                                            util::UniqueFunction<void(std::error_code)>&& handler)
        REQUIRES(!m_mutex) override
    {
        REALM_ASSERT(ensure_service_is_running());
        return Timer::async_wait(m_service, delay, std::move(handler));
    }

    // If the service thread is not running, make sure it has been started
    bool ensure_service_is_running() REQUIRES(!m_mutex);

    //@{
    // Thread Helper Functions
    void thread_update_state(State new_state) REQUIRES(!m_mutex);
    void thread_start_service() REQUIRES(!m_mutex);
    //@}

    util::CheckedMutex m_mutex;
    std::shared_ptr<util::Logger> m_logger_ptr;
    util::Logger& m_logger;
    // The original util::network::Service object that used to live in client_impl
    util::network::Service m_service;
    // The event loop thread that calls Service->run()
    std::unique_ptr<std::thread> m_thread GUARDED_BY(m_mutex);
    // The event loop can only be started once, it cannot be restarted later
    State m_state GUARDED_BY(m_mutex);
};

class DefaultWebSocketFactory : public WebSocketFactory {
public:
    DefaultWebSocketFactory(const std::string& user_agent_string, const std::shared_ptr<util::Logger>& logger)
        : WebSocketFactory(user_agent_string)
        , m_logger_ptr(logger)
    {
    }

    virtual ~DefaultWebSocketFactory()
    {
        if (m_event_loop != nullptr) {
            m_event_loop->stop();
        }
    }

    std::shared_ptr<EventLoopClient> create_event_loop() override
    {
        m_event_loop = std::make_shared<DefaultEventLoopClient>(m_logger_ptr);
        return m_event_loop;
    }

    DefaultWebSocketFactory(DefaultWebSocketFactory&&) = delete;

    std::unique_ptr<WebSocket> connect(WebSocketObserver* observer, Endpoint&& endpoint) override;

private:
    std::shared_ptr<util::Logger> m_logger_ptr;
    std::shared_ptr<DefaultEventLoopClient> m_event_loop;
};

} // namespace realm::util::websocket

#endif // REALM_UTIL_DEFAULT_WEBSOCKET_HPP
