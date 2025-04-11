#include "core/EventSystem.h"
#include "core/Application.h"
#include "core/Logger.h"

namespace poe {

EventSystem::EventSystem(Application& app)
    : m_app(app)
    , m_nextHandlerId(1)
{
}

EventSystem::~EventSystem()
{
    Shutdown();
}

bool EventSystem::Initialize()
{
    m_app.GetLogger().Info("EventSystem initialized");
    return true;
}

void EventSystem::Shutdown()
{
    // Clear all event handlers
    std::lock_guard<std::mutex> lock1(m_handlersMutex);
    m_handlers.clear();

    // Clear event queue
    std::lock_guard<std::mutex> lock2(m_queueMutex);
    m_eventQueue.clear();

    m_app.GetLogger().Info("EventSystem shutdown");
}

void EventSystem::ProcessEvents()
{
    // Get all pending events
    std::vector<std::shared_ptr<Event>> pendingEvents;
    
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_eventQueue.empty()) {
            return; // No events to process
        }
        
        pendingEvents.swap(m_eventQueue);
    }
    
    // Process each event
    for (const auto& event : pendingEvents) {
        DispatchQueuedEvent(event);
    }
}

void EventSystem::DispatchQueuedEvent(const std::shared_ptr<Event>& event)
{
    if (!event) return;
    
    const auto typeName = event->GetTypeName();
    
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    
    auto it = m_handlers.find(typeName);
    if (it == m_handlers.end()) {
        return; // No handlers for this event type
    }
    
    m_app.GetLogger().Debug("Dispatching event: {}", event->ToString());
    
    // Call each handler
    for (const auto& entry : it->second) {
        try {
            // We need to perform a dynamic dispatch here since we don't know
            // the concrete type of the event at compile time. For each registered
            // handler type, we'll try to cast and dispatch.
            
            // This is not ideal and could be improved with a more type-safe
            // approach, but it works for the basic event system.
            
            // This approach limits us to a predefined set of event types that
            // we know about at compile time. In a real system, you might want
            // to use a more flexible approach, possibly with a type registry.
            
            // For now, this stub just logs that we can't dispatch the event
            m_app.GetLogger().Warning("Event dispatch mechanism not fully implemented for type: {}", typeName);
        }
        catch (const std::bad_any_cast&) {
            m_app.GetLogger().Error("Failed to cast event handler for type: {}", typeName);
        }
        catch (const std::exception& e) {
            m_app.GetLogger().Error("Exception in event handler for {}: {}", typeName, e.what());
        }
    }
}

bool EventSystem::Unsubscribe(size_t handlerId)
{
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    
    // Search through all event types
    for (auto& [typeName, handlers] : m_handlers) {
        // Find and remove the handler with matching ID
        for (auto it = handlers.begin(); it != handlers.end(); ++it) {
            if (it->id == handlerId) {
                handlers.erase(it);
                m_app.GetLogger().Debug("Unsubscribed handler ID: {} from event: {}", handlerId, typeName);
                return true;
            }
        }
    }
    
    m_app.GetLogger().Warning("Failed to unsubscribe handler ID: {} (not found)", handlerId);
    return false;
}

} // namespace poe