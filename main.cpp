#include "pch.h"
#include <math.h>
#include <stdio.h>
#include <windows.h>
#include "Config.h"
#include "Controller.h"
#include "Map.h"

char screen[screenH][screenW + 1] = {{0}}; //we'll paint everything in this matrix, then flush it onto the real screen
char Texture[sqRes*sqRes];
int xC = int(2.5f * sqRes), yC = int(2.5f * sqRes), angleC = 10, elevation_perc = 0; //viewer Current position, orientation and elevation_perc

void CastX(int xC, int yC, int angle, int& xHit, int& yHit) { //   hit vertical walls ||
    //prepare as for 1st or 4th quadrant
    xHit = (xC / sqRes) * sqRes + sqRes;
    int dx = sqRes,   adjXMap = 0;
    float dy = sqRes * tanf(angle * 3.1416f / aroundh);
    if ((aroundq < angle) && (angle < around3q)) { //2nd or 3rd quadrant
        xHit -= sqRes;
        adjXMap = -1;
        dx = -dx;
        dy = -dy;
    }
    float fyHit = yC + (xHit - xC) * tanf(angle * 3.1416f / aroundh);

    while ((0 < xHit) && (xHit < mapSizeWidth) && (0 < fyHit) && (fyHit < mapSizeHeight) && (Map[int(fyHit / sqRes)][xHit / sqRes + adjXMap] == 0))
        xHit += dx, fyHit += dy;
    yHit = (int)fyHit;
}

void CastY(int xC, int yC, int angle, int& xHit, int& yHit) { //   hit horizontal walls ==
    //prepare as for 1st or 2nd quadrant
    yHit = (yC / sqRes) * sqRes + sqRes;
    int dy = sqRes,   adjYMap = 0;
    float dx = sqRes / tanf(angle * 3.1416f / aroundh);
    if (angle > aroundh) { //3rd or 4th quadrants
        yHit -= sqRes;
        adjYMap = -1;
        dy = -dy;
        dx = -dx;
    }
    float fxHit = xC + (yHit - yC) / tanf(angle * 3.1416f / aroundh);

    while ((0 < fxHit) && (fxHit < mapSizeWidth) && (0 < yHit) && (yHit < mapSizeHeight) && (Map[yHit / sqRes + adjYMap][int(fxHit / sqRes)] == 0))
        fxHit += dx, yHit += dy;
    xHit = (int)fxHit;
}

void Render() {
    memset(screen, ' ', sizeof(screen));

    const int viewerToScreen_sq = sq(screenWh) * 3; //FOV = 60 degs => viewerToScreen = screenWh * sqrt(3)
    for (int col = 0; col < screenW; col++) {
        int xHit, yHit, xY, yY;
        int ang = (screenWh - col + angleC + around) % around;
        ang += (ang % aroundq == 0) ? 1 : 0; //avoid infinite slope
        CastX(xC, yC, ang, xHit, yHit);
        CastY(xC, yC, ang, xY, yY);
        if (abs(xC - xY) < abs(xC - xHit)) //choose the nearest hit point
            xHit = xY, yHit = yY;

        int h = int(sqRes * sqrt((viewerToScreen_sq + sq(screenWh - col)) / (float)(sq(xC - xHit) + sq(yC - yHit) + 1))); //+1 avoids division by zero
        int Dh_fp = (sqRes << 10) / h; //1 row in screen space represents this many rows in texture space; use fixed point
        int textureRow_fp = 0;
        int minRow = ((100 - elevation_perc) * (2 * screenHh - h) / 2 + elevation_perc * screenHh) / 100;
        int maxRow = min(minRow + h, screenH);
        if (minRow < 0) { //clip
            textureRow_fp = -(minRow * Dh_fp);
            minRow = 0;
        }

        for (int row = minRow; row < maxRow; row++, textureRow_fp += Dh_fp)
            screen[row][col] = *(Texture + (textureRow_fp >> 10) * sqRes + (xHit + yHit) % sqRes); //textureColumn = (xHit + yHit) % sqRes
    }

    for (int row = 0; row < screenH; row++)
        screen[row][screenW] = '\n';

    //flush the screen matrix onto the real screen
    system("cls"); //clear the (real) screen
    screen[screenH - 1][screenW - 1] = 0; //avoid scrolling one row up when the screen is full
    printf("%s", (char*)screen);
    Sleep(6); //tune this one to get less flickering on a specific PC
}

int main() {
    //generate texture
    for (int i = 0; i < sqRes; i++)
        for (int j = 0; j < sqRes; j++)
            *(Texture + i*sqRes + j) = ((0.1*sqRes < i) && (i < 0.7*sqRes) && (0.2*sqRes < j) && (j < 0.8*sqRes)) ? 'O' : '*';

    while (1)
        if (loopController(xC, yC, angleC, around))
            Render();

    return 0;
}