#include <atomic>
#include <barrier>
#include <chrono>
#include <cassert>
#include <iostream>
#include <thread>
#include <exception>
#include <vector>
#include <algorithm>
#include <sstream>
#include <random>
#include <shared_mutex>

#include "common.hh"
#include "graphics.hh"
#include "objects.hh"
#include "events.hh"

using namespace std;
using chrono::duration_cast;
using chrono::microseconds;
using chrono::milliseconds;

using Clock = chrono::steady_clock;

static constexpr int WINDOW_WIDTH = 800;
static constexpr int WINDOW_HEIGHT = 600;
static constexpr int FRAMERATE = 120;

static const std::string FONT_NAME = "fonts/pixeldroidMenuRegular.ttf";
static const Color FONT_COLOR = COLOR_WHITE;
static const int FONT_SIZE = 22;

const int X_OFFSET = 240;
const int Y_OFFSET = 20;

const int X_PAD = 0;
const int Y_PAD = 0;
const int OBJ_SIZE = 4;
const int THREADS = std::thread::hardware_concurrency();

struct Lifeform
{
    Lifeform(bool value)
        : current(value), next(false)
    {
    }

    bool current;
    bool next;
};

class Game
{
public:
    Game(int height, int width)
        : m_next_state_barrier(THREADS),
          m_update_barrier(THREADS),
          m_tick_barrier(THREADS + 1),
          m_height(height),
          m_width(width)
    {
        std::random_device rnd;
        std::mt19937 gen(rnd());
        std::uniform_int_distribution flip(0, 1);

        for (int y = 0; y < m_height; y++)
        {
            for (int x = 0; x < m_width; x++)
            {
                m_obj.emplace_back(flip(gen));
            }
        }

        int y_size = m_height / THREADS;

        for (int i = 0; i < THREADS; i++)
        {
            int y_start = i * y_size;
            int y_end = y_start + y_size;

            if (i == THREADS - 1)
            {
                y_end += m_height % THREADS;
                assert(y_end == m_height);
            }

            m_threads.emplace_back(&Game::update_thr, this, y_start, y_end);
        }
    }

    ~Game()
    {
        m_thr_running = false;
        m_tick_barrier.arrive_and_drop();

        for (auto &t : m_threads)
        {
            t.join();
        }

        m_threads.clear();

        m_obj.clear();
    }

    bool alive_at(int x, int y) const
    {
        if (x == -1)
        {
            x = m_width - 1;
        }
        else if (x == m_width)
        {
            x = 0;
        }

        if (y == -1)
        {
            y = m_height - 1;
        }
        else if (y == m_height)
        {
            y = 0;
        }

        return m_obj[y * m_width + x].current;
    }

    auto at(int x, int y) const
    {
        return m_obj[y * m_width + x];
    }

    void tick()
    {
        m_tick_barrier.arrive_and_wait();
    }

    void set_alive_at(int x, int y)
    {
        assert(x >= 0 && x < m_width);
        assert(y >= 0 && y < m_height);
        int num = 0;
        num += alive_at(x - 1, y - 1);
        num += alive_at(x, y - 1);
        num += alive_at(x + 1, y - 1);
        num += alive_at(x + 1, y);
        num += alive_at(x + 1, y + 1);
        num += alive_at(x, y + 1);
        num += alive_at(x - 1, y + 1);
        num += alive_at(x - 1, y);

        auto &o = m_obj[y * m_width + x];

        if (o.current)
        {
            if (num != 3 && num != 2)
            {
                o.next = false;
            }
        }
        else if (num == 3)
        {
            o.next = true;
        }
    }

    void calculate_next_state(int y_start, int y_end)
    {
        for (int y = y_start; y < y_end; y++)
        {
            for (int x = 0; x < m_width; x++)
            {
                set_alive_at(x, y);
            }
        }
    }

    void update_state(int y_start, int y_end)
    {
        for (int y = y_start; y < y_end; y++)
        {
            for (int x = 0; x < m_width; x++)
            {
                auto &o = m_obj[y * m_width + x];
                o.current = o.next;
            }
        }
    }

    void update_thr(int y_start, int y_end)
    {
        bool running = true;

        while (running)
        {
            auto now = Clock::now();
            m_next_state_barrier.arrive_and_wait();
            calculate_next_state(y_start, y_end);
            m_update_barrier.arrive_and_wait();
            update_state(y_start, y_end);
            m_tick_barrier.arrive_and_wait();

            running = m_thr_running.load(std::memory_order_relaxed);

            if (!running)
            {
                m_next_state_barrier.arrive_and_drop();
                m_update_barrier.arrive_and_drop();
                m_tick_barrier.arrive_and_wait();
            }
        }
    }

private:
    int m_height;
    int m_width;
    std::vector<Lifeform> m_obj;

    std::vector<std::thread> m_threads;
    std::atomic<bool> m_thr_running{true};

    std::barrier<> m_next_state_barrier;
    std::barrier<> m_update_barrier;
    std::barrier<> m_tick_barrier;
};

class Program
{
public:
    Program()
    {
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        {
            throw Error("SDL init failed");
        }

        Text::init();

        m_window = SDL_CreateWindow("Fast Life", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

        if (!m_window)
        {
            throw Error("Window init failed");
        }

        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);

        if (!m_renderer)
        {
            throw Error("Renderer init failed");
        }

        SDL_RenderGetViewport(m_renderer, &m_camera);

        add_text("1: Increase width");
        add_text("2: Decrease width");
        add_text("3: Increase height");
        add_text("4: Decrease height");
        add_text("Click: set width and height");
        add_text("c: Increase speed");
        add_text("z: Decrease speed");
        add_text("b: Increase tile size");
        add_text("v: Decrease tile size");
        add_text("r: Randomize colors");
        add_text("x: Reinitialize game");
        add_text("Esc: Exit game");

        add_variable_text("Width: ", &m_width_str);
        add_variable_text("Height: ", &m_height_str);
        add_variable_text("Speed: ", &m_speed_str);
        add_variable_text("Size: ", &m_size_str);

        EventGenerator::add(this, SDL_QUIT, [this](const auto &event)
                            { m_running = false; });

        EventGenerator::add(this, SDL_MOUSEMOTION, [this](const auto &event)
                            { on_mouse_move(event); });

        EventGenerator::add(this, SDL_MOUSEWHEEL, [this](const auto &event)
                            { on_mouse_wheel(event); });

        EventGenerator::add(this, SDL_KEYDOWN, [this](const auto &event)
                            { on_keydown(event); });

        EventGenerator::add(this, SDL_MOUSEBUTTONUP, [this](const auto &event)
                            { on_mousebuttonup(event); });
    }

    ~Program()
    {
        // Stop processing and clear out objects before stopping SDL
        stop();
        m_labels.clear();
        SDL_DestroyTexture(m_texture);

        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        Text::finish();
        SDL_Quit();
    }

    void run()
    {
        auto next_render = Clock::now();
        auto next_update = next_render;

        while (m_running)
        {
            const milliseconds render_tick{1000 / FRAMERATE};
            const milliseconds update_tick{1000 / m_speed};
            auto now = Clock::now();

            poll_event();

            if (m_game && now >= next_update)
            {
                m_game->tick();
                next_update = now + update_tick;
            }

            if (now >= next_render)
            {
                render();
                next_render = now + render_tick;
            }

            auto next_tick = std::min(next_render, next_update);

            if (next_tick > now)
            {
                this_thread::sleep_until(next_tick);
            }
        }
    }

private:
    void stop()
    {
        m_game.reset();
    }

    void reinitialize()
    {
        m_game = std::make_unique<Game>(m_height, m_width);
        SDL_DestroyTexture(m_texture);
        m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
    }

    void clear_all_text()
    {
        m_labels.clear();
    }

    void add_text(std::string text)
    {
        add_variable_text(text, nullptr);
    }

    void add_variable_text(std::string text, const std::string *variable)
    {
        double x = 0;
        double y = m_labels.empty() ? 0 : m_labels.back()->get_position().y + FONT_SIZE;
        m_labels.push_back(std::make_unique<Text>(m_renderer, variable));
        m_labels.back()->set_font(FONT_NAME, FONT_COLOR, FONT_SIZE);
        m_labels.back()->set_position({x, y});
        m_labels.back()->set_text(text);
    }

    void on_mouse_move(const SDL_Event &event)
    {
        m_mouse.x = event.motion.x;
        m_mouse.y = event.motion.y;
    }

    void on_mouse_wheel(const SDL_Event &event)
    {
    }

    void on_keydown(const SDL_Event &event)
    {
        switch (event.key.keysym.sym)
        {
        case SDLK_LEFT:
            m_camera.x -= 1;
            break;
        case SDLK_RIGHT:
            m_camera.x += 1;
            break;
        case SDLK_UP:
            m_camera.y += 1;
            break;
        case SDLK_DOWN:
            m_camera.y -= 1;
            break;

        case SDLK_x:
            reinitialize();
            break;

        case SDLK_c:
            m_speed++;
            break;

        case SDLK_z:
            if (m_speed > 1)
            {
                m_speed--;
            }
            break;

        case SDLK_r:
            m_alive_color = rand() % 256;
            m_dead_color = rand() % 256;
            break;

        case SDLK_b:
            m_size++;
            stop();
            break;

        case SDLK_v:
            if (m_size > 1)
            {
                m_size--;
            }
            stop();
            break;

        case SDLK_1:
            m_width += 5;
            stop();
            break;

        case SDLK_2:
            if (m_width > 5)
            {
                m_width -= 5;
                stop();
            }
            break;

        case SDLK_3:
            m_height += 5;
            stop();
            break;

        case SDLK_4:
            if (m_height > 5)
            {
                m_height -= 5;
                stop();
            }
            break;

        case SDLK_ESCAPE:
            m_running = false;
            break;
        }

        m_width_str = std::to_string(m_width);
        m_height_str = std::to_string(m_height);
        m_speed_str = std::to_string(m_speed);
        m_size_str = std::to_string(m_size);
    }

    void on_mousebuttonup(const SDL_Event &event)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            int x = m_mouse.x - X_OFFSET;
            int y = m_mouse.y - Y_OFFSET;
            int w = x / m_size;
            int h = y / m_size;

            if (h > 0 && w > 0)
            {
                m_height = h;
                m_width = w;
                stop();
            }
        }
    }

    void poll_event()
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            EventGenerator::handle_event(event);
        }
    }

    void render()
    {
        SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
        SDL_RenderClear(m_renderer);

        if (m_game)
        {
            m_alive.clear();
            m_dead.clear();

            void *pixels;
            int pitch;
            SDL_LockTexture(m_texture, nullptr, &pixels, &pitch);
            uint8_t *ptr = (uint8_t *)pixels;

            for (int y = 0; y < m_height; y++)
            {
                for (int x = 0; x < m_width; x++)
                {
                    ptr[y * pitch + x] = m_game->at(x, y).current ? m_alive_color : m_dead_color;
                }
            }

            SDL_UnlockTexture(m_texture);

            SDL_Rect rect{X_OFFSET, Y_OFFSET, (m_size + X_PAD) * m_width, (m_size + Y_PAD) * m_height};
            SDL_RenderCopyEx(m_renderer, m_texture, nullptr, &m_camera, 0, nullptr, SDL_FLIP_NONE);
        }

        for (const auto &l : m_labels)
        {
            l->render(m_renderer);
        }

        m_camera.x = X_OFFSET;
        m_camera.y = Y_OFFSET;
        m_camera.w = m_width * (m_size + X_PAD) - X_PAD;
        m_camera.h = m_height * (m_size + Y_PAD) - Y_PAD;

        SDL_SetRenderDrawColor(m_renderer, 0, 0, 250, 255);
        SDL_RenderDrawRect(m_renderer, &m_camera);

        SDL_RenderPresent(m_renderer);
    }

private:
    SDL_Window *m_window{nullptr};
    SDL_Renderer *m_renderer{nullptr};
    SDL_Texture *m_texture{nullptr};
    SDL_Rect m_camera;
    bool m_running{true};

    int m_size = OBJ_SIZE;
    int m_speed = 121;
    int m_width = 210;
    int m_height = 120;
    uint8_t m_alive_color = 0x00;
    uint8_t m_dead_color = 0xff;
    std::string m_size_str;
    std::string m_speed_str;
    std::string m_width_str;
    std::string m_height_str;

    std::vector<SDL_Rect> m_alive;
    std::vector<SDL_Rect> m_dead;

    Point m_mouse;
    std::vector<std::unique_ptr<Text>> m_labels;

    std::unique_ptr<Game> m_game;
};

int main(int argc, char **argv)
{
    try
    {
        Program program;
        program.run();
    }
    catch (runtime_error err)
    {
        cout << err.what() << endl;
    }

    return 0;
}