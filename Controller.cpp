#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <windows.h>
#include "Config.h"
#include "Controller.h"
#include "main.h"

extern int Map[mapHeight][mapWidth];

//jump/crunch
int maxJumpHeight = int(1.1 * sqResh); //jump this high
int maxCrunchHeight = -int(0.5 * sqResh); //crunch as most as this low
float fFPS = 30; //approximate FPS
int acceleratedMotion[200];
int maxJump_idx, maxCrunch_idx;
int verticalAdvance = 0;
int jumping = 0, crunching = 0;
int z = 0; //same unit as sqRes
static clock_t keyTime = clock();

//returns wall ID (as map position and cell face)
int Cast(int angle, int& xHit, int& yHit) {
    int xX, yX, xY, yY;
    CastX(xC, yC, angle, xX, yX);
    CastY(xC, yC, angle, xY, yY);
    int adjXMap = ((aroundq < angle) && (angle < around3q)) ? -1 : 0;
    int adjYMap = (angle > aroundh) ? -1 : 0;
    //choose the nearest hit point
    if (abs(xC - xX) < abs(xC - xY)) { //vertical wall ||
        xHit = xX;
        yHit = yX;
        return 2 * ((yHit / sqRes) * mapWidth + (xHit / sqRes + adjXMap)) + 0;
    }
    else { //horizontal wall ==
        xHit = xY;
        yHit = yY;
        return 2 * ((yHit / sqRes + adjYMap) * mapWidth + (xHit / sqRes)) + 1;
    }
}

void move(int& x, int& y, int angle) {
    float rad = angle * 6.2831f / around;
    int xTest = x + int(MOVE_SPD * cos(rad));
    int yTest = y + int(MOVE_SPD * sin(rad));

    //check for wall collision
    int safetyX = ((aroundq < angle) && (angle < around3q)) ? +safety_dist : -safety_dist;
    int safetyY = (angle < aroundh) ? -safety_dist : +safety_dist;
    int adjXMap = ((aroundq < angle) && (angle < around3q)) ? -1 : 0;
    int adjYMap = (angle > aroundh) ? -1 : 0;

    int xWall, yWall;
    int wallID = Cast(angle, xWall, yWall);
    if (sq(x - xTest) + sq(y - yTest) >= sq(x - xWall) + sq(y - yWall)) { //inside wall
        if (wallID % 2 == 0) { //vertical wall ||
            x = xWall + safetyX;
            y = yTest;                          //               __
            if (Map[y / sqRes][x / sqRes] != 0) //it's a corner |
                y = (yTest / sqRes - adjYMap) * sqRes + safetyY;
        }
        else { //horizontal wall ==
            x = xTest;
            y = yWall + safetyY;                //               __
            if (Map[y / sqRes][x / sqRes] != 0) //it's a corner |
                x = (xTest / sqRes - adjXMap) * sqRes + safetyX;
        }
    }
    else //free cell
        x = xTest, y = yTest;
}

void rotate(int& angle, int dir, int around) {
    angle = (angle + dir * ROTATE_SPD + around) % around;
}

int initController() {
    float fDist = 0, fSpeed = 0, G = 8000.f * sqRes / 200; //G was empirically chosen as we don't have a proper world scale here
    for (int i = 0; i < 200; i++) {
        acceleratedMotion[i] = (int)fDist;

        fSpeed += G / fFPS;
        fDist += fSpeed / fFPS;
    }

    //search for the acceleratedMotion entry so that we'll decelerate to zero speed at max jump height
    for (maxJump_idx = 0; maxJump_idx < 200; maxJump_idx++)
        if (acceleratedMotion[maxJump_idx] > maxJumpHeight)
            break;

    if (maxJump_idx >= 200) maxJump_idx = 199;

    elevation_perc = 100 * z / sqResh; //as percentage from wall half height

    return 0;
}

int loopController(int& x, int& y, int& angle, int around) {
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        return 1; //render first frame
    }

    int sign = 1;
#ifdef INVERT_COORDINATE_SYSTEM
    sign = -1;
#endif

    int did = 0;

#ifdef USE_MULTIPLE_KEYS_SIMULTANEOUSLY
    {
        unsigned char ch = 255;
#else
    if (_kbhit()) { //check user's input
        unsigned char ch = toupper(_getch());
#endif
        if ((ch == 27) || (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) //ASCII code for the Esc key
            exit(0); //end the game
        if ((ch == 'W') || (GetAsyncKeyState('W') & 0x8000)) { //pedal forward
            move(x, y, angle);
            did = 1;
        }
        if ((ch == 'S') || (GetAsyncKeyState('S') & 0x8000)) { //pedal backward
            move(x, y, (angle + around / 2) % around);
            did = 1;
        }
        if ((ch == 'A') || (GetAsyncKeyState('A') & 0x8000)) { //strafe left
            move(x, y, (angle + sign * around / 4 + around) % around);
            did = 1;
        }
        if ((ch == 'D') || (GetAsyncKeyState('D') & 0x8000)) { //strafe right
            move(x, y, (angle - sign * around / 4 + around) % around);
            did = 1;
        }

        //jump/crunch
        static int jump_idx;
        if (((ch == 'E') || (GetAsyncKeyState('E') & 0x8000)) && !jumping && !crunching) {
            jumping = 1;
            verticalAdvance = 1;
            jump_idx = maxJump_idx - 1;
            z = maxJumpHeight - acceleratedMotion[jump_idx];
            did = 1;
        }
        else
        if (jumping) {
            if (verticalAdvance > 0) {
                if (jump_idx > 0) {
                    jump_idx--;
                    z = maxJumpHeight - acceleratedMotion[jump_idx];
                }
                else {
                    verticalAdvance = -1;
                    z = maxJumpHeight;
                }
                did = 1;
            }
            else
            if (verticalAdvance < 0) {
                if (z > 0) {
                    jump_idx++;
                    z = max(0, maxJumpHeight - acceleratedMotion[jump_idx]);
                }
                else {
                    verticalAdvance = 0;
                    jumping = 0;
                }
                did = 1;
            }
        }

        //crunch
        if (((ch == 'C') || (GetAsyncKeyState('C') & 0x8000)) && !jumping) {
            crunching = 1;
            if (z > maxCrunchHeight) {
                z -= VERTICAL_SPD;
                if (z < maxCrunchHeight)
                    z = maxCrunchHeight;
                did = 1;
            }
        }
        else
        if (crunching) {
            z += VERTICAL_SPD;
            if (z >= 0) {
                z = 0;
                crunching = 0;
            }
            did = 1;
        }
        
        elevation_perc = 100 * z / sqResh; //as percentage from wall half height

#ifdef USE_MULTIPLE_KEYS_SIMULTANEOUSLY
        {
#else
        if (ch == 224) { //it's a key that generates two bytes when being pressed, the first one being 224
            ch = _getch();
#endif
            if ((ch == 75) || (GetAsyncKeyState(VK_LEFT) & 0x8000)) { //the left arrow key => do turn left
                rotate(angle, +1 * sign, around);
                did = 1;
            }
            if ((ch == 77) || (GetAsyncKeyState(VK_RIGHT) & 0x8000)) { //the right arrow key => do turn right
                rotate(angle, -1 * sign, around);
                did = 1;
            }
        }
    } //if (_kbhit())

    return did;
}

int a = initController(); //for the sake of not calling it from main.cpp, since it's not related to rendering
