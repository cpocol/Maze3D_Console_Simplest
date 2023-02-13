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
int zC = 0; //same unit as sqRes
static clock_t keyTime = clock();

HANDLE hIn;
HWND hWnd;
bool wndHasFocus = true;
//mouse cursor reference position
int refMousePosX = screenWh * 8, refMousePosY = screenH * 16 - 8;

int initController() {
    float fDist = 0, fSpeed = 0, G = 981.f * sqRes / 50; //G was empirically chosen as we don't have a proper world scale here
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

    elevation_perc = 100 * zC / sqResh; //as percentage from wall half height

    //init mouse input
    hIn = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    wchar_t windowTitle[1000];
    GetConsoleTitle(windowTitle, 1000);
    hWnd = FindWindow(NULL, windowTitle);

    //set cursor to a reference position
    POINT mousePoint = {refMousePosX, refMousePosY};
    ClientToScreen(hWnd, &mousePoint);
    SetCursorPos(mousePoint.x, mousePoint.y);

    return 0;
}

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

void rotate(int& angle, int dir, int around, int rotate_spd = ROTATE_SPD) {
    angle = (angle + dir * rotate_spd + around) % around;
}

int loopController(int& x, int& y, int& angle, int around) {
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
        return 1; //always render first frame
    }

    int sign = 1;
#ifdef INVERT_COORDINATE_SYSTEM
    sign = -1;
#endif

    int did = 0;

    //check the mouse (and focus)
    INPUT_RECORD InputRecord[128];
    DWORD RecordsRead = 0;
    GetNumberOfConsoleInputEvents(hIn, &RecordsRead);
    if (RecordsRead > 0)
        ReadConsoleInput(hIn, InputRecord, RecordsRead, &RecordsRead);

    for (DWORD i = 0; i < RecordsRead; i++) {
        if (InputRecord[i].EventType == FOCUS_EVENT) {
            wndHasFocus = InputRecord[i].Event.FocusEvent.bSetFocus;
        }
        if (InputRecord[i].EventType == MOUSE_EVENT) {
            if(InputRecord[i].Event.MouseEvent.dwEventFlags == MOUSE_MOVED) {
                //int mousePosX = InputRecord[i].Event.MouseEvent.dwMousePosition.X; //char level accuracy
                POINT mousePoint;
                GetCursorPos(&mousePoint);
                ScreenToClient(hWnd, &mousePoint);
                int mousePosX = mousePoint.x; //get pixel level accuracy

                rotate(angle, (refMousePosX - mousePosX) / 2 * sign, around, 1);

                //reset cursor to the reference position
                POINT refMousePoint = {refMousePosX, refMousePosY};
                ClientToScreen(hWnd, &refMousePoint);
                SetCursorPos(refMousePoint.x, refMousePoint.y);

                did = 1;
            }
        }
    }
        
    if (!wndHasFocus)
        return 0;

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
            zC = maxJumpHeight - acceleratedMotion[jump_idx];
            did = 1;
        }
        else
        if (jumping) {
            if (verticalAdvance > 0) {
                if (jump_idx > 0) {
                    jump_idx--;
                    zC = maxJumpHeight - acceleratedMotion[jump_idx];
                }
                else {
                    verticalAdvance = -1;
                    zC = maxJumpHeight;
                }
                did = 1;
            }
            else
            if (verticalAdvance < 0) {
                if (zC > 0) {
                    jump_idx++;
                    zC = max(0, maxJumpHeight - acceleratedMotion[jump_idx]);
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
            if (zC > maxCrunchHeight) {
                zC -= VERTICAL_SPD;
                if (zC < maxCrunchHeight)
                    zC = maxCrunchHeight;
                did = 1;
            }
        }
        else
        if (crunching) {
            zC += VERTICAL_SPD;
            if (zC >= 0) {
                zC = 0;
                crunching = 0;
            }
            did = 1;
        }
        
        elevation_perc = 100 * zC / sqResh; //as percentage from wall half height

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
