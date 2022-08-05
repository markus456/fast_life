#include "graphics.hh"

#include <cassert>
#include <unordered_map>

//
// Text
//

class FontLoader
{
public:
    ~FontLoader()
    {
        for (const auto &kv : m_fonts)
        {
            TTF_CloseFont(kv.second);
        }
    }

    TTF_Font *load(std::string filename, int size)
    {
        std::string font_name = filename + '-' + std::to_string(size);
        auto it = m_fonts.find(font_name);

        if (it == m_fonts.end())
        {
            auto font = TTF_OpenFont(filename.c_str(), size);

            if (font)
            {
                it = m_fonts.emplace(font_name, font).first;
            }
            else
            {
                throw Error("Could not load font " + font_name + ": " + std::string(TTF_GetError()));
            }
        }

        return it->second;
    }

private:
    std::unordered_map<std::string, TTF_Font *> m_fonts;
};

static std::unique_ptr<FontLoader> loader;

// static
void Text::init()
{
    if (TTF_Init() != 0)
    {
        throw Error("TTF_Init failed");
    }

    loader = std::make_unique<FontLoader>();
}

void Text::finish()
{
    loader.reset();
    TTF_Quit();
}

Text::Text(SDL_Renderer *renderer)
    : Text(renderer, nullptr)
{
}

Text::Text(SDL_Renderer *renderer, const std::string *variable)
    : m_renderer(renderer), m_var(variable)
{
}

void Text::set_font(std::string font, Color color, int size)
{
    m_font = loader->load(font, size);
    m_color = SDL_Color{color.red, color.green, color.blue, color.alpha};
}

void Text::set_text(std::string text)
{
    m_text = text;
    do_set_text(m_text);
}

void Text::do_set_text(std::string text)
{
    if (m_font)
    {
        SDL_Surface *surface = TTF_RenderText_Solid(m_font, text.c_str(), m_color);
        m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);
        m_rect.w = surface->w;
        m_rect.h = surface->h;
        SDL_FreeSurface(surface);
    }
}

void Text::set_centered(bool enabled)
{
    if (!m_centered && enabled)
    {
        m_rect.x -= m_rect.w / 2;
        m_rect.y -= m_rect.h / 2;
    }
    else if (m_centered && !enabled)
    {
        m_rect.x += m_rect.w / 2;
        m_rect.y += m_rect.h / 2;
    }

    m_centered = enabled;
}

void Text::set_position(Point point)
{
    m_rect.x = point.x;
    m_rect.y = point.y;

    if (m_centered)
    {
        m_rect.x -= m_rect.w / 2;
        m_rect.y -= m_rect.h / 2;
    }
}

Point Text::get_position() const
{
    return {(double)m_rect.x, (double)m_rect.y};
}

void Text::render(SDL_Renderer *renderer)
{
    if (m_var && m_prev != *m_var)
    {
        m_prev = *m_var;
        do_set_text(m_text + m_prev);
    }

    SDL_RenderCopyEx(renderer, m_texture, nullptr, &m_rect, 0, nullptr, SDL_FLIP_NONE);
}

Text::~Text()
{
    SDL_DestroyTexture(m_texture);
}
