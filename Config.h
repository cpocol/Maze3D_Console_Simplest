#pragma once

#define USE_MULTIPLE_KEYS_SIMULTANEOUSLY

#define MOVE_SPD (10 * sqRes / 100)
#define ROTATE_SPD 4
#define VERTICAL_SPD (sqRes / 25)

const int screenW = 80, screenH = 25, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4, around3q = 3 * aroundq; //FOV = 60 degs (6 FOVs = 360 degrees)

const int sqRes = 128, sqResh = sqRes / 2; //must be the size of Texture
const int mapWidth = 14, mapHeight = mapWidth;
const int mapSizeHeight = mapHeight * sqRes, mapSizeWidth = mapWidth * sqRes;
const int safety_dist = 3; //to wall

//viewer Current position, orientation and elevation_perc
extern int xC;
extern int yC;
extern int angleC;
extern int elevation_perc; //percentage

#define sq(x) ((x)*(x))
