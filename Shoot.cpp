#include <SDL2/SDL.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstring>


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600


#define PLAYER_RADIUS 15
#define MAX_OBSTACLE_COUNT 20
#define INIT_OBSTACLE_COUNT 10

#define OBSTACLE_WIDTH 30
#define OBSTACLE_HEIGHT 20

#define PLAYER_SPEED 5

#define BULLET_RADIUS 6
#define BULLET_SPEED 12

#define ENEMY_BULLET_RADIUS 6
#define ENEMY_BULLET_SPEED 8

#define MAX_ENEMY_BULLETS 64
#define MAX_PLAYER_BULLETS 3


const char* HIGH_SCORE_FILE = "highscore.dat";



struct Player
{
    int x;
    int y;
    bool active;
} player;



struct Obstacle
{
    int x;
    int y;

    int width;
    int height;

    int speedX;
    int speedY;

    bool active;

} obstacles[MAX_OBSTACLE_COUNT];



struct Bullet
{
    int x;
    int y;

    float dx;
    float dy;

    bool active;

} playerBullets[MAX_PLAYER_BULLETS];



struct EnemyBullet
{
    int x;
    int y;

    float dx;
    float dy;

    bool active;

} enemyBullets[MAX_ENEMY_BULLETS];



int obstacleHitCount[MAX_OBSTACLE_COUNT] = {};

int playerHitCount = 0;

int enemyBulletFireCounter = 0;

int obstacleCount = INIT_OBSTACLE_COUNT;


bool protectionOn = false;

int protectionFrame = 0;


bool gameOver = false;

int score = 0;

int highScore = 0;



void InitGame();

void UpdateGame();

bool CheckCollision(Obstacle& obs);

void DrawGame(
    SDL_Renderer* renderer,
    int width,
    int height
);

void LoadHighScore();

void SaveHighScore();

void DrawCircle(
    SDL_Renderer* renderer,
    int cx,
    int cy,
    int radius
);



int main()
{

    SDL_Init(SDL_INIT_VIDEO);


    SDL_Window* window =
        SDL_CreateWindow(
            "Shoot",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN
        );


    SDL_Renderer* renderer =
        SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED
        );


    LoadHighScore();


    InitGame();


    bool running = true;


    Uint32 lastUpdate = SDL_GetTicks();



    while(running)
    {

        SDL_Event event;


        while(SDL_PollEvent(&event))
        {

            if(event.type == SDL_QUIT)
            {
                running=false;
            }



            if(event.type == SDL_KEYDOWN)
            {

                switch(event.key.keysym.sym)
                {

                    case SDLK_ESCAPE:
                        running=false;
                        break;


                    case SDLK_LEFT:
                        player.x -= PLAYER_SPEED;
                        break;


                    case SDLK_RIGHT:
                        player.x += PLAYER_SPEED;
                        break;


                    case SDLK_UP:
                        player.y -= PLAYER_SPEED;
                        break;


                    case SDLK_DOWN:
                        player.y += PLAYER_SPEED;
                        break;



                    case SDLK_SPACE:
                    {

                        if(score>=10000)
                            break;


                        int available=0;


                        for(int i=0;i<MAX_PLAYER_BULLETS;i++)
                        {
                            if(!playerBullets[i].active)
                                available++;
                        }


                        if(available==0)
                            break;



                        std::vector<int> targets;


                        for(int i=0;i<obstacleCount;i++)
                        {
                            if(obstacles[i].active)
                                targets.push_back(i);
                        }



                        std::sort(
                            targets.begin(),
                            targets.end(),
                            [](int a,int b)
                            {

                                int ax =
                                    obstacles[a].x-player.x;

                                int ay =
                                    obstacles[a].y-player.y;


                                int bx =
                                    obstacles[b].x-player.x;

                                int by =
                                    obstacles[b].y-player.y;


                                return ax*ax+ay*ay <
                                       bx*bx+by*by;
                            }
                        );



                        int bulletCount =
                            score>=5000 ? 3 : 1;


                        int fired=0;



                        for(int t:targets)
                        {

                            if(fired>=bulletCount)
                                break;


                            float vx =
                                obstacles[t].x-player.x;

                            float vy =
                                obstacles[t].y-player.y;


                            float len =
                                sqrtf(vx*vx+vy*vy);



                            if(len>0.1f)
                            {

                                for(int b=0;b<MAX_PLAYER_BULLETS;b++)
                                {

                                    if(!playerBullets[b].active)
                                    {

                                        playerBullets[b].x=player.x;
                                        playerBullets[b].y=player.y;

                                        playerBullets[b].dx=vx/len;
                                        playerBullets[b].dy=vy/len;

                                        playerBullets[b].active=true;

                                        fired++;

                                        break;
                                    }

                                }

                            }

                        }


                        break;
                    }

                }


                if(player.x<PLAYER_RADIUS)
                    player.x=PLAYER_RADIUS;

                if(player.x>SCREEN_WIDTH-PLAYER_RADIUS)
                    player.x=SCREEN_WIDTH-PLAYER_RADIUS;


                if(player.y<PLAYER_RADIUS)
                    player.y=PLAYER_RADIUS;

                if(player.y>SCREEN_HEIGHT-PLAYER_RADIUS)
                    player.y=SCREEN_HEIGHT-PLAYER_RADIUS;

            }

        }



        Uint32 now=SDL_GetTicks();


        if(now-lastUpdate>=30)
        {

            if(!gameOver)
                UpdateGame();


            lastUpdate=now;

        }



        SDL_SetRenderDrawColor(
            renderer,
            240,
            240,
            240,
            255
        );


        SDL_RenderClear(renderer);



        DrawGame(
            renderer,
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        );



        SDL_RenderPresent(renderer);

    }



    SaveHighScore();


    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);


    SDL_Quit();


    return 0;
}

void InitGame()
{

    player.x = SCREEN_WIDTH / 2;
    player.y = SCREEN_HEIGHT / 2;
    player.active = true;


    obstacleCount = INIT_OBSTACLE_COUNT;


    for(int i=0;i<obstacleCount;i++)
    {

        obstacles[i].x =
            rand()%700+50;

        obstacles[i].y =
            rand()%500+50;


        obstacles[i].width =
            OBSTACLE_WIDTH;

        obstacles[i].height =
            OBSTACLE_HEIGHT;


        obstacles[i].speedX =
            (rand()%2==0?-1:1)
            *(rand()%3+1);


        obstacles[i].speedY =
            (rand()%2==0?-1:1)
            *(rand()%3+1);


        obstacles[i].active=true;

    }



    for(int i=obstacleCount;i<MAX_OBSTACLE_COUNT;i++)
    {
        obstacles[i].active=false;
    }



    for(int i=0;i<MAX_PLAYER_BULLETS;i++)
    {
        playerBullets[i].active=false;
    }


    for(int i=0;i<MAX_ENEMY_BULLETS;i++)
    {
        enemyBullets[i].active=false;
    }



    for(int i=0;i<MAX_OBSTACLE_COUNT;i++)
    {
        obstacleHitCount[i]=0;
    }



    playerHitCount=0;

    enemyBulletFireCounter=0;


    gameOver=false;

    score=0;

    protectionOn=false;

    protectionFrame=0;

}




void UpdateGame()
{

    int activeCount=0;


    for(int i=0;i<obstacleCount;i++)
    {
        if(obstacles[i].active)
            activeCount++;
    }



    if(activeCount<2 && obstacleCount<10)
    {

        for(int i=obstacleCount;i<10;i++)
        {

            obstacles[i].x =
                rand()%700+50;

            obstacles[i].y =
                rand()%500+50;


            obstacles[i].width =
                OBSTACLE_WIDTH;

            obstacles[i].height =
                OBSTACLE_HEIGHT;


            obstacles[i].speedX =
                (rand()%2==0?-1:1)
                *(rand()%3+1);


            obstacles[i].speedY =
                (rand()%2==0?-1:1)
                *(rand()%3+1);


            obstacles[i].active=true;

        }


        obstacleCount=10;

    }




    if(score>=2000 &&
       (score-2000)%500==0 &&
       obstacleCount<MAX_OBSTACLE_COUNT)
    {

        for(int i=obstacleCount;i<MAX_OBSTACLE_COUNT;i++)
        {

            obstacles[i].x =
                rand()%700+50;


            obstacles[i].y =
                rand()%500+50;


            obstacles[i].width =
                OBSTACLE_WIDTH;


            obstacles[i].height =
                OBSTACLE_HEIGHT;



            obstacles[i].speedX =
                (rand()%2==0?-1:1)
                *(rand()%3+1);


            obstacles[i].speedY =
                (rand()%2==0?-1:1)
                *(rand()%3+1);


            obstacles[i].active=true;

        }


        obstacleCount=MAX_OBSTACLE_COUNT;

    }




    int speedUp =
        1 + score/1000;



    for(int i=0;i<obstacleCount;i++)
    {

        if(obstacles[i].active)
        {

            if(obstacles[i].speedX>0)
                obstacles[i].speedX =
                    std::min(
                        obstacles[i].speedX,
                        speedUp*3
                    );
            else
                obstacles[i].speedX =
                    std::max(
                        obstacles[i].speedX,
                        -speedUp*3
                    );



            if(obstacles[i].speedY>0)
                obstacles[i].speedY =
                    std::min(
                        obstacles[i].speedY,
                        speedUp*3
                    );
            else
                obstacles[i].speedY =
                    std::max(
                        obstacles[i].speedY,
                        -speedUp*3
                    );

        }

    }




    for(int i=0;i<obstacleCount;i++)
    {

        if(obstacles[i].active)
        {

            obstacles[i].x +=
                obstacles[i].speedX;


            obstacles[i].y +=
                obstacles[i].speedY;



            if(obstacles[i].x<0 ||
               obstacles[i].x+obstacles[i].width>SCREEN_WIDTH)
            {
                obstacles[i].speedX =
                    -obstacles[i].speedX;
            }



            if(obstacles[i].y<0 ||
               obstacles[i].y+obstacles[i].height>SCREEN_HEIGHT)
            {
                obstacles[i].speedY =
                    -obstacles[i].speedY;
            }



            if(CheckCollision(obstacles[i]))
            {

                if(!protectionOn)
                {
                    gameOver=true;
                    return;
                }

            }

        }

    }




    int bulletSpeed =
        score>=5000 ?
        BULLET_SPEED*2 :
        BULLET_SPEED;



    for(int b=0;b<MAX_PLAYER_BULLETS;b++)
    {

        if(!playerBullets[b].active)
            continue;



        playerBullets[b].x +=
            int(playerBullets[b].dx*bulletSpeed);


        playerBullets[b].y +=
            int(playerBullets[b].dy*bulletSpeed);



        if(playerBullets[b].x<0 ||
           playerBullets[b].x>SCREEN_WIDTH ||
           playerBullets[b].y<0 ||
           playerBullets[b].y>SCREEN_HEIGHT)
        {

            playerBullets[b].active=false;

            continue;
        }




        for(int i=0;i<obstacleCount;i++)
        {

            if(!obstacles[i].active)
                continue;



            int ox =
                obstacles[i].x+
                obstacles[i].width/2;


            int oy =
                obstacles[i].y+
                obstacles[i].height/2;



            int dx =
                playerBullets[b].x-ox;


            int dy =
                playerBullets[b].y-oy;



            int rx =
                obstacles[i].width/2+
                BULLET_RADIUS;


            int ry =
                obstacles[i].height/2+
                BULLET_RADIUS;



            if(abs(dx)<=rx &&
               abs(dy)<=ry)
            {

                obstacleHitCount[i]++;


                playerBullets[b].active=false;



                if(obstacleHitCount[i]>=2)
                {

                    obstacles[i].active=false;

                    obstacleHitCount[i]=0;

                    score+=100;

                }


                break;

            }

        }

    }
    int aliveObs=0;


    for(int i=0;i<obstacleCount;i++)
    {
        if(obstacles[i].active)
            aliveObs++;
    }



    if(aliveObs==0)
    {

        obstacleCount=MAX_OBSTACLE_COUNT;


        for(int i=0;i<MAX_OBSTACLE_COUNT;i++)
        {

            obstacles[i].x =
                rand()%700+50;


            obstacles[i].y =
                rand()%500+50;


            obstacles[i].width =
                OBSTACLE_WIDTH;


            obstacles[i].height =
                OBSTACLE_HEIGHT;



            obstacles[i].speedX =
                (rand()%2==0?-1:1)
                *(rand()%3+1);


            obstacles[i].speedY =
                (rand()%2==0?-1:1)
                *(rand()%3+1);



            obstacles[i].active=true;


            obstacleHitCount[i]=0;

        }

    }





    enemyBulletFireCounter++;


    if(enemyBulletFireCounter>=500)
    {

        enemyBulletFireCounter=0;


        int firedCount=0;



        for(int i=0;i<obstacleCount;i++)
        {

            if(!obstacles[i].active)
                continue;


            if(firedCount>=2)
                break;



            for(int j=0;j<MAX_ENEMY_BULLETS;j++)
            {

                if(!enemyBullets[j].active)
                {

                    enemyBullets[j].x =
                        obstacles[i].x+
                        obstacles[i].width/2;


                    enemyBullets[j].y =
                        obstacles[i].y+
                        obstacles[i].height/2;



                    float vx =
                        player.x -
                        enemyBullets[j].x;


                    float vy =
                        player.y -
                        enemyBullets[j].y;



                    float len =
                        sqrtf(vx*vx+vy*vy);



                    if(len>0.1f)
                    {

                        enemyBullets[j].dx =
                            vx/len;


                        enemyBullets[j].dy =
                            vy/len;


                        enemyBullets[j].active=true;

                    }


                    break;

                }

            }


            firedCount++;

        }

    }




    for(int i=0;i<MAX_ENEMY_BULLETS;i++)
    {

        if(!enemyBullets[i].active)
            continue;



        enemyBullets[i].x +=
            int(enemyBullets[i].dx*
                ENEMY_BULLET_SPEED);



        enemyBullets[i].y +=
            int(enemyBullets[i].dy*
                ENEMY_BULLET_SPEED);




        if(enemyBullets[i].x<0 ||
           enemyBullets[i].x>SCREEN_WIDTH ||
           enemyBullets[i].y<0 ||
           enemyBullets[i].y>SCREEN_HEIGHT)
        {

            enemyBullets[i].active=false;

            continue;

        }




        int dx =
            enemyBullets[i].x-player.x;


        int dy =
            enemyBullets[i].y-player.y;



        if(dx*dx+dy*dy <=
           PLAYER_RADIUS*PLAYER_RADIUS)
        {

            enemyBullets[i].active=false;



            if(!protectionOn)
            {

                playerHitCount++;



                if(playerHitCount>=20)
                {

                    gameOver=true;

                    return;

                }

            }

        }

    }




    score++;



    if(score>highScore)
    {

        highScore=score;

        SaveHighScore();

    }





    static int lastProtectionScore=0;



    if(score/500 >
       lastProtectionScore/500)
    {

        protectionOn=true;

        protectionFrame=200;

    }



    lastProtectionScore=score;




    if(protectionOn)
    {

        protectionFrame--;


        if(protectionFrame<=0)
        {

            protectionOn=false;

            protectionFrame=0;

        }

    }

}




bool CheckCollision(Obstacle& obs)
{

    int distX =
        abs(
            player.x -
            (obs.x+obs.width/2)
        );


    int distY =
        abs(
            player.y -
            (obs.y+obs.height/2)
        );



    if(distX >
       obs.width/2+PLAYER_RADIUS ||
       distY >
       obs.height/2+PLAYER_RADIUS)
    {
        return false;
    }



    if(distX<=obs.width/2 ||
       distY<=obs.height/2)
    {
        return true;
    }




    int dx =
        distX -
        obs.width/2;


    int dy =
        distY -
        obs.height/2;



    return
        dx*dx+dy*dy <=
        PLAYER_RADIUS*PLAYER_RADIUS;

}
void DrawCircle(
    SDL_Renderer* renderer,
    int cx,
    int cy,
    int radius
)
{

    for(int w=-radius;w<=radius;w++)
    {

        for(int h=-radius;h<=radius;h++)
        {

            if(w*w+h*h <= radius*radius)
            {

                SDL_RenderDrawPoint(
                    renderer,
                    cx+w,
                    cy+h
                );

            }

        }

    }

}




void DrawGame(
    SDL_Renderer* renderer,
    int clientWidth,
    int clientHeight
)
{

    if(gameOver)
    {

        SDL_SetRenderDrawColor(
            renderer,
            255,
            0,
            0,
            255
        );


        SDL_Rect r;

        r.x=clientWidth/4;
        r.y=clientHeight/3;

        r.w=clientWidth/2;
        r.h=clientHeight/3;


        SDL_RenderFillRect(
            renderer,
            &r
        );


        return;

    }




    SDL_SetRenderDrawColor(
        renderer,
        255,
        0,
        0,
        255
    );


    DrawCircle(
        renderer,
        player.x,
        player.y,
        PLAYER_RADIUS
    );





    SDL_SetRenderDrawColor(
        renderer,
        0,
        0,
        0,
        255
    );



    for(int i=0;i<obstacleCount;i++)
    {

        if(obstacles[i].active)
        {

            SDL_Rect rect;


            rect.x =
                obstacles[i].x;


            rect.y =
                obstacles[i].y;


            rect.w =
                obstacles[i].width;


            rect.h =
                obstacles[i].height;



            SDL_RenderFillRect(
                renderer,
                &rect
            );

        }

    }





    SDL_SetRenderDrawColor(
        renderer,
        0,
        0,
        255,
        255
    );



    for(int i=0;i<MAX_PLAYER_BULLETS;i++)
    {

        if(playerBullets[i].active)
        {

            DrawCircle(
                renderer,
                playerBullets[i].x,
                playerBullets[i].y,
                BULLET_RADIUS
            );

        }

    }





    SDL_SetRenderDrawColor(
        renderer,
        0,
        128,
        255,
        255
    );



    if(protectionOn)
    {

        for(int angle=0;angle<360;angle++)
        {

            float rad =
                angle*3.1415926f/180.0f;


            int x =
                player.x+
                int(cos(rad)*
                (PLAYER_RADIUS+8));


            int y =
                player.y+
                int(sin(rad)*
                (PLAYER_RADIUS+8));


            SDL_RenderDrawPoint(
                renderer,
                x,
                y
            );

        }

    }





    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        0,
        255
    );



    for(int i=0;i<MAX_ENEMY_BULLETS;i++)
    {

        if(enemyBullets[i].active)
        {

            DrawCircle(
                renderer,
                enemyBullets[i].x,
                enemyBullets[i].y,
                ENEMY_BULLET_RADIUS
            );

        }

    }

}





void LoadHighScore()
{

    std::ifstream fin(
        HIGH_SCORE_FILE,
        std::ios::binary
    );


    if(fin)
    {

        fin.read(
            reinterpret_cast<char*>(&highScore),
            sizeof(highScore)
        );


        fin.close();

    }
    else
    {

        std::cerr
            << "Failed to load high score file: "
            << HIGH_SCORE_FILE
            << std::endl;

    }

}




void SaveHighScore()
{

    std::ofstream fout(
        HIGH_SCORE_FILE,
        std::ios::binary
    );


    if(fout)
    {

        fout.write(
            reinterpret_cast<const char*>(&highScore),
            sizeof(highScore)
        );


        fout.close();

    }
    else
    {

        std::cerr
            << "Failed to save high score file: "
            << HIGH_SCORE_FILE
            << std::endl;

    }

}
