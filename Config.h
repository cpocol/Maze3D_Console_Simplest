#pragma once

const int screenW = 80, screenH = 25, screenWh = screenW / 2, screenHh = screenH / 2;
const int around = 6 * screenW, aroundh = around / 2, aroundq = around / 4, around3q = 3 * aroundq; //FOV = 60 degs (6 FOVs = 360 degrees)

const int sqSize = 128; //must be the size of Texture
const int mapWidth = 14, mapHeight = mapWidth;
const int mapSizeHeight = mapHeight * sqSize, mapSizeWidth = mapWidth * sqSize;

//initial viewer current position and orientation
int xC = int(2.5f * sqSize);
int yC = int(2.5f * sqSize);
int angleC = 10;

#define sq(x) ((x)*(x))
