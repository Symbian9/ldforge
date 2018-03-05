/*
 *  LDForge: LDraw parts authoring CAD
 *  Copyright (C) 2013 - 2017 Teemu Piippo
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

#include "../model.h"
#include "conditionaledge.h"

LDConditionalEdge::LDConditionalEdge (const Vertex& v0, const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
	setVertex (0, v0);
	setVertex (1, v1);
	setVertex (2, v2);
	setVertex (3, v3);
}

QString LDConditionalEdge::asText() const
{
	QString result = format("5 %1", color());

	// Add the coordinates
	for (int i = 0; i < 4; ++i)
		result += format(" %1", vertex(i));

	return result;
}
