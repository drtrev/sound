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
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <vector>
#include "windowgen.h"

/**
 * A struct to hold a colour.
 *
 * \param red the amount of red from 0 to 1.
 * \param green the amount of green from 0 to 1.
 * \param blue the amount of blue from 0 to 1.
 * \param alpha the amount of alpha from 0 to 1.
 */
struct Colour {
  float red;
  float green;
  float blue;
  float alpha;

  Colour() : red(0), green(0), blue(0), alpha(1) {}
  Colour(float _red, float _green, float _blue, float _alpha) : red(_red), green(_green), blue(_blue), alpha(_alpha) {}
};

/**
 * A struct to hold typical graphics information about an object.
 *
 * \param x the x coordinate.
 * \param y the y coordinate.
 * \param z the z coordinate.
 * \param width the width of the object.
 * \param height the height of the object.
 * \param depth the depth of the object.
 * \param angleX the angle of rotation about the x axis.
 * \param angleY the angle of rotation about the y axis.
 * \param angleZ the angle of rotation about the z axis.
 * \param pivotX the x coordinate to pivot rotations around.
 * \param pivotY the y coordinate to pivot rotations around.
 * \param pivotZ the z coordinate to pivot rotations around.
 * \param scaleX the scale factor for the x axis.
 * \param scaleY the scale factor for the y axis.
 * \param scaleZ the scale factor for the z axis.
 * \param colour the colour of the object.
 * \param texture an ID for the object's texture.
 * \param text a string of text associated with the object.
 * \param visible specify whether the object should be rendered.
 */
struct GraphicsInfo {
  float x;
  float y;
  float z;
  float width;
  float height;
  float depth;
  float angleX;
  float angleY;
  float angleZ;
  float pivotX;
  float pivotY;
  float pivotZ;
  float scaleX;
  float scaleY;
  float scaleZ;

  Colour colour;

  int texture;
  std::string text;
  bool visible;
};

class Outverbose; class ogg_stream;

/**
 * The graphics interface.
 *
 * This can be implemented to run using different outputs, e.g. OpenGL, or
 * text-based.
 */
class Graphics {
  protected:
    Outverbose* out;   /**< The output instance. */
    WindowGen* window; /**< The window instance. */

  public:
    virtual ~Graphics(); /**< Destructor. */

    void initShared(Outverbose&); /**< Initialise things that are relevant to all graphics implementations. */

    /**
     * Initialisation.
     *
     * \param o output instance.
     * \param w window information.
     * \param font the name of the font used for rendering text.
     * \param fontsize the size of the font used for rendering text.
     * \return true on success, false on failure (e.g. failed to open window)
     */
    virtual bool init(Outverbose &o, WindowInfo w, const char* font, int fontsize) = 0;

    virtual void kill() = 0; /**< Kill the graphics. Close the window. Delete the font object. Delete display lists. */

    /**
     * Make a colour.
     *
     * \param red the amount of red from 0 to 1.
     * \param green the amount of green from 0 to 1.
     * \param blue the amount of blue from 0 to 1.
     * \param alpha the amount of alpha from 0 to 1.
     * \return the Colour.
     */
    Colour makeColour(float red, float green, float blue, float alpha);

    /**
     * Make graphics information.
     *
     * \param x the x coordinate.
     * \param y the y coordinate.
     * \param z the z coordinate.
     * \param width the width of the object.
     * \param height the height of the object.
     * \param depth the depth of the object.
     * \param angleX the angle of rotation about the x axis.
     * \param angleY the angle of rotation about the y axis.
     * \param angleZ the angle of rotation about the z axis.
     * \param pivotX the x coordinate to pivot rotations around.
     * \param pivotY the y coordinate to pivot rotations around.
     * \param pivotZ the z coordinate to pivot rotations around.
     * \param scaleX the scale factor for the x axis.
     * \param scaleY the scale factor for the y axis.
     * \param scaleZ the scale factor for the z axis.
     * \param colour the colour of the object.
     * \param texture an ID for the object's texture.
     * \param text a string of text associated with the object.
     * \param visible specify whether the object should be rendered.
     * \return the GraphicsInfo.
     */
    GraphicsInfo makeInfo(float x, float y, float z, float width, float height, float depth, float angleX, float angleY, float angleZ, float pivotX, float pivotY, float pivotZ, float scaleX, float scaleY, float scaleZ, Colour color, int texture, std::string text, bool visible);

    /**
     * Get a graphics info struct populated with default info.
     *
     * \return the GraphicsInfo.
     */
    GraphicsInfo defaultInfo();

    /**
     * Make the window info.
     *
     * \param x the x coordinate.
     * \param y the y coordinate.
     * \param width the window width.
     * \param height the window height.
     * \param doubleBuffer enable/disable double buffer.
     * \param depthBuffer enable/disable depth buffer.
     * \param refreshRate set refresh rate.
     * \param colorDepth set color depth.
     * \param fullscreen set fullscreen true/false.
     * \param title set window title.
     * \return WindowInfo struct.
     */
    WindowInfo makeWindowInfo(int x, int y, int width, int height, bool doubleBuffer, bool depthBuffer, int refreshRate, int colorDepth, bool fullscreen, std::string title);

    virtual void drawStart() = 0; /**< Perform any operations required before drawing begins. */
    virtual void drawThumbStart() = 0; /**< Perform any operations required before drawing thumbnail begins. */

    virtual void drawSources(int nogg, ogg_stream ogg[]) = 0;

    /**
     * Render some text. \param g the graphics information for rendering the
     * text. \see GraphicsInfo
     */
    virtual void drawText(GraphicsInfo g) = 0;

    virtual void drawViewbox(double posX, double posY, double scale, double thumbOffsetX, double thumbOffsetY, double thumbScale, float r, float g, float b, float a) = 0;

    virtual void drawStop() = 0; /**< Perform any operations that need to be done after drawing. */
    virtual void drawThumbStop() = 0; /**< Perform any operations that need to be done after drawing the thumbnail. */

    virtual void refresh() = 0; /**< Refresh the graphics output. */

    virtual float getWidth() = 0; /**< Return the width of the graphics output. \return the graphics width. */
    virtual int getWindowWidth() = 0;
    virtual int getWindowHeight() = 0;
};

#endif
