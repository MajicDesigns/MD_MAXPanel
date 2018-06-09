/*
MD_MAXPanel - Library for MAX7219/7221 LED Panel

See header file for comments

This file contains class and hardware related methods.

Copyright (C) 2018 Marco Colli. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <Arduino.h>
#include "MD_MAXPanel.h"
#include "MD_MAXPanel_lib.h"

/**
 * \file
 * \brief Implements class definition and graphics methods
 */

MD_MAXPanel::MD_MAXPanel(MD_MAX72XX::moduleType_t mod, uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint8_t xDevices, uint8_t yDevices):
_xDevices(xDevices), _yDevices(yDevices)
{
  _D = new MD_MAX72XX(mod, dataPin, clkPin, csPin, xDevices*yDevices);
  _killOnDestruct = true;
}

MD_MAXPanel::MD_MAXPanel(MD_MAX72XX::moduleType_t mod, uint8_t csPin, uint8_t xDevices, uint8_t yDevices) :
_xDevices(xDevices), _yDevices(yDevices)
{
  _D = new MD_MAX72XX(mod, csPin, xDevices*yDevices);
  _killOnDestruct = true;
}

MD_MAXPanel::MD_MAXPanel(MD_MAX72XX *D, uint8_t xDevices, uint8_t yDevices) :
_xDevices(xDevices), _yDevices(yDevices)
{
  _D = D;
  _killOnDestruct = false;
}

void MD_MAXPanel::begin(void)
{
  _D->begin();
  _charSpacing = CHAR_SPACING_DEFAULT;
  _updateEnabled = true;
}

MD_MAXPanel::~MD_MAXPanel(void)
{
  if (_killOnDestruct) delete _D;
}


bool MD_MAXPanel::drawHLine(uint16_t y, uint16_t x1, uint16_t x2, bool state = true)
// draw a horizontal line at row y between columns x1 and x2 inclusive
{
  bool b = true;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  if (x1 > x2)      // swap x1/x2
  {
    uint16_t  t = x1;
    x1 = x2;
    x2 = t;
  }

  for (uint16_t i = x1; i <= x2; i++)
    b &= setPoint(i, y, state);

  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::drawVLine(uint16_t x, uint16_t y1, uint16_t y2, bool state = true)
// draw a vertical line at column x between rows y1 and y2 inclusive
{
  bool b = true;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  if (y1 > y2)      // swap y1/y2
  {
    uint8_t  t = y1;
    y1 = y2;
    y2 = t;
  }

  for (uint8_t i = y1; i <= y2; i++)
    b &= setPoint(x, i, state);

  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool state = true)
// draw an arbitrary line between two points using Bresentham's line algorithm
// Bresentham's line algorithm at https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
{
  bool b = true;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  PRINT("\n\nLine from ", x1); PRINT(",", y1);
  PRINT(" to ", x2); PRINT(",", y2);

  if (x1 > x2)    // swap direction for line
  {
    uint16_t t;

    t = x1; x1 = x2; x2 = t;
    t = y1; y1 = y2; y2 = t;
    //    PRINTS(" SWAP X");
  }

  //  PRINT("\nPlotting from ", x1); PRINT(",", y1);
  //  PRINT(" to ", x2); PRINT(",", y2);

  int16_t dx = x2 - x1;
  int16_t sx = 1;
  int16_t dy = y2 - y1;
  if (dy < 0) dy = -dy;
  int16_t sy = y1 < y2 ? 1 : -1;
  int16_t err = (dx > dy ? dx : -dy) / 2;
  int16_t e2;

  //  PRINT("\ndx=", dx);
  //  PRINT(" dy=", dy);
  //  PRINT(" ystep=", sy);
  //  PRINT(" xstep=", sx);

  for (;;)
  {
    //    PRINT("\nerror=", err);
    //    PRINT(" [", x1); PRINT(",", y1); PRINTS("]");

    b &= setPoint(x1, y1, state);
    if (x1 == x2 && y1 == y2) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x1 += sx; }
    if (e2 < dy) { err += dx; y1 += sy; }
  }

  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, bool state = true)
// draw a rectangle given the 2 diagonal vertices
{
  bool b = true;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  b &= drawHLine(y1, x1, x2, state);
  b &= drawHLine(y2, x1, x2, state);
  b &= drawVLine(x1, y1, y2, state);
  b &= drawVLine(x2, y1, y2, state);
  
  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::drawQuadrilateral(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t x4, uint16_t y4, bool state = true)
// draw a arbitrary quadrilateral given the 4 corner vertices
{
  bool b = true;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  b &= drawLine(x1, y1, x2, y2, state);
  b &= drawLine(x2, y2, x3, y3, state);
  b &= drawLine(x3, y3, x4, y4, state);
  b &= drawLine(x4, y4, x1, y1, state);

  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, bool state = true)
// draw an arbitrary triangle given the 3 corner vertices
{
  bool b = true;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  b &= drawLine(x1, y1, x2, y2, state);
  b &= drawLine(x2, y2, x3, y3, state);
  b &= drawLine(x3, y3, x1, y1, state);

  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::drawCirclePoints(uint16_t xc, uint16_t yc, uint16_t x, uint16_t y, bool state)
// draw symmetrical circle points
{
  bool b = true;

  b &= setPoint(xc + x, yc + y, state);
  b &= setPoint(xc - x, yc + y, state);
  b &= setPoint(xc + x, yc - y, state);
  b &= setPoint(xc - x, yc - y, state);
  b &= setPoint(xc + y, yc + x, state);
  b &= setPoint(xc - y, yc + x, state);
  b &= setPoint(xc + y, yc - x, state);
  b &= setPoint(xc - y, yc - x, state);

  return(b);
}

bool MD_MAXPanel::drawCircle(uint16_t xc, uint16_t yc, uint16_t r, bool state = true)
// draw a circle given center and radius
// Bresenhams Algorith from http://www.pracspedia.com/CG/bresenhamcircle.html
{
  int x = 0, y = r;
  int pk = 3 - (2 * r);
  bool b = false;

  _D->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  PRINT("\n\nCircle center ", xc); PRINT(",", yc); PRINT(" radius ", r);

  b &= drawCirclePoints(xc, yc, x, y, state);

  while (x < y)
  {
    // check for decision parameter and correspondingly update pk, x, y
    if (pk <= 0)
    {
      pk = pk + (4 * x) + 6;
      b &= drawCirclePoints(xc, yc, ++x, y, state);
    }
    else
    {
      pk = pk + (4 * (x - y)) + 10;
      b &= drawCirclePoints(xc, yc, ++x, --y, state);
    }
  }

  update(_updateEnabled);

  return(b);
}

bool MD_MAXPanel::getPoint(uint16_t x, uint16_t y)
{
  if (x > getXMax() || y > getYMax())
    return(false);

  return(_D->getPoint(Y2ROW(x,y), X2COL(x,y)));
}

bool MD_MAXPanel::setPoint(uint16_t x, uint16_t y, bool state = true)
{
  if (x > getXMax() || y > getYMax())
    return(false);

  //PRINT("[", x); PRINT(",", y); PRINTS("]");

  return(_D->setPoint(Y2ROW(x,y), X2COL(x,y), state));
}

