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

#ifndef LDFORGE_GLSHARED_H
#define LDFORGE_GLSHARED_H

enum E_VBOArray
{
	VBO_Quads,
	VBO_Triangles,
	VBO_Lines,
	VBO_CondLines,
	VBO_NormalColors,
	VBO_BFCFrontColors,
	VBO_BFCBackColors,
	VBO_PickColors,
	VBO_NumArrays
};

#endif // LDFORGE_GLSHARED_H