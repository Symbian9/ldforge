/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013, 2014 Santeri Piippo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include <QColor>
#include "Main.h"

#define MAX_COLORS 512

class LDColor
{
	public:
		QString name, hexcode;
		QColor faceColor, edgeColor;
		int index;
};

void initColors();
int luma (QColor& col);

// Safely gets a color with the given number or null if no such color.
LDColor* getColor (int colnum);
void setColor (int colnum, LDColor* col);

// Main and edge color identifiers
static const int maincolor = 16;
static const int edgecolor = 24;
