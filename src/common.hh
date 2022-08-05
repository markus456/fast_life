#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_ttf.h>

#include <stdexcept>
#include <cstdint>
#include <string>

struct Error : public std::runtime_error
{
    Error(const std::string &message)
        : std::runtime_error(message.c_str())
    {
    }
};
