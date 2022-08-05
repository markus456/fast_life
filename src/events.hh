#pragma once

#include "common.hh"

#include <map>
#include <set>
#include <cassert>
#include <functional>

using EventHandler = std::function<void(const SDL_Event &)>;

class EventGenerator
{
public:
    static void handle_event(const SDL_Event &event);

    static void add(void *instance, uint32_t event, EventHandler handler);

    static void remove(void *instance);

    static void remove(void *instance, uint32_t event);

private:
    std::map<uint32_t, std::map<void *, EventHandler>> m_listeners;
};

template <class Derived>
class EventListener
{
public:
    virtual ~EventListener()
    {
        EventGenerator::remove(this);
    }

    void listen(uint32_t event, void (Derived::*fnc)(const SDL_Event &))
    {
        EventHandler handler = [=](const auto &event)
        {
            (static_cast<Derived *>(this)->*fnc)(event);
        };

        EventGenerator::add(this, event, handler);
    }

    void stop_listening(uint32_t event)
    {
        EventGenerator::remove(this, event);
    }
};