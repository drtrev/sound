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

#ifndef GRAPHICS_OPENGL_H
#define GRAPHICS_OPENGL_H

#include "graphics.h"

// define NOOGLFT to compile without this library

#ifndef _MSC_VER
#ifndef NOOGLFT
#define OGLFT_NO_SOLID // don't use GLE (OpenGL Tubing and Extrusion library)
#define OGLFT_NO_QT // don't use QT (which is for glyphs)
#include <OGLFT.h>
#endif
#endif

#include <vector>

class Bullet; class Outverbose;

/**
 * The implementation of Graphics to support OpenGL output.
 */
class GraphicsOpenGL : public Graphics {
  private:
    /**
     * Struct to define the view frustum.
     *
     * \param topb the top border.
     * \param rightb the right border.
     * \param nearb the near border (had to add 'b' to the end because 'near' and 'far' are keywords in some compilers).
     * \param farb the far border.
     */
    struct Frustum {
      float topb;
      float rightb;
      float nearb; // near is a keyword in msvc++ so added 'b' for border
      float farb;
    } frustum;

    /**
     * Store the square border (in terms of visible space) for full screen pictures.
     *
     * \param top the top border.
     * \param right the right border.
     * \param bottom the bottom border.
     * \param left the left border.
     */
    struct Border {
      float top;
      float right;
      float bottom;
      float left;
    } border;

    // see demos and http://oglft.sourceforge.net/ for a review of FPS rates and appearance for each type
    // NOTE if change this here, then need to change font initialisation in init()

#ifndef _MSC_VER
#ifndef NOOGLFT
    //OGLFT::Translucent* face; // this one looks best, but is slow
    /**
     * The font face.
     */
    OGLFT::TranslucentTexture* face; // this is faster
#endif
#endif

    /**
     * The names of display lists, ending with DLIST_END (for array size below).
     */
    enum DisplayLists { DLIST_MAP, DLIST_END };
    int displayList[DLIST_END]; /**< ID's of display lists. */

    void flip(); /**< Flip image. */

  public:
    GraphicsOpenGL(); /**< Constructor. */

    // doxygen comments from graphics.h are used here automatically
    bool init(Outverbose &o, WindowInfo w, const char* font, int fontsize);
    void setupLights();
    void setLightPositions();
    void kill();

    void drawStart();
    void drawThumbStart();

    void drawText(GraphicsInfo g);
    void drawViewbox(double posX, double posY, double scale, double thumbOffsetX, double thumbOffsetY, double thumbScale, float r, float g, float b, float a);
    void drawStop();
    void drawThumbStop();

    void refresh();

    float getWidth();
    int getWindowWidth();
    int getWindowHeight();
};

#endif
