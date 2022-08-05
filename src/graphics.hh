#pragma once

#include "common.hh"
#include "objects.hh"

struct Color
{
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t alpha = 255;
};

static constexpr const Color COLOR_RED = {255, 0, 0};
static constexpr const Color COLOR_GREEN = {0, 255, 0};
static constexpr const Color COLOR_BLUE = {0, 0, 255};
static constexpr const Color COLOR_MAGENTA = {255, 0, 255};
static constexpr const Color COLOR_WHITE = {255, 255, 255};
static constexpr const Color COLOR_BLACK = {0, 0, 0};
static constexpr const Color COLOR_GRAY = {125, 125, 125};

// A graphical element that can be rendered
struct Renderable
{
    virtual void render(SDL_Renderer *renderer) = 0;
};

class Text : public Renderable
{
public:
    static void init();
    static void finish();

    Text(SDL_Renderer *renderer);
    Text(SDL_Renderer *renderer, const std::string *variable);

    ~Text();

    void set_font(std::string font, Color color, int size);
    void set_text(std::string text);
    void set_centered(bool enabled);

    // Position is set as the upper left corner
    void set_position(Point point);

    Point get_position() const;

    void render(SDL_Renderer *renderer) override;

private:
    void do_set_text(std::string text);

    SDL_Texture *m_texture{nullptr};
    SDL_Renderer *m_renderer;
    SDL_Rect m_rect;
    TTF_Font *m_font{nullptr};
    SDL_Color m_color;
    bool m_centered{false};
    const std::string *m_var{nullptr};
    std::string m_prev;
    std::string m_text;
};