#include "pch.h"
#include <math.h>
#include <stdio.h>
#include <windows.h>
#include "Config.h"
#include "Controller.h"
#include "Map.h"

char screen[screenH][screenW + 1] = {{0}}; //we'll paint everything in this matrix, then flush it onto the real screen
char Texture[sqSide*sqSide];

void CastX(int xC, int yC, int ang, int& xHit, int& yHit) { //   hit vertical walls ||
    //prepare as for 1st or 4th quadrant
    xHit = (xC / sqSide) * sqSide + sqSide;
	int dx = sqSide,   adjXMap = 0;
    int dy = int(sqSide * tanf(ang * 3.1416f / aroundh));
    if ((aroundq < ang) && (ang < 3 * aroundq)) { //2nd or 3rd quadrant
        xHit -= sqSide;
        adjXMap = -1;
        dx = -dx;
        dy = -dy;
    }
    yHit = yC + int((xHit - xC) * tanf(ang * 3.1416f / aroundh));

    while ((0 < xHit) && (xHit < mapSizeWidth) && (0 < yHit) && (yHit < mapSizeHeight) && (Map[yHit / sqSide][xHit / sqSide + adjXMap] == 0))
        xHit += dx, yHit += dy;
}

void CastY(int xC, int yC, int ang, int& xHit, int& yHit) { //   hit horizontal walls ==
    //prepare as for 1st or 2nd quadrant
    yHit = (yC / sqSide) * sqSide + sqSide;
	int dy = sqSide,   adjYMap = 0;
    int dx = int(sqSide / tanf(ang * 3.1416f / aroundh));
    if (ang > aroundh) { //3rd or 4th quadrants
        yHit -= sqSide;
        adjYMap = -1;
        dy = -dy;
        dx = -dx;
    }
    xHit = xC + int((yHit - yC) / tanf(ang * 3.1416f / aroundh));

    while ((0 < xHit) && (xHit < mapSizeWidth) && (0 < yHit) && (yHit < mapSizeHeight) && (Map[yHit / sqSide + adjYMap][xHit / sqSide] == 0))
        xHit += dx, yHit += dy;
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

        int h = int(sqSide * sqrt((viewerToScreen_sq + sq(screenWh - col)) / (float)(sq(xC - xHit) + sq(yC - yHit))));
        int Dh_fp = (sqSide << 10) / h; //1 row in screen space represents this many rows in texture space; use 10 bits fixed point
        int textureRow_fp = 0;
        int minRow = screenHh - h / 2;
        if (minRow < 0) {
            textureRow_fp = -minRow * Dh_fp;
            minRow = 0;
        }
        int maxRow = min(screenHh + h / 2, screenH);

        for (int row = minRow; row < maxRow; row++, textureRow_fp += Dh_fp)
            screen[row][col] = *(Texture + (textureRow_fp >> 10) * sqSide + (xHit + yHit) % sqSide); //textureColumn = (xHit + yHit) % sqSide
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
    for (int i = 0; i < sqSide; i++)
        for (int j = 0; j < sqSide; j++)
            *(Texture + i*sqSide + j) = ((0.1*sqSide < i) && (i < 0.7*sqSide) && (0.2*sqSide < j) && (j < 0.8*sqSide)) ? 'O' : '*';

    while (1) {
        Render();
        loopController(xC, yC, angleC, around);
    }

    return 0;
}