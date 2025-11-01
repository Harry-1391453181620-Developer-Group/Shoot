#include "framework.h"
#include "Shoot.h"
#include <iostream>
#include <math.h>
#include <fstream>
#include <vector>
#include <algorithm>
#define MAX_LOADSTRING 100
#define TIMER_ID 1 //定时器ID
#define PLAYER_RADIUS 15 // 玩家角色半径
#define MAX_OBSTACLE_COUNT 20 // 最大障碍物数量
#define INIT_OBSTACLE_COUNT 10 // 初始障碍物数量
#define OBSTACLE_WIDTH 30 // 障碍物宽度
#define OBSTACLE_HEIGHT 20 // 障碍物高度
#define PLAYER_SPEED 5 // 玩家移动速度
#define PLAYER_COLOR RGB(255, 0, 0) // 玩家角色颜色
#define BULLET_RADIUS 6
#define BULLET_SPEED 12
#define ENEMY_BULLET_RADIUS 6
#define ENEMY_BULLET_SPEED 8
#define MAX_ENEMY_BULLETS 64
const char* HIGH_SCORE_FILE = "highscore.dat";

// 全局变量
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
int highScore = 0;                              // 最高分

// 玩家角色结构
struct Player {
    int x, y;
    bool active;
} player;

// 障碍物结构
struct Obstacle {
    int x, y;
    int width, height;
    int speedX, speedY;
    bool active;
} obstacles[MAX_OBSTACLE_COUNT];

// 保护盾相关
bool protectionOn = false;
int protectionFrame = 0;

// 多子弹相关
#define MAX_PLAYER_BULLETS 3
struct Bullet {
    int x, y;
    float dx, dy;
    bool active;
};
Bullet playerBullets[MAX_PLAYER_BULLETS] = {};

// 障碍物被击中计数
int obstacleHitCount[MAX_OBSTACLE_COUNT] = {};

struct EnemyBullet {
    int x, y;
    float dx, dy;
    bool active;
};
EnemyBullet enemyBullets[MAX_ENEMY_BULLETS] = {};

int playerHitCount = 0; // 玩家被击中次数
int enemyBulletFireCounter = 0; // 用于控制发射频率
int obstacleCount = INIT_OBSTACLE_COUNT; // 当前障碍物数量

// 游戏状态
bool gameOver = false;
int score = 0;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                InitGame();
void                UpdateGame();
bool                CheckCollision(Obstacle& obs);
void                DrawGame(HDC hdc, int clientWidth, int clientHeight);
void                LoadHighScore();
void                SaveHighScore();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 初始化随机数生成器
    srand(static_cast<unsigned int>(GetTickCount()));

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SHOOT, szWindowClass, MAX_LOADSTRING); //一字之差，谬以千里！IDI_SHOOT 改为 IDC_SHOOT

    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDI_SHOOT));
    MSG msg;

    // 加载最高分
    LoadHighScore();

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // 保存最高分
    SaveHighScore();

    return (int)msg.wParam;
}

//   函数: MyRegisterClass()//
//   目标: 注册窗口类。//
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHOOT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDI_SHOOT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//    函数: InitInstance(HINSTANCE, int)//
//    目标: 保存实例句柄并创建主窗口//
//    注释://
//        在此函数中，我们在全局变量中保存实例句柄并//
//        创建和显示主程序窗口。//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance; // 将实例句柄存储在全局变量中

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    // 初始化游戏
    InitGame();

    // 设置定时器
    SetTimer(hWnd, TIMER_ID, 30, NULL); // 30毫秒定时器

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// 初始化游戏状态
void InitGame() {
    player.x = 400;
    player.y = 300;
    player.active = true;

    obstacleCount = INIT_OBSTACLE_COUNT;
    for (int i = 0; i < obstacleCount; i++) {
        obstacles[i].x = rand() % 700 + 50; // 随机位置
        obstacles[i].y = rand() % 500 + 50;
        obstacles[i].width = OBSTACLE_WIDTH;
        obstacles[i].height = OBSTACLE_HEIGHT;
        obstacles[i].speedX = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1); // 随机速度
        obstacles[i].speedY = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
        obstacles[i].active = true;
    }
    // 初始化多余障碍物为非激活
    for (int i = obstacleCount; i < MAX_OBSTACLE_COUNT; i++) {
        obstacles[i].active = false;
    }

    gameOver = false;
    score = 0;
    protectionOn = false;
    protectionFrame = 0;

    for (int i = 0; i < MAX_PLAYER_BULLETS; ++i) playerBullets[i].active = false;
    for (int i = 0; i < MAX_OBSTACLE_COUNT; ++i) obstacleHitCount[i] = 0;
}

//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)//
//  目标: 处理主窗口的消息。//
//  WM_COMMAND  - 处理应用程序菜单//
//  WM_PAINT    - 绘制主窗口//
//  WM_DESTROY  - 发送退出消息并返回//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        // 分析菜单选择:
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDM_NEWGAME:
            InitGame();
            gameOver = false;
            InvalidateRect(hWnd, NULL, TRUE); // 重绘窗口
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
                   break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 获取客户区大小
        RECT rect;
        GetClientRect(hWnd, &rect);
        int clientWidth = rect.right - rect.left;
        int clientHeight = rect.bottom - rect.top;

        // 绘制游戏
        DrawGame(hdc, clientWidth, clientHeight);

        EndPaint(hWnd, &ps);
    }
                 break;
    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ID); // 停止定时器
        PostQuitMessage(0);
        break;
    case WM_TIMER:
        if (wParam == TIMER_ID && !gameOver) {
            // 更新游戏状态
            UpdateGame();
            // 重绘窗口
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_KEYDOWN:
        if (!gameOver) {
            int speed = PLAYER_SPEED;
            RECT rect;
            GetClientRect(hWnd, &rect);
            int clientWidth = rect.right - rect.left;
            int clientHeight = rect.bottom - rect.top;

            switch (wParam) {
            case VK_LEFT:
                player.x -= speed;
                break;
            case VK_RIGHT:
                player.x += speed;
                break;
            case VK_UP:
                player.y -= speed;
                break;
            case VK_DOWN:
                player.y += speed;
                break;
            case VK_SPACE: {
                if (score >= 10000) break; // 分数超过10000时不允许发射

                // 统计可用子弹槽
                int available = 0;
                for (int i = 0; i < MAX_PLAYER_BULLETS; ++i)
                    if (!playerBullets[i].active) available++;

                if (available == 0) break;

                // 收集最近的障碍物
                struct TargetInfo { int idx; int dist; };
                std::vector<TargetInfo> targets;
                for (int i = 0; i < obstacleCount; ++i) {
                    if (!obstacles[i].active) continue;

                    int ox = obstacles[i].x + obstacles[i].width / 2;
                    int oy = obstacles[i].y + obstacles[i].height / 2;
                    int dx = ox - player.x;
                    int dy = oy - player.y;
                    int dist = dx * dx + dy * dy;
                    targets.push_back({ i, dist });
                }
                std::sort(targets.begin(), targets.end(), [](const TargetInfo& a, const TargetInfo& b) { return a.dist < b.dist; });

                int bulletCount = (score >= 5000) ? 3 : 1;
                int fired = 0;
                for (int t = 0; t < (int)targets.size() && fired < bulletCount; ++t) {
                    int i = targets[t].idx;
                    int ox = obstacles[i].x + obstacles[i].width / 2;
                    int oy = obstacles[i].y + obstacles[i].height / 2;
                    float vx = ox - player.x;
                    float vy = oy - player.y;
                    float len = sqrtf(vx * vx + vy * vy);
                    if (len > 0.1f) {
                        // 找到空闲子弹槽
                        for (int b = 0; b < MAX_PLAYER_BULLETS; ++b) {
                            if (!playerBullets[b].active) {
                                playerBullets[b].x = player.x;
                                playerBullets[b].y = player.y;
                                playerBullets[b].dx = vx / len;
                                playerBullets[b].dy = vy / len;
                                playerBullets[b].active = true;
                                fired++;
                                break;
                            }
                        }
                    }
                }
                break;
            }
            }
            // 确保玩家角色不出界
            if (player.x < PLAYER_RADIUS) player.x = PLAYER_RADIUS;
            if (player.x > clientWidth - PLAYER_RADIUS) player.x = clientWidth - PLAYER_RADIUS;
            if (player.y < PLAYER_RADIUS) player.y = PLAYER_RADIUS;
            if (player.y > clientHeight - PLAYER_RADIUS) player.y = clientHeight - PLAYER_RADIUS;

            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 更新游戏状态
void UpdateGame() {
    RECT rect;
    GetClientRect(GetActiveWindow(), &rect);
    int clientWidth = rect.right - rect.left;
    int clientHeight = rect.bottom - rect.top;
    int specialEnemyBulletCounter = 0;

    // --- 新增逻辑：障碍物数量小于2时补充到10个 ---
    int activeCount = 0;
    for (int i = 0; i < obstacleCount; ++i) {
        if (obstacles[i].active) activeCount++;
    }
    if (activeCount < 2 && obstacleCount < 10) {
        for (int i = obstacleCount; i < 10; ++i) {
            obstacles[i].x = rand() % 700 + 50;
            obstacles[i].y = rand() % 500 + 50;
            obstacles[i].width = OBSTACLE_WIDTH;
            obstacles[i].height = OBSTACLE_HEIGHT;
            obstacles[i].speedX = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            obstacles[i].speedY = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            obstacles[i].active = true;
        }
        obstacleCount = 10;
    }

    // 分数超过2000及其后每500分增加障碍物
    if (score >= 2000 && (score - 2000) % 500 == 0 && obstacleCount < MAX_OBSTACLE_COUNT) {
        for (int i = obstacleCount; i < MAX_OBSTACLE_COUNT; i++) {
            obstacles[i].x = rand() % 700 + 50;
            obstacles[i].y = rand() % 500 + 50;
            obstacles[i].width = OBSTACLE_WIDTH;
            obstacles[i].height = OBSTACLE_HEIGHT;
            obstacles[i].speedX = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            obstacles[i].speedY = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            obstacles[i].active = true;
        }
        obstacleCount = MAX_OBSTACLE_COUNT;
    }

    // 在障碍物移动前，动态调整速度
    int speedUp = 1 + score / 1000; // 每1000分提升一次
    for (int i = 0; i < obstacleCount; i++) {
        if (obstacles[i].active) {
            // 限制最大速度
            if (obstacles[i].speedX > 0)
                obstacles[i].speedX = min(obstacles[i].speedX, speedUp * 3);
            else
                obstacles[i].speedX = max(obstacles[i].speedX, -speedUp * 3);

            if (obstacles[i].speedY > 0)
                obstacles[i].speedY = min(obstacles[i].speedY, speedUp * 3);
            else
                obstacles[i].speedY = max(obstacles[i].speedY, -speedUp * 3);
        }
    }

    // 在UpdateGame中，偶尔让障碍物随机变向
    if (rand() % 1000 < score / 100) { // 分数越高概率越大
        for (int i = 0; i < obstacleCount; i++) {
            if (obstacles[i].active && rand() % 10 == 0) {
                obstacles[i].speedX = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
                obstacles[i].speedY = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            }
        }
    }

    // 更新障碍物位置
    for (int i = 0; i < obstacleCount; i++) {
        if (obstacles[i].active) {
            obstacles[i].x += obstacles[i].speedX;
            obstacles[i].y += obstacles[i].speedY;
            // 碰到边界时反弹
            if (obstacles[i].x < 0 || obstacles[i].x + obstacles[i].width > clientWidth) {
                obstacles[i].speedX = -obstacles[i].speedX;
            }
            if (obstacles[i].y < 0 || obstacles[i].y + obstacles[i].height > clientHeight) {
                obstacles[i].speedY = -obstacles[i].speedY;
            }
            // 检查碰撞
            if (CheckCollision(obstacles[i])) {
                if (!protectionOn) {
                    gameOver = true;
                    return; // 游戏结束
                }
                // 有护盾时不Game Over，直接跳过
            }
        }
    }

    // 玩家子弹移动
    int bulletSpeed = (score >= 5000) ? (BULLET_SPEED * 2) : BULLET_SPEED;
    for (int b = 0; b < MAX_PLAYER_BULLETS; ++b) {
        if (!playerBullets[b].active) continue;
        playerBullets[b].x += int(playerBullets[b].dx * bulletSpeed);
        playerBullets[b].y += int(playerBullets[b].dy * bulletSpeed);
        // 出界
        if (playerBullets[b].x < 0 || playerBullets[b].x > clientWidth ||
            playerBullets[b].y < 0 || playerBullets[b].y > clientHeight) {
            playerBullets[b].active = false;
            continue;
        }
        // 检查与障碍物碰撞
        for (int i = 0; i < obstacleCount; ++i) {
            if (!obstacles[i].active) continue;
            int ox = obstacles[i].x + obstacles[i].width / 2;
            int oy = obstacles[i].y + obstacles[i].height / 2;
            int dx = playerBullets[b].x - ox;
            int dy = playerBullets[b].y - oy;
            int rx = obstacles[i].width / 2 + BULLET_RADIUS;
            int ry = obstacles[i].height / 2 + BULLET_RADIUS;
            if (abs(dx) <= rx && abs(dy) <= ry) {
                // 命中
                obstacleHitCount[i]++;
                playerBullets[b].active = false;
                if (obstacleHitCount[i] >= 2) {
                    obstacles[i].active = false;
                    obstacleHitCount[i] = 0;
                }
                break;
            }
        }
    }

    // 玩家子弹移动后，障碍物被击中处理后添加
    int aliveObs = 0;
    for (int i = 0; i < obstacleCount; ++i) {
        if (obstacles[i].active) aliveObs++;
    }
    if (aliveObs == 0) {
        obstacleCount = MAX_OBSTACLE_COUNT;
        for (int i = 0; i < MAX_OBSTACLE_COUNT; ++i) {
            obstacles[i].x = rand() % 700 + 50;
            obstacles[i].y = rand() % 500 + 50;
            obstacles[i].width = OBSTACLE_WIDTH;
            obstacles[i].height = OBSTACLE_HEIGHT;
            obstacles[i].speedX = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            obstacles[i].speedY = (rand() % 2 == 0 ? -1 : 1) * (rand() % 3 + 1);
            obstacles[i].active = true;
            obstacleHitCount[i] = 0;
        }
    }

    // 敌方子弹发射频率控制（每500帧发射一次）
    enemyBulletFireCounter++;
    if (enemyBulletFireCounter >= 500) {
        enemyBulletFireCounter = 0;
        int firedCount = 0; // 已发射障碍物计数
        for (int i = 0; i < obstacleCount; ++i) {
            if (!obstacles[i].active) continue;
            if (firedCount >= 2) break; // 只允许2个障碍物发射
            // 找到空闲子弹槽
            for (int j = 0; j < MAX_ENEMY_BULLETS; ++j) {
                if (!enemyBullets[j].active) {
                    enemyBullets[j].x = obstacles[i].x + obstacles[i].width / 2;
                    enemyBullets[j].y = obstacles[i].y + obstacles[i].height / 2;
                    float vx = player.x - enemyBullets[j].x;
                    float vy = player.y - enemyBullets[j].y;
                    float len = sqrtf(vx * vx + vy * vy);
                    if (len > 0.1f) {
                        enemyBullets[j].dx = vx / len;
                        enemyBullets[j].dy = vy / len;
                        enemyBullets[j].active = true;
                    }
                    break;
                }
            }
            firedCount++; // 增加已发射障碍物计数
        }
    }

    // 敌方子弹移动与碰撞检测
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        if (!enemyBullets[i].active) continue;
        enemyBullets[i].x += int(enemyBullets[i].dx * ENEMY_BULLET_SPEED);
        enemyBullets[i].y += int(enemyBullets[i].dy * ENEMY_BULLET_SPEED);
        // 出界则消失
        if (enemyBullets[i].x < 0 || enemyBullets[i].x > clientWidth ||
            enemyBullets[i].y < 0 || enemyBullets[i].y > clientHeight) {
            enemyBullets[i].active = false;
            continue;
        }
        // 击中玩家
        int distX = abs(enemyBullets[i].x - player.x);
        int distY = abs(enemyBullets[i].y - player.y);
        if (distX * distX + distY * distY <= PLAYER_RADIUS * PLAYER_RADIUS) {
            enemyBullets[i].active = false;
            if (!protectionOn) {
                playerHitCount++;
                if (playerHitCount >= 20) {
                    gameOver = true;
                    return;
                }
            }
            continue;
        }
    }

    // 增加分数
    score++;
    if (score > highScore) {
        highScore = score; // 更新最高分
        SaveHighScore();
    }

    // 在score++后添加
    static int lastProtectionScore = 0;
    if (score / 500 > lastProtectionScore / 500) {
        protectionOn = true;
        protectionFrame = 200;
    }
    lastProtectionScore = score;

    // 保护盾帧数递减
    if (protectionOn) {
        protectionFrame--;
        if (protectionFrame <= 0) {
            protectionOn = false;
            protectionFrame = 0;
        }
    }

    // 15000分后每400帧有4个障碍物向玩家发射子弹
    if (score >= 15000) {
        specialEnemyBulletCounter++;
        if (specialEnemyBulletCounter >= 400) {
            specialEnemyBulletCounter = 0;
            int firedCount = 0;
            for (int i = 0; i < obstacleCount; ++i) {
                if (!obstacles[i].active) continue;
                if (firedCount >= 4) break;
                // 找到空闲子弹槽
                for (int j = 0; j < MAX_ENEMY_BULLETS; ++j) {
                    if (!enemyBullets[j].active) {
                        enemyBullets[j].x = obstacles[i].x + obstacles[i].width / 2;
                        enemyBullets[j].y = obstacles[i].y + obstacles[i].height / 2;
                        float vx = player.x - enemyBullets[j].x;
                        float vy = player.y - enemyBullets[j].y;
                        float len = sqrtf(vx * vx + vy * vy);
                        if (len > 0.1f) {
                            enemyBullets[j].dx = vx / len;
                            enemyBullets[j].dy = vy / len;
                            enemyBullets[j].active = true;
                        }
                        break;
                    }
                }
                firedCount++;
            }
        }
    }
    else {
        specialEnemyBulletCounter = 0; // 分数不到15000时计数器归零
    }
}

// 检测玩家与障碍物的碰撞
bool CheckCollision(Obstacle& obs) {
    int distX = abs(player.x - (obs.x + obs.width / 2));
    int distY = abs(player.y - (obs.y + obs.height / 2));

    if (distX > (obs.width / 2 + PLAYER_RADIUS) ||
        distY > (obs.height / 2 + PLAYER_RADIUS)) {
        return false; // 没有碰撞
    }

    if (distX <= (obs.width / 2) || distY <= (obs.height / 2)) {
        return true; // 碰撞
    }

    int dx = distX - obs.width / 2;
    int dy = distY - obs.height / 2;
    return (dx * dx + dy * dy <= (PLAYER_RADIUS * PLAYER_RADIUS));
}

// 绘制游戏画面
void DrawGame(HDC hdc, int clientWidth, int clientHeight) {
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    RECT rect = { 0, 0, clientWidth, clientHeight };
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    if (gameOver) {
        HBRUSH gameOverBrush = CreateSolidBrush(RGB(255, 0, 0));
        RECT gameOverRect = { clientWidth / 4, clientHeight / 3, clientWidth * 3 / 4, clientHeight * 2 / 3 };
        FillRect(hdc, &gameOverRect, gameOverBrush);
        DeleteObject(gameOverBrush);

        LOGFONT lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfHeight = 36;
        lf.lfWeight = FW_BOLD;
        wcscpy_s(lf.lfFaceName, L"Arial");
        HFONT font = CreateFontIndirect(&lf);
        HFONT oldFont = (HFONT)SelectObject(hdc, font);
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        wchar_t gameOverText[100];
        swprintf_s(gameOverText, L"Game is over! Points: %d\n press ESC to quit or press the menu to renter", score);
        DrawText(hdc, gameOverText, -1, &gameOverRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
        return;
    }

    // 绘制分数
    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = 24;
    wcscpy_s(lf.lfFaceName, L"Arial");
    HFONT font = CreateFontIndirect(&lf);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    wchar_t scoreText[100];
    swprintf_s(scoreText, L"Score: %d    Highest Score: %d", score, highScore);
    DrawText(hdc, scoreText, -1, &rect, DT_TOP | DT_LEFT);
    SelectObject(hdc, oldFont);
    DeleteObject(font);

    // 绘制玩家角色
    HBRUSH playerBrush = CreateSolidBrush(PLAYER_COLOR);
    Ellipse(hdc, player.x - PLAYER_RADIUS, player.y - PLAYER_RADIUS,
        player.x + PLAYER_RADIUS, player.y + PLAYER_RADIUS);
    DeleteObject(playerBrush);

    // 绘制障碍物
    HBRUSH obstacleBrush = CreateSolidBrush(RGB(0, 0, 0));
    for (int i = 0; i < obstacleCount; i++) {
        if (obstacles[i].active) {
            Rectangle(hdc,
                obstacles[i].x, obstacles[i].y,
                obstacles[i].x + obstacles[i].width,
                obstacles[i].y + obstacles[i].height);
        }
    }
    DeleteObject(obstacleBrush);

    // 绘制所有玩家子弹
    for (int b = 0; b < MAX_PLAYER_BULLETS; ++b) {
        if (playerBullets[b].active) {
            Ellipse(hdc, playerBullets[b].x - BULLET_RADIUS, playerBullets[b].y - BULLET_RADIUS,
                playerBullets[b].x + BULLET_RADIUS, playerBullets[b].y + BULLET_RADIUS);
        }
    }

    // 绘制保护盾
    if (protectionOn) {
        HPEN hPen = CreatePen(PS_DASH, 2, RGB(0, 128, 255));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        int r = PLAYER_RADIUS + 8;
        Arc(hdc, player.x - r, player.y - r, player.x + r, player.y + r, player.x, player.y - r, player.x, player.y + r);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }

    // 计分板右上显示“Protection On”
    if (protectionOn) {
        RECT protRect = rect;
        protRect.left += 260;
        SetTextColor(hdc, RGB(0, 128, 255));
        DrawText(hdc, L"Protection On", -1, &protRect, DT_TOP | DT_LEFT);
    }

    // 在DrawGame函数最后添加
    for (int i = 0; i < MAX_ENEMY_BULLETS; ++i) {
        if (enemyBullets[i].active) {
            HBRUSH eBrush = CreateSolidBrush(RGB(0, 0, 0));
            Ellipse(hdc, enemyBullets[i].x - ENEMY_BULLET_RADIUS, enemyBullets[i].y - ENEMY_BULLET_RADIUS,
                enemyBullets[i].x + ENEMY_BULLET_RADIUS, enemyBullets[i].y + ENEMY_BULLET_RADIUS);
            DeleteObject(eBrush);
        }
    }
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void LoadHighScore() {
    std::ifstream fin(HIGH_SCORE_FILE, std::ios::binary);
    if (fin) {
        fin.read(reinterpret_cast<char*>(&highScore), sizeof(highScore));
        fin.close();
    }
    else {
        // 添加日志记录
        std::cerr << "Failed to load high score file: " << HIGH_SCORE_FILE << std::endl;
    }
}

void SaveHighScore() {
    std::ofstream fout(HIGH_SCORE_FILE, std::ios::binary);
    if (fout) {
        fout.write(reinterpret_cast<const char*>(&highScore), sizeof(highScore));
        fout.close();
    }
    else {
        // 添加日志记录
        std::cerr << "Failed to save high score file: " << HIGH_SCORE_FILE << std::endl;
    }
}
