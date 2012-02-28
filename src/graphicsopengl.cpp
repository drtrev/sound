/* This file is part of Map - A tool for viewing 2D map data using Motlab (a
 * heterogeneous collaborative visualization library).
 *
 * Copyright (C) 2009 Trevor Dodds <trev@comp.leeds.ac.uk>
 *
 * Map is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 * 
 * Map is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * Map.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id$
 */

#include "graphicsopengl.h"
#ifdef _MSC_VER
  #include <windows.h>
  #include <GL/glu.h>
//  #include "map.h" // for polygon
#endif
#include <GL/gl.h>
//#include "windowgenglut.h"
#include "outverbose.h"
#include "windowgensdl.h"
#include "ogg.h"
#include "types.h"

using std::cout;
using std::cerr;
using std::endl;
using namespace sound;

// testing - didn't seem to help
//extern "C" {
//void vertexCallback(GLvoid *vertex);
//void combineCallback(GLdouble coords[3], GLdouble *vertex_data[4],
//    GLfloat weight[4], GLdouble **dataOut);
//}

GraphicsOpenGL::GraphicsOpenGL()
{
  frustum.nearb = 0.1;
  frustum.farb = 100;
  frustum.topb = 10; // calculate properly when window is opened in init
  frustum.rightb = 10;

  border.top = 100, border.right = 100, border.bottom = 100, border.left = 100;

  for (int i = 0; i < DLIST_END; i++) displayList[i] = 0; // not built any lists yet

}

bool GraphicsOpenGL::init(Outverbose &o, WindowInfo w, const char* font, int fontsize)
{
  initShared(o);

#ifndef _MSC_VER
#ifndef NOOGLFT
  // initialise font
  face = new OGLFT::TranslucentTexture( font, fontsize );
  face->setHorizontalJustification( OGLFT::Face::CENTER );
  face->setVerticalJustification( OGLFT::Face::MIDDLE );
#endif
#endif
  //face->setForegroundColor( 0.75, 1., .75, 1. ); -- set in drawText
  //face->setCharacterRotationX( 0 );
  //face->setCharacterRotationY( 0 );
  //face->setCharacterRotationZ( 0 );
  //face->setStringRotation( 0 );

  //window = new WindowGlut;
  window = new WindowSDL;

  if (window->init(*out, w)) {
    frustum.topb = frustum.nearb; // make same as near plane for 90 degree vertical FOV
    frustum.rightb = frustum.topb * (float) window->getWidth() / window->getHeight();

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glFrustum(-frustum.rightb, frustum.rightb, -frustum.topb, frustum.topb, frustum.nearb, frustum.farb);

    glMatrixMode (GL_MODELVIEW);

    // initialise openGL
    glViewport(0, 0, window->getWidth(), window->getHeight());
    //*out << VERBOSE_LOUD << "width: " << window->getWidth() << ", height: " << window->getHeight() << '\n';

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glCullFace(GL_FRONT); // if we mirror it
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    // typical usage: modify incoming colour by it's alpha, and existing colour by 1 - source alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // make lighting use glColor:
    // see man glColorMaterial
    glEnable(GL_COLOR_MATERIAL);

    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    // lights
    setupLights();
    return true; // success

  }else{
    return false;
  }
}

void GraphicsOpenGL::setupLights()
{
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  float ambient[4] = { .8, .8, .8, 1 };
  float diffuse[4] = { .8, .8, .8, 1 };
  float specular[4] = { 0, 0, 0, 1 };
  glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  setLightPositions();
}

void GraphicsOpenGL::setLightPositions()
{
  float pos[4] = { -0.4, 1, 0.7, 0 }; // directional, w = 0
  glLightfv(GL_LIGHT0, GL_POSITION, pos);
}

void GraphicsOpenGL::kill()
{
  cout << "Killing graphics..." << endl;
  if (window->getWid() > -1) window->destroy();
  delete window;
#ifndef _MSC_VER
#ifndef NOOGLFT
  delete face;
#endif
#endif
  for (int i = 0; i < DLIST_END; i++) {
    if (displayList[i] > 0) glDeleteLists(displayList[i], 1);
  }
}

void GraphicsOpenGL::drawStart()
{
  //glDisable(GL_DEPTH_TEST);
  glViewport(0, 0, window->getWidth(), window->getHeight());
  glClearColor(0.5, 0.4, 0.6, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  //glScalef(1, -1, 1);
  //glTranslatef(-out->getWidth(), -out->getWidth(), -out->getWidth()); // move camera out
  glTranslatef(0, -1, -10);
  glRotatef(30, 1, 0, 0);

  /*float zoom = 128;
  glTranslatef(0, 0, -zoom);

  if (window->getWidth() > window->getHeight()) {
    border.top = zoom;
    border.bottom = -border.top;
    border.right = border.top * window->getWidth() / (float) window->getHeight();
    border.left = -border.right;
  }

  glNormal3f(0, 0, 1);

  glColor4f(0, 0, 0, 1);
  glBegin(GL_QUADS);
  glVertex3f(border.left, border.bottom, 0);
  glVertex3f(border.right, border.bottom, 0);
  glVertex3f(border.right, border.top, 0);
  glVertex3f(border.left, border.top, 0);
  glEnd();*/

  // draw axis (temporarily to help)
  /*glColor4f(1, 1, 1, 1);
  glBegin(GL_LINES);
  glNormal3f(0, 0, 1);
  glVertex3f(0, -50, 0);
  glVertex3f(0, 50, 0);

  glVertex3f(-50, 0, 0);
  glVertex3f(50, 0, 0);
  glEnd();*/
  /*glBegin(GL_QUADS); // testing
  glVertex3f(-100, -100, -10);
  glVertex3f(100, -100, -10);
  glVertex3f(100, 100, -10);
  glVertex3f(-100, 100, -10);
  glEnd();*/
}

void GraphicsOpenGL::drawThumbStart()
{
  glDisable(GL_DEPTH_TEST);
  glViewport((int) (window->getWidth() / 4.0 * 3), (int) (window->getHeight() / 4.0 * 3), (int) (window->getWidth() / 4.0), (int) (window->getHeight() / 4.0));
  //*out << VERBOSE_LOUD << "width: " << window->getWidth() << ", height: " << window->getHeight() << '\n';

  //glClearColor(0, 0, 0, 1);
  //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glPushMatrix();

  float zoom = 128;
  glTranslatef(0, 0, -zoom);

  if (window->getWidth() > window->getHeight()) {
    border.top = zoom;
    border.bottom = -border.top;
    border.right = border.top * window->getWidth() / (float) window->getHeight();
    border.left = -border.right;
  }

  glNormal3f(0, 0, 1);

  glColor4f(0, 0, 0, 1);
  glBegin(GL_QUADS);
  glVertex3f(border.left, border.bottom, 0);
  glVertex3f(border.right, border.bottom, 0);
  glVertex3f(border.right, border.top, 0);
  glVertex3f(border.left, border.top, 0);
  glEnd();

}

void GraphicsOpenGL::drawObject(sound::Vector pos, sound::Vector rot, sound::Vector size, Colour col)
{
  glPushMatrix();
  glTranslatef(pos.x, pos.y, pos.z);
  glRotatef(rot.x, 1, 0, 0);
  glRotatef(rot.y+180, 0, 1, 0); // draw facing -z
  glRotatef(rot.z, 0, 0, 1);

  glColor4f(col.red, col.green, col.blue, col.alpha);

  size.x /= 2.0, size.y /= 2.0, size.z /= 2.0;

  glBegin(GL_TRIANGLES);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(-size.x, size.y, size.z); // top
  glVertex3f(size.x, size.y, size.z);
  glVertex3f(0, size.y, -size.z);

  glNormal3f(0.0, -1.0, 0.0);
  glVertex3f(-size.x, -size.y, size.z); // bottom
  glVertex3f(0, -size.y, -size.z);
  glVertex3f(size.x, -size.y, size.z);
  glEnd();

  glBegin(GL_QUADS);
  glNormal3f(-0.7, 0.0, -0.7); // left
  glVertex3f(0, -size.y, -size.z);
  glVertex3f(-size.x, -size.y, size.z);
  glVertex3f(-size.x, size.y, size.z);
  glVertex3f(0, size.y, -size.z);

  glNormal3f(0.7, 0.0, -0.7); // right
  glVertex3f(size.x, -size.y, size.z);
  glVertex3f(0, -size.y, -size.z);
  glVertex3f(0, size.y, -size.z);
  glVertex3f(size.x, size.y, size.z);

  glNormal3f(0.0, 0.0, 1.0); // Back
  glVertex3f(-size.x, -size.y, size.z);
  glVertex3f(size.x, -size.y, size.z);
  glVertex3f(size.x, size.y, size.z);
  glVertex3f(-size.x, size.y, size.z);
  glEnd();

  glPopMatrix();
}

void GraphicsOpenGL::drawSources(int nogg, ogg_stream ogg[])
{
  float x = 0, y = 0, z = 0;
  //float angleX = 0;
  float angleY = 0;
  //float angleZ = 0;
  //
  Vector pos(0, 0, 0);
  Vector rot(0, 180, 0);
  Vector size(0.4, 0.4, 0.8);
  Colour col(0.8, 0.8, 0.8, 1.0);
  drawObject(pos, rot, size, col);

  for (int i = 0; i < nogg; i++) {
    ogg[i].getPosition(x, y, z);

    if (ogg[i].getSpeedX() > 0) {
      if (ogg[i].getSpeedZ() > 0) {
        angleY = deg(atan(ogg[i].getSpeedX()/ogg[i].getSpeedZ()));
      }else if (ogg[i].getSpeedZ() < 0) {
        angleY = 90+deg(atan(-ogg[i].getSpeedZ()/ogg[i].getSpeedX()));
      }else angleY = 90;
    }else if (ogg[i].getSpeedX() < 0) {
      if (ogg[i].getSpeedZ() < 0) {
        angleY = 180+deg(atan(ogg[i].getSpeedX()/ogg[i].getSpeedZ()));
      }else if (ogg[i].getSpeedZ() > 0) {
        angleY = 270+deg(atan(ogg[i].getSpeedZ()/-ogg[i].getSpeedX()));
      }else angleY = -90;
    }else{
      if (ogg[i].getSpeedZ() < 0) angleY = 180;
      else angleY = 0;
    }


    //cout << "angleY: " << angleY << endl;
    //angleX = radToDeg(atan(ogg.getSpeedY()/ogg.getSpeedZ()));

    Vector pos(ogg[i].getX(), ogg[i].getY(), ogg[i].getZ());
    Vector rot(0, angleY, 0);
    Vector size(0.4, 0.4, 0.8);
    Colour col(0.2, 0.2, 0.8, 1.0);
    drawObject(pos, rot, size, col);
  }
}

void GraphicsOpenGL::flip()
// call pushMatrix and pushAttrib(GL_POLYGON_BIT) (for GL_CULL_FACE_MODE) first
{
  glCullFace(GL_FRONT);
  glScalef(2, -4, 1);
  glTranslatef(border.left / 2.0 + 2, -border.top / 4.0 + 3, 1);
}

void GraphicsOpenGL::drawText(GraphicsInfo g)
{
#ifndef _MSC_VER
#ifndef NOOGLFT
  if (g.visible) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glPushMatrix();
    glTranslatef(g.x, g.y, g.z);
    glTranslatef(0, 0, g.pivotZ); // change pivot point
    glRotatef(g.angleX, 1, 0, 0);
    glRotatef(g.angleY, 0, 1, 0);
    glRotatef(g.angleZ, 0, 0, 1);
    glTranslatef(0, 0, -g.pivotZ); // change pivot point
    glScalef(0.5, 0.5, 0.5); // get higher resolution font by choosing large fontsize then halving rendering
    glScalef(g.scaleX, g.scaleY, g.scaleZ);
    glEnable(GL_TEXTURE_2D);

    face->setForegroundColor( g.color.red, g.color.green, g.color.blue, g.color.alpha );
    //face->draw(g.x, g.y, g.z, g.text.c_str());
    face->draw(0, 0, 0, g.text.c_str());

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
    glDisable(GL_BLEND);
  }
#endif
#endif
}

void GraphicsOpenGL::drawViewbox(double posX, double posY, double scale, double thumbOffsetX, double thumbOffsetY, double thumbScale, float r, float g, float b, float a)
  // TODO need to take size of other's window as parameter and calculate border accordingly
{
  //glPopMatrix();
  //glPushMatrix();
  //glTranslatef(0, 0, -128);
  //glScalef(1/scale, 1/scale, 0);
  //glTranslatef(-500, -500, 0);
  /*glLineWidth(2);
  glColor4f(1, 1, 0, 1);
  glBegin(GL_LINE_STRIP);
  glVertex3f(-posX + border.left, -posY + border.bottom, 0);
  glVertex3f(-posX + border.right, -posY + border.bottom, 0);
  glVertex3f(-posX + border.right, -posY + border.top, 0);
  glVertex3f(-posX + border.left, -posY + border.top, 0);
  glVertex3f(-posX + border.left, -posY + border.bottom, 0);
  glEnd();*/

  glPushMatrix();

  glScalef(thumbScale, thumbScale, 0);
  glScalef(1/scale, 1/scale, 0);
  glTranslatef(-posX*scale+thumbOffsetX*scale, -posY*scale+thumbOffsetY*scale, 0);

  // draw viewbox
  glLineWidth(2);
  glColor4f(r, g, b, a);
  glBegin(GL_LINE_STRIP);
  glVertex3f(border.left, border.bottom, 0);
  glVertex3f(border.right, border.bottom, 0);
  glVertex3f(border.right, border.top, 0);
  glVertex3f(border.left, border.top, 0);
  glVertex3f(border.left, border.bottom, 0);
  glEnd();

  glPopMatrix();

}

void GraphicsOpenGL::drawStop()
{
  glPopMatrix();
  //glEnable(GL_DEPTH_TEST);
}

void GraphicsOpenGL::drawThumbStop()
{
  glPopMatrix();
  glEnable(GL_DEPTH_TEST);
}

void GraphicsOpenGL::refresh()
{
  window->refresh();
}

float GraphicsOpenGL::getWidth()
{
  return border.right - border.left;
}

int GraphicsOpenGL::getWindowWidth()
{
  return window->getWidth();
}

int GraphicsOpenGL::getWindowHeight()
{
  return window->getHeight();
}

