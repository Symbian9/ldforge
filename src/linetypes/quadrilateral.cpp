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

#include "quadrilateral.h"

LDQuadrilateral::LDQuadrilateral(Model *model) :
	LDObject {model} {}

LDQuadrilateral::LDQuadrilateral(const Vertex& v1, const Vertex& v2, const Vertex& v3, const Vertex& v4, Model* model) :
	LDObject {model}
{
	setVertex(0, v1);
	setVertex(1, v2);
	setVertex(2, v3);
	setVertex(3, v4);
}

int LDQuadrilateral::numVertices() const
{
	return 4;
}

QString LDQuadrilateral::typeName() const
{
	return "quad";
}

QString LDQuadrilateral::asText() const
{
	QString result = format("4 %1", color());

	for (int i = 0; i < 4; ++i)
		result += format(" %1", vertex(i));

	return result;
}

int LDQuadrilateral::triangleCount(DocumentManager*) const
{
	return 2;
}

LDObjectType LDQuadrilateral::type() const
{
	return SubclassType;
}
