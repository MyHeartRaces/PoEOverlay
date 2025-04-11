#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <any>

namespace poe {

// Forward declarations
class Application;

/**
 * @class Event
 * @brief Base class for all events in the system.
 */
class Event {
public:
    /**
     * @brief Virtual destructor for proper polymorphic destruction.
     */
    virtual ~Event() = default;

    /**
     * @brief Gets the type name of the event.
     * @return The event type name.
     */
    virtual std::string GetTypeName() const = 0;

    /**
     * @brief Gets a string representation of the event for debugging.
     * @return String representation of the event.
     */
    virtual std::string ToString() const { return GetTypeName(); }
};

/**
 * @class EventHandler
 * @brief Handler for events with a specific signature.
 * @tparam EventType The type of event to handle.
 */
template<typename EventType>
using EventHandler = std::function<void(const EventType&)>;

/**
 * @class EventSystem
 * @brief Manages event subscriptions and dispatching.
 * 
 * This class implements a publish-subscribe pattern for events, allowing
 * components to subscribe to specific event types and receive notifications
 * when those events are published.
 */
class EventSystem {
public:
    /**
     * @brief Constructor for the EventSystem class.
     * @param app Reference to the main application instance.
     */
    explicit EventSystem(Application& app);

    /**
     * @brief Destructor for the EventSystem class.
     */
    ~EventSystem();

    /**
     * @brief Initializes the event system.
     * @return True if initialization was successful, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Shuts down the event system.
     */
    void Shutdown();

    /**
     * @brief Processes pending events in the queue.
     */
    void ProcessEvents();

    /**
     * @brief Publishes an event to subscribers.
     * @tparam EventType The type of event to publish.
     * @param event The event instance to publish.
     */
    template<typename EventType>
    void Publish(const EventType& event) {
        static_assert(std::is_base_of<Event, EventType>::value, "EventType must derive from Event");
        
        const auto typeName = event.GetTypeName();
        
        // Log the event
        m_app.GetLogger().Debug("Publishing event: {}", event.ToString());
        
        // Store event in queue for deferred processing
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_eventQueue.push_back(std::make_shared<EventType>(event));
        }
    }

    /**
     * @brief Immediately publishes an event to subscribers, bypassing the queue.
     * @tparam EventType The type of event to publish.
     * @param event The event instance to publish.
     */
    template<typename EventType>
    void PublishImmediate(const EventType& event) {
        static_assert(std::is_base_of<Event, EventType>::value, "EventType must derive from Event");
        
        const auto typeName = event.GetTypeName();
        
        // Log the event
        m_app.GetLogger().Debug("Publishing immediate event: {}", event.ToString());
        
        // Dispatch event immediately
        DispatchEvent<EventType>(event);
    }

    /**
     * @brief Subscribes to an event type.
     * @tparam EventType The type of event to subscribe to.
     * @param handler The handler function to call when the event is published.
     * @return A unique ID for the subscription, used for unsubscribing.
     */
    template<typename EventType>
    size_t Subscribe(const EventHandler<EventType>& handler) {
        static_assert(std::is_base_of<Event, EventType>::value, "EventType must derive from Event");
        
        const auto typeName = EventType().GetTypeName();
        
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        
        // Get or create handler vector for this event type
        auto& handlers = m_handlers[typeName];
        
        // Generate a unique handler ID
        size_t handlerId = m_nextHandlerId++;
        
        // Store handler with its ID
        handlers.push_back({handlerId, std::any(handler)});
        
        m_app.GetLogger().Debug("Subscribed to event: {} (Handler ID: {})", typeName, handlerId);
        
        return handlerId;
    }

    /**
     * @brief Unsubscribes from an event type using the handler ID.
     * @param handlerId The ID of the handler to unsubscribe.
     * @return True if a handler was found and removed, false otherwise.
     */
    bool Unsubscribe(size_t handlerId);

private:
    /**
     * @brief Internal structure for storing event handlers with their IDs.
     */
    struct HandlerEntry {
        size_t id;
        std::any handler;
    };

    /**
     * @brief Dispatches an event to its subscribers.
     * @tparam EventType The type of event to dispatch.
     * @param event The event instance to dispatch.
     */
    template<typename EventType>
    void DispatchEvent(const EventType& event) {
        const auto typeName = event.GetTypeName();
        
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        
        auto it = m_handlers.find(typeName);
        if (it == m_handlers.end()) {
            return; // No handlers for this event type
        }
        
        // Call each handler
        for (const auto& entry : it->second) {
            try {
                const auto& handler = std::any_cast<EventHandler<EventType>>(entry.handler);
                handler(event);
            }
            catch (const std::bad_any_cast&) {
                m_app.GetLogger().Error("Failed to cast event handler for type: {}", typeName);
            }
            catch (const std::exception& e) {
                m_app.GetLogger().Error("Exception in event handler for {}: {}", typeName, e.what());
            }
        }
    }

    /**
     * @brief Dispatches an event from the queue.
     * @param event The event to dispatch.
     */
    void DispatchQueuedEvent(const std::shared_ptr<Event>& event);

    /**
     * @brief Reference to the main application instance.
     */
    Application& m_app;

    /**
     * @brief Map of event type names to their handlers.
     */
    std::unordered_map<std::string, std::vector<HandlerEntry>> m_handlers;

    /**
     * @brief Mutex for thread-safe access to handlers.
     */
    std::mutex m_handlersMutex;

    /**
     * @brief Queue of pending events to be processed.
     */
    std::vector<std::shared_ptr<Event>> m_eventQueue;

    /**
     * @brief Mutex for thread-safe access to the event queue.
     */
    std::mutex m_queueMutex;

    /**
     * @brief Counter for generating unique handler IDs.
     */
    size_t m_nextHandlerId;
};

} // namespace poe