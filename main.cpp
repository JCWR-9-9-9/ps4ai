#include <SDL2/SDL.h>
#include <math.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

// دقة الشاشة 1080p
const int SCREEN_W = 1920;
const int SCREEN_H = 1080;

// أزرار DualShock 4 في SDL2
enum PS4Buttons {
    BTN_CROSS = 0,
    BTN_CIRCLE = 1,
    BTN_SQUARE = 2,
    BTN_TRIANGLE = 3,
    BTN_L1 = 4,
    BTN_R1 = 5,
    BTN_L2 = 6,
    BTN_R2 = 7,
    BTN_OPTIONS = 9
};

struct Vec2 {
    float x, y;
};

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    bool fromPlayer;
    float radius;
    int damage;
};

struct Enemy {
    Vec2 pos;
    Vec2 vel;
    float radius;
    int hp;
    int maxHp;
    int type; // 0: Chaser, 1: Shooter, 2: Tank
    float shootTimer;
    float hitFlash;
};

struct Particle {
    Vec2 pos;
    Vec2 vel;
    float life;
    float maxLife;
    Uint8 r, g, b;
    float size;
};

struct Player {
    Vec2 pos;
    Vec2 vel;
    float angle;
    float radius;
    int hp;
    int maxHp;
    float shootCooldown;
    float dashCooldown;
    float dashTimer;
    float empCharge; // 0.0 to 100.0
    float invulnTimer;
};

// دالة مساعدة لرسم دائرة ممتلئة بدون ملفات خارجية
void DrawFilledCircle(SDL_Renderer* renderer, int cx, int cy, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
}

// رسم خط سميك ليزر / توجيه
void DrawThickLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int thickness) {
    for (int i = -thickness / 2; i <= thickness / 2; i++) {
        SDL_RenderDrawLine(renderer, x1 + i, y1, x2 + i, y2);
        SDL_RenderDrawLine(renderer, x1, y1 + i, x2, y2 + i);
    }
}

// رسم رقم بسيط (7-Segment) لعرض السكور والموجة بدون حاجة لخطوط TTF
void DrawDigit(SDL_Renderer* renderer, int digit, int x, int y, int size) {
    const bool segs[10][7] = {
        {1,1,1,1,1,1,0}, {0,1,1,0,0,0,0}, {1,1,0,1,1,0,1}, {1,1,1,1,0,0,1}, {0,1,1,0,0,1,1},
        {1,0,1,1,0,1,1}, {1,0,1,1,1,1,1}, {1,1,1,0,0,0,0}, {1,1,1,1,1,1,1}, {1,1,1,1,0,1,1}
    };
    if (digit < 0 || digit > 9) return;
    int w = size, h = size;
    if (segs[digit][0]) DrawThickLine(renderer, x, y, x + w, y, 3);
    if (segs[digit][1]) DrawThickLine(renderer, x + w, y, x + w, y + h, 3);
    if (segs[digit][2]) DrawThickLine(renderer, x + w, y + h, x + w, y + 2 * h, 3);
    if (segs[digit][3]) DrawThickLine(renderer, x, y + 2 * h, x + w, y + 2 * h, 3);
    if (segs[digit][4]) DrawThickLine(renderer, x, y + h, x, y + 2 * h, 3);
    if (segs[digit][5]) DrawThickLine(renderer, x, y, x, y + h, 3);
    if (segs[digit][6]) DrawThickLine(renderer, x, y + h, x + w, y + h, 3);
}

void DrawNumber(SDL_Renderer* renderer, int number, int x, int y, int size) {
    if (number == 0) {
        DrawDigit(renderer, 0, x, y, size);
        return;
    }
    std::vector<int> digits;
    while (number > 0) {
        digits.push_back(number % 10);
        number /= 10;
    }
    std::reverse(digits.begin(), digits.end());
    for (size_t i = 0; i < digits.size(); i++) {
        DrawDigit(renderer, digits[i], x + i * (size + 14), y, size);
    }
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) < 0) {
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Neon Arena PS4",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // تفعيل يد DualShock 4
    SDL_Joystick* pad = nullptr;
    if (SDL_NumJoysticks() > 0) {
        pad = SDL_JoystickOpen(0);
    }

    Player player = { {SCREEN_W / 2.0f, SCREEN_H / 2.0f}, {0, 0}, 0.0f, 22.0f, 100, 100, 0.0f, 0.0f, 0.0f, 50.0f, 0.0f };
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Particle> particles;

    int score = 0;
    int wave = 1;
    int enemiesToSpawn = 8;
    float spawnTimer = 0.0f;
    float shakeTimer = 0.0f;
    float shakeIntensity = 0.0f;
    float empWaveRadius = 0.0f;
    bool gameOver = false;

    auto SpawnParticles = [&](Vec2 pos, int count, Uint8 r, Uint8 g, Uint8 b, float speedMax) {
        for (int i = 0; i < count; i++) {
            float ang = ((rand() % 360) * 3.14159f) / 180.0f;
            float spd = 50.0f + (rand() % static_cast<int>(speedMax));
            particles.push_back({
                pos,
                {cosf(ang) * spd, sinf(ang) * spd},
                0.4f + (rand() % 40) / 100.0f,
                0.8f,
                r, g, b,
                3.0f + (rand() % 5)
            });
        }
    };

    auto TriggerShake = [&](float time, float intensity) {
        shakeTimer = time;
        shakeIntensity = intensity;
    };

    Uint64 lastTick = SDL_GetPerformanceCounter();
    bool running = true;

    while (running) {
        Uint64 currentTick = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(currentTick - lastTick) / SDL_GetPerformanceFrequency();
        if (dt > 0.05f) dt = 0.05f;
        lastTick = currentTick;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_JOYBUTTONDOWN) {
                if (gameOver && e.jbutton.button == BTN_CROSS) {
                    // إعادة التشغيل بزر X
                    player = { {SCREEN_W / 2.0f, SCREEN_H / 2.0f}, {0, 0}, 0.0f, 22.0f, 100, 100, 0.0f, 0.0f, 0.0f, 50.0f, 0.0f };
                    bullets.clear();
                    enemies.clear();
                    particles.clear();
                    score = 0;
                    wave = 1;
                    enemiesToSpawn = 8;
                    gameOver = false;
                }
                if (!gameOver) {
                    // زر R2 للاندفاع السريع (Dash)
                    if (e.jbutton.button == BTN_R2 && player.dashCooldown <= 0.0f) {
                        player.dashTimer = 0.18f;
                        player.dashCooldown = 1.2f;
                        player.invulnTimer = 0.25f;
                        SpawnParticles(player.pos, 15, 0, 255, 255, 300.0f);
                    }
                    // زر L2 لتفجير EMP
                    if (e.jbutton.button == BTN_L2 && player.empCharge >= 100.0f) {
                        player.empCharge = 0.0f;
                        empWaveRadius = 10.0f;
                        TriggerShake(0.35f, 18.0f);
                        // مسح رصاص الأعداء وإلحاق ضرر جماعي
                        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
                            return !b.fromPlayer;
                        }), bullets.end());
                        for (auto& en : enemies) {
                            en.hp -= 60;
                            en.hitFlash = 0.2f;
                        }
                    }
                }
            }
        }

        if (!gameOver) {
            // قراءة الأنالوج الأيسر والأيمن (مع Deadzone)
            float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;
            if (pad) {
                lx = SDL_JoystickGetAxis(pad, 0) / 32768.0f;
                ly = SDL_JoystickGetAxis(pad, 1) / 32768.0f;
                rx = SDL_JoystickGetAxis(pad, 2) / 32768.0f;
                ry = SDL_JoystickGetAxis(pad, 3) / 32768.0f;
            }
            if (::fabs(lx) < 0.15f) lx = 0.0f;
            if (::fabs(ly) < 0.15f) ly = 0.0f;

            float moveSpeed = (player.dashTimer > 0.0f) ? 1150.0f : 480.0f;
            player.pos.x += lx * moveSpeed * dt;
            player.pos.y += ly * moveSpeed * dt;

            // حدود الساحة
            player.pos.x = std::max(40.0f, std::min(static_cast<float>(SCREEN_W - 40), player.pos.x));
            player.pos.y = std::max(40.0f, std::min(static_cast<float>(SCREEN_H - 40), player.pos.y));

            if (player.shootCooldown > 0.0f) player.shootCooldown -= dt;
            if (player.dashCooldown > 0.0f) player.dashCooldown -= dt;
            if (player.dashTimer > 0.0f) player.dashTimer -= dt;
            if (player.invulnTimer > 0.0f) player.invulnTimer -= dt;

            // التصويب وإطلاق النار بالأنالوج الأيمن
            float rMag = sqrtf(rx * rx + ry * ry);
            if (rMag > 0.25f) {
                player.angle = atan2f(ry, rx);
                if (player.shootCooldown <= 0.0f) {
                    player.shootCooldown = 0.09f;
                    Vec2 dir = { cosf(player.angle), sinf(player.angle) };
                    bullets.push_back({
                        { player.pos.x + dir.x * 28.0f, player.pos.y + dir.y * 28.0f },
                        { dir.x * 1350.0f, dir.y * 1350.0f },
                        true, 6.0f, 15
                    });
                }
            }

            // نظام توليد الموجات (Waves)
            if (enemiesToSpawn > 0) {
                spawnTimer -= dt;
                if (spawnTimer <= 0.0f) {
                    spawnTimer = std::max(0.25f, 1.1f - wave * 0.07f);
                    enemiesToSpawn--;

                    Vec2 spawnPos;
                    int side = rand() % 4;
                    if (side == 0) spawnPos = { static_cast<float>(rand() % SCREEN_W), 30.0f };
                    else if (side == 1) spawnPos = { static_cast<float>(rand() % SCREEN_W), static_cast<float>(SCREEN_H - 30) };
                    else if (side == 2) spawnPos = { 30.0f, static_cast<float>(rand() % SCREEN_H) };
                    else spawnPos = { static_cast<float>(SCREEN_W - 30), static_cast<float>(rand() % SCREEN_H) };

                    int roll = rand() % 100;
                    if (roll < 60) {
                        enemies.push_back({ spawnPos, {0,0}, 20.0f, 30 + wave * 5, 30 + wave * 5, 0, 0.0f, 0.0f });
                    } else if (roll < 85) {
                        enemies.push_back({ spawnPos, {0,0}, 24.0f, 45 + wave * 8, 45 + wave * 8, 1, 1.5f, 0.0f });
                    } else {
                        enemies.push_back({ spawnPos, {0,0}, 38.0f, 140 + wave * 20, 140 + wave * 20, 2, 0.0f, 0.0f });
                    }
                }
            } else if (enemies.empty()) {
                wave++;
                enemiesToSpawn = 6 + wave * 4;
                player.hp = std::min(player.maxHp, player.hp + 25); // مكافأة نهاية الموجة
            }

            // تحديث الرصاص
            for (auto& b : bullets) {
                b.pos.x += b.vel.x * dt;
                b.pos.y += b.vel.y * dt;
            }
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {
                return b.pos.x < 0 || b.pos.x > SCREEN_W || b.pos.y < 0 || b.pos.y > SCREEN_H;
            }), bullets.end());

            // تحديث الأعداء والتصادم
            for (auto& en : enemies) {
                if (en.hitFlash > 0.0f) en.hitFlash -= dt;
                float dx = player.pos.x - en.pos.x;
                float dy = player.pos.y - en.pos.y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > 0.01f) {
                    float spd = (en.type == 0) ? 260.0f : (en.type == 1 ? 160.0f : 110.0f);
                    if (en.type == 1 && dist < 420.0f) spd = 0.0f; // يوقف ويطلق نار
                    en.pos.x += (dx / dist) * spd * dt;
                    en.pos.y += (dy / dist) * spd * dt;
                }

                // إطلاق نار من العدو القناص
                if (en.type == 1) {
                    en.shootTimer -= dt;
                    if (en.shootTimer <= 0.0f && dist > 0.01f) {
                        en.shootTimer = 2.0f;
                        bullets.push_back({
                            en.pos,
                            { (dx / dist) * 550.0f, (dy / dist) * 550.0f },
                            false, 8.0f, 15
                        });
                    }
                }

                // تصادم العدو مع اللاعب
                if (dist < player.radius + en.radius && player.invulnTimer <= 0.0f) {
                    player.hp -= (en.type == 2) ? 25 : 15;
                    player.invulnTimer = 0.8f;
                    TriggerShake(0.2f, 12.0f);
                    SpawnParticles(player.pos, 20, 255, 50, 50, 350.0f);
                    if (player.hp <= 0) gameOver = true;
                }
            }

            // تصادم الرصاص
            for (size_t i = 0; i < bullets.size(); i++) {
                if (bullets[i].fromPlayer) {
                    for (auto& en : enemies) {
                        float dx = bullets[i].pos.x - en.pos.x;
                        float dy = bullets[i].pos.y - en.pos.y;
                        if (dx * dx + dy * dy < (bullets[i].radius + en.radius) * (bullets[i].radius + en.radius)) {
                            en.hp -= bullets[i].damage;
                            en.hitFlash = 0.08f;
                            bullets[i].pos.x = -9999.0f; // حذف الرصاصة
                            SpawnParticles(en.pos, 5, 255, 220, 50, 200.0f);
                            break;
                        }
                    }
                } else {
                    float dx = bullets[i].pos.x - player.pos.x;
                    float dy = bullets[i].pos.y - player.pos.y;
                    if (dx * dx + dy * dy < (bullets[i].radius + player.radius) * (bullets[i].radius + player.radius)) {
                        if (player.invulnTimer <= 0.0f) {
                            player.hp -= bullets[i].damage;
                            player.invulnTimer = 0.6f;
                            TriggerShake(0.15f, 10.0f);
                            if (player.hp <= 0) gameOver = true;
                        }
                        bullets[i].pos.x = -9999.0f;
                    }
                }
            }

            // حذف الأعداء المقتولين واحتساب السكور
            enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [&](const Enemy& en) {
                if (en.hp <= 0) {
                    score += (en.type == 2) ? 300 : (en.type == 1 ? 150 : 100);
                    player.empCharge = std::min(100.0f, player.empCharge + 8.0f);
                    TriggerShake(0.12f, 8.0f);
                    SpawnParticles(en.pos, 25, 255, 80, 180, 400.0f);
                    return true;
                }
                return false;
            }), enemies.end());
        }

        // تحديث الجزيئات وموجة EMP
        for (auto& p : particles) {
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.life -= dt;
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) {
            return p.life <= 0.0f;
        }), particles.end());

        if (empWaveRadius > 0.0f) {
            empWaveRadius += 2400.0f * dt;
            if (empWaveRadius > 2200.0f) empWaveRadius = 0.0f;
        }

        // إزاحة الكاميرا عند الاهتزاز
        int offsetX = 0, offsetY = 0;
        if (shakeTimer > 0.0f) {
            shakeTimer -= dt;
            offsetX = (rand() % static_cast<int>(shakeIntensity * 2)) - static_cast<int>(shakeIntensity);
            offsetY = (rand() % static_cast<int>(shakeIntensity * 2)) - static_cast<int>(shakeIntensity);
        }

        // ================= الرسم (Rendering) =================
        SDL_SetRenderDrawColor(renderer, 12, 14, 24, 255);
        SDL_RenderClear(renderer);

        // رسم شبكة أرضية نيون خفيفة
        SDL_SetRenderDrawColor(renderer, 25, 32, 55, 255);
        for (int x = 0; x < SCREEN_W; x += 80) SDL_RenderDrawLine(renderer, x + offsetX, 0, x + offsetX, SCREEN_H);
        for (int y = 0; y < SCREEN_H; y += 80) SDL_RenderDrawLine(renderer, 0, y + offsetY, SCREEN_W, y + offsetY);

        // رسم موجة EMP
        if (empWaveRadius > 0.0f) {
            SDL_SetRenderDrawColor(renderer, 0, 240, 255, 255);
            for (int a = 0; a < 360; a += 2) {
                float rad = a * 3.14159f / 180.0f;
                int px = static_cast<int>(player.pos.x + cosf(rad) * empWaveRadius) + offsetX;
                int py = static_cast<int>(player.pos.y + sinf(rad) * empWaveRadius) + offsetY;
                SDL_RenderDrawPoint(renderer, px, py);
            }
        }

        // رسم الجزيئات
        for (const auto& p : particles) {
            SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, 255);
            SDL_Rect r = { static_cast<int>(p.pos.x) + offsetX, static_cast<int>(p.pos.y) + offsetY, static_cast<int>(p.size), static_cast<int>(p.size) };
            SDL_RenderFillRect(renderer, &r);
        }

        // رسم الرصاص
        for (const auto& b : bullets) {
            if (b.fromPlayer) SDL_SetRenderDrawColor(renderer, 0, 255, 220, 255);
            else SDL_SetRenderDrawColor(renderer, 255, 60, 60, 255);
            DrawFilledCircle(renderer, static_cast<int>(b.pos.x) + offsetX, static_cast<int>(b.pos.y) + offsetY, static_cast<int>(b.radius));
        }

        // رسم الأعداء
        for (const auto& en : enemies) {
            if (en.hitFlash > 0.0f) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else if (en.type == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 50, 120, 255); // وردي نيون
            } else if (en.type == 1) {
                SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);  // برتقالي
            } else {
                SDL_SetRenderDrawColor(renderer, 170, 50, 255, 255); // بنفسجي ضخم
            }
            DrawFilledCircle(renderer, static_cast<int>(en.pos.x) + offsetX, static_cast<int>(en.pos.y) + offsetY, static_cast<int>(en.radius));
        }

        // رسم اللاعب ومدفعه
        if (!gameOver) {
            if (player.invulnTimer > 0.0f && (static_cast<int>(player.invulnTimer * 20) % 2 == 0)) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
            }
            DrawFilledCircle(renderer, static_cast<int>(player.pos.x) + offsetX, static_cast<int>(player.pos.y) + offsetY, static_cast<int>(player.radius));

            // سبطانة السلاح
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            int gunX = static_cast<int>(player.pos.x + cosf(player.angle) * 34.0f) + offsetX;
            int gunY = static_cast<int>(player.pos.y + sinf(player.angle) * 34.0f) + offsetY;
            DrawThickLine(renderer, static_cast<int>(player.pos.x) + offsetX, static_cast<int>(player.pos.y) + offsetY, gunX, gunY, 5);
        }

        // ================= واجهة المستخدم (HUD) =================
        // شريط الصحة (HP)
        SDL_SetRenderDrawColor(renderer, 60, 20, 30, 255);
        SDL_Rect hpBg = { 40, 35, 320, 24 };
        SDL_RenderFillRect(renderer, &hpBg);
        SDL_SetRenderDrawColor(renderer, 0, 230, 120, 255);
        SDL_Rect hpFill = { 40, 35, static_cast<int>(320 * (std::max(0, player.hp) / static_cast<float>(player.maxHp))), 24 };
        SDL_RenderFillRect(renderer, &hpFill);

        // شريط طاقة الـ EMP (L2)
        SDL_SetRenderDrawColor(renderer, 20, 40, 60, 255);
        SDL_Rect empBg = { 40, 70, 320, 14 };
        SDL_RenderFillRect(renderer, &empBg);
        if (player.empCharge >= 100.0f) SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
        else SDL_SetRenderDrawColor(renderer, 0, 130, 200, 255);
        SDL_Rect empFill = { 40, 70, static_cast<int>(320 * (player.empCharge / 100.0f)), 14 };
        SDL_RenderFillRect(renderer, &empFill);

        // Display score and wave on top right
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        DrawNumber(renderer, score, SCREEN_W - 260, 35, 20);
        SDL_SetRenderDrawColor(renderer, 0, 255, 220, 255);
        DrawNumber(renderer, wave, SCREEN_W - 260, 95, 14);

        // Game Over screen
        if (gameOver) {
            SDL_SetRenderDrawColor(renderer, 255, 40, 70, 255);
            SDL_Rect box = { SCREEN_W / 2 - 220, SCREEN_H / 2 - 90, 440, 180 };
            SDL_RenderDrawRect(renderer, &box);
            DrawNumber(renderer, score, SCREEN_W / 2 - 90, SCREEN_H / 2 - 25, 24);
        }

        SDL_RenderPresent(renderer);
    }

    if (pad) SDL_JoystickClose(pad);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
