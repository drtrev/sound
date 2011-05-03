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

#include "menu.h"

Menu::Menu()
: x(0), y(0), width(100), height(100), alpha(1), buttonNum(0), scrollY(0), scrollbarWidth(14), scrollable(false)
{
}

Menu::Menu(float sx, float sy, float swidth, float sheight, float salpha)
: x(sx), y(sy), width(swidth), height(sheight), alpha(salpha), buttonNum(0), scrollY(0), scrollbarWidth(14), scrollable(false)
{
}

void Menu::set(float sx, float sy, float swidth, float sheight, float salpha)
  // only constructors take base initializers so this needs to be done by hand!
{
  x = sx, y = sy, width = swidth, height = sheight, alpha = salpha;
}

void Menu::setX(float nx)
{
  x = nx;
}

void Menu::setY(float ny)
{
  y = ny;
}

void Menu::setWidth(float nw)
{
  width = nw;
}

void Menu::setHeight(float nh)
{
  height = nh;
}

void Menu::setAlpha(float a)
{
  alpha = a;
}

float Menu::getX()
{
  return x;
}

float Menu::getY()
{
  return y;
}

float Menu::getWidth()
{
  return width;
}

float Menu::getHeight()
{
  return height;
}

float Menu::getAlpha()
{
  return alpha;
}

void Menu::drawText(string str, float x, float y)
{
  glRasterPos2f(x, y);
  for(unsigned int s = 0; s < str.length(); s++)
  {
    // this moves raster position on by the right amount
    //glutBitmapCharacter(GLUT_BITMAP_9_BY_15, str[s]);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, str[s]);
    //x += 10.5; // useful to know char width
  }
}

void Menu::drawTick(float x, float y)
{
  glBegin(GL_LINES);
  glVertex2f(x-2, y-2);
  glVertex2f(x,y);

  glVertex2f(x,y);
  glVertex2f(x+9, y-9);
  glEnd();
}

void Menu::drawTriangle(int n, float x, float y)
{ 
  float red = 0.7, green = 0.4, blue = 0.2;
  if (n == 1) { red = 0.3, green = 0.1, blue = 0.8; }
  glColor4f(red, green, blue, alpha);

  glBegin(GL_TRIANGLES);
  glVertex2f(x, y);
  glVertex2f(x+4, y-4);
  glVertex2f(x-4, y-4);
  glEnd();
}

void Menu::drawButton(int n, int params)
{
  int scrollWidth = (scrollable) ? scrollbarWidth : 0;
  glColor4f(1.0, 1.0, 1.0, alpha);
  if (button[n].mouseOver) { // highlight
    glBegin(GL_QUADS);
    glVertex2f(button[n].x, button[n].y);
    glVertex2f(button[n].x, button[n].bottom);
    glVertex2f(button[n].right - scrollWidth, button[n].bottom);
    glVertex2f(button[n].right - scrollWidth, button[n].y);
    glEnd();

    glColor4f(0.0, 0.0, 0.0, alpha);
  }
  if (button[n].played) {
    glBegin(GL_LINES);
    glVertex2f(button[n].x, button[n].y + CHAR_HEIGHT / 2 + 1);
    glVertex2f(button[n].right - 5 - scrollWidth, button[n].y + CHAR_HEIGHT / 2 + 1);
    glEnd();
  }
  if (button[n].id == BUTTON_SKIP && button[n].text == "") {
    drawTriangle(n, button[n].x + 4, button[n].y + 8);
  }else{
    drawText(button[n].text, button[n].x, button[n].y + CHAR_HEIGHT - 3);
  }

  if (params & button[n].id) drawTick(button[n].right - 10 - scrollWidth, button[n].bottom - 3);
}

void Menu::drawScrollbar()
{
  // if there are some buttons and the last one is below 'height'
  float scrollBottom = (buttonNum > 0 && button[buttonNum-1].y - height > 0)
    ? height - scrollbarWidth * 2 - 2 * CHAR_HEIGHT - button[buttonNum-1].y + height + scrollY
    : height - scrollbarWidth;

  int drawScrollY = scrollY;

  // minimum size
  if (scrollBottom - scrollY - scrollbarWidth < 10) {
    // find maximum scrollY
    if (buttonNum > 0 && button[buttonNum-1].y - height + scrollbarWidth + 2 * CHAR_HEIGHT > 0) {
      int maxScrollY = (int) (button[buttonNum-1].y - height + scrollbarWidth + 2 * CHAR_HEIGHT);
      if (maxScrollY > height - scrollbarWidth * 2 - 10) {
        //cout << "scrollY: " << scrollY;
        drawScrollY = (int) (drawScrollY * ((height - scrollbarWidth * 2 - 10) / (float) maxScrollY));
        //cout << "  drawScrollY: " << drawScrollY << endl;
      }
      scrollBottom = scrollbarWidth + drawScrollY + 10;
    }
  }

  // box up
  glColor4f(0.5, 0.5, 0.5, 0.7 * alpha);
  glBegin(GL_QUADS);
  glVertex2f(width - scrollbarWidth, scrollbarWidth);
  glVertex2f(width, scrollbarWidth);
  glVertex2f(width, 0);
  glVertex2f(width - scrollbarWidth, 0);
  glEnd();

  // arrow up
  glColor4f(1.0, 1.0, 1.0, alpha);
  glBegin(GL_TRIANGLES);
  glVertex2f(width - scrollbarWidth / 2, 3);
  glVertex2f(width - scrollbarWidth + scrollbarWidth / 10, scrollbarWidth - 3);
  glVertex2f(width - scrollbarWidth / 10, scrollbarWidth - 3);
  glEnd();

  // empty lack of bar
  glColor4f(0.3, 0.3, 0.3, 0.7 * alpha);
  glBegin(GL_QUADS);
  glVertex2f(width - scrollbarWidth, height - scrollbarWidth);
  glVertex2f(width, height - scrollbarWidth);
  glVertex2f(width, scrollbarWidth);
  glVertex2f(width - scrollbarWidth, scrollbarWidth);
  glEnd();

  // bar
  glColor4f(0.7, 0.7, 0.7, 0.7 * alpha);
  glBegin(GL_QUADS);
  glVertex2f(width - scrollbarWidth, drawScrollY + scrollbarWidth);
  glVertex2f(width - scrollbarWidth, scrollBottom);
  glVertex2f(width, scrollBottom);
  glVertex2f(width, drawScrollY + scrollbarWidth);
  glEnd();


  // box down
  glColor4f(0.5, 0.5, 0.5, 0.7 * alpha);
  glBegin(GL_QUADS);
  glVertex2f(width - scrollbarWidth, height - scrollbarWidth);
  glVertex2f(width - scrollbarWidth, height);
  glVertex2f(width, height);
  glVertex2f(width, height - scrollbarWidth);
  glEnd();

  // arrow down
  glColor4f(1.0, 1.0, 1.0, alpha);
  glBegin(GL_TRIANGLES);
  glVertex2f(width - scrollbarWidth / 2, height - 3);
  glVertex2f(width - scrollbarWidth / 10, height - scrollbarWidth + 3);
  glVertex2f(width - scrollbarWidth + scrollbarWidth / 10, height - scrollbarWidth + 3);
  glEnd();
}

void Menu::draw(int params)
{
  glPushMatrix();
  glTranslatef(x, y, 0);
  glColor4f(0.2, 0.2, 0.2, 0.5*alpha);
  glBegin(GL_QUADS);
  glVertex2f(0, 0);
  glVertex2f(0, height);
  glVertex2f(width, height);
  glVertex2f(width, 0);
  glEnd();

  glPushMatrix();
  glTranslatef(0, -scrollY, 0);
  for (int i = 0; i < buttonNum; i++) {
    if (button[i].y - scrollY > 0 && button[i].y - scrollY < height - 18) drawButton(i, params);
  }
  glPopMatrix();

  if (scrollable) drawScrollbar();

  glPopMatrix();
}

void Menu::addButton(ButtonEnum id, string str, float bx, float by)
{
  if (buttonNum < BUTTON_MAX) {
    button[buttonNum].id = id;
    button[buttonNum].text = str;
    button[buttonNum].x = bx;
    button[buttonNum].y = by;
    button[buttonNum].right = width - 5;
    button[buttonNum].bottom = by + CHAR_HEIGHT;
    button[buttonNum].mouseOver = false;
    button[buttonNum].played = false;

    buttonNum++;
  }else{
    cerr << "Error, reached maximum number of buttons" << endl;
  }
}

void Menu::removeButtons()
{
  buttonNum = 0;
}

void Menu::changeButton(int n, string str)
{
  button[n].text = str;
}

string Menu::getButtonText(int n)
{
  return button[n].text;
}

void Menu::moveButton(int n, float nx, float ny)
{
  button[n].x = nx, button[n].y = ny;
  button[n].bottom = ny + CHAR_HEIGHT;
}

void Menu::setPlayed(int n, bool b)
{
  button[n].played = b;
}

bool Menu::getPlayed(int n)
{
  return button[n].played;
}

void Menu::highlightButtons(int mx, int my)
{
  my += scrollY;

  int scrollWidth = (scrollable) ? scrollbarWidth : 0;
  for (int i = 0; i < buttonNum; i++) {
    if (mx - x > button[i].x && mx - x < button[i].right - scrollWidth
        && my - y > button[i].y && my - y < button[i].bottom + 1)
    {
      button[i].mouseOver = true;
    }else{
      button[i].mouseOver = false;
    }
  }
}

bool Menu::checkOver(int mx, int my)
  // check if mouse is over menu
{
  if (mx - x < width && mx - x > 0
      && my - y < height && my - y > 0)
  {
    return true;
  }else{
    return false;
  }
}

ButtonEnum Menu::checkOverButton(int mx, int my)
  // Check if mouse is over a button.
  // Return number of button
{
  my += scrollY;

  for (int i = 0; i < buttonNum; i++) {
    if (button[i].mouseOver) return button[i].id;
  }
  return BUTTON_NONE;
}

int Menu::checkOverButtonNum(int mx, int my)
  // Check if mouse is over a button.
  // Return number of button
{
  my += scrollY;

  for (int i = 0; i < buttonNum; i++) {
    if (button[i].mouseOver) return i;
  }
  return -1;
}

bool Menu::checkOverScrollbar(int mx, int my)
{
  if (mx - x > width - scrollbarWidth && mx - x < width + 1 && my - y > scrollbarWidth && my - y < height - scrollbarWidth) {
    return true;
  }
  return false;
}

void Menu::setScrollable(bool b)
{
  scrollable = b;
}

int Menu::scroll(int n)
{
  scrollY += n;
  
  // if it's not scrollable then this must be program calling this so it can do what it wants,
  // as opposed to when a user is in control!
  if (scrollable) {
    if (scrollY < 0) scrollY = 0;
    // allow to scroll one line too many so you can drag playlist items to the bottom
    if (buttonNum > 0 && button[buttonNum-1].y - height + scrollbarWidth + 2 * CHAR_HEIGHT > 0) {
      if (scrollY > button[buttonNum-1].y - height + scrollbarWidth + 2 * CHAR_HEIGHT) scrollY = (int) (button[buttonNum-1].y - height + scrollbarWidth + 2 * CHAR_HEIGHT);
    }else scrollY = 0;
  }

  return scrollY; // so program can check if scroll was a success
}

void Menu::scrollTo(int n)
{
  scrollY = n;
}

int Menu::getScrollY()
{
  return scrollY;
}
