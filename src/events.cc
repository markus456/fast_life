#include "events.hh"

namespace
{
    EventGenerator s_event_generator;
}

// static
void EventGenerator::handle_event(const SDL_Event &event)
{
    for (auto [key, func] : s_event_generator.m_listeners[event.type])
    {
        func(event);
    }
}

void EventGenerator::add(void *instance, uint32_t event, EventHandler handler)
{
    s_event_generator.m_listeners[event].emplace(instance, handler);
}

void EventGenerator::remove(void *instance)
{
    for (auto &[event, handlers] : s_event_generator.m_listeners)
    {
        handlers.erase(instance);
    }
}

void EventGenerator::remove(void *instance, uint32_t event)
{
    s_event_generator.m_listeners[event].erase(instance);
}
