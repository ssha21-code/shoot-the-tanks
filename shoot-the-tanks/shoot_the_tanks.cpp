#include <array>
#include "ExtraHeader.h"
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <vector>

constexpr int WINDOW_WIDTH = 1000;
constexpr int WINDOW_HEIGHT = 800;

Color accent_color = {255, 255, 255, 170};

Texture2D textureTank;
Texture2D textureEnemyTank;
Texture2D textureBullet;
Texture2D textureGround;
Texture2D textureStart;
Texture2D texturePause;
Texture2D texturePlay;
Texture2D textureTitleScreen;

Sound soundGunshot;
Sound soundHit;
Sound soundSelection;
Music bossMusic;

void loadMedia() {
    textureTank = LoadTexture("images/tank.png");
    textureEnemyTank = LoadTexture("images/enemy_tank.png");
    textureBullet = LoadTexture("images/bullet.png");
    textureGround = LoadTexture("images/ground.png");
    textureStart = LoadTexture("images/start_button.png");
    texturePause = LoadTexture("images/pause_button.png");
    texturePlay = LoadTexture("images/play_button.png");
    textureTitleScreen = LoadTexture("images/title_screen.png");

    Wave waveGunshot = LoadWave("sounds/gunshot.wav");
    Wave waveHit = LoadWave("sounds/hit.wav");
    Wave waveSelection = LoadWave("sounds/selection.wav");
    soundGunshot = LoadSoundFromWave(waveGunshot);
    soundHit = LoadSoundFromWave(waveHit);
    soundSelection = LoadSoundFromWave(waveSelection);
    bossMusic = LoadMusicStream("sounds/boss_music.wav");
}

class Player {
public: 
    Vector2 position = {WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f};
    Vector2 velocity;
    Vector2 velocityOriginal = {8.0f, 8.0f};
    Vector2 size;
    Vector2 origin;
    Vector2 direction;
    float rotation;
    std::array<Vector2, 4> rectCorners;
    int hp = 100;

    bool shouldSpawnBullet = false;
    bool isDead = false;

    Player() {
        this->size = {(float)textureTank.width, (float)textureTank.height};
        this->origin = {size.x / 2, size.y / 2};
    }

    void draw() {
        Rectangle src = {0, 0, size.x, size.y};
        Rectangle dest = {position.x, position.y, size.x, size.y};
        DrawTexturePro(textureTank, src, dest, origin, rotation, WHITE);
    }
    void update() {
        float dt = GetFrameTime();

        Vector2 mousePos = GetMousePosition();
        direction = Vector2Subtract(mousePos, position);
        rotation = atan2f(direction.y, direction.x) * RAD2DEG;

        std::array<Vector2, 4> localCorners = {
            Vector2{ -origin.x, -origin.y },
            Vector2{  origin.x, -origin.y },
            Vector2{  origin.x,  origin.y },
            Vector2{ -origin.x,  origin.y }
        };

        for (int i = 0; i < 4; i++) {
            Vector2 rotated = Vector2Rotate(localCorners[i], rotation * DEG2RAD);
            rectCorners[i] = Vector2Add(position, rotated);
        }

        if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            shouldSpawnBullet = true;
        }
        if (IsKeyDown(KEY_W) && (fabs(direction.x) > 20.0f || fabs(direction.y) > 20.0f)) {
            float velocityX = cos(rotation * DEG2RAD) * velocityOriginal.x;
            float velocityY = sin(rotation * DEG2RAD) * velocityOriginal.y;
            velocity = {velocityX, velocityY};
            position = Vector2Add(position, velocity);
        }
    }
};

class Bullet {
public: 
    Vector2 position;
    Vector2 velocity;
    Vector2 velocityOriginal = {15.0f, 15.0f};
    Vector2 size;
    Vector2 origin;
    float rotation;
    std::array<Vector2, 4> rectCorners;

    bool isPlayerBullet;
    bool shouldBeDestroyed = false;

    Bullet(float rotation, Vector2 position, bool isPlayerBullet) {
        this->rotation = rotation;
        this->position = position;
        this->isPlayerBullet = isPlayerBullet;
        this->size = {(float)textureBullet.width, (float)textureBullet.height};
        this->origin = {size.x / 2.0f, size.y / 2.0f};
    }

    void draw() {
        Rectangle src = {0, 0, size.x, size.y};
        Rectangle dest = {position.x, position.y, size.x, size.y};
        DrawTexturePro(textureBullet, src, dest, origin, rotation, WHITE);
    }
    void update() {
        std::array<Vector2, 4> localCorners = {
            Vector2{ -origin.x, -origin.y },
            Vector2{  origin.x, -origin.y },
            Vector2{  origin.x,  origin.y },
            Vector2{ -origin.x,  origin.y }
        };

        for (int i = 0; i < 4; i++) {
            Vector2 rotated = Vector2Rotate(localCorners[i], rotation * DEG2RAD);
            rectCorners[i] = Vector2Add(position, rotated);
        }
        
        float velocityX = cos(rotation * DEG2RAD) * velocityOriginal.x;
        float velocityY = sin(rotation * DEG2RAD) * velocityOriginal.y;
        velocity = {velocityX, velocityY};
        position = Vector2Add(position, velocity);
    }
};

class Enemy {
public: 
    Vector2 position;
    Vector2 velocity;
    Vector2 velocityOriginal = {8.0f, 8.0f};
    Vector2 size;
    Vector2 origin;
    Vector2 direction;
    Vector2 *pPlayerPos;
    float reloadTime;
    float movedDistance = 0;
    float rotation;
    std::array<Vector2, 4> rectCorners;
    double lastUpdatedTime = 0;
    int hp = 10;

    bool shouldSpawnBullet = false;
    bool hasReachedPosition = false;
    bool shouldBeDestroyed = false;

    Enemy(Vector2 *playerPos, Vector2 position) {
        this->pPlayerPos = playerPos;
        this->position = position;
        this->reloadTime = Random::GetRandomFloat(0.5f, 1.5f);
        this->size = {(float)textureEnemyTank.width, (float)textureEnemyTank.height};
        this->origin = {size.x / 2.0f, size.y / 2.0f};
    }

    void draw() {
        Rectangle src = {0, 0, size.x, size.y};
        Rectangle dest = {position.x, position.y, size.x, size.y};
        DrawTexturePro(textureEnemyTank, src, dest, origin, rotation, WHITE);
        DrawText(TextFormat("%i", hp), position.x, position.y, 30, accent_color);
    }
    void update() {
        direction = Vector2Subtract(*pPlayerPos, position);
        rotation = atan2f(direction.y, direction.x) * RAD2DEG;
        if (hasReachedPosition) {
            double currentTime = GetTime();
            if (currentTime - lastUpdatedTime >= reloadTime) {
                shouldSpawnBullet = true;
                lastUpdatedTime = currentTime;
            }
        } else {
            float velocityX = cos(rotation * DEG2RAD) * velocityOriginal.x;
            float velocityY = sin(rotation * DEG2RAD) * velocityOriginal.y;
            velocity = {velocityX, velocityY};
            position = Vector2Add(position, velocity);
            movedDistance += Vector2Length(velocity);
            if (movedDistance >= 150) {
                hasReachedPosition = true;
            }
        }
        std::array<Vector2, 4> localCorners = {
            Vector2{ -origin.x, -origin.y },
            Vector2{  origin.x, -origin.y },
            Vector2{  origin.x,  origin.y },
            Vector2{ -origin.x,  origin.y }
        };

        for (int i = 0; i < 4; i++) {
            Vector2 rotated = Vector2Rotate(localCorners[i], rotation * DEG2RAD);
            rectCorners[i] = Vector2Add(position, rotated);
        }
    }
};

class Game {
public: 
    Player player = Player();
    std::vector<Enemy> enemies = {};
    std::vector<Bullet> bullets = {};
    std::vector<int> scores = {};
    float delayBetweenWaves = 1.0;
    double lastUpdateTime = 0;
    int spawnNumber = 8;
    int maximumBulletDamagePlayer = 7;
    int maximumBulletDamageEnemy = 3;
    int wavesDefeated = -1;
    int gamesPlayed = 0;
    int highScore = 0;

    Rectangle startButtonArea;
    Rectangle pausePlayButtonArea;

    bool shouldSpawnTanks = false;
    bool isPaused = false;
    bool isInTitleScreen = true;

    Game() {
        startButtonArea = {(WINDOW_WIDTH - textureStart.width) / 2.0f, 550, (float)textureStart.width, (float)textureStart.height};
        pausePlayButtonArea = {WINDOW_WIDTH - 200, 20, (float)texturePause.width, (float)texturePause.height};
        PlayMusicStream(bossMusic);
    }

    void draw() {
        DrawText("Please Download textures and sounds", 0, 0, 40, WHITE);
        if (isInTitleScreen) {
            DrawTexture(textureTitleScreen, 0, 0, WHITE);
            if (gamesPlayed > 0) {
                DrawText(TextFormat("Games Played: %i", gamesPlayed), 50, 400, 40, accent_color);
                DrawText(TextFormat("High score: %i", highScore), 50, 450, 40, accent_color);
            }
            DrawTexture(textureStart, startButtonArea.x, startButtonArea.y, WHITE);
        } else {
            DrawTexture(textureGround, 0, 0, WHITE);
            for (Bullet &bullet: bullets) {
                bullet.draw();
            }
            player.draw();
            for (Enemy &enemy: enemies) {
                enemy.draw();
            }
            DrawTexture(isPaused ? texturePlay : texturePause, pausePlayButtonArea.x, pausePlayButtonArea.y, WHITE);
            DrawText(TextFormat("Health: %i", player.hp), 50, 50, 40, accent_color);
            DrawText(TextFormat("Waves Defeated : %i", wavesDefeated), 50, 90, 40, accent_color);
        }
    }
    void update() {
        Vector2 mousePos = GetMousePosition();
        if (isInTitleScreen) {
            if (CheckCollisionPointRec(mousePos, startButtonArea) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySound(soundSelection);
                isInTitleScreen = false;
            }
        } else {
            if (!isPaused) {
                UpdateMusicStream(bossMusic);
                if (enemies.empty() && !shouldSpawnTanks) {
                    player.hp = 100;
                    wavesDefeated++;
                    shouldSpawnTanks = true;
                    lastUpdateTime = GetTime();
                }
                if (shouldSpawnTanks) {
                    double currentTime = GetTime();
                    if (currentTime - lastUpdateTime >= delayBetweenWaves) {
                        bullets.clear();
                        Vector2 spawnPositions[] = {{0, 200}, {0, WINDOW_HEIGHT - 200},
                                                    {200, WINDOW_HEIGHT}, {WINDOW_WIDTH - 200, WINDOW_HEIGHT},
                                                    {WINDOW_WIDTH, 200}, {WINDOW_WIDTH, WINDOW_HEIGHT - 200}, 
                                                    {200, 0}, {WINDOW_WIDTH - 200, 0}
                                                   };
                        for (int i = 0; i < spawnNumber; i++) {
                            enemies.push_back(Enemy(&player.position, spawnPositions[i]));
                        }
                        shouldSpawnTanks = false;
                    }
                }
                player.update();
                if (player.shouldSpawnBullet) {
                    PlaySound(soundGunshot);
                    Vector2 forward = Vector2Normalize(player.direction);
                    Vector2 bulletPos = Vector2Add(player.position, Vector2Scale(forward, player.size.x / 2.0f + 8.0f));
                    bullets.push_back(Bullet(player.rotation, bulletPos, true));
                    player.shouldSpawnBullet = false;
                }
                
                if (player.isDead) {
                    scores.push_back(wavesDefeated);
                    calculateHighScore();
                    player.hp = 100;
                    bullets.clear();
                    enemies.clear();
                    gamesPlayed++;
                    wavesDefeated = -1;
                    player.isDead = false;
                    isInTitleScreen = true;
                }
                for (Enemy &enemy: enemies) {
                    if (enemy.shouldSpawnBullet) {
                        PlaySound(soundGunshot);
                        Vector2 forward = Vector2Normalize(enemy.direction);
                        Vector2 bulletPos = Vector2Add(enemy.position, Vector2Scale(forward, enemy.size.x / 2.0f + 25.0f));
                        bullets.push_back(Bullet(enemy.rotation, bulletPos, false));
                        enemy.shouldSpawnBullet = false;
                    }
                }
                for (Bullet &bullet: bullets) {
                    bullet.update();
                }
                for (Enemy &enemy: enemies) {
                    enemy.update();
                }
                checkForRemoval();
                checkForCollisions();
                garbageCollect();
            }
            if (CheckCollisionPointRec(mousePos, pausePlayButtonArea) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                isPaused = !isPaused;
                PlaySound(soundSelection);
            }
        }
        if (!IsWindowFocused() || IsWindowMinimized()) {
            isPaused = true;
        }
    }
    
    void checkForRemoval() {
        for (Bullet &bullet: bullets) {
            if (bullet.position.x + bullet.size.x/2 > WINDOW_WIDTH || bullet.position.x + bullet.size.x/2 < 0 || bullet.position.y + bullet.size.y/2 > WINDOW_HEIGHT || bullet.position.y + bullet.size.y < 0) {
                bullet.shouldBeDestroyed = true;
            }
        }
    }
    void garbageCollect() {
        for (int i = 0; i < bullets.size(); i++) {
            if (bullets.at(i).shouldBeDestroyed) {
                bullets.erase(bullets.begin() + i);
                i--;
            }
        }
        for (int i = 0; i < enemies.size(); i++) {
            if (enemies.at(i).shouldBeDestroyed) {
                enemies.erase(enemies.begin() + i);
            }
        }
    }
    void checkForCollisions() {
        for (Enemy &enemy: enemies) {
            for (Bullet &bullet: bullets) {
                if (Collision::CheckCollisionRectCorners(enemy.rectCorners, bullet.rectCorners)) {
                    if (bullet.isPlayerBullet) {
                        PlaySound(soundHit);
                        enemy.hp -= GetRandomValue(1, maximumBulletDamagePlayer);
                        if (enemy.hp <= 0) {
                            enemy.shouldBeDestroyed = true;
                        }
                    }
                    bullet.shouldBeDestroyed = true;
                }
            }
        }
        for (Bullet &bullet: bullets) {
            if (Collision::CheckCollisionRectCorners(player.rectCorners, bullet.rectCorners)) {
                if (!bullet.isPlayerBullet) {
                    PlaySound(soundHit);
                    player.hp -= GetRandomValue(1, maximumBulletDamageEnemy);
                    if (player.hp <= 0) {
                        player.hp = 0;
                        player.isDead = true;
                    }
                    bullet.shouldBeDestroyed = true;
                }
            }
        }
    }
    void calculateHighScore() {
        if (scores.empty()) return;
        int highestScore = 0;
        for (int score: scores) {
            if (highestScore > score) {
                continue;
            } else if (score >= highestScore) {
                highestScore = score;
            }
        }
        highScore = highestScore;
    }
};

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Shoot The Tanks");
    InitAudioDevice();
    SetTargetFPS(60);

    loadMedia();

    Game game = Game();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        game.update();
        game.draw();

        EndDrawing();
    }

    CloseWindow();
    std::abort();
    return 0;
}