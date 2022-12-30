#include "pch.h"
#include <conio.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>
#include "Controller.h"

#define MOVE_SPD 15
#define ROTATE_SPD 5

void loopController(int& x, int& y, int& angle, int around)
{
    float rad = angle * 6.2831f / around;
    if (_kbhit()) { //check user's input
        unsigned char ch = _getch();
        if (ch == 27) //ASCII code for the Esc key
            exit(0); //end the game
        if (tolower(ch) == 'w') { //the up arrow key => pedal forward
            x += int(MOVE_SPD * cos(rad));
            y += int(MOVE_SPD * sin(rad));
        }
        if (tolower(ch) == 's') { //the down arrow key => pedal backward
            x -= int(MOVE_SPD * cos(rad));
            y -= int(MOVE_SPD * sin(rad));
        }
        if (tolower(ch) == 'a') { //strafe left
            x += int(MOVE_SPD * cos(rad + 1.57));
            y += int(MOVE_SPD * sin(rad + 1.57));
        }
        if (tolower(ch) == 'd') { //strafe right
            x += int(MOVE_SPD * cos(rad - 1.57));
            y += int(MOVE_SPD * sin(rad - 1.57));
        }
        if (ch == 224) { //it's a key that generates two bytes when being pressed, the first one being 224
            ch = _getch();
            if (ch == 75) //the left arrow key => do turn left
                angle = (angle + ROTATE_SPD) % around;
            if (ch == 77) //the right arrow key => do turn right
                angle = (angle - ROTATE_SPD + around) % around;
        }
    } //if (_kbhit())
}
