#include <SDL2/SDL.h>
#include <vector>
#include <deque>
#include <random>
#include <cstdlib>
#include <ctime>
#include <iostream>

static const int SCREEN_WIDTH = 640;
static const int SCREEN_HEIGHT = 480;
static const int GRID_SIZE = 20;
static const int GRID_COLS = SCREEN_WIDTH / GRID_SIZE;   // 32
static const int GRID_ROWS = SCREEN_HEIGHT / GRID_SIZE;  // 24
static const int FRAME_DELAY_MS = 100;

enum class Direction { UP, DOWN, LEFT, RIGHT, NONE };

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};

static const unsigned char DIGIT_FONT[10][5] = {
    { 0b01110, 0b10001, 0b10001, 0b10001, 0b01110 }, // 0
    { 0b00100, 0b01100, 0b00100, 0b00100, 0b01110 }, // 1
    { 0b01110, 0b10001, 0b00100, 0b01000, 0b11111 }, // 2
    { 0b01110, 0b10001, 0b00100, 0b10001, 0b01110 }, // 3
    { 0b10001, 0b10001, 0b11111, 0b00001, 0b00001 }, // 4
    { 0b11111, 0b10000, 0b11110, 0b00001, 0b11110 }, // 5
    { 0b01110, 0b10000, 0b11110, 0b10001, 0b01110 }, // 6
    { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000 }, // 7
    { 0b01110, 0b10001, 0b01110, 0b10001, 0b01110 }, // 8
    { 0b01110, 0b10001, 0b01111, 0b00001, 0b01110 }, // 9
};

class Game {
public:
    Game()
        : window(nullptr)
        , renderer(nullptr)
        , running(true)
        , score(0)
        , dir(Direction::RIGHT)
        , nextDir(Direction::RIGHT)
        , lastDir(Direction::RIGHT)
        , gameOver(false)
        , rng(std::random_device{}())
        , distCol(0, GRID_COLS - 1)
        , distRow(0, GRID_ROWS - 1)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    ~Game() { cleanup(); }

    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        window = SDL_CreateWindow("Snake Game (SDL2)",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  SCREEN_WIDTH, SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        reset();
        return true;
    }

    void run() {
        Uint32 lastTick = SDL_GetTicks();
        SDL_Event e;
        while (running) {
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) {
                    running = false;
                } else if (e.type == SDL_KEYDOWN) {
                    handleKey(e.key.keysym.sym);
                }
            }
            Uint32 now = SDL_GetTicks();
            if (now - lastTick >= FRAME_DELAY_MS) {
                lastTick = now;
                if (!gameOver) {
                    dir = nextDir;
                    update();
                }
            }
            render();
        }
    }

private:
    void reset() {
        snake.clear();
        int startX = GRID_COLS / 2;
        int startY = GRID_ROWS / 2;
        snake.push_back({startX, startY});
        snake.push_back({startX - 1, startY});
        snake.push_back({startX - 2, startY});
        dir = Direction::RIGHT;
        nextDir = Direction::RIGHT;
        lastDir = Direction::RIGHT;
        score = 0;
        gameOver = false;
        spawnFood();
    }
    void spawnFood() {
        bool valid = false;
        while (!valid) {
            food.x = distCol(rng);
            food.y = distRow(rng);
            valid = true;
            for (const auto& seg : snake) {
                if (seg == food) { valid = false; break; }
            }
        }
    }

    void handleKey(SDL_Keycode key) {
        switch (key) {
            case SDLK_UP:    case SDLK_w:
                if (dir != Direction::DOWN) nextDir = Direction::UP;
                break;
            case SDLK_DOWN:  case SDLK_s:
                if (dir != Direction::UP) nextDir = Direction::DOWN;
                break;
            case SDLK_LEFT:  case SDLK_a:
                if (dir != Direction::RIGHT) nextDir = Direction::LEFT;
                break;
            case SDLK_RIGHT: case SDLK_d:
                if (dir != Direction::LEFT) nextDir = Direction::RIGHT;
                break;
            case SDLK_r:
                if (gameOver) reset();
                break;
            case SDLK_ESCAPE:
            case SDLK_q:
                running = false;
                break;
            default:
                break;
        }
    }

    void update() {
        if (gameOver) return;
        lastDir = dir;
        Point head = snake.front();
        switch (dir) {
            case Direction::UP:    head.y -= 1; break;
            case Direction::DOWN:  head.y += 1; break;
            case Direction::LEFT:  head.x -= 1; break;
            case Direction::RIGHT: head.x += 1; break;
            default: break;
        }
        if (head.x < 0 || head.x >= GRID_COLS || head.y < 0 || head.y >= GRID_ROWS) {
            gameOver = true; return;
        }
        for (const auto& seg : snake) {
            if (seg == head) { gameOver = true; return; }
        }
        snake.push_front(head);
        if (head == food) {
            score += 10;
            spawnFood();
        } else {
            snake.pop_back();
        }
    }
    void drawRect(int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
        SDL_Rect rect = {x * GRID_SIZE, y * GRID_SIZE, w * GRID_SIZE, h * GRID_SIZE};
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderFillRect(renderer, &rect);
    }

    void drawCell(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect rect = {x * GRID_SIZE + 1, y * GRID_SIZE + 1, GRID_SIZE - 2, GRID_SIZE - 2};
        SDL_RenderFillRect(renderer, &rect);
    }

    void drawDigit(int digit, int x, int y, int scale) {
        if (digit < 0 || digit > 9) return;
        for (int row = 0; row < 5; ++row) {
            unsigned char bits = DIGIT_FONT[digit][row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (1 << (4 - col))) {
                    SDL_Rect rect = {x + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
    }

    void drawScore() {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int value = score;
        int digits[6];
        int count = 0;
        if (value == 0) {
            digits[count++] = 0;
        } else {
            int tmp = value;
            while (tmp > 0) {
                digits[count++] = tmp % 10;
                tmp /= 10;
            }
        }
        const int scale = 2;
        const int startX = 10;
        const int startY = 10;
        int x = startX;
        for (int i = count - 1; i >= 0; --i) {
            drawDigit(digits[i], x, startY, scale);
            x += 5 * scale + 2;
        }
    }

    void drawGameOver() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        const int scale = 3;
        int totalWidth = 9 * (5 * scale + 2) - 2;
        int startX = (SCREEN_WIDTH - totalWidth) / 2;
        int startY = (SCREEN_HEIGHT - 5 * scale) / 2;
        int x = startX;
        for (int i = 0; i < 9; ++i) {
            SDL_Rect rect = {x, startY, 5 * scale, 5 * scale};
            SDL_RenderFillRect(renderer, &rect);
            x += 5 * scale + 2;
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        for (int x = 0; x < GRID_COLS; ++x) {
            drawCell(x, 0, 40, 40, 40);
            drawCell(x, GRID_ROWS - 1, 40, 40, 40);
        }
        for (int y = 0; y < GRID_ROWS; ++y) {
            drawCell(0, y, 40, 40, 40);
            drawCell(GRID_COLS - 1, y, 40, 40, 40);
        }

        drawCell(food.x, food.y, 255, 60, 60);

        for (size_t i = 0; i < snake.size(); ++i) {
            const Point& seg = snake[i];
            if (i == 0) {
                drawCell(seg.x, seg.y, 60, 255, 60);
            } else {
                drawCell(seg.x, seg.y, 40, 180, 40);
            }
        }

        drawScore();

        if (gameOver) {
            drawGameOver();
        }

        SDL_RenderPresent(renderer);
    }

    void cleanup() {
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    std::deque<Point> snake;
    Point food;
    int score;
    Direction dir;
    Direction nextDir;
    Direction lastDir;
    bool gameOver;
    std::mt19937 rng;
    std::uniform_int_distribution<int> distCol;
    std::uniform_int_distribution<int> distRow;
};

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    Game game;
    if (!game.init()) {
        return 1;
    }
    game.run();
    return 0;
}