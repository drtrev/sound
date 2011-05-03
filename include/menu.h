/* sound - 3D sound project
 *
 * Copyright (C) 2011 Trevor Dodds <@gmail.com trev.dodds>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MENU
#define MENU

#include <GL/glut.h>
#include <iostream>

using namespace std;

#define BUTTON_MAX 100 // maximum number of buttons
enum ButtonEnum { BUTTON_NONE = 0, BUTTON_MOVE = 1, BUTTON_FRICTION = 2, BUTTON_HOMING = 4, BUTTON_FLY_CENTRE = 8, BUTTON_FLY_LEFT = 16, BUTTON_FLY_RIGHT = 32, BUTTON_SKIP = 64, BUTTON_TIME = 128, BUTTON_CENTRE = 256, BUTTON_PAUSE = 512, BUTTON_AUTO_DJ = 1024, BUTTON_NEXT_SOURCE = 2048, BUTTON_REWIND_START = 4096, BUTTON_REWIND = 8192, BUTTON_FORWARD = 16384, BUTTON_SHUFFLE = 32768, BUTTON_UNCROSS_ALL = 65536, BUTTON_TIDY = 131072};

#define CHAR_WIDTH 10.5
#define CHAR_HEIGHT 15

class Menu
{
  private:
    float x, y, width, height;
    float alpha;

    struct Button {
      ButtonEnum id;
      string text;
      float x;
      float y;
      float right;
      float bottom;
      bool mouseOver;
      bool played;
    };
    //typedef struct Button Button; C++ declares an implicit typedef. This would be needed for C
    Button button[BUTTON_MAX];

    int buttonNum;

    int scrollY;
    int scrollbarWidth;
    bool scrollable;

  protected:
    void drawText(string, float, float);
    void drawButton(int, int);
    void drawTick(float, float);
    void drawTriangle(int, float, float);
    void drawScrollbar();

  public:
    Menu();
    Menu(float, float, float, float, float);
    void set(float, float, float, float, float);
    void setX(float);
    void setY(float);
    void setWidth(float);
    void setHeight(float);
    void setAlpha(float);
    float getX();
    float getY();
    float getWidth();
    float getHeight();
    float getAlpha();
    void draw(int);
    void addButton(ButtonEnum, string, float, float);
    void removeButtons();
    void changeButton(int, string);
    string getButtonText(int);
    void moveButton(int, float, float);
    void setPlayed(int, bool);
    bool getPlayed(int);
    void highlightButtons(int, int);
    bool checkOver(int, int);
    ButtonEnum checkOverButton(int, int);
    int checkOverButtonNum(int, int);
    bool checkOverScrollbar(int, int);
    void setScrollable(bool);
    int scroll(int);
    void scrollTo(int);
    int getScrollY();
};

#endif
